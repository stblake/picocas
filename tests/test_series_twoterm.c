/*
 * test_series_twoterm.c -- direct unit tests for series_split_two_term.
 *
 * The probe is called before any series expansion to recognise expressions
 * of the form  a + b*x^(p/q)  where a and b are free of x. Tests here drive
 * the probe with synthetic inputs and check:
 *
 *   1. The return value (true/false matches the expected match/no-match).
 *   2. The decomposed a and b (via FullForm comparison against a rebuilt
 *      expression a + b*x^(p/q)).
 *   3. The exponent numerator and denominator.
 *
 * We also cover Puiseux exponents, mixed Plus with shared / conflicting
 * exponents, Times with at most one non-free factor, and the failure cases
 * (compound expressions, two competing non-free factors, Power bases that
 * aren't the expansion variable).
 */

#include "eval.h"
#include "parse.h"
#include "expr.h"
#include "symtab.h"
#include "core.h"
#include "series.h"
#include "test_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

static bool g_setup_done = false;
static void setup(void) {
    symtab_init();
    core_init();
    if (!g_setup_done) {
        const char* ups[] = { ".", "..", "../..", "../../..", "../../../..", NULL };
        for (int i = 0; ups[i]; i++) {
            char path[256];
            snprintf(path, sizeof(path), "%s/src/internal/init.m", ups[i]);
            if (access(path, F_OK) == 0) {
                (void)!chdir(ups[i]);
                break;
            }
        }
        g_setup_done = true;
    }
    Expr* c = parse_expression("Get[\"src/internal/init.m\"]");
    if (c) {
        Expr* r = evaluate(c);
        expr_free(c);
        if (r) expr_free(r);
    }
}

/* Parse `in`, run split_two_term against the symbol `x`, and compare the
 * outputs to the FullForm strings given. Also compares exp_num / exp_den. */
static void check_split_matches(const char* input_src,
                                const char* x_name,
                                const char* expected_a_ff,
                                const char* expected_b_ff,
                                int64_t expected_en,
                                int64_t expected_ed) {
    Expr* input = parse_expression(input_src);
    Expr* input_eval = evaluate(input);
    Expr* x = expr_new_symbol(x_name);

    Expr* a = NULL;
    Expr* b = NULL;
    int64_t en = 0, ed = 0;
    bool ok = series_split_two_term(input_eval, x, &a, &b, &en, &ed);
    ASSERT_MSG(ok, "split failed for %s", input_src);

    char* a_str = expr_to_string_fullform(a);
    char* b_str = expr_to_string_fullform(b);
    ASSERT_MSG(strcmp(a_str, expected_a_ff) == 0,
               "a mismatch for %s: expected %s, got %s",
               input_src, expected_a_ff, a_str);
    ASSERT_MSG(strcmp(b_str, expected_b_ff) == 0,
               "b mismatch for %s: expected %s, got %s",
               input_src, expected_b_ff, b_str);
    ASSERT_MSG(en == expected_en,
               "exp_num mismatch for %s: expected %lld, got %lld",
               input_src, (long long)expected_en, (long long)en);
    ASSERT_MSG(ed == expected_ed,
               "exp_den mismatch for %s: expected %lld, got %lld",
               input_src, (long long)expected_ed, (long long)ed);

    free(a_str); free(b_str);
    expr_free(a); expr_free(b);
    expr_free(x); expr_free(input_eval);
}

static void check_split_fails(const char* input_src, const char* x_name) {
    Expr* input = parse_expression(input_src);
    Expr* input_eval = evaluate(input);
    Expr* x = expr_new_symbol(x_name);

    Expr* a = NULL;
    Expr* b = NULL;
    int64_t en = 0, ed = 0;
    bool ok = series_split_two_term(input_eval, x, &a, &b, &en, &ed);
    ASSERT_MSG(!ok, "split unexpectedly succeeded for %s", input_src);

    expr_free(x); expr_free(input_eval);
}

/* Atomic inputs */

static void test_bare_variable(void) {
    setup();
    check_split_matches("x", "x", "0", "1", 1, 1);
}

static void test_variable_with_different_name(void) {
    setup();
    /* y is free of x, so it's the a part. */
    check_split_matches("y", "x", "y", "0", 1, 1);
}

static void test_integer_constant(void) {
    setup();
    check_split_matches("7", "x", "7", "0", 1, 1);
}

static void test_rational_constant(void) {
    setup();
    check_split_matches("3/4", "x", "Rational[3, 4]", "0", 1, 1);
}

/* Plus: a + b*x^c */

static void test_one_plus_x(void) {
    setup();
    check_split_matches("1 + x", "x", "1", "1", 1, 1);
}

static void test_one_plus_three_x(void) {
    setup();
    check_split_matches("1 + 3*x", "x", "1", "3", 1, 1);
}

static void test_one_plus_x_squared(void) {
    setup();
    check_split_matches("1 + x^2", "x", "1", "1", 2, 1);
}

static void test_a_plus_b_x_to_five(void) {
    setup();
    check_split_matches("a + b*x^5", "x", "a", "b", 5, 1);
}

static void test_puiseux_sqrt_exp(void) {
    setup();
    check_split_matches("1 + x^(1/2)", "x", "1", "1", 1, 2);
}

static void test_puiseux_two_thirds(void) {
    setup();
    check_split_matches("1 + x^(2/3)", "x", "1", "1", 2, 3);
}

static void test_negative_exp(void) {
    setup();
    check_split_matches("5 - 3*x^(-2)", "x", "5", "-3", -2, 1);
}

static void test_same_exp_sum(void) {
    setup();
    /* x + 2x parses as 3x after simp, collapses to b=3, so this is the same
     * as "3x" -- ensure we still get the right decomposition. */
    check_split_matches("x + 2*x", "x", "0", "3", 1, 1);
}

static void test_multiple_free_plus_single_x(void) {
    setup();
    /* (a + b) + c*x^2 -- a, b, c free of x. */
    check_split_matches("a + b + c*x^2", "x", "Plus[a, b]", "c", 2, 1);
}

/* Times: prod * (a + b*x^c) */

static void test_three_times_x(void) {
    setup();
    check_split_matches("3*x", "x", "0", "3", 1, 1);
}

static void test_three_times_x_squared(void) {
    setup();
    check_split_matches("3*x^2", "x", "0", "3", 2, 1);
}

static void test_constant_product(void) {
    setup();
    /* 3*5 with both free of x -> (15, 0, 1/1) after simp. */
    check_split_matches("3*5", "x", "15", "0", 1, 1);
}

static void test_pre_factored_power(void) {
    setup();
    /* 5 * x^3 -- one non-free factor and one free. */
    check_split_matches("5*x^3", "x", "0", "5", 3, 1);
}

/* Power forms */

static void test_plain_power(void) {
    setup();
    check_split_matches("x^4", "x", "0", "1", 4, 1);
}

static void test_power_rational_exp(void) {
    setup();
    check_split_matches("x^(3/2)", "x", "0", "1", 3, 2);
}

static void test_power_with_symbolic_coefficient(void) {
    setup();
    check_split_matches("a*x^(1/3)", "x", "0", "a", 1, 3);
}

/* Failure cases */

static void test_fails_sin(void) {
    setup();
    check_split_fails("Sin[x]", "x");
}

static void test_fails_different_exps(void) {
    setup();
    /* x + x^2 has two different non-zero b-parts at incompatible exponents. */
    check_split_fails("x + x^2", "x");
}

static void test_fails_two_nonfree_factors(void) {
    setup();
    /* (1 + x) * (2 + x) has two non-free factors -> cross terms we can't
     * represent as a single a + b*x^c. */
    check_split_fails("(1 + x)*(2 + x)", "x");
}

static void test_fails_power_of_polynomial(void) {
    setup();
    /* (1 + x)^2 -- the Power branch only handles Power[x, rational]. */
    check_split_fails("(1 + x)^2", "x");
}

static void test_fails_symbolic_exp(void) {
    setup();
    /* x^n where n isn't a rational integer literal. */
    check_split_fails("x^n", "x");
}

static void test_fails_exp_x_in_exponent(void) {
    setup();
    /* 2^x has x in the exponent -- Power branch requires base == x. */
    check_split_fails("2^x", "x");
}

/* Cross-variable tests: if we ask about the wrong variable, the expression
 * should read as "free of x" entirely. */
static void test_cross_variable_free(void) {
    setup();
    /* Asked about y: the whole expression is free of y. */
    check_split_matches("1 + 3*x", "y", "Plus[1, Times[3, x]]", "0", 1, 1);
}

static void test_cross_variable_nonfree(void) {
    setup();
    /* Asked about y: y appears as the linear term. */
    check_split_matches("a + b*y", "y", "a", "b", 1, 1);
}

/* Stress: many free-of-x Plus terms with one x-dependent. */
static void test_stress_many_free_summands(void) {
    setup();
    /* a + b + c + d + e + f + g + 2*x^3 */
    check_split_matches(
        "a + b + c + d + e + f + g + 2*x^3",
        "x",
        "Plus[a, b, c, d, e, f, g]",
        "2", 3, 1);
}

/* Stress: many free-of-x factors in a Times. */
static void test_stress_many_free_factors(void) {
    setup();
    /* a*b*c*d * 3*x^2 -- all free except x^2. */
    check_split_matches(
        "a*b*c*d*3*x^2",
        "x",
        "0",
        "Times[3, a, b, c, d]", 2, 1);
}

int main(void) {
    TEST(test_bare_variable);
    TEST(test_variable_with_different_name);
    TEST(test_integer_constant);
    TEST(test_rational_constant);

    TEST(test_one_plus_x);
    TEST(test_one_plus_three_x);
    TEST(test_one_plus_x_squared);
    TEST(test_a_plus_b_x_to_five);
    TEST(test_puiseux_sqrt_exp);
    TEST(test_puiseux_two_thirds);
    TEST(test_negative_exp);
    TEST(test_same_exp_sum);
    TEST(test_multiple_free_plus_single_x);

    TEST(test_three_times_x);
    TEST(test_three_times_x_squared);
    TEST(test_constant_product);
    TEST(test_pre_factored_power);

    TEST(test_plain_power);
    TEST(test_power_rational_exp);
    TEST(test_power_with_symbolic_coefficient);

    TEST(test_fails_sin);
    TEST(test_fails_different_exps);
    TEST(test_fails_two_nonfree_factors);
    TEST(test_fails_power_of_polynomial);
    TEST(test_fails_symbolic_exp);
    TEST(test_fails_exp_x_in_exponent);

    TEST(test_cross_variable_free);
    TEST(test_cross_variable_nonfree);

    TEST(test_stress_many_free_summands);
    TEST(test_stress_many_free_factors);

    printf("All series split-two-term tests passed.\n");
    return 0;
}
