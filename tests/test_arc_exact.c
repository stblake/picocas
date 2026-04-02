
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

void test_arc_exact() {
    struct {
        const char* input;
        const char* expected;
    } cases[] = {
        {"ArcSin[Sqrt[5/8 + -1/8*Sqrt[5]]]", "Times[Rational[1, 5], Pi]"},
        {"ArcCos[1/4*(1 + Sqrt[5])]", "Times[Rational[1, 5], Pi]"},
        {"ArcSin[1/2]", "Times[Rational[1, 6], Pi]"},
        {"ArcTan[1]", "Times[Rational[1, 4], Pi]"},
        {"ArcTan[2 - Sqrt[3]]", "Times[Rational[1, 12], Pi]"},
        {"ArcSec[2]", "Times[Rational[1, 3], Pi]"},
        {"ArcCsc[Sqrt[2]]", "Times[Rational[1, 4], Pi]"},
        {"ArcCot[2 + Sqrt[3]]", "Times[Rational[1, 12], Pi]"},
        {NULL, NULL}
    };

    for (int i = 0; cases[i].input != NULL; i++) {
        Expr* e = parse_expression(cases[i].input);
        Expr* res = evaluate(e);
        char* s = expr_to_string_fullform(res);
        
        // printf("Input: %s\nOutput: %s\nExpected: %s\n\n", cases[i].input, s, cases[i].expected);
        ASSERT_MSG(strcmp(s, cases[i].expected) == 0, "Case %s: expected %s, got %s", cases[i].input, cases[i].expected, s);
        
        free(s);
        expr_free(e);
        expr_free(res);
    }
}

int main() {
    symtab_init();
    core_init();
    
    TEST(test_arc_exact);
    
    return 0;
}
