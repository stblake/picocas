#include "expr.h"
#include "eval.h"
#include "core.h"
#include "symtab.h"
#include "test_utils.h"
#include "parse.h"
#include <string.h>
#include <stdlib.h>

void test_block() {
    evaluate(parse_expression("x = 1"));
    Expr* e1 = parse_expression("Block[{x = 2}, x + 1]");
    Expr* res1 = evaluate(e1);
    ASSERT(res1->type == EXPR_INTEGER);
    ASSERT(res1->data.integer == 3);
    
    Expr* res2 = evaluate(parse_expression("x"));
    ASSERT(res2->data.integer == 1);
    
    expr_free(e1); expr_free(res1); expr_free(res2);
}

void test_module() {
    evaluate(parse_expression("x = 1"));
    // Module should rename local x and not affect global x
    Expr* e1 = parse_expression("Module[{x = 10}, x + 5]");
    Expr* res1 = evaluate(e1);
    ASSERT(res1->data.integer == 15);
    
    Expr* res2 = evaluate(parse_expression("x"));
    ASSERT(res2->data.integer == 1);
    
    expr_free(e1); expr_free(res1); expr_free(res2);
}

void test_with() {
    evaluate(parse_expression("x = 1"));
    Expr* e1 = parse_expression("With[{x = 100}, x / 2]");
    Expr* res1 = evaluate(e1);
    ASSERT(res1->data.integer == 50);
    
    Expr* res2 = evaluate(parse_expression("x"));
    ASSERT(res2->data.integer == 1);
    
    expr_free(e1); expr_free(res1); expr_free(res2);
}

int main() {
    symtab_init();
    core_init();
    
    TEST(test_block);
    TEST(test_module);
    TEST(test_with);
    
    printf("All modular tests passed!\n");
    return 0;
}
