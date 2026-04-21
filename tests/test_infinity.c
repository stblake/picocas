/*
 * test_infinity.c -- correctness tests for arithmetic involving the
 * symbols Infinity, ComplexInfinity and Indeterminate.
 *
 * Each case below mirrors a Mathematica reference behaviour:
 *   - Infinity + (-Infinity)  -> Indeterminate
 *   - 0 * Infinity            -> Indeterminate
 *   - Indeterminate poisons   any arithmetic combination
 *   - Power[Infinity, q] cases per the standard table
 *
 * Indeterminate-emitting paths additionally write a diagnostic to
 * stderr (Power::infy / Infinity::indet); these tests assert only the
 * value, not the message text.
 */

#include "eval.h"
#include "parse.h"
#include "expr.h"
#include "symtab.h"
#include "core.h"
#include "test_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void check_full(const char* input, const char* expected_fullform) {
    Expr* e = parse_expression(input);
    ASSERT_MSG(e != NULL, "parse failed: %s", input);
    Expr* v = evaluate(e);
    char* got = expr_to_string_fullform(v);
    ASSERT_MSG(strcmp(got, expected_fullform) == 0,
        "Mismatch for %s\n  expected: %s\n  got:      %s",
        input, expected_fullform, got);
    free(got);
    expr_free(v);
    expr_free(e);
}

/* --------------------------------------------------------------- */
/* Times                                                           */
/* --------------------------------------------------------------- */
static void test_times_zero_infinity(void) {
    /* 0 * Infinity is Indeterminate (Infinity::indet message). */
    check_full("0 Infinity", "Indeterminate");
    check_full("Infinity * 0", "Indeterminate");
    check_full("0 ComplexInfinity", "Indeterminate");
}

static void test_times_scaled_infinity(void) {
    /* Positive coefficient absorbs into Infinity. */
    check_full("5 Infinity", "Infinity");
    check_full("Infinity * 7", "Infinity");
    /* Rational coefficient. */
    check_full("Infinity / 4", "Infinity");
    /* Any negative coefficient collapses to -Infinity (Times[-1, Infinity]),
     * matching Mathematica's directed-infinity semantics: magnitude does
     * not survive multiplication by an unbounded value. */
    check_full("-3 Infinity", "Times[-1, Infinity]");
    check_full("-Infinity", "Times[-1, Infinity]");
}

static void test_times_inf_inf(void) {
    /* Infinity * Infinity simplifies to Infinity (positive sign). */
    check_full("Infinity * Infinity", "Infinity");
    /* ComplexInfinity * c (c != 0) -> ComplexInfinity. */
    check_full("3 ComplexInfinity", "ComplexInfinity");
}

static void test_times_indeterminate(void) {
    /* Indeterminate poisons the whole product. */
    check_full("2 Indeterminate", "Indeterminate");
    check_full("Indeterminate * x", "Indeterminate");
    check_full("Indeterminate * Infinity", "Indeterminate");
}

/* --------------------------------------------------------------- */
/* Plus                                                            */
/* --------------------------------------------------------------- */
static void test_plus_inf_minus_inf(void) {
    /* Infinity + (-Infinity), in any flavour, -> Indeterminate. */
    check_full("Infinity - Infinity", "Indeterminate");
    check_full("5 Infinity - Infinity", "Indeterminate");
    check_full("5 Infinity - 7 Infinity", "Indeterminate");
}

static void test_plus_indeterminate(void) {
    /* Any Indeterminate term poisons the sum. */
    check_full("Indeterminate - Indeterminate", "Indeterminate");
    check_full("2 Indeterminate - 7", "Indeterminate");
    check_full("Indeterminate + x + 1", "Indeterminate");
}

static void test_plus_finite_with_infinity(void) {
    /* Finite + Infinity -> Infinity. */
    check_full("5 + Infinity", "Infinity");
    check_full("Infinity + x", "Infinity");
    /* Finite + ComplexInfinity -> ComplexInfinity. */
    check_full("3 + ComplexInfinity", "ComplexInfinity");
}

static void test_plus_complex_infinity_collisions(void) {
    /* ComplexInfinity + ComplexInfinity is Indeterminate. */
    check_full("ComplexInfinity + ComplexInfinity", "Indeterminate");
    /* ComplexInfinity + Infinity is Indeterminate. */
    check_full("ComplexInfinity + Infinity", "Indeterminate");
}

/* --------------------------------------------------------------- */
/* Power                                                           */
/* --------------------------------------------------------------- */
static void test_power_one_to_infinity(void) {
    /* 1^Infinity, 1^ComplexInfinity, 1^-Infinity all Indeterminate. */
    check_full("1 ^ Infinity", "Indeterminate");
    check_full("1 ^ ComplexInfinity", "Indeterminate");
    check_full("1 ^ (-Infinity)", "Indeterminate");
}

static void test_power_infinity_to_zero(void) {
    /* Infinity^0, ComplexInfinity^0, -Infinity^0 all Indeterminate. */
    check_full("Infinity ^ 0", "Indeterminate");
    check_full("ComplexInfinity ^ 0", "Indeterminate");
    check_full("(-Infinity) ^ 0", "Indeterminate");
}

static void test_power_infinity_inf_exponent(void) {
    check_full("Infinity ^ Infinity", "ComplexInfinity");
    check_full("Infinity ^ (-Infinity)", "0");
}

static void test_power_zero_to_infinity(void) {
    /* 0^Infinity = 0; 0^-Infinity = ComplexInfinity (with msg);
     * 0^ComplexInfinity = Indeterminate (with msg). */
    check_full("0 ^ Infinity", "0");
    check_full("0 ^ (-Infinity)", "ComplexInfinity");
    check_full("0 ^ ComplexInfinity", "Indeterminate");
}

static void test_power_infinity_finite_exponent(void) {
    /* Infinity^positive -> Infinity, Infinity^negative -> 0. */
    check_full("Infinity ^ 3", "Infinity");
    check_full("Infinity ^ (-2)", "0");
    /* ComplexInfinity^positive -> ComplexInfinity, ^negative -> 0. */
    check_full("ComplexInfinity ^ 5", "ComplexInfinity");
    check_full("ComplexInfinity ^ (-1)", "0");
}

static void test_power_indeterminate(void) {
    /* Indeterminate ^ x and x ^ Indeterminate -> Indeterminate. */
    check_full("Indeterminate ^ 5", "Indeterminate");
    check_full("5 ^ Indeterminate", "Indeterminate");
}

/* --------------------------------------------------------------- */
/* Division                                                        */
/* --------------------------------------------------------------- */
static void test_div_by_zero(void) {
    /* 1/0 -> ComplexInfinity (Power::infy message).
     * 0/0 -> Indeterminate (Power::infy + Infinity::indet messages). */
    check_full("1/0", "ComplexInfinity");
    check_full("0/0", "Indeterminate");
    /* 5/0 -> ComplexInfinity. */
    check_full("5/0", "ComplexInfinity");
}

static void test_div_infinity(void) {
    /* 1/Infinity -> 0; Infinity/Infinity -> Indeterminate. */
    check_full("1/Infinity", "0");
    check_full("Infinity/Infinity", "Indeterminate");
}

int main(void) {
    symtab_init();
    core_init();

    TEST(test_times_zero_infinity);
    TEST(test_times_scaled_infinity);
    TEST(test_times_inf_inf);
    TEST(test_times_indeterminate);

    TEST(test_plus_inf_minus_inf);
    TEST(test_plus_indeterminate);
    TEST(test_plus_finite_with_infinity);
    TEST(test_plus_complex_infinity_collisions);

    TEST(test_power_one_to_infinity);
    TEST(test_power_infinity_to_zero);
    TEST(test_power_infinity_inf_exponent);
    TEST(test_power_zero_to_infinity);
    TEST(test_power_infinity_finite_exponent);
    TEST(test_power_indeterminate);

    TEST(test_div_by_zero);
    TEST(test_div_infinity);

    printf("All infinity/indeterminate tests passed!\n");
    return 0;
}
