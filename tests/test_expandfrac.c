#include "expand.h"
#include "eval.h"
#include "parse.h"
#include "expr.h"
#include "symtab.h"
#include "core.h"
#include "print.h"
#include "test_utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Compare evaluated input to an expected string in InputForm. */
static void run(const char* input, const char* expected) {
    Expr* e = parse_expression(input);
    if (!e) {
        printf("Failed to parse: %s\n", input);
        ASSERT(0);
    }
    Expr* res = evaluate(e);
    char* res_str = expr_to_string(res);
    if (strcmp(res_str, expected) != 0) {
        printf("FAIL: %s\n  expected: %s\n  got:      %s\n", input, expected, res_str);
        ASSERT(0);
    }
    free(res_str);
    expr_free(res);
    expr_free(e);
}

/* Some printers may reorder commutative operands; use FullForm equality of
 * two evaluated inputs to verify mathematical equivalence. */
static void run_eq(const char* lhs_input, const char* rhs_input) {
    Expr* l = parse_expression(lhs_input);
    Expr* r = parse_expression(rhs_input);
    ASSERT(l && r);
    Expr* le = evaluate(l);
    Expr* re = evaluate(r);
    char* ls = expr_to_string_fullform(le);
    char* rs = expr_to_string_fullform(re);
    if (strcmp(ls, rs) != 0) {
        printf("FAIL: %s != %s\n  lhs: %s\n  rhs: %s\n", lhs_input, rhs_input, ls, rs);
        ASSERT(0);
    }
    free(ls); free(rs);
    expr_free(l); expr_free(r);
    expr_free(le); expr_free(re);
}

void test_expand_numerator_basic(void) {
    /* Standard fraction with product numerator and product denominator. */
    run_eq("ExpandNumerator[(x-1)(x-2)/((x-3)(x-4))]",
           "(2 - 3 x + x^2)/((x-3)(x-4))");

    /* Multiple variables, high powers in numerator. */
    run_eq("ExpandNumerator[(1-x^2+y^4-z^6)^2/((x-3)(y-3))]",
           "(1 - 2 x^2 + x^4 + 2 y^4 - 2 x^2 y^4 + y^8 "
           "- 2 z^6 + 2 x^2 z^6 - 2 y^4 z^6 + z^12)/((x-3)(y-3))");

    /* Negative integer power in denominator stays untouched. */
    run_eq("ExpandNumerator[(x-y)^2/(u-v)^2]",
           "(x^2 - 2 x y + y^2)/(u-v)^2");

    /* Numerator with only a positive integer power. */
    run_eq("ExpandNumerator[(x+y)^2/(z+y)^2]",
           "(x^2 + 2 x y + y^2)/(y+z)^2");

    /* Power expansion of just a product (no denominator). */
    run_eq("ExpandNumerator[(a+b)^2]", "a^2 + 2 a b + b^2");

    /* No-op for atoms / non-fractional non-expansible expressions. */
    run("ExpandNumerator[5]", "5");
    run("ExpandNumerator[x]", "x");
    run("ExpandNumerator[Sin[x]]", "Sin[x]");

    /* Pure denominator: leave alone. */
    run_eq("ExpandNumerator[1/(x+1)^2]", "1/(x+1)^2");
    run_eq("ExpandNumerator[1/(x-1)]",  "1/(x-1)");

    /* Numerator unchanged when no Plus inside its factors. */
    run_eq("ExpandNumerator[Sin[x]/(y+1)^2]", "Sin[x]/(y+1)^2");

    /* Denominator stays unexpanded for the canonical example. */
    run_eq("ExpandNumerator[(a+b)(a-b)/((c+d)(c-d))]",
           "(a^2-b^2)/((c+d)(c-d))");
}

void test_expand_numerator_threading(void) {
    /* Threads over Plus (sums of fractions). */
    run_eq("ExpandNumerator[(a+b)^2/x + (c+d)(c-d)/y]",
           "(a^2 + 2 a b + b^2)/x + (c^2 - d^2)/y");

    /* Threads over List. */
    run_eq("ExpandNumerator[{(a+b)^2/c, (x-1)(x+1)/(y-1)}]",
           "{(a^2 + 2 a b + b^2)/c, (x^2-1)/(y-1)}");

    /* Threads over Equal. */
    run_eq("ExpandNumerator[x == (a+b)^2/c]",
           "x == (a^2 + 2 a b + b^2)/c");

    /* Threads over And/inequalities. */
    run_eq("ExpandNumerator[x == (a+b)^2/c && y >= (a-b)^2/c]",
           "x == (a^2 + 2 a b + b^2)/c && y >= (a^2 - 2 a b + b^2)/c");

    /* Threads over Less, Greater, LessEqual, GreaterEqual. */
    run_eq("ExpandNumerator[(a+b)^2/c < (a-b)^2/c]",
           "(a^2 + 2 a b + b^2)/c < (a^2 - 2 a b + b^2)/c");
    run_eq("ExpandNumerator[(a+b)^2/c > (a-b)^2/c]",
           "(a^2 + 2 a b + b^2)/c > (a^2 - 2 a b + b^2)/c");

    /* Threads over Or, Not. */
    run_eq("ExpandNumerator[x == (a+b)^2/c || y == (c+d)(c-d)/e]",
           "x == (a^2 + 2 a b + b^2)/c || y == (c^2 - d^2)/e");
    run_eq("ExpandNumerator[!(x == (a+b)^2/c)]",
           "!(x == (a^2 + 2 a b + b^2)/c)");
}

void test_expand_denominator_basic(void) {
    /* Combines all denominator factors into one expanded polynomial. */
    run_eq("ExpandDenominator[(x-1)(x-2)/((x-3)(x-4))]",
           "((x-1)(x-2))/(12 - 7 x + x^2)");

    /* Per-term expansion of differing negative powers. */
    run_eq("ExpandDenominator[1/(x+1) + 2/(x+1)^2 + 3/(x+1)^3]",
           "1/(1+x) + 2/(1 + 2 x + x^2) + 3/(1 + 3 x + 3 x^2 + x^3)");

    /* Single Power with negative exponent. */
    run_eq("ExpandDenominator[1/(x+1)^2]", "1/(1 + 2 x + x^2)");

    /* Leaves numerator unexpanded. */
    run_eq("ExpandDenominator[(x-y)^2/(u-v)^2]",
           "(x-y)^2/(u^2 - 2 u v + v^2)");

    /* Combines multi-factor denominator into single polynomial. */
    run_eq("ExpandDenominator[(a+b)(a-b)/((c+d)(c-d))]",
           "((a+b)(a-b))/(c^2 - d^2)");

    /* No denominator: no change. */
    run_eq("ExpandDenominator[(a+b)^2]", "(a+b)^2");
    run("ExpandDenominator[5]", "5");
    run("ExpandDenominator[x]", "x");

    /* Non-Plus base: nothing to expand at the top level. */
    run_eq("ExpandDenominator[1/x^3]", "1/x^3");
}

void test_expand_denominator_threading(void) {
    /* Threads over Plus (sums of fractions). */
    run_eq("ExpandDenominator[x/(a+b)^2 + y/((c+d)(c-d))]",
           "x/(a^2 + 2 a b + b^2) + y/(c^2 - d^2)");

    /* Threads over List. */
    run_eq("ExpandDenominator[{1/(x+1)^2, 5/((c+d)(c-d))}]",
           "{1/(1 + 2 x + x^2), 5/(c^2 - d^2)}");

    /* Threads over Equal. */
    run_eq("ExpandDenominator[x == c/(a+b)^2]",
           "x == c/(a^2 + 2 a b + b^2)");

    /* Threads over And + inequalities. */
    run_eq("ExpandDenominator[x == c/(a+b)^2 && y >= c/(a-b)^2]",
           "x == c/(a^2 + 2 a b + b^2) && y >= c/(a^2 - 2 a b + b^2)");

    /* Threads over Less. */
    run_eq("ExpandDenominator[c/(a+b)^2 < c/(a-b)^2]",
           "c/(a^2 + 2 a b + b^2) < c/(a^2 - 2 a b + b^2)");

    /* Threads over Or, Not. */
    run_eq("ExpandDenominator[x == 1/(a+b)^2 || y == 1/(c-d)^2]",
           "x == 1/(a^2 + 2 a b + b^2) || y == 1/(c^2 - 2 c d + d^2)");
    run_eq("ExpandDenominator[!(x == 1/(a+b)^2)]",
           "!(x == 1/(a^2 + 2 a b + b^2))");
}

void test_top_level_only(void) {
    /* ExpandNumerator/ExpandDenominator do NOT recurse into function bodies. */
    run_eq("ExpandNumerator[Sin[(a+b)^2/c]]", "Sin[(a+b)^2/c]");
    run_eq("ExpandDenominator[Sin[1/(a+b)^2]]", "Sin[1/(a+b)^2]");

    /* But threading through Plus is the documented exception. */
    run_eq("ExpandNumerator[Sin[(a+b)^2/c] + (a+b)^2/c]",
           "Sin[(a+b)^2/c] + (a^2 + 2 a b + b^2)/c");
}

void test_zero_arg_and_misuse(void) {
    /* Wrong arity: stays unevaluated. */
    run("ExpandNumerator[]", "ExpandNumerator[]");
    run("ExpandDenominator[a, b]", "ExpandDenominator[a, b]");
}

int main(void) {
    symtab_init();
    core_init();

    TEST(test_expand_numerator_basic);
    TEST(test_expand_numerator_threading);
    TEST(test_expand_denominator_basic);
    TEST(test_expand_denominator_threading);
    TEST(test_top_level_only);
    TEST(test_zero_arg_and_misuse);

    printf("All ExpandNumerator/ExpandDenominator tests passed!\n");
    return 0;
}
