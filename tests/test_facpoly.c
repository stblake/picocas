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
    run_test("FactorSquareFree[Exp[2 x] - Log[x] - 1/x]", "Plus[Times[-1, Power[x, -1]], Times[-1, Log[x]], Power[E, Times[2, x]]]");
}

int main() {
    symtab_init();
    core_init();
    
    printf("Running facpoly tests...\n");
    TEST(test_factorsquarefree);
    printf("All facpoly tests passed!\n");
    symtab_clear();
    return 0;
}
