/*
 * test_list_set.c -- unit tests for list-destructuring assignment
 * ({a,b,...} = {...}), with a particular focus on the regression where
 * reassigning to symbols that already held OwnValues silently no-op'd
 * because Set evaluated the LHS elements (binding targets) to their
 * current values before destructuring.
 *
 * Reported symptom:
 *     {a,b,c,d} = {1,1,1,1}; {a++, ++b, c--, --d}     (* -> {1,2,1,0} *)
 *     {a,b,c,d} = {1,1,1,1}; {a++, ++b, c--, --d}     (* was {2,3,0,-1} *)
 * The second line returned the same mutation applied to the stale post-
 * increment state because the reassignment never took effect.
 */

#include "eval.h"
#include "parse.h"
#include "expr.h"
#include "symtab.h"
#include "core.h"
#include "print.h"
#include "test_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Evaluate an input string and discard the result. Handy for setup lines. */
static void eval_and_discard(const char* src) {
    Expr* parsed = parse_expression(src);
    assert(parsed != NULL);
    Expr* evaluated = evaluate(parsed);
    expr_free(parsed);
    expr_free(evaluated);
}

/* Evaluate input and return its printed form; caller must free. */
static char* eval_to_string(const char* src) {
    Expr* parsed = parse_expression(src);
    assert(parsed != NULL);
    Expr* evaluated = evaluate(parsed);
    expr_free(parsed);
    char* s = expr_to_string(evaluated);
    expr_free(evaluated);
    return s;
}

static void clear_symbols(const char* const* names, size_t n) {
    for (size_t i = 0; i < n; i++) symtab_clear_symbol(names[i]);
}

/* --- Tests ---------------------------------------------------------- */

/* Baseline: first-time destructuring still works. */
static void test_destructure_fresh(void) {
    const char* syms[] = {"a", "b", "c", "d"};
    clear_symbols(syms, 4);

    eval_and_discard("{a, b, c, d} = {1, 1, 1, 1}");
    assert_eval_eq("{a, b, c, d}", "{1, 1, 1, 1}", 0);
}

/* Reassigning symbols that already have OwnValues must actually update
 * them. Before the fix, Set would evaluate a,b,c,d on the LHS to their
 * existing values and apply_assignment would silently no-op. */
static void test_reassignment_updates_targets(void) {
    const char* syms[] = {"a", "b", "c", "d"};
    clear_symbols(syms, 4);

    eval_and_discard("{a, b, c, d} = {1, 2, 3, 4}");
    eval_and_discard("{a, b, c, d} = {10, 20, 30, 40}");
    assert_eval_eq("{a, b, c, d}", "{10, 20, 30, 40}", 0);
}

/* The exact scenario from the user's bug report. */
static void test_reassign_then_increment_decrement(void) {
    const char* syms[] = {"a", "b", "c", "d"};
    clear_symbols(syms, 4);

    /* First round: fresh state, should produce {1, 2, 1, 0}. */
    eval_and_discard("{a, b, c, d} = {1, 1, 1, 1}");
    assert_eval_eq("{a++, ++b, c--, --d}", "{1, 2, 1, 0}", 0);

    /* After first round, values are {2, 2, 0, 0}. Reassign and repeat -- the
     * reported bug returned {2, 3, 0, -1} because the reassignment no-op'd. */
    eval_and_discard("{a, b, c, d} = {1, 1, 1, 1}");
    assert_eval_eq("{a++, ++b, c--, --d}", "{1, 2, 1, 0}", 0);
    assert_eval_eq("{a, b, c, d}", "{2, 2, 0, 0}", 0);
}

/* Simultaneous swap: {x, y} = {y, x} requires that the RHS is evaluated
 * in the old environment, and the LHS bindings take effect. */
static void test_simultaneous_swap(void) {
    const char* syms[] = {"x", "y"};
    clear_symbols(syms, 2);

    eval_and_discard("x = 1");
    eval_and_discard("y = 2");
    eval_and_discard("{x, y} = {y, x}");
    assert_eval_eq("{x, y}", "{2, 1}", 0);

    /* And swapping back must also work. */
    eval_and_discard("{x, y} = {y, x}");
    assert_eval_eq("{x, y}", "{1, 2}", 0);
}

/* Nested destructuring with reassignment: inner Lists are nested binding
 * groups, not expressions to evaluate. */
static void test_nested_destructuring(void) {
    const char* syms[] = {"a", "b", "c"};
    clear_symbols(syms, 3);

    eval_and_discard("{{a, b}, c} = {{10, 20}, 30}");
    assert_eval_eq("{a, b, c}", "{10, 20, 30}", 0);

    /* Reassign nested targets. */
    eval_and_discard("{{a, b}, c} = {{100, 200}, 300}");
    assert_eval_eq("{a, b, c}", "{100, 200, 300}", 0);
}

/* Non-symbol elements in a List LHS are function-shaped targets (DownValue
 * creation). Their inner arguments must still be evaluated so that, e.g.,
 * {a[x], b[y]} = {p, q} (with x=5, y=7) produces a[5]=p and b[7]=q. */
static void test_downvalue_destructuring(void) {
    const char* syms[] = {"a", "b", "x", "y"};
    clear_symbols(syms, 4);

    eval_and_discard("x = 5");
    eval_and_discard("y = 7");
    eval_and_discard("{a[x], b[y]} = {100, 200}");

    /* Values of x and y untouched. */
    assert_eval_eq("x", "5", 0);
    assert_eval_eq("y", "7", 0);

    /* DownValues created against the *evaluated* x and y. */
    assert_eval_eq("a[5]", "100", 0);
    assert_eval_eq("b[7]", "200", 0);
}

/* Pattern-bearing destructuring: {a[p_], b[q_]} = {1, 2} should create
 * generic DownValues (any a[anything] -> 1, any b[anything] -> 2). */
static void test_pattern_downvalue_destructuring(void) {
    const char* syms[] = {"a", "b"};
    clear_symbols(syms, 2);

    eval_and_discard("{a[p_], b[q_]} = {1, 2}");
    assert_eval_eq("a[42]", "1", 0);
    assert_eval_eq("b[99]", "2", 0);
}

/* Length mismatch leaves targets untouched. */
static void test_length_mismatch(void) {
    const char* syms[] = {"a", "b"};
    clear_symbols(syms, 2);

    eval_and_discard("a = 7");
    eval_and_discard("b = 8");

    /* Attempt a mismatched destructuring -- should NOT alter a or b. */
    Expr* parsed = parse_expression("{a, b} = {1, 2, 3}");
    Expr* evaluated = evaluate(parsed);
    expr_free(parsed);
    expr_free(evaluated);

    assert_eval_eq("{a, b}", "{7, 8}", 0);
}

/* Literal-integer LHS element: {1, a} = {1, 2} -- the prior code would
 * silently return {1, 2} and leave a unbound. Post-fix, the destructuring
 * reports failure, so the Set expression is returned unevaluated (or, at
 * minimum, 'a' does NOT get set to 2 via the literal path).
 *
 * We assert only the observable invariant: that `a` was not assigned to.
 */
static void test_literal_lhs_element_does_not_pretend_success(void) {
    const char* syms[] = {"a"};
    clear_symbols(syms, 1);

    Expr* parsed = parse_expression("{1, a} = {1, 2}");
    Expr* evaluated = evaluate(parsed);
    expr_free(parsed);
    expr_free(evaluated);

    /* Because the literal 1-vs-1 child recursion fails, the whole
     * destructuring is now treated as a failure and 'a' is untouched.
     * (Mathematica's own behavior here is to emit an error and leave a
     * unbound; we accept "a stays symbolic" as the correct outcome.) */
    char* s = eval_to_string("a");
    ASSERT_STR_EQ(s, "a");
    free(s);
}

/* Destructuring must not leak or corrupt when assigning in a loop (smoke
 * test for memory behavior under repeated reassignment). */
static void test_repeated_reassignment_smoke(void) {
    const char* syms[] = {"a", "b", "c"};
    clear_symbols(syms, 3);

    for (int i = 0; i < 50; i++) {
        char buf[64];
        snprintf(buf, sizeof(buf), "{a, b, c} = {%d, %d, %d}", i, i + 1, i + 2);
        eval_and_discard(buf);
    }
    assert_eval_eq("{a, b, c}", "{49, 50, 51}", 0);
}

int main(void) {
    symtab_init();
    core_init();

    TEST(test_destructure_fresh);
    TEST(test_reassignment_updates_targets);
    TEST(test_reassign_then_increment_decrement);
    TEST(test_simultaneous_swap);
    TEST(test_nested_destructuring);
    TEST(test_downvalue_destructuring);
    TEST(test_pattern_downvalue_destructuring);
    TEST(test_length_mismatch);
    TEST(test_literal_lhs_element_does_not_pretend_success);
    TEST(test_repeated_reassignment_smoke);

    printf("All tests passed!\n");
    return 0;
}
