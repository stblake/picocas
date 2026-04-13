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


void test_median() {
    assert_eval_eq("Median[{1,2,3,4,5,6,7}]", "4", 0);
    assert_eval_eq("Median[{1,2,3,4,5,6,7,8}]", "9/2", 0);
    assert_eval_eq("Median[{1,2,3,4}]", "5/2", 0);
    assert_eval_eq("Median[{Pi,E,2}]", "E", 0);
    assert_eval_eq("Median[{1.,2.,3.,4.}]", "2.5", 0);
    assert_eval_eq("Median[{{1,11,3},{4,6,7}}]", "{5/2, 17/2, 5}", 0);
    assert_eval_eq("Median[{{{3,7},{2,1}},{{5,19},{12,4}}}]", "{{4, 13}, {7, 5/2}}", 0);
    assert_eval_eq("Median[{a,b,c}]", "Median[{a, b, c}]", 0);
}


void test_quartiles() {
    assert_eval_eq("Quartiles[{1,3,4,2,5,6}]", "{2, 7/2, 5}", 0);
    assert_eval_eq("Quartiles[{1,2,3,4}]", "{3/2, 5/2, 7/2}", 0);
    assert_eval_eq("Quartiles[{1.,2.,3.,4.}]", "{1.5, 2.5, 3.5}", 0);
    assert_eval_eq("Quartiles[{-1,5,10,4,25,2,1}]", "{5/4, 4, 35/4}", 0);
    assert_eval_eq("Quartiles[{-1,5,10,4,25,2,1},{{0,0},{1,0}}]", "{1, 4, 10}", 0);
    assert_eval_eq("Quartiles[{{1,11,3},{4,6,7}}]", "{{1, 5/2, 4}, {6, 17/2, 11}, {3, 5, 7}}", 0);
    assert_eval_eq("Quartiles[{{{3,7},{2,1}},{{5,19},{12,4}}}]", "{{{3, 4, 5}, {7, 13, 19}}, {{2, 7, 12}, {1, 5/2, 4}}}", 0);
    assert_eval_eq("Quartiles[{a,b,c}]", "Quartiles[{a, b, c}]", 0);
}

int main() {
    symtab_init();
    core_init();
    TEST(test_quartiles);
    TEST(test_median);
    
    TEST(test_mean);
    TEST(test_variance);
    TEST(test_standard_deviation);
    
    printf("All stats tests passed!\n");
    return 0;
}
