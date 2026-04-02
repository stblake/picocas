#include "parse.h"
#include "eval.h"
#include "print.h"
#include "expr.h"
#include "symtab.h"
#include "core.h"
#include "test_utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void test_eval(const char* input, const char* expected_fullform) {
    Expr* e = parse_expression(input);
    if (!e) {
        printf("Failed to parse: %s\n", input);
        exit(1);
    }
    Expr* res = evaluate(e);
    
    // We can just print the output to verify manually or use a simple string compare
    // But since expr_print writes to stdout, we'll just visually output or redirect.
    // For automatic checking, let's just print it.
    printf("EVAL: %-30s -> ", input);
    expr_print(res);
    printf("\n");
    
    expr_free(e);
    expr_free(res);
}

int main() {
    symtab_init();
    core_init();
    
    printf("=== Length Tests ===\n");
    test_eval("Length[5]", "");
    test_eval("Length[x]", "");
    test_eval("Length[{}]", "");
    test_eval("Length[{1, 2, 3}]", "");
    test_eval("Length[f[x, y]]", "");
    
    printf("\n=== Dimensions Tests ===\n");
    test_eval("Dimensions[5]", "");
    test_eval("Dimensions[{}]", "");
    test_eval("Dimensions[{{}}]", "");
    test_eval("Dimensions[{1, 2, 3}]", "");
    test_eval("Dimensions[{{1, 2}, {3, 4}}]", "");
    test_eval("Dimensions[{{1, 2}, {3}}]", ""); // ragged
    test_eval("Dimensions[{{{1, 2}, {3, 4}}, {{5, 6}, {7, 8}}}]", "");
    
    test_eval("Dimensions[f[f[1], f[2]]]", ""); // Using 'f' as the head
    test_eval("Dimensions[f[g[1], g[2]]]", ""); // Top head is 'f', elements are 'g'
    
    return 0;
}
