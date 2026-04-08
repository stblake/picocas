#include "iter.h"
#include "print.h"
#include "test_utils.h"
#include "symtab.h"
#include "core.h"
#include <stdio.h>

void test_do() {
    assert_eval_eq("Do[x, 3]", "Null", 0);
    assert_eval_eq("Do[x, {i, 2}]", "Null", 0);
    
    // Testing variable substitution
    assert_eval_eq("x = 0; Do[x = x + i, {i, 5}]; x", "15", 0);
    
    // Testing multiple loops
    assert_eval_eq("x = 0; Do[x = x + i * j, {i, 3}, {j, 2}]; x", "18", 0);
    
    // Testing Control Flow (Break)
    assert_eval_eq("x = 0; Do[If[i == 3, Break[]]; x = x + i, {i, 5}]; x", "3", 0);
    
    // Testing Control Flow (Continue)
    assert_eval_eq("x = 0; Do[If[i == 3, Continue[]]; x = x + i, {i, 5}]; x", "12", 0);
    
    // Testing Control Flow (Return)
    assert_eval_eq("Do[If[i == 3, Return[i * 10]], {i, 5}]", "30", 0);
    
    // Testing infinite loop bounded by Break
    assert_eval_eq("x = 0; i = 1; Do[If[i > 5, Break[]]; x = x + i; i = i + 1, Infinity]; x", "15", 0);
}

void test_for() {
    assert_eval_eq("For[i=0, i<4, i=i+1, x]", "Null", 0);
    assert_eval_eq("x = 0; For[i=0, i<4, i=i+1, x = x + i]; x", "6", 0);
    assert_eval_eq("t=1; For[k=1, k<=5, k=k+1, t=t*k; If[k<2, Continue[]]; t=t+2]; t", "292", 0);
    assert_eval_eq("For[i=1, i<1000, i=i+1, If[i>10, Break[]]]; i", "11", 0);
    assert_eval_eq("For[a < b, 1, 0]", "Null", 0);
}

int main() {
    symtab_init();
    core_init();
    
    TEST(test_do);
    TEST(test_for);
    
    printf("All iter tests passed!\n");
    symtab_clear();
    return 0;
}