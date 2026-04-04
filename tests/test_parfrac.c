#include "expr.h"
#include "parse.h"
#include "eval.h"
#include "symtab.h"
#include "core.h"
#include "print.h"
#include "test_utils.h"

void test_apart_basic(void) {
    struct {
        const char* input;
        const char* expected;
    } tests[] = {
        {"Apart[1/((1+x)(5+x))]", "1/(4 (1 + x)) - 1/(4 (5 + x))"},
        {"Apart[(x^5-2)/((1+x+x^2)(2+x)(1-x))]", "2 - x + (-1 - x/3)/(-1 - x - x^2) + (4 - (11 x)/3)/(2 - x - x^2)"},
        {"Apart[16x/((1+x)^2*(5+x))]", "-4/(1 + x)^2 + 5/(1 + x) - 5/(5 + x)"},
        {"Apart[1/((x+a)*(x+b)*(x+c))]", "1/((a + x) (-a + b) (-a + c)) + 1/((-a + c) (-b + c) (c + x)) - 1/((-a + b) (b + x) (-b + c))"},
        {"Apart[(x+y)/((x+1)(y+1)(x-y)),x]", "2 y/((-1 - y)^2 (x - y)) - (-1 + y)/((-1 - y)^2 (1 + x))"},
        {"Apart[(x+y)/((x+1)(y+1)(x-y)),y]", "2 x/((1 + x)^2 (x - y)) + (-1 + x)/((1 + x)^2 (1 + y))"},
        {"Apart[(x+y)/((x+1)(y+1)(x-y))]", "2 x/((1 + x)^2 (x - y)) + (-1 + x)/((1 + x)^2 (1 + y))"},
    };

    for (int i = 0; i < (int)(sizeof(tests) / sizeof(tests[0])); i++) {
        Expr* e = parse_expression(tests[i].input);
        Expr* res = evaluate(e);
        char* res_str = expr_to_string(res);
        if (strcmp(res_str, tests[i].expected) != 0) {
            printf("FAIL: %s\n  expected: %s\n  got:      %s\n", tests[i].input, tests[i].expected, res_str);
            ASSERT(false);
        } else {
            printf("PASS: %s -> %s\n", tests[i].input, tests[i].expected);
        }
        free(res_str);
        expr_free(e);
        expr_free(res);
    }
}

int main() {
    symtab_init();
    core_init();
    
    printf("Running parfrac tests...\n");
    TEST(test_apart_basic);
    printf("All parfrac tests passed!\n");
    return 0;
}