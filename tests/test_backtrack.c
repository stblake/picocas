#include "test_utils.h"
#include "match.h"
#include "expr.h"
#include "parse.h"
#include "symtab.h"
#include "core.h"
#include "print.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void test_backtrack_condition() {
    symtab_init();
    core_init();
    
    // MatchQ[Range[10], {1, ___, n_, ___, 5, ___, 10} /; EvenQ[n]] -> True
    const char* input1 = "MatchQ[Range[10], List[1, BlankNullSequence[], Pattern[n, Blank[]], BlankNullSequence[], 5, BlankNullSequence[], 10] /; EvenQ[n]]";
    Expr* e1 = parse_expression(input1);
    Expr* res1 = evaluate(e1);
    char* s1 = expr_to_string_fullform(res1);
    printf("Result 1: %s\n", s1);
    ASSERT(strcmp(s1, "True") == 0);
    free(s1); expr_free(e1); expr_free(res1);

    // MatchQ[Range[10], {1, _, n_, ___, 5, ___, 10} /; EvenQ[n]] -> False
    const char* input2 = "MatchQ[Range[10], List[1, Blank[], Pattern[n, Blank[]], BlankNullSequence[], 5, BlankNullSequence[], 10] /; EvenQ[n]]";
    Expr* e2 = parse_expression(input2);
    Expr* res2 = evaluate(e2);
    char* s2 = expr_to_string_fullform(res2);
    printf("Result 2: %s\n", s2);
    ASSERT(strcmp(s2, "False") == 0);
    free(s2); expr_free(e2); expr_free(res2);
}

int main() {
    TEST(test_backtrack_condition);
    return 0;
}
