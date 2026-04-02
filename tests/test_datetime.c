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

void run_test(const char* input) {
    Expr* e = parse_expression(input);
    if (!e) {
        printf("Failed to parse: %s\n", input);
        assert(false);
    }
    Expr* res = evaluate(e);
    if (!res) {
        printf("Failed to evaluate: %s\n", input);
        expr_free(e);
        assert(false);
    }
    
    char* s = expr_to_string_fullform(res);
    printf("EVAL: %s -> %s\n", input, s);
    free(s);
    expr_free(e);
    expr_free(res);
}

void test_timing() {
    // Timing should return a list: {time, result}
    // E.g. Timing[Plus[2, 3]] -> {time, 5}
    Expr* e = evaluate(parse_expression("Timing[2 + 3]"));
    assert(e->type == EXPR_FUNCTION);
    assert(strcmp(e->data.function.head->data.symbol, "List") == 0);
    assert(e->data.function.arg_count == 2);
    assert(e->data.function.args[0]->type == EXPR_REAL); // time
    assert(e->data.function.args[1]->type == EXPR_INTEGER);
    assert(e->data.function.args[1]->data.integer == 5);
    expr_free(e);
}

void test_repeated_timing() {
    Expr* e = evaluate(parse_expression("RepeatedTiming[2 + 3]"));
    assert(e->type == EXPR_FUNCTION);
    assert(strcmp(e->data.function.head->data.symbol, "List") == 0);
    assert(e->data.function.arg_count == 2);
    assert(e->data.function.args[0]->type == EXPR_REAL); // time
    assert(e->data.function.args[1]->type == EXPR_INTEGER);
    assert(e->data.function.args[1]->data.integer == 5);
    expr_free(e);
}

int main() {
    symtab_init();
    core_init();
    
    printf("Running datetime tests...\n");
    test_timing();
    test_repeated_timing();
    printf("All datetime tests passed!\n");
    symtab_clear();
    return 0;
}
