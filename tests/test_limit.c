/*
 * test_limit.c -- Correctness tests for the native Limit builtin.
 *
 * Exercises the public surface of src/limit.c across all four return
 * categories called out in limit_candidate_spec.md:
 *   - definite numeric / symbolic limits,
 *   - directional limits (including the Mathematica sign flip),
 *   - bounded-oscillation Interval returns,
 *   - unevaluated fall-through for unknown heads.
 *
 * Organised so that each new test can be added with a single assertion
 * line; the helpers do the plumbing.
 */

#include "test_utils.h"
#include "eval.h"
#include "parse.h"
#include "expr.h"
#include "symtab.h"
#include "core.h"
#include "print.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Compare the InputForm (default) printed result of `input` to `expected`.
 * Mismatches print both strings for easy debugging. */
static void check(const char* input, const char* expected) {
    Expr* e = parse_expression(input);
    ASSERT_MSG(e != NULL, "Failed to parse: %s", input);
    Expr* v = evaluate(e);
    char* got = expr_to_string(v);
    ASSERT_MSG(strcmp(got, expected) == 0,
               "Limit mismatch for %s:\n    expected: %s\n    got:      %s",
               input, expected, got);
    free(got);
    expr_free(v);
    expr_free(e);
}

/* Structural-equality check: accept any spelling that the printer might
 * emit by testing whether (result) - (expected) simplifies to 0. */
static void check_equiv(const char* input, const char* expected) {
    char buf[2048];
    snprintf(buf, sizeof(buf), "Expand[Together[(%s) - (%s)]]", input, expected);
    Expr* e = parse_expression(buf);
    ASSERT_MSG(e != NULL, "Failed to parse: %s", buf);
    Expr* v = evaluate(e);
    char* got = expr_to_string_fullform(v);
    ASSERT_MSG(strcmp(got, "0") == 0,
               "Limit not equivalent: %s\n    expected ~ %s\n    diff:       %s",
               input, expected, got);
    free(got);
    expr_free(v);
    expr_free(e);
}

/* ----------------------------------------------------------------- */
/* Layer 1 -- fast paths                                              */
/* ----------------------------------------------------------------- */
static void test_fast_paths(void) {
    /* Free of the variable. */
    check("Limit[7, x -> 0]", "7");
    check("Limit[a, x -> 5]", "a");
    /* The variable itself. */
    check("Limit[x, x -> 5]", "5");
    /* Continuous polynomial / trig. */
    check("Limit[x^2, x -> 2]", "4");
    check("Limit[a x^2 + 4 x + 7, x -> 1]", "11 + a");
    check("Limit[Sin[Pi/2], x -> 0]", "1");
}

/* ----------------------------------------------------------------- */
/* Layer 2 -- series-based                                            */
/* ----------------------------------------------------------------- */
static void test_series(void) {
    check("Limit[Sin[x]/x, x -> 0]", "1");
    check("Limit[(Cos[x]-1)/(Exp[x^2]-1), x -> 0]", "-1/2");
    check("Limit[(Tan[x]-x)/x^3, x -> 0]", "1/3");
    check("Limit[Sin[2 x]/Sin[x], x -> Pi]", "-2");
    check("Limit[(x^a - 1)/a, a -> 0]", "Log[x]");
}

/* ----------------------------------------------------------------- */
/* Layer 3 -- rational functions                                      */
/* ----------------------------------------------------------------- */
static void test_rational(void) {
    check("Limit[x/(x + 1), x -> Infinity]", "1");
    check("Limit[3 x^2/(x^2 - 2), x -> -Infinity]", "3");
    check("Limit[x/(x - 2 a), x -> a]", "-1");
}

/* ----------------------------------------------------------------- */
/* Layer 5 -- log reduction                                           */
/* ----------------------------------------------------------------- */
static void test_log_reduction(void) {
    /* Classical 1^Infinity forms. */
    check("Limit[(1 + A x)^(1/x), x -> 0]", "E^A");
    check("Limit[(1 + a/x)^(b x), x -> Infinity]", "E^(a b)");
}

/* ----------------------------------------------------------------- */
/* Layer 4-ish -- RP-form families via Series                         */
/* ----------------------------------------------------------------- */
static void test_rp_forms(void) {
    check_equiv("Limit[Sqrt[x^2 + a x] - Sqrt[x^2 + b x], x -> Infinity]",
                "(a - b)/2");
}

/* ----------------------------------------------------------------- */
/* Directional limits + sign-convention tests                         */
/* ----------------------------------------------------------------- */
static void test_directions(void) {
    check("Limit[1/x, x -> 0, Direction -> \"FromAbove\"]", "Infinity");
    check("Limit[1/x, x -> 0, Direction -> \"FromBelow\"]", "-1 Infinity");
    /* Mathematica sign convention: -1 == FromAbove, +1 == FromBelow. */
    check("Limit[1/x, x -> 0, Direction -> -1]",           "Infinity");
    check("Limit[1/x, x -> 0, Direction -> 1]",            "-1 Infinity");
}

/* ----------------------------------------------------------------- */
/* Layer 6 -- bounded oscillation (Interval)                          */
/* ----------------------------------------------------------------- */
static void test_interval_returns(void) {
    check("Limit[Sin[1/x], x -> 0]", "Interval[{-1, 1}]");
    check("Limit[Sin[x], x -> Infinity]", "Interval[{-1, 1}]");
}

/* ----------------------------------------------------------------- */
/* Iterated / multivariate forms                                      */
/* ----------------------------------------------------------------- */
static void test_iterated(void) {
    /* Iterated right-to-left: first y -> 0 gives x/x = 1, then x -> 0
     * of 1 is 1. */
    check("Limit[x/(x + y), {x -> 0, y -> 0}]", "1");
}

/* ----------------------------------------------------------------- */
/* Calling-form error handling                                        */
/* ----------------------------------------------------------------- */
static void test_unevaluated(void) {
    /* Non-rule second argument: Limit[f, a] should stay unevaluated. */
    check("Limit[x, x]", "Limit[x, x]");
    /* Unknown direction option: unevaluated. */
    check("Limit[x, x -> 0, Direction -> foo]",
          "Limit[x, x -> 0, Direction -> foo]");
}

/* ----------------------------------------------------------------- */
/* Reciprocal trig at singular points                                 */
/*                                                                    */
/* Every one of these collapses the 0 * ComplexInfinity trap unless   */
/* Csc/Sec/Cot are rewritten in terms of Sin/Cos before the layered   */
/* dispatcher runs. Covers the bulk of the new regression suite.     */
/* ----------------------------------------------------------------- */
static void test_reciprocal_trig(void) {
    check_equiv("Limit[x Csc[x], x -> 0]",              "1");
    check_equiv("Limit[x Csc[2 x], x -> 0]",            "1/2");
    check_equiv("Limit[Csc[7 x] Tan[2 x], x -> 0]",     "2/7");
    check_equiv("Limit[Csc[a x] Tan[b x], x -> 0]",     "b/a");
    check_equiv("Limit[Cot[4 x] Tan[3 x], x -> 0]",     "3/4");
    check_equiv("Limit[Cot[a x] Tan[b x], x -> 0]",     "b/a");
    check_equiv("Limit[x Cot[a x], x -> 0]",            "1/a");
    check_equiv("Limit[x Cot[x], x -> 0]",              "1");
    check_equiv("Limit[x^2 Csc[x], x -> 0]",            "0");
    check_equiv("Limit[(-1 + E^x) Cot[a x], x -> 0]",   "1/a");
    check_equiv("Limit[Csc[x] (x + Tan[x]), x -> 0]",   "2");
    check_equiv("Limit[(x - ArcSin[x]) Csc[x]^3, x -> 0]", "-1/6");
    check_equiv("Limit[Csc[Pi x] Log[x], x -> 1]",      "-1/Pi");
    check_equiv("Limit[Csch[a x] Sin[b x], x -> 0]",    "b/a");
    check_equiv("Limit[Csc[x]^2 Log[Cos[x]], x -> 0]",  "-1/2");
    check_equiv("Limit[Csc[2 x] Sin[3 x], x -> Pi]",    "-3/2");
    check_equiv("Limit[Sec[2 x] (1 - Tan[x]), x -> Pi/4]", "1");
    check_equiv("Limit[(-Pi/2 + x) Tan[3 x], x -> Pi/2]", "-1/3");
}

/* ----------------------------------------------------------------- */
/* Bounded envelope at +Infinity                                      */
/*                                                                    */
/* Families where |f| is dominated by something that decays to zero;  */
/* the squeeze layer short-circuits before Series trips on Sin[t^2]   */
/* (which has no Taylor expansion at infinity).                       */
/* ----------------------------------------------------------------- */
static void test_bounded_envelope(void) {
    check_equiv("Limit[Sin[t^2]/t^2, t -> Infinity]",         "0");
    check_equiv("Limit[(1 - Cos[x])/x, x -> Infinity]",       "0");
    check_equiv("Limit[(1 + Sin[x])/x, x -> Infinity]",       "0");
    check_equiv("Limit[(x Sin[x])/(5 + x^2), x -> Infinity]", "0");
    /* Interior bounded oscillation at 0 -- Sin[1/x] has no series at 0,
     * so the squeeze is the cleanest route.                              */
    check_equiv("Limit[x^2 Sin[1/x], x -> 0]",                "0");
}

/* ----------------------------------------------------------------- */
/* ArcTan / ArcCot with divergent inner argument                     */
/* ----------------------------------------------------------------- */
static void test_arctan_infinity(void) {
    check_equiv("Limit[ArcTan[x^2 - x^4], x -> Infinity]", "-Pi/2");
}

/* ----------------------------------------------------------------- */
/* Log-reduction of Power-in-x-exponent indeterminate forms          */
/*                                                                    */
/* Covers both the single-Power case (handled by the old code) and   */
/* the product-of-Powers generalization.                              */
/* ----------------------------------------------------------------- */
static void test_exp_indeterminate(void) {
    /* Classical 1^Infinity / 0^0 shapes that continuous substitution
     * cannot touch. */
    check_equiv("Limit[(-1 + x)^(-1 + x), x -> 1]",                 "1");
    check_equiv("Limit[Cos[x]^Cot[x], x -> 0]",                     "1");
    check_equiv("Limit[Sec[x]^Cot[x], x -> 0]",                     "1");
    check_equiv("Limit[(1 - 1/x^2)^Cot[1/x], x -> Infinity]",       "1");
    check_equiv("Limit[(1 - 1/x)^(x^2), x -> Infinity]",            "0");
    check_equiv("Limit[(1 + Sin[a x])^Cot[b x], x -> 0]",           "E^(a/b)");
    check_equiv("Limit[x^Tan[(Pi x)/2], x -> 1]",                   "E^(-2/Pi)");
    check_equiv("Limit[((-3 + 2*x)/(5 + 2*x))^(1 + 2*x), x -> Infinity]",
                "1/E^8");
    /* Polynomial-dominates-exponential decay (product-of-Powers log-
     * reduction discovers this via Log[E^(-x) x^20] = -x + 20 Log[x] -> -Inf). */
    check_equiv("Limit[E^(-x) x^20, x -> Infinity]", "0");
}

/* ----------------------------------------------------------------- */
/* Continuous-substitution residuals                                  */
/*                                                                    */
/* Power[0, positive] appears naturally when substituting directly:   */
/* Sqrt[x-1]/x at x=1 evaluates to Sqrt[0]/1 which PicoCAS's Power    */
/* evaluator leaves un-folded. The limit module has a small folder   */
/* so the answer printed is the plain 0.                              */
/* ----------------------------------------------------------------- */
static void test_power_zero_folding(void) {
    check_equiv("Limit[Sqrt[x - 1]/x, x -> 1]", "0");
}

/* ----------------------------------------------------------------- */
/* Miscellaneous exact cases                                          */
/* ----------------------------------------------------------------- */
static void test_misc(void) {
    check_equiv("Limit[x (-Sqrt[x] + Sqrt[2 + x]), x -> Infinity]", "Infinity");
    check_equiv("Limit[E^(9/x)*(-2 + x) - x, x -> Infinity]", "7");
    check_equiv("Limit[(Tan[x] - x)/x^3, x -> 0]", "1/3");
}

/* Regression set added 2026-04-20 (batch 2).
 * Includes the Factor[(1+t)^(-1/2)] segfault case and eight challenging
 * limits reported from the REPL, covering Csc * Log, (1+sin)^cot forms,
 * ArcTan at infinity, Csc^3 singularity, Log merge at infinity, and
 * Sin/Cos removable singularities. */
static void test_regression_batch2(void) {
    check_equiv("Limit[1/(t*Sqrt[t + 1]) - 1/t, t -> 0]", "-1/2");
    check_equiv("Limit[Csc[Pi*x]*Log[x], x -> 1]", "-1/Pi");
    check_equiv("Limit[(1 + Sin[a x])^Cot[b x], x -> 0]", "E^(a/b)");
    check_equiv("Limit[ArcTan[x^2 - x^4], x -> Infinity]", "-Pi/2");
    check_equiv("Limit[(x - ArcSin[x]) Csc[x]^3, x -> 0]", "-1/6");
    check_equiv("Limit[Cos[x]^Cot[x], x -> 0]", "1");
    check_equiv("Limit[-x + Log[2 + E^x], x -> Infinity]", "0");
    check_equiv("Limit[(Sin[x] - Cos[x])/Cos[2x], x -> Pi/4]", "-1/Sqrt[2]");
    check_equiv("Limit[(Sqrt[x^2 + 4] - Sqrt[8])/(x^2 - 4), x -> 2]", "Sqrt[2]/8");
    /* Radical-ratio post-processing: Sqrt[6]/Sqrt[2] -> Sqrt[3], and the
     * 0/0 Sqrt-denominator path must not emit a spurious Power::infy. */
    check("Limit[Sqrt[x^2 - 9]/Sqrt[2 x - 6], x -> 3]", "Sqrt[3]");
}

int main(void) {
    symtab_init();
    core_init();

    TEST(test_fast_paths);
    TEST(test_series);
    TEST(test_rational);
    TEST(test_log_reduction);
    TEST(test_rp_forms);
    TEST(test_directions);
    TEST(test_interval_returns);
    TEST(test_iterated);
    TEST(test_unevaluated);

    TEST(test_reciprocal_trig);
    TEST(test_bounded_envelope);
    TEST(test_arctan_infinity);
    TEST(test_exp_indeterminate);
    TEST(test_power_zero_folding);
    TEST(test_misc);
    TEST(test_regression_batch2);

    printf("All limit tests passed.\n");
    return 0;
}
