#include "eval.h"
#include "parse.h"
#include "expr.h"
#include "symtab.h"
#include "core.h"
#include "print.h"
#include "test_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static void run_full(const char* input, const char* expected) {
    Expr* e = parse_expression(input);
    Expr* r = evaluate(e);
    char* s = expr_to_string_fullform(r);
    ASSERT_MSG(strcmp(s, expected) == 0, "NestWhile %s: expected %s, got %s", input, expected, s);
    free(s);
    expr_free(e);
    expr_free(r);
}

static void run_real_approx(const char* input, double expected, double tol) {
    Expr* e = parse_expression(input);
    Expr* r = evaluate(e);
    double got = 0.0;
    if (r->type == EXPR_REAL) got = r->data.real;
    else if (r->type == EXPR_INTEGER) got = (double)r->data.integer;
    else {
        char* s = expr_to_string(r);
        ASSERT_MSG(0, "NestWhile %s: result not numeric: %s", input, s);
        free(s);
    }
    ASSERT_MSG(fabs(got - expected) <= tol,
               "NestWhile %s: expected ~%g, got %g", input, expected, got);
    expr_free(e);
    expr_free(r);
}

/* ---------- Spec examples ---------- */

static void test_nestwhile_divide_by_two_until_odd(void) {
    /* NestWhile[#/2 &, 123456, EvenQ] -> 1929 */
    run_full("NestWhile[#/2 &, 123456, EvenQ]", "1929");
}

static void test_nestwhile_log_until_nonpositive(void) {
    /* NestWhile[Log, 100., # > 0 &] -> ~-0.859384 */
    run_real_approx("NestWhile[Log, 100., # > 0 &]", -0.859384, 1e-4);
}

static void test_nestwhile_unsameq_pair(void) {
    /* NestWhile[Floor[#/2] &, 10, UnsameQ, 2] -> 0 */
    run_full("NestWhile[Floor[#/2] &, 10, UnsameQ, 2]", "0");
}

static void test_nestwhile_max_iterations(void) {
    /* NestWhile[#/2 &, 123456, EvenQ, 1, 4] -> 7716 */
    run_full("NestWhile[#/2 &, 123456, EvenQ, 1, 4]", "7716");
}

static void test_nestwhile_infinity_max(void) {
    /* NestWhile[Floor[#/2] &, 20, # > 1 &, 1, Infinity] -> 1 */
    run_full("NestWhile[Floor[#/2] &, 20, # > 1 &, 1, Infinity]", "1");
}

static void test_nestwhile_extra_step(void) {
    /* NestWhile[Floor[#/2] &, 20, # > 1 &, 1, Infinity, 1] -> 0 */
    run_full("NestWhile[Floor[#/2] &, 20, # > 1 &, 1, Infinity, 1]", "0");
}

static void test_nestwhile_backtrack_one(void) {
    /* NestWhile[Floor[#/2] &, 20, # > 1 &, 1, Infinity, -1] -> 2 */
    run_full("NestWhile[Floor[#/2] &, 20, # > 1 &, 1, Infinity, -1]", "2");
}

static void test_nestwhile_next_prime(void) {
    /* NestWhile[# + 1 &, 888, !PrimeQ[#] &] -> 907 */
    run_full("NestWhile[# + 1 &, 888, !PrimeQ[#] &]", "907");
}

static void test_nestwhile_next_twin_prime(void) {
    /* NestWhile[# + 1 &, 888, !PrimeQ[#1] || !PrimeQ[#3] &, 3] -> 1021 */
    run_full("NestWhile[# + 1 &, 888, !PrimeQ[#1] || !PrimeQ[#3] &, 3]", "1021");
}

/* ---------- Additional behavior ---------- */

static void test_nestwhile_test_false_initially(void) {
    /* test fails immediately -> return expr unchanged */
    run_full("NestWhile[f, x, False &]", "x");
    run_full("NestWhile[# + 1 &, 10, # < 5 &]", "10");
}

static void test_nestwhile_max_zero(void) {
    /* max = 0 means no applications at all */
    run_full("NestWhile[f, x, TrueQ, 1, 0]", "x");
    run_full("NestWhile[# + 1 &, 10, True &, 1, 0]", "10");
}

static void test_nestwhile_all_all(void) {
    /* NestWhile[f, expr, UnsameQ, All] detects any repeated value */
    run_full("NestWhile[Mod[# + 3, 7] &, 0, UnsameQ, All]", "0");
}

static void test_nestwhile_all_with_extra(void) {
    /* All with negative n returns one step before the repeat */
    run_full("NestWhile[Mod[# + 3, 7] &, 0, UnsameQ, All, Infinity, -1]", "4");
}

static void test_nestwhile_range_spec(void) {
    /* {1, Infinity} is equivalent to All: stop when any prior value repeats */
    run_full("NestWhile[Mod[# + 3, 7] &, 0, UnsameQ, {1, Infinity}]", "0");
}

static void test_nestwhile_range_min_delay(void) {
    /* {mmin, mmax} defers the first test until mmin results have been generated */
    run_full("NestWhile[# + 1 &, 0, False &, {3, 3}]", "2");
}

static void test_nestwhile_fixedpoint_equivalence(void) {
    /* NestWhile[f, expr, UnsameQ, 2] == FixedPoint-style behavior */
    run_full("NestWhile[Floor[#/2] &, 1000, UnsameQ, 2]", "0");
}

static void test_nestwhile_collatz_reaches_one(void) {
    /* Collatz iterate starting at 27 terminates at 1 */
    run_full("NestWhile[If[EvenQ[#], #/2, 3 # + 1] &, 27, # != 1 &]", "1");
}

static void test_nestwhile_max_infinity_neg1_last_true(void) {
    /* NestWhile[f,expr,test,m,Infinity,-1] returns last value for which test yielded True */
    run_full("NestWhile[# + 1 &, 1, # < 10 &, 1, Infinity, -1]", "9");
}

static void test_nestwhile_extra_two(void) {
    /* n=2 extra applications */
    run_full("NestWhile[# + 1 &, 0, # < 3 &, 1, Infinity, 2]", "5");
}

static void test_nestwhile_backtrack_two(void) {
    /* n=-2 returns two applications before the end */
    run_full("NestWhile[Floor[#/2] &, 20, # > 1 &, 1, Infinity, -2]", "5");
}

static void test_nestwhile_symbolic(void) {
    /* With symbolic test -> test is not True, so we stop immediately */
    run_full("NestWhile[f, x, g]", "x");
}

/* ---------- Unevaluated cases ---------- */

static void test_nestwhile_wrong_arg_count(void) {
    run_full("NestWhile[f, x]", "NestWhile[f, x]");
    run_full("NestWhile[f, x, g, 1, 2, 3, 4]", "NestWhile[f, x, g, 1, 2, 3, 4]");
}

static void test_nestwhile_bad_m(void) {
    /* m = 0 is not valid (must be >= 1) */
    run_full("NestWhile[f, x, g, 0]", "NestWhile[f, x, g, 0]");
    /* m = negative integer */
    run_full("NestWhile[f, x, g, -2]", "NestWhile[f, x, g, -2]");
    /* m is non-integer, non-All, non-List */
    run_full("NestWhile[f, x, g, y]", "NestWhile[f, x, g, y]");
}

static void test_nestwhile_bad_max(void) {
    /* max negative */
    run_full("NestWhile[f, x, g, 1, -1]", "NestWhile[f, x, g, 1, -1]");
    /* max non-integer */
    run_full("NestWhile[f, x, g, 1, y]", "NestWhile[f, x, g, 1, y]");
}

static void test_nestwhile_bad_n(void) {
    /* n non-integer */
    run_full("NestWhile[f, x, g, 1, 2, y]", "NestWhile[f, x, g, 1, 2, y]");
}

static void test_nestwhile_bad_range(void) {
    /* {mmin, mmax} with mmax < mmin */
    run_full("NestWhile[f, x, g, {3, 2}]", "NestWhile[f, x, g, List[3, 2]]");
    /* Three-element list is not a valid range spec */
    run_full("NestWhile[f, x, g, {1, 2, 3}]", "NestWhile[f, x, g, List[1, 2, 3]]");
}

int main(void) {
    symtab_init();
    core_init();

    /* Spec examples */
    TEST(test_nestwhile_divide_by_two_until_odd);
    TEST(test_nestwhile_log_until_nonpositive);
    TEST(test_nestwhile_unsameq_pair);
    TEST(test_nestwhile_max_iterations);
    TEST(test_nestwhile_infinity_max);
    TEST(test_nestwhile_extra_step);
    TEST(test_nestwhile_backtrack_one);
    TEST(test_nestwhile_next_prime);
    TEST(test_nestwhile_next_twin_prime);

    /* Additional behavior */
    TEST(test_nestwhile_test_false_initially);
    TEST(test_nestwhile_max_zero);
    TEST(test_nestwhile_all_all);
    TEST(test_nestwhile_all_with_extra);
    TEST(test_nestwhile_range_spec);
    TEST(test_nestwhile_range_min_delay);
    TEST(test_nestwhile_fixedpoint_equivalence);
    TEST(test_nestwhile_collatz_reaches_one);
    TEST(test_nestwhile_max_infinity_neg1_last_true);
    TEST(test_nestwhile_extra_two);
    TEST(test_nestwhile_backtrack_two);
    TEST(test_nestwhile_symbolic);

    /* Unevaluated cases */
    TEST(test_nestwhile_wrong_arg_count);
    TEST(test_nestwhile_bad_m);
    TEST(test_nestwhile_bad_max);
    TEST(test_nestwhile_bad_n);
    TEST(test_nestwhile_bad_range);

    printf("All NestWhile tests passed!\n");
    return 0;
}
