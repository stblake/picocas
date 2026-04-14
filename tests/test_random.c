#include "expr.h"
#include "eval.h"
#include "core.h"
#include "symtab.h"
#include "test_utils.h"
#include "parse.h"
#include "print.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* Helper: parse, evaluate, return real value. Returns NAN on error. */
static double eval_to_real(const char* input) {
    Expr* e = parse_expression(input);
    Expr* r = evaluate(e);
    double val = NAN;
    if (r->type == EXPR_REAL) val = r->data.real;
    else if (r->type == EXPR_INTEGER) val = (double)r->data.integer;
    expr_free(e);
    expr_free(r);
    return val;
}

/* Helper: parse, evaluate, return integer value. Returns -9999 on error. */
static int64_t eval_to_int(const char* input) {
    Expr* e = parse_expression(input);
    Expr* r = evaluate(e);
    int64_t val = -9999;
    if (r->type == EXPR_INTEGER) val = r->data.integer;
    expr_free(e);
    expr_free(r);
    return val;
}

/* Helper: parse, evaluate, return string (caller must free). */
static char* eval_to_str(const char* input) {
    Expr* e = parse_expression(input);
    Expr* r = evaluate(e);
    char* s = expr_to_string(r);
    expr_free(e);
    expr_free(r);
    return s;
}

/* ---- RandomInteger[] ---- */
void test_randominteger_no_args(void) {
    /* RandomInteger[] should return 0 or 1 */
    for (int i = 0; i < 100; i++) {
        int64_t v = eval_to_int("RandomInteger[]");
        ASSERT_MSG(v == 0 || v == 1,
                   "RandomInteger[] returned %lld, expected 0 or 1",
                   (long long)v);
    }
}

/* ---- RandomInteger[imax] ---- */
void test_randominteger_single_arg(void) {
    /* RandomInteger[0] should always return 0 */
    for (int i = 0; i < 20; i++) {
        int64_t v = eval_to_int("RandomInteger[0]");
        ASSERT_MSG(v == 0, "RandomInteger[0] returned %lld", (long long)v);
    }

    /* RandomInteger[10] should return values in [0, 10] */
    for (int i = 0; i < 200; i++) {
        int64_t v = eval_to_int("RandomInteger[10]");
        ASSERT_MSG(v >= 0 && v <= 10,
                   "RandomInteger[10] returned %lld", (long long)v);
    }

    /* RandomInteger[1] should return 0 or 1 */
    int seen0 = 0, seen1 = 0;
    for (int i = 0; i < 200; i++) {
        int64_t v = eval_to_int("RandomInteger[1]");
        ASSERT_MSG(v == 0 || v == 1,
                   "RandomInteger[1] returned %lld", (long long)v);
        if (v == 0) seen0 = 1;
        if (v == 1) seen1 = 1;
    }
    ASSERT_MSG(seen0 && seen1, "RandomInteger[1] never produced both 0 and 1");
}

/* ---- RandomInteger[{imin, imax}] ---- */
void test_randominteger_range(void) {
    /* RandomInteger[{5, 5}] should always be 5 */
    for (int i = 0; i < 20; i++) {
        int64_t v = eval_to_int("RandomInteger[{5, 5}]");
        ASSERT_MSG(v == 5, "RandomInteger[{5,5}] returned %lld", (long long)v);
    }

    /* RandomInteger[{-3, 3}] should be in [-3, 3] */
    for (int i = 0; i < 200; i++) {
        int64_t v = eval_to_int("RandomInteger[{-3, 3}]");
        ASSERT_MSG(v >= -3 && v <= 3,
                   "RandomInteger[{-3,3}] returned %lld", (long long)v);
    }

    /* RandomInteger[{10, 20}] should be in [10, 20] */
    for (int i = 0; i < 200; i++) {
        int64_t v = eval_to_int("RandomInteger[{10, 20}]");
        ASSERT_MSG(v >= 10 && v <= 20,
                   "RandomInteger[{10,20}] returned %lld", (long long)v);
    }
}

/* ---- RandomInteger[range, n] - flat list ---- */
void test_randominteger_flat_list(void) {
    /* RandomInteger[{0, 9}, 5] should give a list of 5 elements */
    char* s = eval_to_str("Length[RandomInteger[{0, 9}, 5]]");
    ASSERT_STR_EQ(s, "5");
    free(s);

    /* All elements should be integers in [0, 9] */
    /* Use Table-style check: Min and Max of the list */
    s = eval_to_str("Min[RandomInteger[{0, 9}, 1000]] >= 0");
    ASSERT_STR_EQ(s, "True");
    free(s);

    s = eval_to_str("Max[RandomInteger[{0, 9}, 1000]] <= 9");
    ASSERT_STR_EQ(s, "True");
    free(s);

    /* RandomInteger[5, 0] should give an empty list */
    s = eval_to_str("RandomInteger[5, 0]");
    ASSERT_STR_EQ(s, "{}");
    free(s);
}

/* ---- RandomInteger[range, {n1, n2}] - multi-dimensional ---- */
void test_randominteger_multidim(void) {
    /* RandomInteger[{0, 1}, {3, 4}] should give a 3x4 matrix */
    char* s = eval_to_str("Dimensions[RandomInteger[{0, 1}, {3, 4}]]");
    ASSERT_STR_EQ(s, "{3, 4}");
    free(s);

    /* RandomInteger[{0, 1}, {2, 3, 4}] should give a 2x3x4 tensor */
    s = eval_to_str("Dimensions[RandomInteger[{0, 1}, {2, 3, 4}]]");
    ASSERT_STR_EQ(s, "{2, 3, 4}");
    free(s);

    /* All values in range */
    s = eval_to_str("Min[Flatten[RandomInteger[{1, 6}, {10, 10}]]] >= 1");
    ASSERT_STR_EQ(s, "True");
    free(s);

    s = eval_to_str("Max[Flatten[RandomInteger[{1, 6}, {10, 10}]]] <= 6");
    ASSERT_STR_EQ(s, "True");
    free(s);
}

/* ---- SeedRandom reproducibility ---- */
void test_seedrandom(void) {
    /* SeedRandom[42] should make RandomInteger deterministic */
    assert_eval_eq("SeedRandom[42]", "Null", 0);

    /* Generate a sequence */
    int64_t seq1[10];
    for (int i = 0; i < 10; i++) {
        seq1[i] = eval_to_int("RandomInteger[{0, 1000}]");
    }

    /* Re-seed with same value */
    assert_eval_eq("SeedRandom[42]", "Null", 0);

    /* Should get same sequence */
    for (int i = 0; i < 10; i++) {
        int64_t v = eval_to_int("RandomInteger[{0, 1000}]");
        ASSERT_MSG(v == seq1[i],
                   "SeedRandom reproducibility failed at index %d: %lld != %lld",
                   i, (long long)v, (long long)seq1[i]);
    }
}

/* ---- SeedRandom[] reseeds from entropy ---- */
void test_seedrandom_no_args(void) {
    assert_eval_eq("SeedRandom[]", "Null", 0);
}

/* ---- Symbolic arguments remain unevaluated ---- */
void test_randominteger_symbolic(void) {
    assert_eval_eq("RandomInteger[x]", "RandomInteger[x]", 0);
    assert_eval_eq("RandomInteger[{a, b}]", "RandomInteger[{a, b}]", 0);
    assert_eval_eq("RandomInteger[5, x]", "RandomInteger[5, x]", 0);
}

/* ---- SeedRandom returns Null ---- */
void test_seedrandom_returns_null(void) {
    assert_eval_eq("SeedRandom[123]", "Null", 0);
}

/* ---- Bignum range ---- */
void test_randominteger_bignum(void) {
    /* RandomInteger with a large range that exceeds int64 */
    /* SeedRandom for determinism, then just check it doesn't crash
       and returns an integer */
    Expr* e = parse_expression("SeedRandom[99]");
    Expr* r = evaluate(e);
    expr_free(e);
    expr_free(r);

    /* 10^20 is a bignum */
    e = parse_expression("IntegerQ[RandomInteger[10^20]]");
    r = evaluate(e);
    char* s = expr_to_string(r);
    ASSERT_STR_EQ(s, "True");
    free(s);
    expr_free(e);
    expr_free(r);

    /* Range with bignums: RandomInteger[{-10^20, 10^20}] */
    e = parse_expression("IntegerQ[RandomInteger[{-10^20, 10^20}]]");
    r = evaluate(e);
    s = expr_to_string(r);
    ASSERT_STR_EQ(s, "True");
    free(s);
    expr_free(e);
    expr_free(r);
}

/* ---- Negative range: RandomInteger[{-10, -5}] ---- */
void test_randominteger_negative_range(void) {
    for (int i = 0; i < 100; i++) {
        int64_t v = eval_to_int("RandomInteger[{-10, -5}]");
        ASSERT_MSG(v >= -10 && v <= -5,
                   "RandomInteger[{-10,-5}] returned %lld", (long long)v);
    }
}

/* ---- Coverage check: all values in a small range appear ---- */
void test_randominteger_coverage(void) {
    /* With SeedRandom and enough trials, RandomInteger[{1,3}]
       should produce 1, 2, and 3 */
    int seen[4] = {0};
    for (int i = 0; i < 200; i++) {
        int64_t v = eval_to_int("RandomInteger[{1, 3}]");
        ASSERT_MSG(v >= 1 && v <= 3,
                   "RandomInteger[{1,3}] returned %lld", (long long)v);
        seen[v] = 1;
    }
    ASSERT_MSG(seen[1] && seen[2] && seen[3],
               "Not all values 1,2,3 were produced by RandomInteger[{1,3}]");
}

/* ---- Attributes check ---- */
void test_randominteger_attributes(void) {
    /* RandomInteger should be Protected */
    char* s = eval_to_str("MemberQ[Attributes[RandomInteger], Protected]");
    ASSERT_STR_EQ(s, "True");
    free(s);

    s = eval_to_str("MemberQ[Attributes[SeedRandom], Protected]");
    ASSERT_STR_EQ(s, "True");
    free(s);
}

/* ---- Information/docstring ---- */
void test_randominteger_info(void) {
    /* Verify ?RandomInteger returns something (not unevaluated) */
    Expr* e = parse_expression("Information[RandomInteger]");
    Expr* r = evaluate(e);
    /* Information returns a string */
    ASSERT(r->type == EXPR_STRING);
    expr_free(e);
    expr_free(r);
}

/* ---- Edge: RandomInteger[range, {n}] is a flat list ---- */
void test_randominteger_single_dim_list(void) {
    char* s = eval_to_str("Dimensions[RandomInteger[{0, 1}, {5}]]");
    ASSERT_STR_EQ(s, "{5}");
    free(s);
}

/* ---- RandomReal[] ---- */
void test_randomreal_no_args(void) {
    /* RandomReal[] should return a real in [0, 1) */
    for (int i = 0; i < 100; i++) {
        double v = eval_to_real("RandomReal[]");
        ASSERT_MSG(!isnan(v), "RandomReal[] did not return a number");
        ASSERT_MSG(v >= 0.0 && v < 1.0,
                   "RandomReal[] returned %g, expected in [0, 1)", v);
    }
}

/* ---- RandomReal[xmax] ---- */
void test_randomreal_single_arg(void) {
    /* RandomReal[0] should always return 0.0 */
    for (int i = 0; i < 20; i++) {
        double v = eval_to_real("RandomReal[0]");
        ASSERT_MSG(v == 0.0, "RandomReal[0] returned %g", v);
    }

    /* RandomReal[10] should return values in [0, 10) */
    for (int i = 0; i < 200; i++) {
        double v = eval_to_real("RandomReal[10]");
        ASSERT_MSG(v >= 0.0 && v < 10.0,
                   "RandomReal[10] returned %g", v);
    }

    /* RandomReal[1.5] should return values in [0, 1.5) */
    for (int i = 0; i < 100; i++) {
        double v = eval_to_real("RandomReal[1.5]");
        ASSERT_MSG(v >= 0.0 && v < 1.5,
                   "RandomReal[1.5] returned %g", v);
    }
}

/* ---- RandomReal[{xmin, xmax}] ---- */
void test_randomreal_range(void) {
    /* RandomReal[{5.0, 5.0}] should always be 5.0 */
    for (int i = 0; i < 20; i++) {
        double v = eval_to_real("RandomReal[{5.0, 5.0}]");
        ASSERT_MSG(v == 5.0, "RandomReal[{5.0,5.0}] returned %g", v);
    }

    /* RandomReal[{-3, 3}] should be in [-3, 3) */
    for (int i = 0; i < 200; i++) {
        double v = eval_to_real("RandomReal[{-3, 3}]");
        ASSERT_MSG(v >= -3.0 && v < 3.0,
                   "RandomReal[{-3,3}] returned %g", v);
    }

    /* RandomReal[{10, 20}] should be in [10, 20) */
    for (int i = 0; i < 200; i++) {
        double v = eval_to_real("RandomReal[{10, 20}]");
        ASSERT_MSG(v >= 10.0 && v < 20.0,
                   "RandomReal[{10,20}] returned %g", v);
    }
}

/* ---- RandomReal with rational range ---- */
void test_randomreal_rational_range(void) {
    /* RandomReal[{1/4, 3/4}] should be in [0.25, 0.75) */
    for (int i = 0; i < 100; i++) {
        double v = eval_to_real("RandomReal[{1/4, 3/4}]");
        ASSERT_MSG(v >= 0.25 && v < 0.75,
                   "RandomReal[{1/4,3/4}] returned %g", v);
    }
}

/* ---- RandomReal[range, n] - flat list ---- */
void test_randomreal_flat_list(void) {
    /* RandomReal[{0, 1}, 5] should give a list of 5 elements */
    char* s = eval_to_str("Length[RandomReal[{0, 1}, 5]]");
    ASSERT_STR_EQ(s, "5");
    free(s);

    /* RandomReal[5, 0] should give an empty list */
    s = eval_to_str("RandomReal[5, 0]");
    ASSERT_STR_EQ(s, "{}");
    free(s);

    /* All elements should be reals (Head == Real) */
    s = eval_to_str("SeedRandom[99]; Head[RandomReal[{0, 1}, 3][[1]]]");
    ASSERT_STR_EQ(s, "Real");
    free(s);
}

/* ---- RandomReal[range, {n1, n2}] - multi-dimensional ---- */
void test_randomreal_multidim(void) {
    /* RandomReal[{0, 1}, {3, 4}] should give a 3x4 matrix */
    char* s = eval_to_str("Dimensions[RandomReal[{0, 1}, {3, 4}]]");
    ASSERT_STR_EQ(s, "{3, 4}");
    free(s);

    /* RandomReal[{0, 1}, {2, 3, 4}] should give a 2x3x4 tensor */
    s = eval_to_str("Dimensions[RandomReal[{0, 1}, {2, 3, 4}]]");
    ASSERT_STR_EQ(s, "{2, 3, 4}");
    free(s);
}

/* ---- SeedRandom reproducibility with RandomReal ---- */
void test_randomreal_seedrandom(void) {
    /* SeedRandom[42] should make RandomReal deterministic */
    assert_eval_eq("SeedRandom[42]", "Null", 0);

    /* Generate a sequence */
    double seq1[10];
    for (int i = 0; i < 10; i++) {
        seq1[i] = eval_to_real("RandomReal[]");
    }

    /* Re-seed with same value */
    assert_eval_eq("SeedRandom[42]", "Null", 0);

    /* Should get same sequence */
    for (int i = 0; i < 10; i++) {
        double v = eval_to_real("RandomReal[]");
        ASSERT_MSG(v == seq1[i],
                   "SeedRandom reproducibility failed at index %d: %g != %g",
                   i, v, seq1[i]);
    }
}

/* ---- Symbolic arguments remain unevaluated ---- */
void test_randomreal_symbolic(void) {
    assert_eval_eq("RandomReal[x]", "RandomReal[x]", 0);
    assert_eval_eq("RandomReal[{a, b}]", "RandomReal[{a, b}]", 0);
    assert_eval_eq("RandomReal[5, x]", "RandomReal[5, x]", 0);
}

/* ---- Attributes check ---- */
void test_randomreal_attributes(void) {
    char* s = eval_to_str("MemberQ[Attributes[RandomReal], Protected]");
    ASSERT_STR_EQ(s, "True");
    free(s);
}

/* ---- Information/docstring ---- */
void test_randomreal_info(void) {
    Expr* e = parse_expression("Information[RandomReal]");
    Expr* r = evaluate(e);
    ASSERT(r->type == EXPR_STRING);
    expr_free(e);
    expr_free(r);
}

/* ---- RandomReal[{xmin, xmax}] with negative range ---- */
void test_randomreal_negative_range(void) {
    for (int i = 0; i < 100; i++) {
        double v = eval_to_real("RandomReal[{-10, -5}]");
        ASSERT_MSG(v >= -10.0 && v < -5.0,
                   "RandomReal[{-10,-5}] returned %g", v);
    }
}

/* ---- Coverage: values spread across range ---- */
void test_randomreal_coverage(void) {
    /* With enough trials, RandomReal[{0,1}] should produce values
       in both [0, 0.5) and [0.5, 1) */
    int seen_low = 0, seen_high = 0;
    for (int i = 0; i < 200; i++) {
        double v = eval_to_real("RandomReal[]");
        if (v < 0.5) seen_low = 1;
        if (v >= 0.5) seen_high = 1;
    }
    ASSERT_MSG(seen_low && seen_high,
               "RandomReal[] did not produce values in both halves of [0,1)");
}

/* ---- RandomReal[{xmin, xmax}] with single dim list ---- */
void test_randomreal_single_dim_list(void) {
    char* s = eval_to_str("Dimensions[RandomReal[{0, 1}, {5}]]");
    ASSERT_STR_EQ(s, "{5}");
    free(s);
}

/* ---- RandomReal always returns EXPR_REAL type ---- */
void test_randomreal_type(void) {
    /* Even with integer range args, result should be Real */
    char* s = eval_to_str("Head[RandomReal[{0, 10}]]");
    ASSERT_STR_EQ(s, "Real");
    free(s);

    s = eval_to_str("Head[RandomReal[]]");
    ASSERT_STR_EQ(s, "Real");
    free(s);
}

int main(void) {
    symtab_init();
    core_init();

    TEST(test_randominteger_no_args);
    TEST(test_randominteger_single_arg);
    TEST(test_randominteger_range);
    TEST(test_randominteger_flat_list);
    TEST(test_randominteger_multidim);
    TEST(test_seedrandom);
    TEST(test_seedrandom_no_args);
    TEST(test_seedrandom_returns_null);
    TEST(test_randominteger_symbolic);
    TEST(test_randominteger_bignum);
    TEST(test_randominteger_negative_range);
    TEST(test_randominteger_coverage);
    TEST(test_randominteger_attributes);
    TEST(test_randominteger_info);
    TEST(test_randominteger_single_dim_list);

    /* RandomReal tests */
    TEST(test_randomreal_no_args);
    TEST(test_randomreal_single_arg);
    TEST(test_randomreal_range);
    TEST(test_randomreal_rational_range);
    TEST(test_randomreal_flat_list);
    TEST(test_randomreal_multidim);
    TEST(test_randomreal_seedrandom);
    TEST(test_randomreal_symbolic);
    TEST(test_randomreal_attributes);
    TEST(test_randomreal_info);
    TEST(test_randomreal_negative_range);
    TEST(test_randomreal_coverage);
    TEST(test_randomreal_single_dim_list);
    TEST(test_randomreal_type);

    printf("All random tests passed!\n");
    return 0;
}
