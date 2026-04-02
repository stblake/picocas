#include <stdio.h>
#include <stdlib.h>
#include "parse.h"
#include "expr.h"
#include "print.h"

int main() {
    Expr* e1 = parse_expression("body&");
    Expr* e2 = parse_expression("body&[x]");
    Expr* e3 = parse_expression("3 + # & [x]");
    Expr* e4 = parse_expression("f[##, #2]");
    
    char* s1 = expr_to_string_fullform(e1);
    char* s2 = expr_to_string_fullform(e2);
    char* s3 = expr_to_string_fullform(e3);
    char* s4 = expr_to_string_fullform(e4);
    
    printf("1: %s\n2: %s\n3: %s\n4: %s\n", s1, s2, s3, s4);
    
    free(s1); free(s2); free(s3); free(s4);
    expr_free(e1); expr_free(e2); expr_free(e3); expr_free(e4);
    return 0;
}
