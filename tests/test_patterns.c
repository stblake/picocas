#include "patterns.h"
#include "print.h"
#include "test_utils.h"
#include "symtab.h"
#include "core.h"
#include <stdio.h>

void test_cases() {
    assert_eval_eq("Cases[{1, 1, f[a], 2, 3, y, f[8], 9, f[10]}, _Integer]", "{1, 1, 2, 3, 9}", 0);
    assert_eval_eq("Cases[{1, 1, f[a], 2, 3, y, f[8], 9, f[10]}, f[x_] -> x]", "{a, 8, 10}", 0);
    assert_eval_eq("Cases[{{1, 2}, {2}, {3, 4, 1}, {5, 4}, {3, 3}}, {_, _}]", "{{1, 2}, {5, 4}, {3, 3}}", 0);
    assert_eval_eq("Cases[{{1, 4, a, 0}, {b, 3, 2, 2}, {c, c, 5, 5}}, _Integer, 2]", "{1, 4, 0, 3, 2, 2, 5, 5}", 0);
    assert_eval_eq("Cases[{f[{a, b}], f[{a}], g[{a}], f[{a, b, c, d}]}, f[x_] :> Length[x]]", "{2, 1, 4}", 0);
}

void test_position() {
    assert_eval_eq("Position[{a,b,a,a,b,c,b},b]", "{{2}, {5}, {7}}", 0);
    assert_eval_eq("Position[{{a,a,b},{b,a,a},{a,b,a}},b]", "{{1, 3}, {2, 1}, {3, 2}}", 0);
    assert_eval_eq("Position[{1+x^2,5,x^4,a+(1+x^2)^2},x^_]", "{{1, 2}, {3}, {4, 2, 1, 2}}", 0);
    assert_eval_eq("Position[{1+x^2,5,x^4,a+(1+x^2)^2},x^_,2]", "{{1, 2}, {3}}", 0);
    assert_eval_eq("Position[{a,b,a,a,b,c,b,a,b},b,1,2]", "{{2}, {5}}", 0);
    assert_eval_eq("Position[f[g[h[x]]],_,Infinity]", "{{0}, {1, 0}, {1, 1, 0}, {1, 1, 1}, {1, 1}, {1}}", 0);
    assert_eval_eq("Position[f[a,b,b,a],b]", "{{2}, {3}}", 0);
    assert_eval_eq("Position[x^2+y^2,Power]", "{{1, 0}, {2, 0}}", 0);
    assert_eval_eq("Position[x^2+y^2,Power,Heads->False]", "{}", 0);
}

void test_count() {
    assert_eval_eq("Count[{a,b,a,a,b,c,b},b]", "3", 0);
    assert_eval_eq("Count[{a,2,a,a,1,c,b,3,3},_Integer]", "4", 0);
    assert_eval_eq("Count[{a,b,a,a,b,c,b,a,a},Except[b]]", "6", 0);
    assert_eval_eq("Count[{{a,a,b},b,{a,b,a}},b,2]", "3", 0);
    assert_eval_eq("Count[{{a,a,b},b,{a,b,a}},b,{2}]", "2", 0);
    assert_eval_eq("Count[x^3+1.5x^2+Pi x +7,_?NumberQ,-1]", "4", 0);
    assert_eval_eq("Count[5,_?NumberQ,-1]", "1", 0);
    assert_eval_eq("Count[5,_?NumberQ,{0,-1}]", "0", 0);
}

void test_memberq() {
    assert_eval_eq("MemberQ[{1,3,4,1,2},2]", "True", 0);
    assert_eval_eq("MemberQ[{1,3,4,1,5},2]", "False", 0);
    assert_eval_eq("MemberQ[{x^2,y^2,x^3},x^_]", "True", 0);
    assert_eval_eq("MemberQ[{{1,1,3,0},{2,1,2,2}},0,2]", "True", 0);
    assert_eval_eq("MemberQ[{{1,1,3,0},{2,1,2,2}},0]", "False", 0);
    assert_eval_eq("MemberQ[Table[Mod[2^i,7],{i,10}],1]", "True", 0);
    assert_eval_eq("MemberQ[f[a,b,c],b]", "True", 0);
    assert_eval_eq("MemberQ[f[a,b,c],d]", "False", 0);
    assert_eval_eq("MemberQ[1]", "Function[MemberQ[Slot[1], 1]]", 1);
}

int main() {
    symtab_init();
    core_init();
    
    TEST(test_cases);
    TEST(test_position);
    TEST(test_count);
    TEST(test_memberq);
    
    printf("All patterns tests passed!\n");
    symtab_clear();
    return 0;
}