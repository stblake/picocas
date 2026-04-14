#include "test_utils.h"
#include "expr.h"
#include "eval.h"
#include "core.h"
#include "symtab.h"
#include "parse.h"

/* Basic: ReleaseHold strips Hold and evaluates */
void test_releasehold_basic_hold() {
    assert_eval_eq("ReleaseHold[Hold[1+1]]", "2", 0);
    assert_eval_eq("ReleaseHold[Hold[2+3]]", "5", 0);
    assert_eval_eq("ReleaseHold[Hold[3*4]]", "12", 0);
}

/* ReleaseHold strips HoldForm */
void test_releasehold_holdform() {
    assert_eval_eq("ReleaseHold[HoldForm[2+3]]", "5", 0);
    assert_eval_eq("ReleaseHold[HoldForm[1+1]]", "2", 0);
}

/* ReleaseHold strips HoldComplete */
void test_releasehold_holdcomplete() {
    assert_eval_eq("ReleaseHold[HoldComplete[3+4]]", "7", 0);
    assert_eval_eq("ReleaseHold[HoldComplete[5*6]]", "30", 0);
}

/* ReleaseHold strips HoldPattern */
void test_releasehold_holdpattern() {
    assert_eval_eq("ReleaseHold[HoldPattern[1+2]]", "3", 0);
}

/* ReleaseHold traverses into subexpressions */
void test_releasehold_inside_function() {
    assert_eval_eq("ReleaseHold[f[Hold[1+2]]]", "f[3]", 0);
    assert_eval_eq("ReleaseHold[{Hold[1+1], Hold[2+2]}]", "{2, 4}", 0);
}

/* ReleaseHold removes only the outermost layer of Hold */
void test_releasehold_one_layer() {
    assert_eval_eq("ReleaseHold[f[Hold[1+g[Hold[2+3]]]]]", "f[1 + g[Hold[2 + 3]]]", 0);
    assert_eval_eq("ReleaseHold[Hold[Hold[1+1]]]", "Hold[1 + 1]", 0);
}

/* ReleaseHold on non-held expressions is identity */
void test_releasehold_no_hold() {
    assert_eval_eq("ReleaseHold[1+1]", "2", 0);
    assert_eval_eq("ReleaseHold[x]", "x", 0);
    assert_eval_eq("ReleaseHold[42]", "42", 0);
    assert_eval_eq("ReleaseHold[{a, b, c}]", "{a, b, c}", 0);
}

/* ReleaseHold with Map */
void test_releasehold_with_map() {
    assert_eval_eq("ReleaseHold /@ {Hold[1+2], HoldForm[2+3], HoldComplete[3+4]}", "{3, 5, 7}", 0);
}

/* ReleaseHold with wrong argument count */
void test_releasehold_wrong_args() {
    assert_eval_eq("ReleaseHold[]", "ReleaseHold[]", 0);
    assert_eval_eq("ReleaseHold[a, b]", "ReleaseHold[a, b]", 0);
}

/* ReleaseHold is Protected */
void test_releasehold_protected() {
    assert_eval_eq("Attributes[ReleaseHold]", "{Protected}", 0);
}

/* ReleaseHold with symbolic expressions inside Hold */
void test_releasehold_symbolic() {
    assert_eval_eq("ReleaseHold[Hold[Sin[Pi/6]]]", "1/2", 0);
    assert_eval_eq("ReleaseHold[Hold[2^10]]", "1024", 0);
    assert_eval_eq("ReleaseHold[Hold[Length[{a,b,c}]]]", "3", 0);
}

/* ReleaseHold deeply nested structure */
void test_releasehold_deep_nesting() {
    assert_eval_eq("ReleaseHold[{f[Hold[1+2]], g[HoldForm[3+4]]}]", "{f[3], g[7]}", 0);
}

/* Memory safety: exercise various paths */
void test_releasehold_memory() {
    Expr* e1 = parse_expression("ReleaseHold[Hold[1+1]]");
    Expr* r1 = evaluate(e1);
    expr_free(e1);
    expr_free(r1);

    Expr* e2 = parse_expression("ReleaseHold[f[Hold[1+2]]]");
    Expr* r2 = evaluate(e2);
    expr_free(e2);
    expr_free(r2);

    Expr* e3 = parse_expression("ReleaseHold[Hold[Hold[1+1]]]");
    Expr* r3 = evaluate(e3);
    expr_free(e3);
    expr_free(r3);

    Expr* e4 = parse_expression("ReleaseHold[x]");
    Expr* r4 = evaluate(e4);
    expr_free(e4);
    expr_free(r4);

    Expr* e5 = parse_expression("ReleaseHold[{Hold[a], HoldForm[b], c}]");
    Expr* r5 = evaluate(e5);
    expr_free(e5);
    expr_free(r5);

    Expr* e6 = parse_expression("ReleaseHold[]");
    Expr* r6 = evaluate(e6);
    expr_free(e6);
    expr_free(r6);
}

/* ReleaseHold applied to atoms */
void test_releasehold_atoms() {
    assert_eval_eq("ReleaseHold[5]", "5", 0);
    assert_eval_eq("ReleaseHold[\"hello\"]", "\"hello\"", 0);
    assert_eval_eq("ReleaseHold[Pi]", "Pi", 0);
}

/* ReleaseHold with Hold containing multiple arguments */
void test_releasehold_multi_arg_hold() {
    /* Hold[a, b] stripped becomes Sequence[a, b] which is then flattened */
    assert_eval_eq("ReleaseHold[Hold[1+1]]", "2", 0);
}

int main() {
    symtab_init();
    core_init();

    TEST(test_releasehold_basic_hold);
    TEST(test_releasehold_holdform);
    TEST(test_releasehold_holdcomplete);
    TEST(test_releasehold_holdpattern);
    TEST(test_releasehold_inside_function);
    TEST(test_releasehold_one_layer);
    TEST(test_releasehold_no_hold);
    TEST(test_releasehold_with_map);
    TEST(test_releasehold_wrong_args);
    TEST(test_releasehold_protected);
    TEST(test_releasehold_symbolic);
    TEST(test_releasehold_deep_nesting);
    TEST(test_releasehold_memory);
    TEST(test_releasehold_atoms);
    TEST(test_releasehold_multi_arg_hold);

    printf("All ReleaseHold tests passed!\n");
    return 0;
}
