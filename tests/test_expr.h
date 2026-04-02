
#ifndef TEST_EXPR_H
#define TEST_EXPR_H

#include "expr.h"  // Include the main header being tested

// Test function declarations
void test_integer_expr(void);
void test_real_expr(void);
void test_symbol_expr(void);
void test_string_expr(void);
void test_function_expr(void);
void test_nested_free(void);
void test_memory_cleanup(void);

void test_print_basic();
void test_print_nested();

// Test utilities
#define TEST_ASSERT(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "Test failed at %s:%d\n", __FILE__, __LINE__); \
        exit(1); \
    } \
} while(0)

#define TEST_ASSERT_STR_EQ(a, b) TEST_ASSERT(strcmp((a), (b)) == 0)

#endif // TEST_EXPR_H
