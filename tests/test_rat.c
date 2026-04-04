#include "expr.h"
#include "parse.h"
#include "eval.h"
#include "symtab.h"
#include "core.h"
#include "print.h"
#include "test_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void run_test(const char* input, const char* expected) {
    Expr* e = parse_expression(input);
    Expr* res = evaluate(e);
    char* res_str = expr_to_string_fullform(res);
    if (strcmp(res_str, expected) != 0) {
        printf("FAIL: %s\n  expected: %s\n  got:      %s\n", input, expected, res_str);
        ASSERT_STR_EQ(res_str, expected);
    }
    free(res_str);
    expr_free(e);
    expr_free(res);
}

void test_numerator() {
    run_test("Numerator[2/3]", "2");
    run_test("Numerator[(x-1)(x-2)/(x-3)^2]", "Times[Plus[-2, x], Plus[-1, x]]");
    run_test("Numerator[Sin[x]^a (Sin[x]-2)^-b]", "Power[Sin[x], a]");
    run_test("Numerator[3/7]", "3");
    run_test("Numerator[3/7 + I/11]", "Complex[33, 7]");
    run_test("Numerator[(x-1)^2/((x-2)(x-3))]", "Power[Plus[-1, x], 2]");
    run_test("Numerator[a x^n y^-m Exp[a - b - 2c + 3d]]", "Times[Power[E, Plus[a, Times[3, d]]], a, Power[x, n]]");
    run_test("Numerator[a^-b/x]", "1");
    run_test("Numerator[2 x^y b^2]", "Times[2, Power[b, 2], Power[x, y]]");
    run_test("Numerator[{1,2,3,4,5,6}/3]", "List[1, 2, 1, 4, 5, 2]");
    run_test("Numerator[5]", "5");
}

void test_denominator() {
    run_test("Denominator[2/3]", "3");
    run_test("Denominator[(x-1)(x-2)/(x-3)^2]", "Power[Plus[-3, x], 2]");
    run_test("Denominator[Sin[x]^a (Sin[x]-2)^-b]", "Power[Plus[-2, Sin[x]], b]");
    run_test("Denominator[3/7]", "7");
    run_test("Denominator[3/7 + I/11]", "77");
    run_test("Denominator[(x-1)^2/((x-2)(x-3))]", "Times[Plus[-3, x], Plus[-2, x]]");
    run_test("Denominator[a x^n y^-m Exp[a - b - 2c + 3d]]", "Times[Power[E, Plus[b, Times[2, c]]], Power[y, m]]");
    run_test("Denominator[a^-b/x]", "Times[Power[a, b], x]");
    run_test("Denominator[2 x^y b^2]", "1");
    run_test("Denominator[{1,2,3,4,5,6}/3]", "List[3, 3, 1, 3, 3, 1]");
    run_test("Denominator[5]", "1");
}

void test_cancel() {
    run_test("Cancel[(x^2 - 1)/(x - 1)]", "Plus[1, x]");
    run_test("Cancel[(x - y)/(x^2 - y^2) + (x^3 - 27)/(x^2 - 9) + (x^3 + 1)/(x^2 - x + 1)]", "Plus[1, x, Times[Power[Plus[3, x], -1], Plus[9, Times[3, x], Power[x, 2]]], Times[-1, Power[Plus[Times[-1, x], Times[-1, y]], -1]]]");
    run_test("Cancel[(x - a)/(x^2 - a^2) == 0 && (x^2 - 2 x + 1)/(x - 1) >= 0]", "And[Equal[Power[Plus[a, x], -1], 0], GreaterEqual[Plus[-1, x], 0]]");
    run_test("Cancel[(x - 1)/(x^2 - 1) + (x - 2)/(x^2 - 4)]", "Plus[Power[Plus[1, x], -1], Power[Plus[2, x], -1]]");
}

void test_together() {
    run_test("Together[a/b + c/d]", "Times[Power[b, -1], Power[d, -1], Plus[Times[a, d], Times[b, c]]]");
    run_test("Together[x^2/(x^2 - 1) + x/(x^2 - 1)]", "Times[x, Power[Plus[-1, x], -1]]");
    run_test("Together[1/x + 1/(x + 1) + 1/(x + 2) + 1/(x + 3)]", "Times[Plus[6, Times[22, x], Times[18, Power[x, 2]], Times[4, Power[x, 3]]], Power[Plus[Times[6, x], Times[11, Power[x, 2]], Times[6, Power[x, 3]], Power[x, 4]], -1]]");
    run_test("Together[x^2/(x - y) - x y/(x - y)]", "x");
    run_test("Together[{1/x + 1/(x + 1), 1/(x + 2) + 1/(x + 3)}]", "List[Times[Plus[1, Times[2, x]], Power[Plus[x, Power[x, 2]], -1]], Times[Plus[5, Times[2, x]], Power[Plus[6, Times[5, x], Power[x, 2]], -1]]]");
    run_test("Together[(x - 1)/(x^2 - 1) + (x - 2)/(x^2 - 4)]", "Times[Plus[3, Times[2, x]], Power[Plus[2, Times[3, x], Power[x, 2]], -1]]");
}

int main() {
    symtab_init();
    core_init();

    printf("Running rat tests...\n");
    TEST(test_numerator);
    TEST(test_denominator);
    TEST(test_cancel);
    TEST(test_together);
    printf("All rat tests passed!\n");

    return 0;
}
