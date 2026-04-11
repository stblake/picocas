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
#include <unistd.h>

void test_print_basic() {
    // We can't easily capture stdout here without pipe/dup, 
    // but we can check if it returns Null.
    Expr* e = parse_expression("Print[1, \" \", x + y]");
    Expr* res = evaluate(e);
    ASSERT(res != NULL);
    ASSERT(res->type == EXPR_SYMBOL);
    ASSERT(strcmp(res->data.symbol, "Null") == 0);
    expr_free(e);
    expr_free(res);
}

void test_fullform_wrapper() {
    Expr* e = parse_expression("Print[FullForm[x + y]]");
    Expr* res = evaluate(e);
    ASSERT(res != NULL);
    ASSERT(strcmp(res->data.symbol, "Null") == 0);
    expr_free(e);
    expr_free(res);
}

void test_inputform_wrapper() {
    Expr* e = parse_expression("Print[InputForm[x + y]]");
    Expr* res = evaluate(e);
    ASSERT(res != NULL);
    ASSERT(strcmp(res->data.symbol, "Null") == 0);
    expr_free(e);
    expr_free(res);
}

int main() {
    symtab_init();
    core_init();
    
    TEST(test_print_basic);
    TEST(test_fullform_wrapper);
    TEST(test_inputform_wrapper);
    
    printf("All print tests passed!\n");
    return 0;
}
