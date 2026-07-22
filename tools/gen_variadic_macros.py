#!/usr/bin/env python3
"""
Generate variadic macro definitions for the test framework.
This script generates TRANSFORM_N_* macros for argument counts 1-100.
"""

def generate_eval_macro():
    """Generate the EVAL macro for forcing macro expansion."""
    return """/* EVAL macro - forces double expansion of macro arguments */
#define EVAL(...) EVAL_I(__VA_ARGS__)
#define EVAL_I(...) __VA_ARGS__

"""

def generate_varcount_macro(max_trailing=103):
    """Generate the VARCOUNT and VARCOUNT_I macros."""
    lines = []

    # VARCOUNT macro - uses trailing numbers so X_ gets the correct count N
    # With N args + K trailing values, X_ is at position K+2
    # Trailing values are at positions N+1 to N+K
    # If K+2 > N: X_ falls in trailing region
    # Position in trailing (1-indexed from start) = (K+2) - N
    # Value at position P = K - P + 1 = K - (K+2-N) + 1 = N - 1. Off by 1!
    # Solution: use trailing values (K+1, K, ..., 1) instead of (K, K-1, ..., 1)
    # Then value at position P = (K+1) - P + 1 = (K+1) - (K+2-N) + 1 = N. Correct!

    # Generate trailing values starting from max_trailing+1 down to 1
    trailing_values = list(range(max_trailing + 1, 0, -1))

    # Format in rows (10 values per row)
    trailing_lines = []
    for i in range(0, len(trailing_values), 10):
        chunk = trailing_values[i:i+10]
        if i + 10 < len(trailing_values):
            # Not the last line, add trailing comma
            trailing_lines.append("                                   " + ", ".join(map(str, chunk)) + ",")
        else:
            # Last line, no trailing comma (will be followed by ))
            trailing_lines.append("                                   " + ", ".join(map(str, chunk)))

    lines.append("#define VARCOUNT(...) \\")
    if len(trailing_lines) == 1:
        # All values fit on one line
        lines.append("    EVAL(VARCOUNT_I(__VA_ARGS__, " + trailing_lines[0].strip() + ",))")
    else:
        # Multiple lines needed
        lines.append("    EVAL(VARCOUNT_I(__VA_ARGS__, " + trailing_lines[0].strip() + " \\")
        for line in trailing_lines[1:-1]:
            lines.append(line + " \\")
        lines.append("                                   " + trailing_lines[-1].strip() + ",))")
    lines.append("")

    # VARCOUNT_I macro - parameters from _K down to _1, then X_
    # With K+1 trailing values (K+1, K, ..., 1), we need K+2 params: _, _K.._1, X_
    # X_ at position K+2, for N args gets trailing at index (K+2)-N
    # Value = (K+1) - ((K+2)-N) + 1 = N. Correct!
    # 
    # Note: K = max_trailing, trailing values = K+1 = max_trailing+1 values
    # Params: _, _max_trailing, ..., _1, X_ = max_trailing+2 params
    # With N args + (max_trailing+1) trailing = N + max_trailing + 1 tokens
    # For N=1: 1 + max_trailing + 1 = max_trailing + 2 tokens. Matches params!

    params = ["_" + str(i) for i in range(max_trailing, 0, -1)]
    param_str = ", ".join(params)
    lines.append(f"#define VARCOUNT_I(_, {param_str}, X_, ...) X_")
    lines.append("")

    return "\n".join(lines)


def generate_glue_macro():
    """Generate the GLUE macros for token concatenation with macro expansion."""
    return """/* GLUE macro for token concatenation with macro expansion
 * Uses double-indirection to force macro argument expansion before concatenation
 */
#define GLUE(X, Y) GLUE_I(X, Y)
#define GLUE_I(X, Y) X##Y

"""


def generate_transform_macros(max_count=100):
    """Generate TRANSFORM_N_* macros from 1 to max_count."""
    lines = []
    lines.append("/* Direct count-based macro definitions")
    lines.append(" * Each TRANSFORM_N_K applies TEST_CASE to K arguments")
    lines.append(" */")

    for k in range(1, max_count + 1):
        # Generate A1, A2, ..., AK
        args = [f"A{i}" for i in range(1, k + 1)]
        args_str = ", ".join(args)

        # Generate TEST_CASE_EXPAND calls
        expands = " ".join([f"TEST_CASE_EXPAND(A{i})" for i in range(1, k + 1)])

        lines.append(f"#define TRANSFORM_N_{k}({args_str}) {expands}")

    return "\n".join(lines)


def generate_transform_macro():
    """Generate the TRANSFORM macro."""
    return """/* Transform macro - applies TEST_CASE to each test function name
 * Direct expansion approach: dispatch on argument count
 * Note: VARCOUNT must be fully expanded before GLUE concatenation
 */
#define TRANSFORM(...) \\
    GLUE(TRANSFORM_N_, VARCOUNT(__VA_ARGS__))(__VA_ARGS__)

"""


def generate_test_case_macros():
    """Generate TEST_CASE and related macros."""
    lines = []

    lines.append("/* TEST_CASE/TEST_CASE_SETUP_CLEANUP macros expand to a tuple that")
    lines.append(" * TEST_SUITE can convert into a test_case initializer.")
    lines.append(" */")
    lines.append("#define TEST_CASE_SETUP_CLEANUP(NAME_, SETUP_, CLEANUP_) (NAME_, SETUP_, CLEANUP_)")
    lines.append("#define TEST_CASE(NAME_) TEST_CASE_SETUP_CLEANUP(NAME_, NULL, NULL)")
    lines.append("")
    lines.append("#define TEST_CASE_INIT(NAME_, SETUP_, CLEANUP_) \\")
    lines.append("    {.name = #NAME_, .fn = test_##NAME_, .setup = SETUP_, .cleanup = CLEANUP_},")
    lines.append("")
    lines.append("#define TEST_CASE_EXPAND(ARG_) TEST_CASE_EXPAND_I(TEST_CASE_WRAP(ARG_))")
    lines.append("#define TEST_CASE_EXPAND_I(ARGS_) TEST_CASE_INIT ARGS_")
    lines.append("")

    return "\n".join(lines)


def generate_helper_macros():
    """Generate helper macros for variadic processing."""
    lines = []

    lines.append("#define PROBE() ~, 1")
    lines.append("#define SECOND(A_, B_, ...) B_")
    lines.append("#define IS_PROBE(...) SECOND(__VA_ARGS__, 0)")
    lines.append("#define IS_PAREN(X_) IS_PROBE(IS_PAREN_PROBE X_)")
    lines.append("#define IS_PAREN_PROBE(...) PROBE()")
    lines.append("")

    return "\n".join(lines)


def generate_test_case_wrap_macros():
    """Generate TEST_CASE_WRAP macros."""
    lines = []

    lines.append("#define TEST_CASE_WRAP(ARG_) TEST_CASE_WRAP_I(IS_PAREN(ARG_), ARG_)")
    lines.append("#define TEST_CASE_WRAP_I(IS_PAREN_, ARG_) TEST_CASE_WRAP_II(IS_PAREN_, ARG_)")
    lines.append("#define TEST_CASE_WRAP_II(IS_PAREN_, ARG_) GLUE(TEST_CASE_WRAP_, IS_PAREN_)(ARG_)")
    lines.append("#define TEST_CASE_WRAP_0(ARG_) TEST_CASE_SETUP_CLEANUP(ARG_, NULL, NULL)")
    lines.append("#define TEST_CASE_WRAP_1(ARG_) ARG_")
    lines.append("")

    return "\n".join(lines)


def main(max_trailing=103):
    # Generate all macro sections
    varcount = generate_varcount_macro(max_trailing)
    eval_macro = generate_eval_macro()
    glue = generate_glue_macro()
    transform = generate_transform_macro()
    transform_macros = generate_transform_macros(150 if max_trailing >= 150 else max_trailing)
    test_case = generate_test_case_macros()
    helper = generate_helper_macros()
    test_case_wrap = generate_test_case_wrap_macros()

    # Combine all sections
    output = []
    output.append("/* Auto-generated variadic macros for test framework")
    output.append(" * Generated by tools/gen_variadic_macros.py")
    output.append(" * DO NOT EDIT MANUALLY")
    output.append(" */")
    output.append(eval_macro)
    output.append("")
    output.append(varcount)
    output.append("")
    output.append(glue)
    output.append("")
    output.append(helper)
    output.append("")
    output.append(transform)
    output.append("")
    output.append(transform_macros)
    output.append("")
    output.append(test_case_wrap)
    output.append("")
    output.append(test_case)

    print("\n".join(output))


if __name__ == "__main__":
    import sys
    max_trailing = int(sys.argv[1]) if len(sys.argv) > 1 else 103
    main(max_trailing)
