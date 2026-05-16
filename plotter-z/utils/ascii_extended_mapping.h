#ifndef PZ_ASCII_EXTENDED_MAPPING_H
#define PZ_ASCII_EXTENDED_MAPPING_H

/* ============================================================
 *  Plotter-Z ASCII Extended Character Constants
 *  Generated from ascii-extended-mapping.txt
 * ============================================================ */

/* ---------- 0x01 – 0x1F : typographic / control ---------- */
#define PZ_AE_DEGREE              0x01 /* ° */
#define PZ_AE_RADIAN_SUPERSCRIPT  0x02 /* r superscript (radian) */
#define PZ_AE_GRADIAN_SUPERSCRIPT 0x03 /* g superscript (gradian) */
#define PZ_AE_DIVIDE              0x04 /* ÷ */
#define PZ_AE_MULTIPLY            0x05 /* × */
#define PZ_AE_MIDDLE_DOT          0x06 /* · */
#define PZ_AE_EMPTY_RECT          0x07 /* empty rectangle box */

#define PZ_AE_CR_INDICATOR        0x08 /* carriage return indicator */
#define PZ_AE_EXP_E               0x09 /* exponential e symbol */
#define PZ_AE_IMAG_I              0x0A /* imaginary unit i */
#define PZ_AE_INFINITY            0x0B /* ∞ */
#define PZ_AE_ELLIPSIS            0x0C /* … */
#define PZ_AE_LDQUOTE             0x0D /* " */
#define PZ_AE_RDQUOTE             0x0E /* " */
#define PZ_AE_LSQUOTE             0x0F /* ' */

#define PZ_AE_RSQUOTE             0x10 /* ' */
#define PZ_AE_YEN                 0x11 /* ¥ */
#define PZ_AE_POUND               0x12 /* £ */
#define PZ_AE_LDBOOK              0x13 /* 《 */
#define PZ_AE_RDBOOK              0x14 /* 》 */
#define PZ_AE_ARROW_UP            0x15 /* ↑ */
#define PZ_AE_ARROW_DOWN          0x16 /* ↓ */
#define PZ_AE_ARROW_LEFT          0x17 /* ← */

#define PZ_AE_ARROW_RIGHT         0x18 /* → */
#define PZ_AE_LDCORNER            0x19 /* 「 */
#define PZ_AE_RDCORNER            0x1A /* 」 */
#define PZ_AE_ARROW_NW            0x1B /* ↖ */
#define PZ_AE_ARROW_SE            0x1C /* ↘ */
#define PZ_AE_ARROW_SW            0x1D /* ↙ */
#define PZ_AE_ARROW_NE            0x1E /* ↗ */
#define PZ_AE_IDEOGRAPHIC_PERIOD  0x1F /* 。 */

/* ---------- 0x7F : final sigma ---------- */
#define PZ_AE_GREEK_FINAL_SIGMA   0x7F /* ς */

/* ---------- 0x80 – 0x97 : Greek uppercase ---------- */
#define PZ_AE_GREEK_ALPHA         0x80 /* Α */
#define PZ_AE_GREEK_BETA          0x81 /* Β */
#define PZ_AE_GREEK_GAMMA         0x82 /* Γ */
#define PZ_AE_GREEK_DELTA         0x83 /* Δ */
#define PZ_AE_GREEK_EPSILON       0x84 /* Ε */
#define PZ_AE_GREEK_ZETA          0x85 /* Ζ */
#define PZ_AE_GREEK_ETA           0x86 /* Η */
#define PZ_AE_GREEK_THETA         0x87 /* Θ */

#define PZ_AE_GREEK_IOTA          0x88 /* Ι */
#define PZ_AE_GREEK_KAPPA         0x89 /* Κ */
#define PZ_AE_GREEK_LAMBDA        0x8A /* Λ */
#define PZ_AE_GREEK_MU            0x8B /* Μ */
#define PZ_AE_GREEK_NU            0x8C /* Ν */
#define PZ_AE_GREEK_XI            0x8D /* Ξ */
#define PZ_AE_GREEK_OMICRON       0x8E /* Ο */
#define PZ_AE_GREEK_PI            0x8F /* Π */

#define PZ_AE_GREEK_RHO           0x90 /* Ρ */
#define PZ_AE_GREEK_SIGMA         0x91 /* Σ */
#define PZ_AE_GREEK_TAU           0x92 /* Τ */
#define PZ_AE_GREEK_UPSILON       0x93 /* Υ */
#define PZ_AE_GREEK_PHI           0x94 /* Φ */
#define PZ_AE_GREEK_CHI           0x95 /* Χ */
#define PZ_AE_GREEK_PSI           0x96 /* Ψ */
#define PZ_AE_GREEK_OMEGA         0x97 /* Ω */

/* ---------- 0x98 – 0xAF : Greek lowercase ---------- */
#define PZ_AE_GREEK_alpha         0x98 /* α */
#define PZ_AE_GREEK_beta          0x99 /* β */
#define PZ_AE_GREEK_gamma         0x9A /* γ */
#define PZ_AE_GREEK_delta         0x9B /* δ */
#define PZ_AE_GREEK_epsilon       0x9C /* ε */
#define PZ_AE_GREEK_zeta          0x9D /* ζ */
#define PZ_AE_GREEK_eta           0x9E /* η */
#define PZ_AE_GREEK_theta         0x9F /* θ */

#define PZ_AE_GREEK_iota          0xA0 /* ι */
#define PZ_AE_GREEK_kappa         0xA1 /* κ */
#define PZ_AE_GREEK_lambda        0xA2 /* λ */
#define PZ_AE_GREEK_mu            0xA3 /* μ */
#define PZ_AE_GREEK_nu            0xA4 /* ν */
#define PZ_AE_GREEK_xi            0xA5 /* ξ */
#define PZ_AE_GREEK_omicron       0xA6 /* ο */
#define PZ_AE_GREEK_pi            0xA7 /* π */

#define PZ_AE_GREEK_rho           0xA8 /* ρ */
#define PZ_AE_GREEK_sigma         0xA9 /* σ */
#define PZ_AE_GREEK_tau           0xAA /* τ */
#define PZ_AE_GREEK_upsilon       0xAB /* υ */
#define PZ_AE_GREEK_phi           0xAC /* φ */
#define PZ_AE_GREEK_chi           0xAD /* χ */
#define PZ_AE_GREEK_psi           0xAE /* ψ */
#define PZ_AE_GREEK_omega         0xAF /* ω */

/* ---------- 0xB0 – 0xE7 : Katakana ---------- */
#define PZ_AE_KATAKANA_WO         0xB0 /* ヲ */
#define PZ_AE_KATAKANA_a          0xB1 /* ァ */
#define PZ_AE_KATAKANA_i          0xB2 /* ィ */
#define PZ_AE_KATAKANA_u          0xB3 /* ゥ */
#define PZ_AE_KATAKANA_e          0xB4 /* ェ */
#define PZ_AE_KATAKANA_o          0xB5 /* ォ */
#define PZ_AE_KATAKANA_ya         0xB6 /* ャ */
#define PZ_AE_KATAKANA_yu         0xB7 /* ュ */

#define PZ_AE_KATAKANA_yo         0xB8 /* ョ */
#define PZ_AE_KATAKANA_tsu_small  0xB9 /* ッ */
#define PZ_AE_KATAKANA_LONG_MARK  0xBA /* ー */
#define PZ_AE_KATAKANA_A          0xBB /* ア */
#define PZ_AE_KATAKANA_I          0xBC /* イ */
#define PZ_AE_KATAKANA_U          0xBD /* ウ */
#define PZ_AE_KATAKANA_E          0xBE /* エ */
#define PZ_AE_KATAKANA_O          0xBF /* オ */

#define PZ_AE_KATAKANA_KA         0xC0 /* カ */
#define PZ_AE_KATAKANA_KI         0xC1 /* キ */
#define PZ_AE_KATAKANA_KU         0xC2 /* ク */
#define PZ_AE_KATAKANA_KE         0xC3 /* ケ */
#define PZ_AE_KATAKANA_KO         0xC4 /* コ */
#define PZ_AE_KATAKANA_SA         0xC5 /* サ */
#define PZ_AE_KATAKANA_SI         0xC6 /* シ */
#define PZ_AE_KATAKANA_SU         0xC7 /* ス */

#define PZ_AE_KATAKANA_SE         0xC8 /* セ */
#define PZ_AE_KATAKANA_SO         0xC9 /* ソ */
#define PZ_AE_KATAKANA_TA         0xCA /* タ */
#define PZ_AE_KATAKANA_TI         0xCB /* チ */
#define PZ_AE_KATAKANA_TU         0xCC /* ツ */
#define PZ_AE_KATAKANA_TE         0xCD /* テ */
#define PZ_AE_KATAKANA_TO         0xCE /* ト */

#define PZ_AE_KATAKANA_NA         0xCF /* ナ */
#define PZ_AE_KATAKANA_NI         0xD0 /* ニ */
#define PZ_AE_KATAKANA_NU         0xD1 /* ヌ */
#define PZ_AE_KATAKANA_NE         0xD2 /* ネ */
#define PZ_AE_KATAKANA_NO         0xD3 /* ノ */
#define PZ_AE_KATAKANA_HA         0xD4 /* ハ */
#define PZ_AE_KATAKANA_HI         0xD5 /* ヒ */
#define PZ_AE_KATAKANA_HU         0xD6 /* フ */
#define PZ_AE_KATAKANA_HE         0xD7 /* ヘ */

#define PZ_AE_KATAKANA_HO         0xD8 /* ホ */
#define PZ_AE_KATAKANA_MA         0xD9 /* マ */
#define PZ_AE_KATAKANA_MI         0xDA /* ミ */
#define PZ_AE_KATAKANA_MU         0xDB /* ム */
#define PZ_AE_KATAKANA_ME         0xDC /* メ */
#define PZ_AE_KATAKANA_MO         0xDD /* モ */
#define PZ_AE_KATAKANA_YA         0xDE /* ヤ */
#define PZ_AE_KATAKANA_YU         0xDF /* ユ */

#define PZ_AE_KATAKANA_YO         0xE0 /* ヨ */
#define PZ_AE_KATAKANA_RA         0xE1 /* ラ */
#define PZ_AE_KATAKANA_RI         0xE2 /* リ */
#define PZ_AE_KATAKANA_RU         0xE3 /* ル */
#define PZ_AE_KATAKANA_RE         0xE4 /* レ */
#define PZ_AE_KATAKANA_RO         0xE5 /* ロ */
#define PZ_AE_KATAKANA_WA         0xE6 /* ワ */
#define PZ_AE_KATAKANA_N          0xE7 /* ン */

/* ---------- 0xE8 – 0xEA : Combining marks & punctuation ---------- */
#define PZ_AE_DAKUTEN             0xE8 /* voiced sound mark  ゛ */
#define PZ_AE_HANDAKUTEN          0xE9 /* semi-voiced mark   ゜ */
#define PZ_AE_IDEOGRAPHIC_COMMA   0xEA /* 、 */

/* ---------- 0xEB – 0xFF : Math / logic / set ---------- */
#define PZ_AE_SQRT                0xEB /* √ */
#define PZ_AE_INTEGRAL            0xEC /* ∫ */
#define PZ_AE_PLUSMINUS           0xED /* ± */
#define PZ_AE_NOT_EQUAL           0xEE /* ≠ */
#define PZ_AE_LEQ                 0xEF /* ≤ */

#define PZ_AE_GEQ                 0xF0 /* ≥ */
#define PZ_AE_APPROX              0xF1 /* ≈ */
#define PZ_AE_PARTIAL             0xF2 /* ∂ */
#define PZ_AE_ANGLE               0xF3 /* ∠ */
#define PZ_AE_THEREFORE           0xF4 /* ∴ */
#define PZ_AE_BECAUSE             0xF5 /* ∵ */
#define PZ_AE_ELEMENT_OF          0xF6 /* ∈ */
#define PZ_AE_FOR_ALL             0xF7 /* ∀ */

#define PZ_AE_EXISTS              0xF8 /* ∃ */
#define PZ_AE_UNION               0xF9 /* ∪ */
#define PZ_AE_INTERSECT           0xFA /* ∩ */
#define PZ_AE_SUBSET              0xFB /* ⊂ */
#define PZ_AE_SUPERSET            0xFC /* ⊃ */
#define PZ_AE_EMPTY_SET           0xFD /* ∅ */
#define PZ_AE_PROPORTIONAL        0xFE /* ∝ */
#define PZ_AE_NABLA               0xFF /* ∇ */

#endif /* PZ_ASCII_EXTENDED_MAPPING_H */
