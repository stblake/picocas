#include "expr.h"
#include "eval.h"
#include "core.h"
#include "symtab.h"
#include "test_utils.h"
#include "parse.h"
#include "print.h"
#include <string.h>
#include <stdlib.h>

void test_mean() {
    struct {
        const char* input;
        const char* expected;
    } tests[] = {
        {"Mean[{1, 2, 3, 4}]", "5/2"},
        {"Mean[{1.2, 2.8}]", "2.0"},
        {"Mean[{a, b, c, d}]", "1/4 (a + b + c + d)"},
        {"Mean[{{a, u}, {b, v}, {c, w}}]", "{1/3 (a + b + c), 1/3 (u + v + w)}"}
    };

    for (int i = 0; i < (int)(sizeof(tests) / sizeof(tests[0])); i++) {
        Expr* e = parse_expression(tests[i].input);
        Expr* res = evaluate(e);
        char* res_str = expr_to_string(res);
        if (strcmp(res_str, tests[i].expected) != 0) {
            printf("Mean test failed: %s expected %s, got %s\n", tests[i].input, tests[i].expected, res_str);
            ASSERT(0);
        }
        free(res_str);
        expr_free(e);
        expr_free(res);
    }
}

void test_variance() {
    struct {
        const char* input;
        const char* expected;
    } tests[] = {
        {"Variance[{1, 2, 3}]", "1"},
        {"Variance[{1, 2, 3, 4}]", "5/3"},
        {"Variance[{{5.2, 7}, {5.3, 8}, {5.4, 9}}]", "{0.01, 1}"}
    };

    for (int i = 0; i < (int)(sizeof(tests) / sizeof(tests[0])); i++) {
        Expr* e = parse_expression(tests[i].input);
        Expr* res = evaluate(e);
        char* res_str = expr_to_string(res);
        if (strcmp(res_str, tests[i].expected) != 0) {
            printf("Variance test failed: %s expected %s, got %s\n", tests[i].input, tests[i].expected, res_str);
            ASSERT(0);
        }
        free(res_str);
        expr_free(e);
        expr_free(res);
    }
}

void test_standard_deviation() {
    struct {
        const char* input;
        const char* expected;
    } tests[] = {
        {"StandardDeviation[{1, 2, 3}]", "1"},
        {"StandardDeviation[{{5.2, 7}, {5.3, 8}, {5.4, 9}}]", "{0.1, 1}"}
    };

    for (int i = 0; i < (int)(sizeof(tests) / sizeof(tests[0])); i++) {
        Expr* e = parse_expression(tests[i].input);
        Expr* res = evaluate(e);
        char* res_str = expr_to_string(res);
        if (strcmp(res_str, tests[i].expected) != 0) {
            printf("StandardDeviation test failed: %s expected %s, got %s\n", tests[i].input, tests[i].expected, res_str);
            ASSERT(0);
        }
        free(res_str);
        expr_free(e);
        expr_free(res);
    }
}

int main() {
    symtab_init();
    core_init();
    
    TEST(test_mean);
    TEST(test_variance);
    TEST(test_standard_deviation);
    
    printf("All stats tests passed!\n");
    return 0;
}
