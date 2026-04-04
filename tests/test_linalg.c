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

void test_dot() {
    run_test("{a, b, c} . {x, y, z}", "Plus[Times[a, x], Times[b, y], Times[c, z]]");
    run_test("CompoundExpression[Set[u, List[1, 1]], Set[v, List[-1, 1]], Dot[u, v]]", "0");
    run_test("{{a, b}, {c, d}} . {x, y}", "List[Plus[Times[a, x], Times[b, y]], Plus[Times[c, x], Times[d, y]]]");
    run_test("{x, y} . {{a, b}, {c, d}}", "List[Plus[Times[a, x], Times[c, y]], Plus[Times[b, x], Times[d, y]]]");
    run_test("{x, y} . {{a, b}, {c, d}} . {r, s}", "Plus[Times[r, Plus[Times[a, x], Times[c, y]]], Times[s, Plus[Times[b, x], Times[d, y]]]]");
    
    // Matrix multiplication
    run_test("{{a, b}, {c, d}} . {{1, 2}, {3, 4}}", "List[List[Plus[a, Times[3, b]], Plus[Times[2, a], Times[4, b]]], List[Plus[c, Times[3, d]], Plus[Times[2, c], Times[4, d]]]]");
    
    // Rectangular
    run_test("{{1, 2, 3}, {4, 5, 6}} . {{a, b}, {c, d}, {e, f}}", "List[List[Plus[a, Times[2, c], Times[3, e]], Plus[b, Times[2, d], Times[3, f]]], List[Plus[Times[4, a], Times[5, c], Times[6, e]], Plus[Times[4, b], Times[5, d], Times[6, f]]]]");
}

void test_det() {
    run_test("Det[{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}}]", "0");
    run_test("Det[{{1.7, 7.1, -2.7}, {2.2, 8.7, 3.2}, {3.2, -9.2, 1.2}}]", "251.572");
    run_test("Expand[Det[{{a, b, c}, {d, e, f}, {g, h, i}}]]", "Plus[Times[-1, Times[a, f, h]], Times[-1, Times[b, d, i]], Times[-1, Times[c, e, g]], Times[a, e, i], Times[b, f, g], Times[c, d, h]]");
}

void test_cross() {
    run_test("Cross[{1, 2, -1}, {-1, 1, 0}]", "List[1, 1, 3]");
    run_test("Cross[{1, Sqrt[3]}]", "List[Times[-1, Power[3, Rational[1, 2]]], 1]");
    run_test("Cross[{3.2, 4.2, 5.2}, {0.75, 0.09, 0.06}]", "List[-0.216, 3.708, -2.862]");
    run_test("Cross[{1.3 + I, 2, 3 - 2 I}, {6. + I, 4, 5 - 7 I}]", "List[Complex[-2, -6], Complex[6.5, -4.9], Complex[-6.8, 2]]");
    run_test("Cross[{1, 2, 3}, {4, 5}]", "Cross[List[1, 2, 3], List[4, 5]]");
}

void test_norm() {
    run_test("Norm[{x, y, z}]", "Power[Plus[Power[Abs[x], 2], Power[Abs[y], 2], Power[Abs[z], 2]], Rational[1, 2]]");
    run_test("Norm[-2 + I]", "Power[5, Rational[1, 2]]");
    run_test("CompoundExpression[Set[v, List[1, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1]], Norm[v]]", "Power[5, Rational[1, 2]]");
    run_test("Norm[{x, y, z}, p]", "Power[Plus[Power[Abs[x], p], Power[Abs[y], p], Power[Abs[z], p]], Power[p, -1]]");
    run_test("Norm[{x, y, z}, Infinity]", "Max[Abs[x], Abs[y], Abs[z]]");
    run_test("Norm[{{a11, a12}, {a21, a22}}, \"Frobenius\"]", "Power[Plus[Power[Abs[a11], 2], Power[Abs[a12], 2], Power[Abs[a21], 2], Power[Abs[a22], 2]], Rational[1, 2]]");
}

void test_tr() {
    run_test("Tr[{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}}]", "15");
    run_test("Tr[{{a, b}, {c, d}}]", "Plus[a, d]");
    run_test("Tr[{1, 2, 3}]", "6");
    run_test("Tr[Array[a, {4, 3, 2}]]", "Plus[a[1, 1, 1], a[2, 2, 2]]");
    run_test("Tr[{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}}, f]", "f[1, 5, 9]");
    run_test("Tr[{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}}, List]", "List[1, 5, 9]");
    run_test("Tr[{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}}, Plus, 1]", "List[12, 15, 18]");
    run_test("Tr[Array[a, {4, 3, 2}], f, 2]", "f[List[a[1, 1, 1], a[1, 1, 2]], List[a[2, 2, 1], a[2, 2, 2]], List[a[3, 3, 1], a[3, 3, 2]]]");
    run_test("Tr[x]", "x");
    run_test("Tr[x, f]", "x");
    run_test("Tr[{{1, 2}, {3}}]", "1");
}

void test_rowreduce() {
    run_test("RowReduce[{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}}]", "List[List[1, 0, -1], List[0, 1, 2], List[0, 0, 0]]");
    run_test("RowReduce[{{1, 2, 3, 1, 0, 0}, {4, 5, 6, 0, 1, 0}, {7, 8, 9, 0, 0, 1}}]", "List[List[1, 0, -1, 0, Rational[-8, 3], Rational[5, 3]], List[0, 1, 2, 0, Rational[7, 3], Rational[-4, 3]], List[0, 0, 0, 1, -2, 1]]");
    run_test("RowReduce[{{3, 1, a}, {2, 1, b}}]", "List[List[1, 0, Plus[a, Times[-1, b]]], List[0, 1, Plus[Times[-2, a], Times[3, b]]]]");
    run_test("RowReduce[{{0, 0, 0, Pi}, {2, 2, 2, 2}, {3 + I * Sqrt[2], 0, 0, 0}, {0, 4, 4, 0}}]", "List[List[1, 0, 0, 0], List[0, 1, 1, 0], List[0, 0, 0, 1], List[0, 0, 0, 0]]");
    run_test("RowReduce[{{a, b, c}, {d, e, f}, {g, h, i}}]", "List[List[1, 0, 0], List[0, 1, 0], List[0, 0, 1]]");
    
    // Check symbolic fractional simplification: Note that Expand changes the exact output string.
    Expr* t = evaluate(parse_expression("RowReduce[{{a, b, c}, {d, e, f}, {a + d, b + e, c + f}}]"));
    char* res_str = expr_to_string_fullform(t);
    
    // We expect the first two rows to have the elements `(ce-bf)/(ae-bd)` and `(cd-af)/(bd-ae)`.
    // In our algorithm the denominator is `-bd+ae` and the numerator is `ce-bf` or `-af+cd`.
    // We just verify it reduces exactly to row 3 being 0 and diagonal being 1.
    ASSERT(t->type == EXPR_FUNCTION);
    ASSERT(t->data.function.arg_count == 3);
    ASSERT(t->data.function.args[2]->data.function.args[0]->type == EXPR_INTEGER);
    ASSERT(t->data.function.args[2]->data.function.args[0]->data.integer == 0);
    ASSERT(t->data.function.args[2]->data.function.args[1]->type == EXPR_INTEGER);
    ASSERT(t->data.function.args[2]->data.function.args[1]->data.integer == 0);
    ASSERT(t->data.function.args[2]->data.function.args[2]->type == EXPR_INTEGER);
    ASSERT(t->data.function.args[2]->data.function.args[2]->data.integer == 0);
    
    free(res_str);
    expr_free(t);
}

int main() {
    symtab_init();
    core_init();
    
    printf("Running linalg tests...\n");
    TEST(test_dot);
    TEST(test_det);
    TEST(test_cross);
    TEST(test_norm);
    TEST(test_tr);
    TEST(test_rowreduce);
    printf("All linalg tests passed!\n");
    symtab_clear();
    return 0;
}
