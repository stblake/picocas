#include "list.h"
#include "eval.h"
#include "parse.h"
#include "expr.h"
#include "symtab.h"
#include "core.h"
#include "print.h"
#include "test_utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void run_test(const char* input, const char* expected) {
    Expr* e = parse_expression(input);
    Expr* res = evaluate(e);
    char* s = expr_to_string_fullform(res);
    if (strcmp(s, expected) != 0) {
        printf("FAIL: %s\n  expected: %s\n  got:      %s\n", input, expected, s);
    }
    ASSERT_MSG(strcmp(s, expected) == 0, "%s: expected %s, got %s", input, expected, s);
    free(s);
    expr_free(e); expr_free(res);
}

void test_split() {
    run_test("Split[{a,a,a,b,b,a,a,c}]", "List[List[a, a, a], List[b, b], List[a, a], List[c]]");
    run_test("Split[{1,2,3,4,3,2,1,5,6,7,4,3}, Less]", "List[List[1, 2, 3, 4], List[3], List[2], List[1, 5, 6, 7], List[4], List[3]]");
    run_test("Split[{1,2,3,4,3,2,1,5,6,7,4,3}, Greater]", "List[List[1], List[2], List[3], List[4, 3, 2, 1], List[5], List[6], List[7, 4, 3]]");
    run_test("Split[{a,a,a,b,a,b,b,a,a,a,c,a}, UnsameQ]", "List[List[a], List[a], List[a, b, a, b], List[b, a], List[a], List[a, c, a]]");
}

int main() {
    symtab_init();
    core_init();
    TEST(test_split);
    printf("All split tests passed!\n");
    return 0;
}
