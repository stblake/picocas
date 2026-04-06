#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "expr.h"
#include "parse.h"
#include "eval.h"
#include "symtab.h"
#include "core.h"
#include "print.h"
#include "test_utils.h"

void run_test(const char* input, const char* expected) {
    Expr* e = parse_expression(input);
    if (!e) {
        printf("Failed to parse: %s\n", input);
        ASSERT(0);
    }
    Expr* res = evaluate(e);
    char* res_str = expr_to_string_fullform(res);
    if (strcmp(res_str, expected) != 0) {
        printf("FAIL: %s\n  expected: %s\n  got:      %s\n", input, expected, res_str);
        ASSERT(0);
    } else {
        printf("PASS: %s -> %s\n", input, res_str);
    }
    free(res_str);
    expr_free(res);
    expr_free(e);
}

void test_factorsquarefree() {
    run_test("FactorSquareFree[x^5-x^3-x^2+1]", "Times[Power[Plus[-1, x], 2], Plus[1, Times[2, x], Times[2, Power[x, 2]], Power[x, 3]]]");
    run_test("FactorSquareFree[x^4-9x^3+29x^2-39x+18]", "Times[Power[Plus[-3, x], 2], Plus[2, Times[-3, x], Power[x, 2]]]");
    run_test("FactorSquareFree[x^5-x^3*y^2-x^2*y^3+y^5]", "Times[Power[Plus[Times[-1, x], y], 2], Plus[Times[2, Times[x, Power[y, 2]]], Power[x, 3], Times[2, Times[Power[x, 2], y]], Power[y, 3]]]");
    run_test("FactorSquareFree[{(x^2-1)(x-1),(x^4-1)(x^2-1)}]", "List[Times[Power[Plus[-1, x], 2], Plus[1, x]], Times[Power[Plus[-1, Power[x, 2]], 2], Plus[1, Power[x, 2]]]]");
    run_test("FactorSquareFree[Exp[2 x] - Log[x] - 1/x]", "Plus[Times[-1, Power[x, -1]], Exp[Times[2, x]], Times[-1, Log[x]]]");
    run_test("FactorSquareFree[1 - x^3]", "Plus[1, Times[-1, Power[x, 3]]]");
    run_test("FactorSquareFree[(1 - x^3)^2]", "Power[Plus[1, Times[-1, Power[x, 3]]], 2]");
}

void test_factor() {
    run_test("Factor[1 + 2 x + x^2]", "Power[Plus[1, x], 2]");
    run_test("Factor[x^10 - 1]", "Times[Plus[-1, x], Plus[1, x], Plus[1, x, Power[x, 2], Power[x, 3], Power[x, 4]], Plus[1, Times[-1, x], Times[-1, Power[x, 3]], Power[x, 2], Power[x, 4]]]");
    run_test("Factor[1 - x^3]", "Times[-1, Plus[-1, x], Plus[1, x, Power[x, 2]]]");
    run_test("Factor[x^10 - y^10]", "Times[Plus[x, y], Plus[x, Times[-1, y]], Plus[Power[x, 4], Power[y, 4], Times[x, Power[y, 3]], Times[Power[x, 2], Power[y, 2]], Times[Power[x, 3], y]], Plus[Times[-1, Times[x, Power[y, 3]]], Power[x, 4], Times[-1, Times[Power[x, 3], y]], Power[y, 4], Times[Power[x, 2], Power[y, 2]]]]");
    run_test("Factor[x^3 - 6x^2 + 11x - 6]", "Times[Plus[-3, x], Plus[-2, x], Plus[-1, x]]");
    run_test("Factor[2x^3 y - 2a^2 x y - 3a^2 x^2 + 3a^4]", "Times[Plus[a, x], Plus[Times[-1, a], x], Plus[Times[-3, Power[a, 2]], Times[2, Times[x, y]]]]");
    run_test("Factor[(x^3+2x^2)/(x^2-4y^2)-(x+2)/(x^2-4y^2)]", "Times[Plus[-1, x], Plus[1, x], Plus[2, x], Power[Plus[x, Times[-2, y]], -1], Power[Plus[x, Times[2, y]], -1]]");
    run_test("Factor[{x^2-1, x^4-1, x^8-1}]", "List[Times[Plus[-1, x], Plus[1, x]], Times[Plus[-1, x], Plus[1, x], Plus[1, Power[x, 2]]], Times[Plus[-1, x], Plus[1, x], Plus[1, Power[x, 2]], Plus[1, Power[x, 4]]]]");
    run_test("Factor[1 < 1 + 2 x + x^2 + 1/(1+x) < 2]", "Less[Less[1, Times[Power[Plus[1, x], -1], Plus[2, x], Plus[1, x, Power[x, 2]]]], 2]");
}

int main() {
    symtab_init();
    core_init();

    printf("Running facpoly tests...\n");
    TEST(test_factorsquarefree);
    TEST(test_factor);
    printf("All facpoly tests passed!\n");
    return 0;
}
