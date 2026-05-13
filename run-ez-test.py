#!/usr/bin/env python3
"""Run ez-test.exe against a suite of evaluator test expressions and generate an HTML report.

Compatible with Python 3.4+.
"""

import subprocess
import sys
import os
import time
import math

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
EZ_TEST_EXE = os.path.join(SCRIPT_DIR, "ez-test.exe")

TOLERANCE = 1e-6

# --- Test case definitions ---

LIST_TESTS = [
    # (expression, {var: value, ...}, expected_rc, description)
    ("1+2*3",        {},                         0, "Literal arithmetic"),
    ("x+y",          {"x": 5, "y": 10},          0, "Variable access"),
    ("sin(x)",       {"x": 0},                   0, "Function call"),
    ("-x",           {"x": 5},                   0, "Unary negation"),
    ("(x+y)*z",      {"x": 1, "y": 2, "z": 3},  0, "Parentheses grouping"),
    ("x>y",          {"x": 5, "y": 3},           0, "Comparison operator"),
    ("!x&&y",        {"x": 0, "y": 1},           0, "Logical operators"),
    ("x^y",          {"x": 2, "y": 3},           0, "Power operator"),
]

SEMANTIC_TESTS = [
    # (expression, {var: value, ...}, expected_rc, expected_output_contains, description)
    ("x+y",            {},                 1, "VARIABLE_UNDEFINED",   "Undefined variable"),
    ("foo(1)",         {},                 1, "FUNCTION_UNDEFINED",   "Undefined function"),
    ("sin(1,2)",       {},                 1, "FUNCTION_PARAM_MISMATCH", "Wrong parameter count"),
]

EVAL_TESTS = [
    # (expression, {var: value, ...}, expected_result, description)
    ("1+2*3",          {},                          7.0,     "Literal arithmetic"),
    ("(1+2)*3",        {},                          9.0,     "Parentheses arithmetic"),
    ("x+y*z",          {"x": 1, "y": 2, "z": 3},   7.0,     "Variable expression"),
    ("-x+y",           {"x": 5, "y": 3},           -2.0,     "Unary negation"),
    ("x/y",            {"x": 10, "y": 4},           2.5,     "Division"),
    ("x^y",            {"x": 2, "y": 3},            8.0,     "Power"),
    ("sin(0)",         {},                          0.0,     "sin(0)"),
    ("cos(0)",         {},                          1.0,     "cos(0)"),
    ("sqr(16)",        {},                          math.sqrt(16), "sqrt"),
    ("abs(-5)",        {},                          5.0,     "abs"),
    ("x>y",            {"x": 5, "y": 3},            1.0,     "GT true"),
    ("x>y",            {"x": 3, "y": 5},            0.0,     "GT false"),
    ("!x",             {"x": 0},                    1.0,     "NOT on false"),
    ("!x",             {"x": 1},                    0.0,     "NOT on true"),
    ("x<y&&y<z",       {"x": 1, "y": 2, "z": 3},   1.0,     "Chained logic true"),
    ("x<y&&y>z",       {"x": 1, "y": 2, "z": 3},   0.0,     "Chained logic false"),
    ("x+y*z+w",        {"x": 1, "y": 2, "z": 3, "w": 4}, 11.0, "Multiple variables"),
    ("ln(1)",          {},                          0.0,     "ln(1)"),
    ("x/y+z",          {"x": 10, "y": 4, "z": 1.5}, 4.0,    "Mixed vars and literals"),
    ("exp(0)",         {},                          1.0,     "exp(0)"),
]


HTML_TEMPLATE = """<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>EZ Test Report</title>
<style>
body {{ font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background: #1e1e2e; color: #cdd6f4; margin: 0; padding: 24px; }}
h1 {{ color: #cba6f7; border-bottom: 2px solid #45475a; padding-bottom: 8px; }}
h2 {{ color: #89b4fa; margin-top: 32px; }}
.summary {{ display: flex; gap: 24px; margin: 16px 0 24px 0; flex-wrap: wrap; }}
.summary-box {{ background: #313244; border-radius: 8px; padding: 16px 24px; text-align: center; min-width: 120px; }}
.summary-box .label {{ font-size: 13px; color: #a6adc8; text-transform: uppercase; }}
.summary-box .value {{ font-size: 28px; font-weight: bold; margin-top: 4px; }}
.summary-box.pass .value {{ color: #a6e3a1; }}
.summary-box.fail .value {{ color: #f38ba8; }}
.summary-box.total .value {{ color: #89b4fa; }}
table {{ width: 100%; border-collapse: collapse; margin-top: 12px; }}
th {{ background: #45475a; color: #cdd6f4; padding: 10px 14px; text-align: left; font-size: 14px; white-space: nowrap; }}
td {{ padding: 10px 14px; border-bottom: 1px solid #45475a; font-size: 14px; vertical-align: top; }}
tr:hover {{ background: #313244; }}
td.pass {{ color: #a6e3a1; font-weight: bold; }}
td.fail {{ color: #f38ba8; font-weight: bold; }}
.output {{ font-family: 'Cascadia Code', 'Fira Code', 'Consolas', monospace; font-size: 12px; color: #bac2de; max-width: 900px; overflow-x: auto; white-space: pre; margin: 0; line-height: 1.5; }}
.error-msg {{ font-family: 'Cascadia Code', 'Fira Code', 'Consolas', monospace; font-size: 12px; color: #f38ba8; max-width: 900px; overflow-x: auto; white-space: pre-wrap; margin: 0; }}
.no-output {{ color: #585b70; font-style: italic; }}
.vars {{ font-family: 'Cascadia Code', 'Fira Code', 'Consolas', monospace; font-size: 12px; color: #a6adc8; }}
.footer {{ margin-top: 24px; font-size: 12px; color: #585b70; }}
</style>
</head>
<body>
<h1>EZ Test Report</h1>
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
</div>
{list_section}
{eval_section}
{semantic_section}
<div class="footer">Generated at {timestamp} &mdash; EZ Test Runner</div>
</body>
</html>
"""

LIST_SECTION = """<h2>Instruction List Tests</h2>
<table>
<tr>
  <th>#</th>
  <th>Expression</th>
  <th>Variables</th>
  <th>Status</th>
  <th>Instructions</th>
</tr>
{rows}
</table>"""

EVAL_SECTION = """<h2>Evaluation Tests</h2>
<table>
<tr>
  <th>#</th>
  <th>Expression</th>
  <th>Variables</th>
  <th>Expected</th>
  <th>Actual</th>
  <th>Delta</th>
  <th>Status</th>
  <th>Stderr</th>
</tr>
{rows}
</table>"""

SEMANTIC_SECTION = """<h2>Semantic Error Tests</h2>
<table>
<tr>
  <th>#</th>
  <th>Expression</th>
  <th>Expected Error</th>
  <th>Actual Output</th>
  <th>Status</th>
</tr>
{rows}
</table>"""

ROW_LIST = """<tr>
  <td>{index}</td>
  <td><code>{expression}</code></td>
  <td class="vars">{vars_display}</td>
  <td class="{status_class}">{status_label}</td>
  <td><pre class="output">{output}</pre></td>
</tr>"""

ROW_EVAL = """<tr>
  <td>{index}</td>
  <td><code>{expression}</code></td>
  <td class="vars">{vars_display}</td>
  <td>{expected}</td>
  <td>{actual}</td>
  <td>{delta}</td>
  <td class="{status_class}">{status_label}</td>
  <td><pre class="error-msg">{stderr}</pre></td>
</tr>"""

ROW_SEM = """<tr>
  <td>{index}</td>
  <td><code>{expression}</code></td>
  <td><code>{expected_error}</code></td>
  <td><pre class="error-msg">{actual_output}</pre></td>
  <td class="{status_class}">{status_label}</td>
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


def vars_display(vars_dict):
    """Format a variables dict for display."""
    if not vars_dict:
        return "(none)"
    items = sorted(vars_dict.items())
    return " ".join("{}={}".format(k, v) for k, v in items)


def build_cmd_line(expression, vars_dict, mode):
    """Build command-line arguments for ez-test.exe."""
    cmd = [EZ_TEST_EXE]
    for name, value in sorted(vars_dict.items()):
        cmd.extend(["-v", name, str(value)])
    if mode == "list":
        cmd.extend(["-l"])
    elif mode == "eval":
        cmd.extend(["-e"])
    cmd.append(expression)
    return cmd


def run_ez_test(cmd_line):
    """Run ez-test.exe and return (return_code, stdout, stderr)."""
    try:
        proc = subprocess.Popen(
            cmd_line,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
            shell=False
        )
        stdout, stderr = proc.communicate()
        return proc.returncode, stdout.rstrip("\n\r"), stderr.rstrip("\n\r")
    except OSError as e:
        return -1, "", "Failed to launch: " + str(e)


def parse_float(s):
    """Parse a float from a string. Returns None on failure."""
    if not s:
        return None
    try:
        return float(s.strip())
    except (ValueError, TypeError):
        return None


def main():
    if not os.path.isfile(EZ_TEST_EXE):
        print("ERROR: ez-test.exe not found at {}".format(EZ_TEST_EXE))
        print("Build the project first, e.g.:  make ez-test")
        sys.exit(1)

    all_results = []
    total_passed = 0
    total_failed = 0

    # --- List instruction tests ---
    list_results = []
    list_passed = 0
    list_failed = 0

    for index, (expr, vars_d, expected_rc, desc) in enumerate(LIST_TESTS, start=1):
        cmd = build_cmd_line(expr, vars_d, "list")
        actual_rc, stdout, stderr = run_ez_test(cmd)
        is_pass = (actual_rc == expected_rc and stdout.strip() != "")

        if is_pass:
            list_passed += 1
        else:
            list_failed += 1

        status_word = "PASS" if is_pass else "FAIL"
        print("[L{:>2}] {}  |  {!r}  rc={}  {}".format(
            index, status_word, expr, actual_rc, desc))

        list_results.append({
            "index": index,
            "expression": expr,
            "vars": vars_d,
            "is_pass": is_pass,
            "status_word": status_word,
            "output": (stdout + ("\n" + stderr if stderr else "")),
            "description": desc,
        })

    total_passed += list_passed
    total_failed += list_failed

    # --- Evaluation tests ---
    eval_results = []
    eval_passed = 0
    eval_failed = 0

    for index, (expr, vars_d, expected, desc) in enumerate(EVAL_TESTS, start=1):
        cmd = build_cmd_line(expr, vars_d, "eval")
        actual_rc, stdout, stderr = run_ez_test(cmd)

        if actual_rc != 0:
            is_pass = False
            actual_float = None
            delta_str = "N/A"
        else:
            actual_float = parse_float(stdout)
            if actual_float is None:
                is_pass = False
                delta_str = "N/A"
            else:
                delta = abs(actual_float - expected)
                delta_str = "{:.6e}".format(delta)
                is_pass = delta < TOLERANCE

        if is_pass:
            eval_passed += 1
        else:
            eval_failed += 1

        status_word = "PASS" if is_pass else "FAIL"
        print("[E{:>2}] {}  |  {!r}  expected={}  actual={}  {}".format(
            index, status_word, expr, expected,
            actual_float if actual_float is not None else "ERR",
            desc))

        eval_results.append({
            "index": index,
            "expression": expr,
            "vars": vars_d,
            "expected": expected,
            "actual": actual_float,
            "delta": delta_str,
            "is_pass": is_pass,
            "status_word": status_word,
            "stderr": stderr,
            "description": desc,
        })

    total_passed += eval_passed
    total_failed += eval_failed

    # --- Semantic error tests ---
    sem_results = []
    sem_passed = 0
    sem_failed = 0

    for index, (expr, vars_d, expected_rc, expected_text, desc) in enumerate(SEMANTIC_TESTS, start=1):
        cmd = build_cmd_line(expr, vars_d, "eval")
        actual_rc, stdout, stderr = run_ez_test(cmd)
        combined_output = (stdout + " " + stderr).strip()
        is_pass = (
            actual_rc == expected_rc
            and expected_text in combined_output
        )

        if is_pass:
            sem_passed += 1
        else:
            sem_failed += 1

        status_word = "PASS" if is_pass else "FAIL"
        print("[S{:>2}] {}  |  {!r}  expected='{}'  actual_rc={}  {}".format(
            index, status_word, expr, expected_text, actual_rc, desc))

        sem_results.append({
            "index": index,
            "expression": expr,
            "expected_error": expected_text,
            "actual_output": combined_output,
            "is_pass": is_pass,
            "status_word": status_word,
            "description": desc,
        })

    total_passed += sem_passed
    total_failed += sem_failed

    # --- Build HTML ---
    list_rows = []
    for r in list_results:
        sc = "pass" if r["is_pass"] else "fail"
        sl = r["status_word"]
        out = html_escape(r["output"]) if r["output"] else "(none)"
        list_rows.append(ROW_LIST.format(
            index=r["index"], expression=html_escape(r["expression"]),
            vars_display=html_escape(vars_display(r["vars"])),
            status_class=sc, status_label=sl, output=out))

    eval_rows = []
    for r in eval_results:
        sc = "pass" if r["is_pass"] else "fail"
        sl = r["status_word"]
        actual_str = str(r["actual"]) if r["actual"] is not None else "ERR"
        eval_rows.append(ROW_EVAL.format(
            index=r["index"], expression=html_escape(r["expression"]),
            vars_display=html_escape(vars_display(r["vars"])),
            expected=str(r["expected"]),
            actual=actual_str,
            delta=r["delta"],
            status_class=sc, status_label=sl,
            stderr=html_escape(r["stderr"]) if r["stderr"] else ""))

    sem_rows = []
    for r in sem_results:
        sc = "pass" if r["is_pass"] else "fail"
        sl = r["status_word"]
        sem_rows.append(ROW_SEM.format(
            index=r["index"], expression=html_escape(r["expression"]),
            expected_error=html_escape(r["expected_error"]),
            actual_output=html_escape(r["actual_output"]),
            status_class=sc, status_label=sl))

    list_section = LIST_SECTION.format(rows="\n".join(list_rows))
    eval_section = EVAL_SECTION.format(rows="\n".join(eval_rows))
    semantic_section = SEMANTIC_SECTION.format(rows="\n".join(sem_rows))

    timestamp = time.strftime("%Y-%m-%d %H:%M:%S")

    html = HTML_TEMPLATE.format(
        total=total_passed + total_failed,
        passed=total_passed,
        failed=total_failed,
        list_section=list_section,
        eval_section=eval_section,
        semantic_section=semantic_section,
        timestamp=html_escape(timestamp),
    )

    report_path = os.path.join(SCRIPT_DIR, "report-ez.html")
    with open(report_path, "w", encoding="utf-8") as f:
        f.write(html)

    print("")
    print("Report written to: {}".format(report_path))
    print("Total: {}  Passed: {}  Failed: {}".format(
        total_passed + total_failed, total_passed, total_failed))
    print("  List:   {} passed, {} failed".format(list_passed, list_failed))
    print("  Eval:   {} passed, {} failed".format(eval_passed, eval_failed))
    print("  Semantics: {} passed, {} failed".format(sem_passed, sem_failed))

    if total_failed > 0:
        sys.exit(1)
    else:
        sys.exit(0)


if __name__ == "__main__":
    main()
