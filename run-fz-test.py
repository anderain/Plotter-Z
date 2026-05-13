#!/usr/bin/env python3
"""Run fz-test.exe against a suite of test expressions and generate an HTML report.

Compatible with Python 3.4+.
"""

import subprocess
import sys
import os
import time
import xml.etree.ElementTree as ET
import xml.dom.minidom as minidom

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
FZ_TEST_EXE = os.path.join(SCRIPT_DIR, "fz-test.exe")

TESTS = [
    # === Valid expressions (expected return code: 0) ===
    ("1+2*3", 0),
    ("(1+2)*3", 0),
    ("sin(x*5/2)", 0),
    ("a+b*c-d/e^f", 0),
    ("-5+3*2", 0),
    ("!1&&0||1", 0),
    ("max(a,b)+min(c,d)", 0),
    ("1.5+2*3+sin(x*5/2)", 0),
    ("x+y*z", 0),
    ("(3+4)*(5-2)/6", 0),

    # === Invalid expressions (expected return code: 1) ===
    ("1+", 1),
    ("*2+3", 1),
    ("()", 1),
    ("(1+2", 1),
    ("1+2)", 1),
    ("sin(1,", 1),
    ("1 2", 1),
    ("1++2", 1),
    ("1+*2", 1),
    ("+1", 1),
]

HTML_TEMPLATE = """<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>Formula-Z Test Report</title>
<style>
body {{ font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background: #1e1e2e; color: #cdd6f4; margin: 0; padding: 24px; }}
h1 {{ color: #cba6f7; border-bottom: 2px solid #45475a; padding-bottom: 8px; }}
.summary {{ display: flex; gap: 24px; margin: 16px 0 24px 0; flex-wrap: wrap; }}
.summary-box {{ background: #313244; border-radius: 8px; padding: 16px 24px; text-align: center; min-width: 120px; }}
.summary-box .label {{ font-size: 13px; color: #a6adc8; text-transform: uppercase; }}
.summary-box .value {{ font-size: 28px; font-weight: bold; margin-top: 4px; }}
.summary-box.pass .value {{ color: #a6e3a1; }}
.summary-box.fail .value {{ color: #f38ba8; }}
.summary-box.total .value {{ color: #89b4fa; }}
.summary-box.xml-ok .value {{ color: #94e2d5; }}
.summary-box.xml-err .value {{ color: #fab387; }}
table {{ width: 100%; border-collapse: collapse; margin-top: 12px; }}
th {{ background: #45475a; color: #cdd6f4; padding: 10px 14px; text-align: left; font-size: 14px; white-space: nowrap; }}
td {{ padding: 10px 14px; border-bottom: 1px solid #45475a; font-size: 14px; vertical-align: top; }}
tr:hover {{ background: #313244; }}
td.pass {{ color: #a6e3a1; font-weight: bold; }}
td.fail {{ color: #f38ba8; font-weight: bold; }}
td.rc-pass {{ color: #a6e3a1; }}
td.rc-fail {{ color: #f38ba8; }}
td.xml-ok {{ color: #94e2d5; font-weight: bold; }}
td.xml-err {{ color: #fab387; font-weight: bold; }}
.xml-output {{ font-family: 'Cascadia Code', 'Fira Code', 'Consolas', monospace; font-size: 12px; color: #bac2de; max-width: 720px; overflow-x: auto; white-space: pre; margin: 0; line-height: 1.5; }}
.error-msg {{ font-family: 'Cascadia Code', 'Fira Code', 'Consolas', monospace; font-size: 12px; color: #f38ba8; max-width: 720px; overflow-x: auto; white-space: pre-wrap; margin: 0; }}
.no-output {{ color: #585b70; font-style: italic; }}
.footer {{ margin-top: 24px; font-size: 12px; color: #585b70; }}
</style>
</head>
<body>
<h1>FZ Test Report</h1>
<div class="summary">
  <div class="summary-box total">
    <div class="label">Total</div>
    <div class="value">{total}</div>
  </div>
  <div class="summary-box pass">
    <div class="label">Passed</div>
    <div class="value">{passed}</div>
  </div>
  <div class="summary-box fail">
    <div class="label">Failed</div>
    <div class="value">{failed}</div>
  </div>
  <div class="summary-box xml-ok">
    <div class="label">XML Valid</div>
    <div class="value">{xml_ok}</div>
  </div>
  <div class="summary-box xml-err">
    <div class="label">XML Invalid</div>
    <div class="value">{xml_err}</div>
  </div>
</div>
<table>
<tr>
  <th>#</th>
  <th>Expression</th>
  <th>Exp. RC</th>
  <th>Act. RC</th>
  <th>Status</th>
  <th>XML Valid</th>
  <th>Beautified XML Output</th>
  <th>Stderr / XML Error</th>
</tr>
{rows}
</table>
<div class="footer">Generated at {timestamp} &mdash; FZ Test Runner</div>
</body>
</html>
"""

ROW_TEMPLATE = """<tr>
  <td>{index}</td>
  <td><code>{expression}</code></td>
  <td>{expected_rc}</td>
  <td class="rc-{rc_class}">{actual_rc}</td>
  <td class="{status_class}">{status_label}</td>
  <td class="{xml_class}">{xml_label}</td>
  <td>{xml_display}</td>
  <td>{error_display}</td>
</tr>"""


def html_escape(text):
    """Escape text for safe HTML embedding."""
    if text is None:
        return ""
    return (text
            .replace("&", "&amp;")
            .replace("<", "&lt;")
            .replace(">", "&gt;")
            .replace('"', "&quot;")
            .replace("'", "&#39;"))


def validate_and_beautify_xml(xml_str):
    """Parse XML with ElementTree to validate, then pretty-print with minidom.
    Returns (is_valid, beautified_xml_or_none, error_message_or_none).
    """
    if not xml_str or not xml_str.strip():
        return False, None, "Empty output"

    try:
        ET.fromstring(xml_str)
    except ET.ParseError as e:
        return False, None, "XML ParseError: {}".format(e)

    try:
        dom = minidom.parseString(xml_str)
        pretty = dom.toprettyxml(indent="  ")
        lines = pretty.splitlines()
        if lines and lines[0].startswith("<?xml"):
            lines = lines[1:]
        pretty = "\n".join(lines).strip()
        return True, pretty, None
    except Exception as e:
        return False, None, "Minidom error: {}".format(e)


def run_single_test(expression):
    """Run fz-test.exe with the given expression (without -h flag).
    Returns (return_code, stdout, stderr).
    """
    try:
        proc = subprocess.Popen(
            [FZ_TEST_EXE, expression],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
            shell=False
        )
        stdout, stderr = proc.communicate()
        return proc.returncode, stdout, stderr
    except OSError as e:
        return -1, "", "Failed to launch: " + str(e)


def main():
    if not os.path.isfile(FZ_TEST_EXE):
        print("ERROR: fz-test.exe not found at {}".format(FZ_TEST_EXE))
        print("Build the project first, e.g.:  make fz-test")
        sys.exit(1)

    results = []
    passed = 0
    failed = 0
    xml_ok = 0
    xml_err = 0

    for index, (expression, expected_rc) in enumerate(TESTS, start=1):
        actual_rc, stdout, stderr = run_single_test(expression)
        is_pass = (actual_rc == expected_rc)

        stdout_stripped = stdout.rstrip("\n\r")
        stderr_stripped = stderr.rstrip("\n\r")

        xml_valid, xml_pretty, xml_error = False, None, None
        if actual_rc == 0 and stdout_stripped:
            xml_valid, xml_pretty, xml_error = validate_and_beautify_xml(
                stdout_stripped
            )

        if xml_valid:
            xml_ok += 1
        elif actual_rc == 0:
            xml_err += 1

        if is_pass:
            passed += 1
        else:
            failed += 1

        results.append({
            "index": index,
            "expression": expression,
            "expected_rc": expected_rc,
            "actual_rc": actual_rc,
            "is_pass": is_pass,
            "stdout": stdout_stripped,
            "stderr": stderr_stripped,
            "xml_valid": xml_valid,
            "xml_pretty": xml_pretty,
            "xml_error": xml_error,
        })

        status_word = "PASS" if is_pass else "FAIL"
        xml_word = "OK" if xml_valid else ("ERR" if actual_rc == 0 else "N/A")
        print("[{:>2}] {}  |  expr={!r}  ec={} ac={}  xml={}".format(
            index, status_word, expression, expected_rc, actual_rc, xml_word))

    # Build HTML table rows
    rows = []
    for r in results:
        status_class = "pass" if r["is_pass"] else "fail"
        status_label = "PASS" if r["is_pass"] else "FAIL"
        rc_class = "pass" if r["is_pass"] else "fail"

        if r["actual_rc"] != 0:
            xml_class = ""
            xml_label = "N/A"
        elif r["xml_valid"]:
            xml_class = "xml-ok"
            xml_label = "OK"
        else:
            xml_class = "xml-err"
            xml_label = "INVALID"

        if r["xml_valid"] and r["xml_pretty"]:
            xml_display = '<pre class="xml-output">{}</pre>'.format(
                html_escape(r["xml_pretty"]))
        elif r["actual_rc"] == 0 and r["stdout"]:
            xml_display = '<pre class="xml-output">{}</pre>'.format(
                html_escape(r["stdout"]))
        else:
            xml_display = '<span class="no-output">(none)</span>'

        error_parts = []
        if r["stderr"]:
            error_parts.append(html_escape(r["stderr"]))
        if r["xml_error"]:
            error_parts.append(html_escape(r["xml_error"]))
        if error_parts:
            error_display = '<pre class="error-msg">{}</pre>'.format(
                "\n".join(error_parts))
        else:
            error_display = '<span class="no-output">(none)</span>'

        rows.append(ROW_TEMPLATE.format(
            index=r["index"],
            expression=html_escape(r["expression"]),
            expected_rc=r["expected_rc"],
            actual_rc=r["actual_rc"],
            rc_class=rc_class,
            status_class=status_class,
            status_label=status_label,
            xml_class=xml_class,
            xml_label=xml_label,
            xml_display=xml_display,
            error_display=error_display,
        ))

    timestamp = time.strftime("%Y-%m-%d %H:%M:%S")

    html = HTML_TEMPLATE.format(
        total=len(TESTS),
        passed=passed,
        failed=failed,
        xml_ok=xml_ok,
        xml_err=xml_err,
        rows="\n".join(rows),
        timestamp=html_escape(timestamp),
    )

    report_path = os.path.join(SCRIPT_DIR, "report-fz.html")
    with open(report_path, "w", encoding="utf-8") as f:
        f.write(html)

    print("")
    print("Report written to: {}".format(report_path))
    print("Total: {}  Passed: {}  Failed: {}  XML-Valid: {}  XML-Invalid: {}".format(
        len(TESTS), passed, failed, xml_ok, xml_err))

    if failed > 0:
        sys.exit(1)
    else:
        sys.exit(0)


if __name__ == "__main__":
    main()
