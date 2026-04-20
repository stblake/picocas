
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

void test_hyperbolic_forward() {
    struct {
        const char* input;
        const char* expected;
    } cases[] = {
        {"Sinh[0]", "0"},
        {"Cosh[0]", "1"},
        {"Tanh[0]", "0"},
        {"Sinh[Infinity]", "Infinity"},
        {"Cosh[Infinity]", "Infinity"},
        {"Sinh[-Infinity]", "Times[-1, Infinity]"},
        {"Tanh[Infinity]", "1"},
        {"Tanh[-Infinity]", "-1"},
        {"Coth[Infinity]", "1"},
        {"Sech[Infinity]", "0"},
        {"Csch[Infinity]", "0"},
        {NULL, NULL}
    };

    for (int i = 0; cases[i].input != NULL; i++) {
        Expr* e = parse_expression(cases[i].input);
        Expr* res = evaluate(e);
        char* s = expr_to_string_fullform(res);
        ASSERT_MSG(strcmp(s, cases[i].expected) == 0, "Forward %s: expected %s, got %s", cases[i].input, cases[i].expected, s);
        free(s);
        expr_free(e);
        expr_free(res);
    }
}

void test_hyperbolic_inverse() {
    struct {
        const char* input;
        const char* expected;
    } cases[] = {
        {"ArcSinh[0]", "0"},
        {"ArcCosh[1]", "0"},
        {"ArcTanh[0]", "0"},
        {"ArcCoth[Infinity]", "0"},
        {"ArcSech[1]", "0"},
        {"ArcSinh[Infinity]", "Infinity"},
        {NULL, NULL}
    };

    for (int i = 0; cases[i].input != NULL; i++) {
        Expr* e = parse_expression(cases[i].input);
        Expr* res = evaluate(e);
        char* s = expr_to_string_fullform(res);
        ASSERT_MSG(strcmp(s, cases[i].expected) == 0, "Inverse %s: expected %s, got %s", cases[i].input, cases[i].expected, s);
        free(s);
        expr_free(e);
        expr_free(res);
    }
}

void test_hyperbolic_forward_of_inverse() {
    /* f[f_inv[x]] = x for each direct hyperbolic / inverse-hyperbolic
     * pair. Identity holds over the complex numbers since each ArcXh is a
     * right inverse of Xh by construction. */
    struct {
        const char* input;
        const char* expected;
    } cases[] = {
        {"Sinh[ArcSinh[x]]", "x"},
        {"Cosh[ArcCosh[y]]", "y"},
        {"Tanh[ArcTanh[z]]", "z"},
        {"Coth[ArcCoth[w]]", "w"},
        {"Sech[ArcSech[u]]", "u"},
        {"Csch[ArcCsch[v]]", "v"},
        {"Sinh[ArcSinh[x^2 - 1]]", "-1 + x^2"},
        /* Opposite direction is NOT folded */
        {"ArcSinh[Sinh[x]]", "ArcSinh[Sinh[x]]"},
        {NULL, NULL}
    };

    for (int i = 0; cases[i].input != NULL; i++) {
        Expr* e = parse_expression(cases[i].input);
        Expr* res = evaluate(e);
        char* s = expr_to_string(res);
        ASSERT_MSG(strcmp(s, cases[i].expected) == 0,
                   "Forward-of-inverse %s: expected %s, got %s",
                   cases[i].input, cases[i].expected, s);
        free(s);
        expr_free(e);
        expr_free(res);
    }
}

int main() {
    symtab_init();
    core_init();

    TEST(test_hyperbolic_forward);
    TEST(test_hyperbolic_inverse);
    TEST(test_hyperbolic_forward_of_inverse);

    return 0;
}
