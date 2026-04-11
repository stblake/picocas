#include "cond.h"
#include "print.h"
#include "test_utils.h"
#include "symtab.h"
#include "core.h"
#include <stdio.h>

void test_if() {
    assert_eval_eq("If[True, 1, 0]", "1", 0);
    assert_eval_eq("If[False, 1, 0]", "0", 0);
    assert_eval_eq("If[a < b, 1, 0]", "If[a < b, 1, 0]", 0);
    assert_eval_eq("If[a < b, 1, 0, Indeterminate]", "Indeterminate", 0);
    assert_eval_eq("If[False, 1]", "Null", 0);
    
    assert_eval_eq("abs[x_]:=If[x<0,-x,x]", "Null", 0);
    assert_eval_eq("abs /@ {-1, 0, 1}", "{1, 0, 1}", 0);
}

void test_trueq() {
    assert_eval_eq("TrueQ[True]", "True", 0);
    assert_eval_eq("TrueQ[False]", "False", 0);
    assert_eval_eq("TrueQ[a < b]", "False", 0);
    assert_eval_eq("If[TrueQ[a < b], 1, 0]", "0", 0);
}

int main() {
    symtab_init();
    core_init();
    
    TEST(test_if);
    TEST(test_trueq);
    
    printf("All cond tests passed!\n");
    symtab_clear();
    return 0;
}