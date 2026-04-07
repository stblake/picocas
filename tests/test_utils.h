

#pragma once
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

extern struct Expr* parse_expression(const char*);
extern struct Expr* evaluate(struct Expr*);
extern char* expr_to_string(struct Expr*);
extern char* expr_to_string_fullform(struct Expr*);
extern void expr_free(struct Expr*);

static inline void assert_eval_eq(const char* input, const char* expected, int is_fullform) {
    struct Expr* parsed = parse_expression(input);
    assert(parsed != NULL);
    struct Expr* evaluated = evaluate(parsed);
    expr_free(parsed);
    char* str = is_fullform ? expr_to_string_fullform(evaluated) : expr_to_string(evaluated);
    assert(strcmp(str, expected) == 0);
    free(str);
    expr_free(evaluated);
}

#define TEST(name) printf("Running test: %s\n", #name); name()
#define ASSERT(cond) assert(cond)
#define ASSERT_STR_EQ(a, b) assert(strcmp(a, b) == 0)

#define ASSERT_MSG(cond, fmt, ...) do { \
    if (!(cond)) { \
        fprintf(stderr, "Assertion failed: " fmt "\n", ##__VA_ARGS__); \
        fprintf(stderr, "    at %s:%d\n", __FILE__, __LINE__); \
        exit(1); \
    } \
} while(0)

#ifdef __GNUC__
__attribute__((constructor))
static void set_timeout() {
    alarm(10);
}
#endif

