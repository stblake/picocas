#include "test_utils.h"
#include "symtab.h"
#include "core.h"

/*
 * Tests for Assuming and the $Assumptions OwnValue. Assuming is a thin
 * scoping wrapper around Block; these tests verify that:
 *   - Assuming makes its assumption visible to Simplify in the body.
 *   - $Assumptions is restored on exit.
 *   - Nested Assuming calls compose.
 *   - Lists of assumptions are converted to conjunctions.
 *   - An explicit Assumptions option overrides $Assumptions.
 */

void test_dollar_assumptions_default(void) {
    assert_eval_eq("$Assumptions", "True", 0);
}

void test_assuming_simplifies_under_assumption(void) {
    assert_eval_eq("Assuming[x > 0, Simplify[Sqrt[x^2]]]", "x", 0);
}

void test_assuming_restores_dollar_on_exit(void) {
    /* Run the Assuming expression, then verify $Assumptions is back to
     * True and Simplify no longer reduces Sqrt[x^2]. */
    assert_eval_eq("Assuming[x > 0, Simplify[Sqrt[x^2]]]", "x", 0);
    assert_eval_eq("$Assumptions", "True", 0);
    assert_eval_eq("Simplify[Sqrt[x^2]]", "Sqrt[x^2]", 0);
}

void test_assuming_nested(void) {
    assert_eval_eq("Assuming[x > 0, Assuming[y > 0, Simplify[Sqrt[x^2] + Sqrt[y^2]]]]",
                   "x + y", 0);
    assert_eval_eq("$Assumptions", "True", 0);
}

void test_assuming_list_assumption(void) {
    /* {x>0, y>0} converts to And[x>0, y>0]. */
    assert_eval_eq("Assuming[{x > 0, y > 0}, Simplify[Sqrt[x^2] + Sqrt[y^2]]]",
                   "x + y", 0);
    assert_eval_eq("$Assumptions", "True", 0);
}

void test_assumptions_option_override(void) {
    /* When an Assumptions option is given explicitly, it REPLACES the
     * $Assumptions default rather than appending to it. Inside
     * Assuming[x > 0, ...], an inner Simplify[..., Assumptions -> y<0]
     * therefore sees only y < 0; x > 0 is not visible. We verify this
     * with a probe expression that depends on the assumption set: with
     * x > 0 visible Sqrt[x^2] reduces to x; with only y < 0 visible it
     * does NOT reduce. */
    assert_eval_eq("Assuming[x > 0, Simplify[Sqrt[x^2], Assumptions -> y < 0]]",
                   "Sqrt[x^2]", 0);
}

void test_assuming_expr_arg_evaluation(void) {
    /* Assuming has HoldRest, so the assumption (arg 1) is evaluated but
     * the body (arg 2) is held until the Block fires. Verify that
     * Assuming[True, expr] returns expr unchanged. */
    assert_eval_eq("Assuming[True, 1 + 2]", "3", 0);
}

int main(void) {
    symtab_init();
    core_init();

    TEST(test_dollar_assumptions_default);
    TEST(test_assuming_simplifies_under_assumption);
    TEST(test_assuming_restores_dollar_on_exit);
    TEST(test_assuming_nested);
    TEST(test_assuming_list_assumption);
    TEST(test_assumptions_option_override);
    TEST(test_assuming_expr_arg_evaluation);

    printf("All Assuming tests passed!\n");
    return 0;
}
