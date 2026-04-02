
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

void test_trig_forward() {
    struct {
        const char* input;
        const char* expected;
    } cases[] = {
        {"Sin[0]", "0"},
        {"Sin[Pi/2]", "1"},
        {"Sin[Pi/6]", "Rational[1, 2]"},
        {"Sin[Pi/4]", "Power[2, Rational[-1, 2]]"},
        {"Sin[Pi/3]", "Times[Rational[1, 2], Power[3, Rational[1, 2]]]"},
        {"Cos[0]", "1"},
        {"Cos[Pi/2]", "0"},
        {"Cos[Pi/3]", "Rational[1, 2]"},
        {"Tan[Pi/4]", "1"},
        {"Cot[Pi/4]", "1"},
        {"Sec[0]", "1"},
        {"Csc[Pi/2]", "1"},
        // Exact values for d=12
        {"Sin[Pi/12]", "Times[Rational[1, 4], Plus[Times[-1, Power[2, Rational[1, 2]]], Power[6, Rational[1, 2]]]]"},
        {"Cos[Pi/12]", "Times[Rational[1, 4], Plus[Power[2, Rational[1, 2]], Power[6, Rational[1, 2]]]]"},
        // Exact values for d=10, 5
        {"Sin[Pi/10]", "Times[Rational[1, 4], Plus[-1, Power[5, Rational[1, 2]]]]"},
        {"Cos[Pi/5]", "Times[Rational[1, 4], Plus[1, Power[5, Rational[1, 2]]]]"},
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

void test_trig_inverse() {
    struct {
        const char* input;
        const char* expected;
    } cases[] = {
        {"ArcSin[0]", "0"},
        {"ArcSin[1]", "Times[Rational[1, 2], Pi]"},
        {"ArcSin[1/2]", "Times[Rational[1, 6], Pi]"},
        {"ArcCos[0]", "Times[Rational[1, 2], Pi]"},
        {"ArcCos[1]", "0"},
        {"ArcTan[0]", "0"},
        {"ArcTan[1]", "Times[Rational[1, 4], Pi]"},
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

int main() {
    symtab_init();
    core_init();
    
    TEST(test_trig_forward);
    TEST(test_trig_inverse);
    
    return 0;
}
