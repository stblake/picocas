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

void run_test(const char* input, const char* expected) {
    Expr* e = parse_expression(input);
    Expr* res = evaluate(e);
    char* s = expr_to_string_fullform(res);
    ASSERT_MSG(strcmp(s, expected) == 0, "Through %s: expected %s, got %s", input, expected, s);
    free(s);
    expr_free(e); expr_free(res);
}

void test_through() {
    run_test("Through[{f, g, h}[x]]", "List[f[x], g[x], h[x]]");
    run_test("Through[(f + g + h)[x, y]]", "Plus[f[x, y], g[x, y], h[x, y]]");
    run_test("Through[(f + g + h)[x, y], Plus]", "Plus[f[x, y], g[x, y], h[x, y]]");
    run_test("Through[(f * g)[x, y], Plus]", "Times[f, g][x, y]"); // Doesn't match Plus
    run_test("Through[(f * g)[x, y], Times]", "Times[f[x, y], g[x, y]]");
}

void test_freeq() {
    run_test("FreeQ[{1, 2, 4, 1, 0}, 0]", "False");
    run_test("FreeQ[{{1, 1, 3, 0}, {2, 1, 2, 2}}, 0]", "False");
    run_test("FreeQ[{a, b, b, a, a, a}, _Integer]", "True");
    run_test("FreeQ[{x^2, y^3, x^5, x^6}, y^_]", "False");
    run_test("FreeQ[{1, 2, 4, 1, 0}, 0, {1}]", "False");
    run_test("FreeQ[{1, 2, 4, 1, 0}, 0, {2}]", "True"); // 0 is at level 1 inside List
    run_test("FreeQ[{f[0], 2}, 0, {1}]", "True");
    run_test("FreeQ[{f[0], 2}, 0, {2}]", "False");
    run_test("FreeQ[f[x], f]", "False");
    run_test("FreeQ[f[x], f, Heads -> False]", "True");
}

int main() {
    symtab_init();
    core_init();
    TEST(test_through);
    TEST(test_freeq);
    printf("All funcprog through tests passed!\n");
    return 0;
}
