#include "expr.h"
#include "eval.h"
#include "core.h"
#include "symtab.h"
#include "test_utils.h"
#include "parse.h"
#include <string.h>
#include <stdlib.h>
#include "print.h"

void run_test(const char* input, const char* expected) {
    Expr* e = parse_expression(input);
    Expr* res = evaluate(e);
    char* res_str = expr_to_string_fullform(res);
    if (strcmp(res_str, expected) != 0) {
        printf("FAIL: %s\n  expected: %s\n  got:      %s\n", input, expected, res_str);
        ASSERT_STR_EQ(res_str, expected);
    }
    free(res_str);
    expr_free(e);
    expr_free(res);
}

void test_block() {
    Expr* te = parse_expression("x = 1");
    Expr* tr = evaluate(te);
    expr_free(te); expr_free(tr);
    
    Expr* e1 = parse_expression("Block[{x = 2}, x + 1]");
    Expr* res1 = evaluate(e1);
    ASSERT(res1->type == EXPR_INTEGER);
    ASSERT(res1->data.integer == 3);
    
    Expr* e2 = parse_expression("x");
    Expr* res2 = evaluate(e2);
    ASSERT(res2->data.integer == 1);
    
    expr_free(e1); expr_free(res1); 
    expr_free(e2); expr_free(res2);
}

void test_module() {
    Expr* te = parse_expression("x = 1");
    Expr* tr = evaluate(te);
    expr_free(te); expr_free(tr);

    // Module should rename local x and not affect global x
    Expr* e1 = parse_expression("Module[{x = 10}, x + 5]");
    Expr* res1 = evaluate(e1);
    ASSERT(res1->data.integer == 15);
    
    Expr* e2 = parse_expression("x");
    Expr* res2 = evaluate(e2);
    ASSERT(res2->data.integer == 1);
    
    expr_free(e1); expr_free(res1);
    expr_free(e2); expr_free(res2);
}

void test_with() {
    Expr* te = parse_expression("x = 1");
    Expr* tr = evaluate(te);
    expr_free(te); expr_free(tr);

    Expr* e1 = parse_expression("With[{x = 100}, x / 2]");
    Expr* res1 = evaluate(e1);
    ASSERT(res1->data.integer == 50);
    
    Expr* e2 = parse_expression("x");
    Expr* res2 = evaluate(e2);
    ASSERT(res2->data.integer == 1);
    
    expr_free(e1); expr_free(res1);
    expr_free(e2); expr_free(res2);
}

void test_powermod() {
    run_test("PowerMod[2, 10, 3]", "1");
    run_test("PowerMod[3, 2, 7]", "2");
    run_test("PowerMod[3, -2, 7]", "4");
    run_test("PowerMod[2, {10, 11, 12, 13, 14}, 5]", "List[4, 3, 1, 2, 4]");
    run_test("PowerMod[3, -1, 7]", "5");
    run_test("PowerMod[3, -1, 6]", "PowerMod[3, -1, 6]"); // no inverse
}

int main() {
    symtab_init();
    core_init();
    
    TEST(test_block);
    TEST(test_module);
    TEST(test_with);
    TEST(test_powermod);
    
    printf("All modular tests passed!\n");
    return 0;
}
