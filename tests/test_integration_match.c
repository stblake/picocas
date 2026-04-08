#include "core.h"
#include "test_utils.h"
#include "expr.h"
#include "eval.h"
#include "parse.h"
#include "symtab.h"
#include "match.h"
#include "print.h"
#include <stdio.h>
#include <string.h>

void test_integration_match() {
    symtab_init();
    core_init();
    
    // Define the rule: int[(b_. x_ + a_.)^n_, x] /; FreeQ[{a, b, n}, x] && n =!= -1 := ((a + b x)^(n + 1))/(b*(n + 1))
    
    // In PicoCAS, we can use evaluate(parse_expression("... := ..."))
    Expr* rule = parse_expression("int[(b_. x_ + a_.)^n_, x] /; FreeQ[{a, b, n}, x] && n =!= -1 := ((a + b x)^(n + 1))/(b*(n + 1))");
    ASSERT(rule != NULL);
    Expr* rule_res = evaluate(rule);
    expr_free(rule);
    expr_free(rule_res);
    
    // Now evaluate int[(5 x + 1)^3, x]
    Expr* input = parse_expression("int[(5 x + 1)^3, x]");
    ASSERT(input != NULL);
    Expr* output = evaluate(input);
    ASSERT(output != NULL);
    
    char* s = expr_to_string(output);
    printf("Result: %s\n", s);
    
    // Expected result: (1 + 5 x)^4 / 20
    // PicoCAS prints it as "(1 + 5 x)^4/20"
    ASSERT_STR_EQ(s, "(1 + 5 x)^4/20");

    
    free(s);
    expr_free(input);
    expr_free(output);
    
    Expr* input2 = parse_expression("int[(4 x + 5)^2, x]");
    ASSERT(input2 != NULL);
    Expr* output2 = evaluate(input2);
    ASSERT(output2 != NULL);
    
    char* s2 = expr_to_string(output2);
    printf("Result 2: %s\n", s2);
    ASSERT_STR_EQ(s2, "(5 + 4 x)^3/12");
    
    free(s2);
    expr_free(input2);
    expr_free(output2);
    
    symtab_clear();
}

int main() {
    TEST(test_integration_match);
    return 0;
}
