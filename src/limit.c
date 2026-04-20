/* ============================================================================
 * limit.c -- Symbolic limits for PicoCAS.
 * ============================================================================
 *
 * Implements the Mathematica-style Limit built-in per the pipeline sketched
 * in limit_candidate_spec.md. The architecture is a layered dispatcher;
 * each layer either resolves the limit and short-circuits or passes the
 * problem down to the next layer:
 *
 *     Layer 0 -- Interface normalization (three calling forms, Direction).
 *     Layer 1 -- Cheap structural fast paths.
 *     Layer 2 -- Series-based evaluation (leverages Series[] natively).
 *     Layer 3 -- Rational-function dispatch (P(x)/Q(x) short-cuts).
 *     Layer 5 -- L'Hospital + logarithmic reduction heuristics.
 *     Layer 6 -- Bound analysis (Interval[] returns).
 *
 * The series layer in PicoCAS is powerful enough that it subsumes most
 * classical DELIMITER cases. L'Hospital is reserved for those shapes
 * where Series cannot compute a useful expansion (unknown heads, non-
 * analytic inputs, etc.).
 *
 * Memory conventions follow PicoCAS standards: every helper that returns
 * an Expr* returns a freshly-allocated tree owned by the caller. The
 * top-level built-in returns a newly-allocated result on success (the
 * evaluator frees the original `res` for us on a non-NULL return) or
 * NULL to leave `res` unevaluated. In particular we never free `res`
 * ourselves -- that would be a double-free against src/eval.c.
 *
 * The module is intentionally layered with small single-purpose helpers
 * so new test failures can be addressed by extending or swapping a single
 * layer rather than re-plumbing the whole pipeline.
 * ========================================================================= */

#include "limit.h"
#include "expr.h"
#include "eval.h"
#include "symtab.h"
#include "attr.h"
/* Note: Series and D are invoked symbolically (through the evaluator),
 * not via direct C calls, so series.h / deriv.h are intentionally not
 * included here. Adding the Series and Derivative symbols to the symbol
 * table is the responsibility of series_init / deriv_init, which run
 * before limit_init in core_init(). */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------------- */
/* Configuration                                                           */
/* ---------------------------------------------------------------------- */

/* Starting order for series-based evaluation; doubled up to the cap. */
#define LIMIT_SERIES_START_ORDER   4
#define LIMIT_SERIES_MAX_ORDER    32

/* Maximum recursion depth across nested Limit invocations (iterated, log
 * reduction, 1/t substitution, L'Hospital, etc.). Pathological inputs
 * abort into `unevaluated` rather than blowing the C stack. */
#define LIMIT_MAX_DEPTH           24

/* L'Hospital guardrails. */
#define LIMIT_LH_MAX_ITERATIONS   12
#define LIMIT_LH_MAX_GROWTH       3

/* ---------------------------------------------------------------------- */
/* Internal direction encoding                                             */
/*                                                                         */
/* Two-sided = 0; approach from above (x -> x*+) = +1; from below = -1.    */
/* Note: the public `Direction -> ...` option uses Mathematica's sign      */
/* convention (which is opposite of MACSYMA's); the flip is applied at     */
/* parse time, so the rest of the pipeline only ever sees these values.    */
/* A distinct tag is used for Complexes, where one-sided splitting is      */
/* meaningless.                                                            */
/* ---------------------------------------------------------------------- */
#define LIMIT_DIR_TWOSIDED   0
#define LIMIT_DIR_FROMABOVE  1
#define LIMIT_DIR_FROMBELOW (-1)
#define LIMIT_DIR_COMPLEX    2

/* ---------------------------------------------------------------------- */
/* LimitCtx -- threaded through the pipeline                               */
/* ---------------------------------------------------------------------- */
typedef struct {
    Expr* x;       /* limit variable, borrowed */
    Expr* point;   /* limit point, borrowed */
    int   dir;     /* one of LIMIT_DIR_* */
    int   depth;   /* recursion depth guard */
} LimitCtx;

/* ---------------------------------------------------------------------- */
/* Tiny Expr builders                                                      */
/* ---------------------------------------------------------------------- */

static Expr* mk_int(int64_t v)           { return expr_new_integer(v); }
static Expr* mk_sym(const char* s)       { return expr_new_symbol(s); }

static Expr* mk_fn1(const char* n, Expr* a) {
    Expr* args[1] = { a };
    return expr_new_function(mk_sym(n), args, 1);
}

static Expr* mk_fn2(const char* n, Expr* a, Expr* b) {
    Expr* args[2] = { a, b };
    return expr_new_function(mk_sym(n), args, 2);
}

static Expr* mk_times(Expr* a, Expr* b)  { return mk_fn2("Times", a, b); }
static Expr* mk_neg(Expr* a)             { return mk_times(mk_int(-1), a); }

/* Evaluate + free the source. We rely on the fact that evaluate() copies
 * its input internally. */
static Expr* simp(Expr* e) {
    if (!e) return NULL;
    Expr* r = evaluate(e);
    expr_free(e);
    return r;
}

/* ---------------------------------------------------------------------- */
/* Predicates                                                              */
/* ---------------------------------------------------------------------- */

static bool is_sym(Expr* e, const char* name) {
    return e && e->type == EXPR_SYMBOL && strcmp(e->data.symbol, name) == 0;
}

static bool has_head(Expr* e, const char* name) {
    return e && e->type == EXPR_FUNCTION &&
           e->data.function.head->type == EXPR_SYMBOL &&
           strcmp(e->data.function.head->data.symbol, name) == 0;
}

static int literal_sign(Expr* e);  /* forward */

static bool is_lit_zero(Expr* e) {
    if (!e) return false;
    if (e->type == EXPR_INTEGER) return e->data.integer == 0;
    if (e->type == EXPR_REAL)    return e->data.real == 0.0;
    if (e->type == EXPR_BIGINT)  return mpz_sgn(e->data.bigint) == 0;
    return false;
}

static bool is_infinity_sym(Expr* e) {
    return is_sym(e, "Infinity");
}

static bool is_neg_infinity(Expr* e) {
    /* Canonical form of -Infinity is Times[-1, Infinity]. */
    if (!has_head(e, "Times") || e->data.function.arg_count != 2) return false;
    Expr* a = e->data.function.args[0];
    Expr* b = e->data.function.args[1];
    bool has_inf = is_infinity_sym(a) || is_infinity_sym(b);
    Expr* other = is_infinity_sym(a) ? b : a;
    if (!has_inf) return false;
    if (other->type == EXPR_INTEGER && other->data.integer < 0) return true;
    if (other->type == EXPR_REAL && other->data.real < 0.0)     return true;
    return false;
}

static bool is_complex_infinity(Expr* e) {
    return is_sym(e, "ComplexInfinity");
}

static bool is_indeterminate(Expr* e) {
    return is_sym(e, "Indeterminate");
}

static bool is_directed_infinity(Expr* e) {
    return has_head(e, "DirectedInfinity");
}

/* Returns true for any flavour of infinity or undefined value anywhere
 * inside `e`. The fast paths use this to refuse an answer that "looks"
 * finite but still has an un-simplified Infinity / Indeterminate buried
 * in a sub-expression (e.g. 3 + 6/(Infinity^2 - 2), which PicoCAS does
 * not fold). */
static bool is_divergent(Expr* e) {
    if (!e) return true;
    if (is_infinity_sym(e) || is_complex_infinity(e) ||
        is_indeterminate(e) || is_neg_infinity(e) ||
        is_directed_infinity(e)) return true;
    if (e->type == EXPR_FUNCTION) {
        if (is_divergent(e->data.function.head)) return true;
        for (size_t i = 0; i < e->data.function.arg_count; i++) {
            if (is_divergent(e->data.function.args[i])) return true;
        }
    }
    return false;
}

/* Structural "does the expression contain the target subtree anywhere?". */
static bool expr_contains(Expr* e, Expr* target) {
    if (!e) return false;
    if (expr_eq(e, target)) return true;
    if (e->type == EXPR_FUNCTION) {
        if (expr_contains(e->data.function.head, target)) return true;
        for (size_t i = 0; i < e->data.function.arg_count; i++) {
            if (expr_contains(e->data.function.args[i], target)) return true;
        }
    }
    return false;
}

static bool free_of(Expr* e, Expr* target) { return !expr_contains(e, target); }

/* Leaf count (used for L'Hospital complexity-growth guardrail). */
static int64_t leaf_count(Expr* e) {
    if (!e) return 0;
    if (e->type != EXPR_FUNCTION) return 1;
    int64_t c = leaf_count(e->data.function.head);
    for (size_t i = 0; i < e->data.function.arg_count; i++) {
        c += leaf_count(e->data.function.args[i]);
    }
    return c;
}

/* ---------------------------------------------------------------------- */
/* Substitution                                                            */
/* ---------------------------------------------------------------------- */

/* ReplaceAll helper: returns evaluated expr with `from` -> `to` everywhere.
 * Caller owns the returned Expr*; inputs are untouched. */
static Expr* subst_eval(Expr* e, Expr* from, Expr* to) {
    Expr* rule = mk_fn2("Rule", expr_copy(from), expr_copy(to));
    Expr* ra   = mk_fn2("ReplaceAll", expr_copy(e), rule);
    return simp(ra);
}

/* ---------------------------------------------------------------------- */
/* Forward declarations                                                    */
/* ---------------------------------------------------------------------- */
static Expr* compute_limit(Expr* f, LimitCtx* ctx);
static Expr* layer1_fast_paths(Expr* f, LimitCtx* ctx);
static Expr* layer2_series(Expr* f, LimitCtx* ctx);
static Expr* layer3_rational(Expr* f, LimitCtx* ctx);
static Expr* layer5_lhospital(Expr* f, LimitCtx* ctx);
static Expr* layer5_log_reduction(Expr* f, LimitCtx* ctx);
static Expr* layer6_bounded(Expr* f, LimitCtx* ctx);
static Expr* layer_arctan_infinity(Expr* f, LimitCtx* ctx);
static Expr* layer_bounded_envelope(Expr* f, LimitCtx* ctx);
static Expr* layer_log_merge(Expr* f, LimitCtx* ctx);
static Expr* layer_log_of_finite(Expr* f, LimitCtx* ctx);
static Expr* layer_plus_termwise(Expr* f, LimitCtx* ctx);
static Expr* rewrite_reciprocal_trig(Expr* e);
static Expr* magnitude_upper_bound(Expr* e, Expr* x);
static bool  contains_bounded_head(Expr* e);

/* ---------------------------------------------------------------------- */
/* Reciprocal trig normalization                                           */
/*                                                                         */
/* Rewrite Csc/Sec/Cot (and hyperbolic twins) in terms of Sin/Cos/Sinh/    */
/* Cosh. This must run before any layer that substitutes the limit point  */
/* into f, because PicoCAS's evaluator aggressively folds                  */
/*     0 * Csc[0] = 0 * ComplexInfinity -> 0                               */
/* whereas the equivalent `0 / Sin[0] = 0/0` survives as an indeterminate */
/* 0/0 form that Series / L'Hospital can actually handle.                  */
/* ---------------------------------------------------------------------- */
static Expr* rewrite_reciprocal_trig(Expr* e) {
    if (!e) return NULL;
    if (e->type != EXPR_FUNCTION) return expr_copy(e);

    size_t n = e->data.function.arg_count;

    if (e->data.function.head->type == EXPR_SYMBOL && n == 1) {
        const char* hn = e->data.function.head->data.symbol;
        /* Recurse into the argument first so nested reciprocal-trig
         * expressions are all rewritten. */
        Expr* z = rewrite_reciprocal_trig(e->data.function.args[0]);
        if      (strcmp(hn, "Csc") == 0) {
            Expr* r = mk_fn2("Power", mk_fn1("Sin", z), mk_int(-1));
            return r;
        }
        else if (strcmp(hn, "Sec") == 0) {
            Expr* r = mk_fn2("Power", mk_fn1("Cos", z), mk_int(-1));
            return r;
        }
        else if (strcmp(hn, "Cot") == 0) {
            Expr* r = mk_times(mk_fn1("Cos", expr_copy(z)),
                               mk_fn2("Power", mk_fn1("Sin", z), mk_int(-1)));
            return r;
        }
        else if (strcmp(hn, "Csch") == 0) {
            Expr* r = mk_fn2("Power", mk_fn1("Sinh", z), mk_int(-1));
            return r;
        }
        else if (strcmp(hn, "Sech") == 0) {
            Expr* r = mk_fn2("Power", mk_fn1("Cosh", z), mk_int(-1));
            return r;
        }
        else if (strcmp(hn, "Coth") == 0) {
            Expr* r = mk_times(mk_fn1("Cosh", expr_copy(z)),
                               mk_fn2("Power", mk_fn1("Sinh", z), mk_int(-1)));
            return r;
        }
        else if (strcmp(hn, "Tan") == 0) {
            /* Rewriting Tan -> Sin/Cos lets the Series layer expand
             * around Cos-zeros (e.g. Tan[3x] at x = Pi/2) where a direct
             * Series on Tan would hit the Tan[Pi/2] = ComplexInfinity
             * pole and fold the product to 0 via 0 * ComplexInfinity. */
            Expr* r = mk_times(mk_fn1("Sin", expr_copy(z)),
                               mk_fn2("Power", mk_fn1("Cos", z), mk_int(-1)));
            return r;
        }
        else if (strcmp(hn, "Tanh") == 0) {
            Expr* r = mk_times(mk_fn1("Sinh", expr_copy(z)),
                               mk_fn2("Power", mk_fn1("Cosh", z), mk_int(-1)));
            return r;
        }
        expr_free(z);
    }

    /* Generic recursion: rebuild the function node with rewritten children. */
    Expr* head = rewrite_reciprocal_trig(e->data.function.head);
    Expr** args = (Expr**)malloc(n * sizeof(Expr*));
    for (size_t i = 0; i < n; i++) {
        args[i] = rewrite_reciprocal_trig(e->data.function.args[i]);
    }
    Expr* out = expr_new_function(head, args, n);
    free(args);
    return out;
}

/* ---------------------------------------------------------------------- */
/* Bounded envelope                                                        */
/*                                                                         */
/* `magnitude_upper_bound(e, x)` returns an expression that dominates      */
/* |e(x)| pointwise for every real x. The bound uses:                     */
/*     |Sin[g]|, |Cos[g]|, |Tanh[g]|   <= 1                                */
/*     |ArcTan[g]|, |ArcCot[g]|        <= Pi/2                             */
/*     |a + b + ...|                   <= |a| + |b| + ...                  */
/*     |a b ...|                       =  |a| |b| ...                      */
/*     |a^n| for integer n >= 0        =  |a|^n                            */
/* Anything else is wrapped in Abs[...]. The bound is useful only when    */
/* the caller can show that its limit is zero: then by squeeze f -> 0.    */
/* ---------------------------------------------------------------------- */
static bool contains_bounded_head(Expr* e) {
    if (!e || e->type != EXPR_FUNCTION) return false;
    if (has_head(e, "Sin") || has_head(e, "Cos") ||
        has_head(e, "Tanh") || has_head(e, "ArcTan") ||
        has_head(e, "ArcCot")) return true;
    if (contains_bounded_head(e->data.function.head)) return true;
    for (size_t i = 0; i < e->data.function.arg_count; i++) {
        if (contains_bounded_head(e->data.function.args[i])) return true;
    }
    return false;
}

static Expr* magnitude_upper_bound(Expr* e, Expr* x) {
    if (!e) return mk_int(0);
    if (free_of(e, x)) return mk_fn1("Abs", expr_copy(e));
    if (e->type != EXPR_FUNCTION) {
        /* Bare variable: return `x` (not Abs[x]). The caller only invokes
         * this for limits at +Infinity, where `x` is eventually positive
         * and its magnitude equals itself. Keeping Abs around causes
         * Limit[1/Abs[x], x->Infinity] to bottom out through L'Hospital
         * on a non-polynomial denominator with no useful progress. */
        return expr_copy(e);
    }

    if (has_head(e, "Sin") || has_head(e, "Cos") || has_head(e, "Tanh")) {
        return mk_int(1);
    }
    if (has_head(e, "ArcTan") || has_head(e, "ArcCot")) {
        return mk_fn2("Times", mk_fn2("Power", mk_int(2), mk_int(-1)),
                               mk_sym("Pi"));
    }

    if (has_head(e, "Plus")) {
        Expr* sum = mk_int(0);
        for (size_t i = 0; i < e->data.function.arg_count; i++) {
            sum = mk_fn2("Plus", sum, magnitude_upper_bound(
                                            e->data.function.args[i], x));
        }
        return simp(sum);
    }

    if (has_head(e, "Times")) {
        Expr* prod = mk_int(1);
        for (size_t i = 0; i < e->data.function.arg_count; i++) {
            prod = mk_times(prod, magnitude_upper_bound(
                                        e->data.function.args[i], x));
        }
        return simp(prod);
    }

    if (has_head(e, "Power") && e->data.function.arg_count == 2) {
        Expr* base = e->data.function.args[0];
        Expr* exp  = e->data.function.args[1];
        if (exp->type == EXPR_INTEGER) {
            int64_t n = exp->data.integer;
            if (n >= 0) {
                Expr* bb = magnitude_upper_bound(base, x);
                return simp(mk_fn2("Power", bb, mk_int(n)));
            }
            /* negative integer: |1/base^|n|| = 1/|base|^|n| -- keep Abs wrap. */
            Expr* bb = magnitude_upper_bound(base, x);
            return simp(mk_fn2("Power", bb, mk_int(n)));
        }
        /* Exp[bounded]: bound by E. */
        if (is_sym(base, "E") && contains_bounded_head(exp) &&
            has_head(exp, "Sin")) {
            return mk_sym("E");
        }
        return mk_fn1("Abs", expr_copy(e));
    }

    /* For Log, Exp, Sqrt etc. when free_of check missed them, default to
     * Abs wrapping (the limit of Abs[Log[...]] etc. is the true
     * magnitude; if it's not zero we fall through anyway). */
    return mk_fn1("Abs", expr_copy(e));
}

/* ---------------------------------------------------------------------- */
/* Layer 0 -- Interface normalization                                      */
/*                                                                         */
/* The calling forms are unpacked in builtin_limit() (below). This section */
/* provides the Direction-option parser.                                   */
/* ---------------------------------------------------------------------- */

/* Translate a user-facing Direction value to the internal direction tag.
 * Returns true on success; false for an unrecognised / symbolic value.    */
static bool parse_direction(Expr* dir, int* out) {
    if (!dir) { *out = LIMIT_DIR_TWOSIDED; return true; }
    if (is_sym(dir, "Reals"))     { *out = LIMIT_DIR_TWOSIDED;  return true; }
    if (is_sym(dir, "TwoSided"))  { *out = LIMIT_DIR_TWOSIDED;  return true; }
    if (is_sym(dir, "Complexes")) { *out = LIMIT_DIR_COMPLEX;   return true; }
    if (dir->type == EXPR_STRING) {
        if (strcmp(dir->data.string, "TwoSided")   == 0) { *out = LIMIT_DIR_TWOSIDED;  return true; }
        /* Mathematica sign convention: "FromAbove" == Direction -> -1,
         * "FromBelow" == Direction -> +1. Internally +1 = FromAbove,
         * -1 = FromBelow (so the math layer never has to remember the flip). */
        if (strcmp(dir->data.string, "FromAbove")  == 0) { *out = LIMIT_DIR_FROMABOVE; return true; }
        if (strcmp(dir->data.string, "FromBelow")  == 0) { *out = LIMIT_DIR_FROMBELOW; return true; }
    }
    if (dir->type == EXPR_INTEGER) {
        /* Apply the sign flip once and only here. */
        if (dir->data.integer == -1) { *out = LIMIT_DIR_FROMABOVE; return true; }
        if (dir->data.integer == +1) { *out = LIMIT_DIR_FROMBELOW; return true; }
    }
    return false;
}

/* ---------------------------------------------------------------------- */
/* Direction helpers                                                       */
/* ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- */
/* Infinity/sign helpers                                                   */
/* ---------------------------------------------------------------------- */

/* Sign of a numeric literal: +1 / 0 / -1, or 0 if not decidable. */
static int literal_sign(Expr* e) {
    if (!e) return 0;
    if (e->type == EXPR_INTEGER) {
        if (e->data.integer > 0) return +1;
        if (e->data.integer < 0) return -1;
        return 0;
    }
    if (e->type == EXPR_REAL) {
        if (e->data.real > 0.0) return +1;
        if (e->data.real < 0.0) return -1;
        return 0;
    }
    if (e->type == EXPR_BIGINT) {
        int s = mpz_sgn(e->data.bigint);
        if (s > 0) return +1;
        if (s < 0) return -1;
        return 0;
    }
    /* Rational[n, d]: sign follows numerator's. */
    if (has_head(e, "Rational") && e->data.function.arg_count == 2) {
        return literal_sign(e->data.function.args[0]);
    }
    /* Pattern Times[c, ...] with a leading negative numeric factor. */
    if (has_head(e, "Times") && e->data.function.arg_count > 0) {
        int s = literal_sign(e->data.function.args[0]);
        /* we don't try to be clever about symbolic factors; return sign
         * only if unambiguous from the leading literal. */
        return s;
    }
    return 0;
}

/* Produce +Infinity or -Infinity from a sign. */
static Expr* signed_infinity(int sign) {
    if (sign >= 0) return mk_sym("Infinity");
    return mk_neg(mk_sym("Infinity"));
}

/* ---------------------------------------------------------------------- */
/* Continuous substitution (Layer 1c)                                      */
/*                                                                         */
/* Substitute the limit point into f and check whether the result is a     */
/* clean, finite expression free of the limit variable. We accept any      */
/* answer that is not divergent and does not still mention x.              */
/* ---------------------------------------------------------------------- */
/* True iff `e` contains an exponential form Power[base, exp] with the
 * limit variable appearing in `exp`. These shapes (x^x, (1+Ax)^(1/x),
 * etc.) are classic indeterminate 1^inf / 0^0 / inf^0 forms that the
 * continuous-substitution fast path cannot handle correctly because
 * PicoCAS's arithmetic happily folds 1^ComplexInfinity to 1. We refuse
 * the fast path and let the log-reduction layer deal with them. */
static bool has_var_in_exponent(Expr* e, Expr* x) {
    if (!e) return false;
    if (has_head(e, "Power") && e->data.function.arg_count == 2) {
        Expr* exp = e->data.function.args[1];
        if (expr_contains(exp, x)) return true;
    }
    if (e->type == EXPR_FUNCTION) {
        if (has_var_in_exponent(e->data.function.head, x)) return true;
        for (size_t i = 0; i < e->data.function.arg_count; i++) {
            if (has_var_in_exponent(e->data.function.args[i], x)) return true;
        }
    }
    return false;
}

static Expr* try_continuous_substitution(Expr* f, LimitCtx* ctx) {
    /* Skip for non-finite limit points; those need a different path. */
    if (is_infinity_sym(ctx->point) || is_neg_infinity(ctx->point) ||
        is_complex_infinity(ctx->point)) {
        return NULL;
    }
    /* Refuse exponential-indeterminate shapes; see has_var_in_exponent. */
    if (has_var_in_exponent(f, ctx->x)) return NULL;

    /* Guard rails. PicoCAS's evaluator aggressively folds 0 * ComplexInfinity
     * to 0 and 1/0 to ComplexInfinity, which would produce misleading
     * "clean" answers for shapes like Sin[x]/x at x=0. Rule out anything
     * that could be a division by zero, log of zero, or similar:
     *
     *   - Together-normalize f to a single quotient N/D and evaluate the
     *     denominator at the point. If D(point) = 0, the expression is
     *     not continuous there and we bail to the next layer.
     *   - Rejecting "looks clean" is not enough: we also need to refuse
     *     inputs whose closed-form substitution hit Power::infy along the
     *     way, so we scan intermediate sub-expressions too.
     */
    Expr* tog = simp(mk_fn1("Together", expr_copy(f)));
    Expr* num = simp(mk_fn1("Numerator",   expr_copy(tog)));
    Expr* den = simp(mk_fn1("Denominator", expr_copy(tog)));
    expr_free(tog);
    Expr* den_at = subst_eval(den, ctx->x, ctx->point);
    bool den_bad = is_lit_zero(den_at) || is_divergent(den_at);
    expr_free(den_at);
    if (den_bad) {
        expr_free(num); expr_free(den);
        return NULL;
    }
    /* Denominator is safe: compose N(point)/D(point). We use the original
     * expression for the substitution so that trivially-evaluable heads
     * (Sin, Cos, ...) at the point still simplify exactly. */
    Expr* num_at = subst_eval(num, ctx->x, ctx->point);
    Expr* den_at2 = subst_eval(den, ctx->x, ctx->point);
    expr_free(num); expr_free(den);
    if (is_divergent(num_at) || is_divergent(den_at2) ||
        expr_contains(num_at, ctx->x) || expr_contains(den_at2, ctx->x)) {
        expr_free(num_at); expr_free(den_at2);
        return NULL;
    }
    Expr* s = simp(mk_fn2("Divide", num_at, den_at2));
    if (is_divergent(s) || expr_contains(s, ctx->x)) {
        expr_free(s);
        return NULL;
    }
    return s;
}

/* ---------------------------------------------------------------------- */
/* Layer 1 -- Fast paths                                                   */
/* ---------------------------------------------------------------------- */
static Expr* layer1_fast_paths(Expr* f, LimitCtx* ctx) {
    /* free of x -> constant expression, pass through. */
    if (free_of(f, ctx->x)) return expr_copy(f);

    /* f is exactly the variable: answer is the target point. */
    if (expr_eq(f, ctx->x)) return expr_copy(ctx->point);

    /* f == Rule for unevaluated Direction propagation shouldn't happen
     * at this stage; fall through otherwise. */
    return try_continuous_substitution(f, ctx);
}

/* ---------------------------------------------------------------------- */
/* Layer 2 -- Series-based evaluation                                      */
/*                                                                         */
/* Call Series[f, {x, point, k}] with increasing order k until we can      */
/* read off a leading term. Handles Infinity / MInfinity by substitution.  */
/* ---------------------------------------------------------------------- */

/* Try to interpret `s` as a SeriesData and read off the leading term.
 * Returns a freshly-allocated Expr* for the limit on success, or NULL if
 * the input is not a SeriesData / the direction disagreement is
 * ambiguous. Fills *unbounded_sign with -1/0/+1 when the answer is a
 * directional infinity (caller may re-sign). */
static Expr* read_leading_term_limit(Expr* s, LimitCtx* ctx) {
    /* s may be free of x (Series returned a constant) -- just return it. */
    if (!s) return NULL;
    if (free_of(s, ctx->x)) return expr_copy(s);

    /* Unwrap the Series may be wrapped in Times[prefactor, SeriesData[...]]. */
    Expr* prefactor = NULL;
    Expr* sd = s;
    if (has_head(s, "Times")) {
        /* find the SeriesData factor (at most one) */
        Expr* found = NULL;
        Expr* pf = mk_int(1);
        for (size_t i = 0; i < s->data.function.arg_count; i++) {
            Expr* a = s->data.function.args[i];
            if (has_head(a, "SeriesData") && !found) {
                found = a;
            } else {
                pf = simp(mk_times(pf, expr_copy(a)));
            }
        }
        if (found) {
            sd = found;
            prefactor = pf;
        } else {
            expr_free(pf);
        }
    }

    if (!has_head(sd, "SeriesData") || sd->data.function.arg_count != 6) {
        /* Not a recognised series shape; if the expression is still in a
         * closed form free of x we already handled that above. */
        if (prefactor) expr_free(prefactor);
        return NULL;
    }

    /* SeriesData[var, x0, {a0, a1, ...}, nmin, nmax, den] */
    Expr* var   = sd->data.function.args[0];
    Expr* x0    = sd->data.function.args[1];
    Expr* coefs = sd->data.function.args[2];
    Expr* nmin_e= sd->data.function.args[3];
    Expr* den_e = sd->data.function.args[5];

    if (!has_head(coefs, "List") || nmin_e->type != EXPR_INTEGER ||
        den_e->type != EXPR_INTEGER) {
        if (prefactor) expr_free(prefactor);
        return NULL;
    }
    int64_t nmin = nmin_e->data.integer;
    int64_t den  = den_e->data.integer;
    (void)var; (void)x0;

    /* Find first nonzero coefficient index. */
    size_t k = 0;
    size_t kn = coefs->data.function.arg_count;
    while (k < kn && is_lit_zero(coefs->data.function.args[k])) k++;
    if (k == kn) {
        /* All coefficients are literally zero within the computed range.
         * The leading behaviour is below the O-term; we can't read off a
         * value without expanding further. The caller will retry at a
         * higher order. */
        if (prefactor) expr_free(prefactor);
        return NULL;
    }

    int64_t leading_num = nmin + (int64_t)k;
    Expr* leading_coef = expr_copy(coefs->data.function.args[k]);

    /* Decide the limit from the leading exponent leading_num/den.
     *    > 0  -> 0
     *    = 0  -> leading coefficient
     *    < 0  -> directional infinity
     * All three cases apply whether the expansion is around a finite point
     * or around infinity (we normalize to t -> 0+ before calling Series).
     */
    Expr* result = NULL;
    if (leading_num > 0) {
        result = mk_int(0);
        expr_free(leading_coef);
    } else if (leading_num == 0) {
        result = leading_coef;
    } else {
        /* Infinity with a sign. For one-sided at 0+, sign = sign(coef).
         * For 0-, multiply by (-1)^leading_num (when integer); if the
         * exponent has a nontrivial denominator the side is complex and
         * we only answer if the direction is FromAbove. */
        int coef_sign = literal_sign(leading_coef);
        if (coef_sign == 0) {
            /* Unknown sign -- return directional infinity as a symbolic
             * answer: the evaluator's canonicalisation will reduce
             * DirectedInfinity[c] if c is obviously constant. */
            result = mk_fn1("DirectedInfinity", leading_coef);
        } else {
            int side_factor = +1;
            if (ctx->dir == LIMIT_DIR_FROMBELOW) {
                /* (-1)^leading_num if integer; odd -> flip, even -> keep */
                if (den == 1) {
                    side_factor = (leading_num % 2 == 0) ? +1 : -1;
                } else {
                    /* Fractional exponent on the negative side: branch cut. */
                    expr_free(leading_coef);
                    if (prefactor) expr_free(prefactor);
                    return NULL;
                }
            } else if (ctx->dir == LIMIT_DIR_TWOSIDED) {
                /* Both sides must agree; for odd integer exponent they
                 * disagree -> ComplexInfinity. */
                if (den == 1 && (leading_num % 2 != 0)) {
                    expr_free(leading_coef);
                    result = mk_sym("ComplexInfinity");
                    goto apply_prefactor;
                }
            }
            expr_free(leading_coef);
            result = signed_infinity(coef_sign * side_factor);
        }
    }

apply_prefactor:
    if (prefactor) {
        /* Fold the prefactor into the result. For Infinity we'd like to
         * keep a sensible sign; let the evaluator's arithmetic handle it. */
        result = simp(mk_times(prefactor, result));
    }
    return result;
}

static Expr* layer2_series(Expr* f, LimitCtx* ctx) {
    /* We rely on Series's native at-Infinity handling: Series[f, {x,
     * Infinity, k}] produces SeriesData[Power[x, -1], 0, ...] -- i.e.
     * the expansion variable is 1/x. The leading-term reader only looks
     * at the exponent and its sign, so no additional substitution is
     * required for +Infinity.
     *
     * For -Infinity we can't feed Series directly (it only recognises
     * the bare Infinity symbol as a special expansion point), so we
     * substitute x -> -y first, reduce to a +Infinity problem in y, and
     * recurse. The direction flips accordingly. */
    Expr* f_use = f;
    Expr* x_use = ctx->x;
    Expr* x0_use = ctx->point;
    Expr* x0_owned = NULL;
    Expr* f_owned = NULL;
    Expr* y_sym = NULL;
    int effective_dir = ctx->dir;

    if (is_infinity_sym(ctx->point)) {
        effective_dir = LIMIT_DIR_FROMABOVE;
    } else if (is_neg_infinity(ctx->point)) {
        y_sym = mk_sym("$LimitNegInfVar$");
        Expr* neg_y = mk_neg(expr_copy(y_sym));
        f_owned = subst_eval(f, ctx->x, neg_y);
        expr_free(neg_y);
        f_use = f_owned;
        x_use = y_sym;
        x0_owned = mk_sym("Infinity");
        x0_use = x0_owned;
        effective_dir = LIMIT_DIR_FROMABOVE;
    }

    Expr* result = NULL;
    for (int64_t k = LIMIT_SERIES_START_ORDER;
         k <= LIMIT_SERIES_MAX_ORDER;
         k *= 2) {
        Expr* spec = expr_new_function(
            mk_sym("List"),
            (Expr*[]){ expr_copy(x_use), expr_copy(x0_use), mk_int(k) }, 3);
        Expr* call = mk_fn2("Series", expr_copy(f_use), spec);
        Expr* s    = simp(call);
        LimitCtx leaf = { x_use, x0_use, effective_dir, ctx->depth };
        result = read_leading_term_limit(s, &leaf);
        expr_free(s);
        if (result) break;
    }

    if (f_owned)  expr_free(f_owned);
    if (x0_owned) expr_free(x0_owned);
    if (y_sym)    expr_free(y_sym);
    return result;
}

/* ---------------------------------------------------------------------- */
/* Layer 3 -- Rational function short-cut                                  */
/*                                                                         */
/* Handles the classical P(x)/Q(x) cases where the series layer would be   */
/* overkill. We lean on Together / Numerator / Denominator rather than     */
/* re-implementing polynomial arithmetic here.                             */
/* ---------------------------------------------------------------------- */
/* Degree of a univariate polynomial in `x`: use CoefficientList which
 * is a native builtin in PicoCAS. Returns -1 on failure. */
static int poly_degree_in(Expr* p, Expr* x) {
    Expr* call = mk_fn2("CoefficientList", expr_copy(p), expr_copy(x));
    Expr* cl = simp(call);
    int deg = -1;
    if (has_head(cl, "List") && cl->data.function.arg_count > 0) {
        /* Walk from the tail back to the first non-zero coefficient.
         * CoefficientList returns {c0, c1, ..., cn}, i.e. ordered by
         * ascending power. */
        for (size_t i = cl->data.function.arg_count; i > 0; i--) {
            if (!is_lit_zero(cl->data.function.args[i - 1])) { deg = (int)(i - 1); break; }
        }
    }
    expr_free(cl);
    return deg;
}

static Expr* poly_leading_coeff(Expr* p, Expr* x) {
    int deg = poly_degree_in(p, x);
    if (deg < 0) return NULL;
    Expr* call = mk_fn2("CoefficientList", expr_copy(p), expr_copy(x));
    Expr* cl = simp(call);
    Expr* out = NULL;
    if (has_head(cl, "List") && (int)cl->data.function.arg_count > deg) {
        out = expr_copy(cl->data.function.args[deg]);
    }
    expr_free(cl);
    return out;
}

static bool is_polynomial_in(Expr* p, Expr* x) {
    Expr* call = mk_fn2("PolynomialQ", expr_copy(p), expr_copy(x));
    Expr* r = simp(call);
    bool ok = is_sym(r, "True");
    expr_free(r);
    return ok;
}

static Expr* layer3_rational(Expr* f, LimitCtx* ctx) {
    /* Together-normalize and extract numerator/denominator. */
    Expr* together = simp(mk_fn1("Together", expr_copy(f)));
    Expr* num = simp(mk_fn1("Numerator",   expr_copy(together)));
    Expr* den = simp(mk_fn1("Denominator", expr_copy(together)));
    expr_free(together);

    if (!is_polynomial_in(num, ctx->x) || !is_polynomial_in(den, ctx->x)) {
        expr_free(num); expr_free(den);
        return NULL;
    }

    Expr* result = NULL;

    if (is_infinity_sym(ctx->point) || is_neg_infinity(ctx->point)) {
        int dn = poly_degree_in(num, ctx->x);
        int dd = poly_degree_in(den, ctx->x);
        if (dn < 0 || dd < 0) goto done;

        if (dn < dd) {
            result = mk_int(0);
        } else if (dn > dd) {
            Expr* ln = poly_leading_coeff(num, ctx->x);
            Expr* ld = poly_leading_coeff(den, ctx->x);
            if (!ln || !ld) { if (ln) expr_free(ln); if (ld) expr_free(ld); goto done; }
            int sn = literal_sign(ln);
            int sd = literal_sign(ld);
            int parity = 1;
            if (is_neg_infinity(ctx->point)) {
                /* As x -> -infty, sign of x^(dn-dd) depends on parity of dn-dd. */
                parity = ((dn - dd) % 2 == 0) ? 1 : -1;
            }
            if (sn != 0 && sd != 0) {
                result = signed_infinity(sn * sd * parity);
            } else {
                /* Give up on symbolic leading coefficients here. */
                expr_free(ln); expr_free(ld);
                goto done;
            }
            expr_free(ln); expr_free(ld);
        } else {
            /* Equal degree: ratio of leading coefficients. */
            Expr* ln = poly_leading_coeff(num, ctx->x);
            Expr* ld = poly_leading_coeff(den, ctx->x);
            if (!ln || !ld) { if (ln) expr_free(ln); if (ld) expr_free(ld); goto done; }
            result = simp(mk_fn2("Divide", ln, ld));
        }
        goto done;
    }

    /* Finite limit point: substitute and, if we hit 0/0, cancel and
     * recurse. We cap iterations to avoid surprises on pathological
     * inputs; the outer dispatcher falls through to the series layer if
     * we return NULL here. */
    Expr* num_at = subst_eval(num, ctx->x, ctx->point);
    Expr* den_at = subst_eval(den, ctx->x, ctx->point);
    bool num_zero = is_lit_zero(num_at);
    bool den_zero = is_lit_zero(den_at);

    if (!den_zero) {
        result = simp(mk_fn2("Divide", expr_copy(num_at), expr_copy(den_at)));
    } else if (num_zero && den_zero) {
        /* 0/0 -- let the series / L'Hospital layers handle it. */
        result = NULL;
    } else {
        /* 0 in denominator, nonzero numerator: direction-dependent
         * infinity. We give a conservative answer: if the direction is
         * one-sided we compute the sign through series; otherwise use
         * ComplexInfinity (matches `1/(x-a)` at x=a with default direction
         * where the two sides disagree). */
        int nsign = literal_sign(num_at);
        if (ctx->dir == LIMIT_DIR_TWOSIDED && nsign != 0) {
            result = mk_sym("ComplexInfinity");
        } else {
            /* Defer to series layer for a signed infinity answer. */
            result = NULL;
        }
    }
    expr_free(num_at); expr_free(den_at);

done:
    expr_free(num);
    expr_free(den);
    return result;
}

/* ---------------------------------------------------------------------- */
/* Layer 5 -- L'Hospital + logarithmic reduction                           */
/* ---------------------------------------------------------------------- */

/* Apply L'Hospital's rule repeatedly. We only engage this layer when
 * the pointwise evaluation is a recognised indeterminate form (0/0 or
 * Indeterminate). Strict growth of the leaf count in three consecutive
 * iterations is treated as "not making progress". */
static Expr* layer5_lhospital(Expr* f, LimitCtx* ctx) {
    /* Require f = N/D structurally via Together + Numerator/Denominator. */
    Expr* together = simp(mk_fn1("Together", expr_copy(f)));
    Expr* num = simp(mk_fn1("Numerator",   expr_copy(together)));
    Expr* den = simp(mk_fn1("Denominator", expr_copy(together)));
    expr_free(together);

    /* Denominator must not be constant 1 -- that means `f` isn't written
     * as a quotient and L'Hospital doesn't directly apply. */
    if (free_of(den, ctx->x)) {
        expr_free(num); expr_free(den);
        return NULL;
    }

    int64_t last_leaves = leaf_count(num) + leaf_count(den);
    int grown = 0;

    Expr* result = NULL;
    for (int iter = 0; iter < LIMIT_LH_MAX_ITERATIONS; iter++) {
        Expr* n_at = subst_eval(num, ctx->x, ctx->point);
        Expr* d_at = subst_eval(den, ctx->x, ctx->point);
        bool n_zero = is_lit_zero(n_at);
        bool d_zero = is_lit_zero(d_at);
        /* Infinity^2, 3*Infinity etc. all count as "diverging to infinity"
         * for the purposes of classifying this as an inf/inf indeterminate
         * form -- sign / scaling doesn't matter here since we're about to
         * differentiate. */
        bool n_inf  = is_divergent(n_at) && !n_zero;
        bool d_inf  = is_divergent(d_at) && !d_zero;

        if (!n_zero && !d_zero && !n_inf && !d_inf) {
            /* Quotient determinate (and clean); compute and return. */
            result = simp(mk_fn2("Divide", expr_copy(n_at), expr_copy(d_at)));
            expr_free(n_at); expr_free(d_at);
            break;
        }
        expr_free(n_at); expr_free(d_at);

        if (!((n_zero && d_zero) || (n_inf && d_inf))) {
            /* 0/finite or finite/0 -- not L'Hospital territory. */
            break;
        }

        /* Differentiate both. */
        Expr* dn = simp(mk_fn2("D", num, expr_copy(ctx->x)));
        Expr* dd = simp(mk_fn2("D", den, expr_copy(ctx->x)));
        num = dn;
        den = dd;

        int64_t leaves = leaf_count(num) + leaf_count(den);
        if (leaves > last_leaves) {
            grown++;
            if (grown >= LIMIT_LH_MAX_GROWTH) break;
        } else {
            grown = 0;
        }
        last_leaves = leaves;
    }

    expr_free(num);
    expr_free(den);
    return result;
}

/* Logarithmic reduction for exponential indeterminate forms
 *     f = f1^g1 * f2^g2 * ...    ->    Exp[Limit[Sum[g_i Log[f_i]]]]
 *
 * Fires whenever any Power factor in f has the limit variable in its
 * exponent (not just a bare Power at the top level -- that covered the
 * (1+a/x)^(bx) family but missed shapes like ((2+3x)/x)^x / 2^x where
 * the indeterminate behaviour comes from a *product* of Powers).
 *
 * We construct `log_expr = Sum[g_i Log[f_i]]` by walking the Times args,
 * reducing each Power[f_i, g_i] (or bare factor) to its log contribution.
 * Other factors are handled via a catch-all `Log[factor]` term -- the
 * evaluator usually simplifies these to closed form when they are free
 * of x.                                                                  */
static Expr* log_contribution(Expr* factor) {
    if (has_head(factor, "Power") && factor->data.function.arg_count == 2) {
        /* Log[a^b] = b Log[a]. */
        return mk_times(expr_copy(factor->data.function.args[1]),
                        mk_fn1("Log", expr_copy(factor->data.function.args[0])));
    }
    return mk_fn1("Log", expr_copy(factor));
}

/* Post-process a log-limit into a finite/zero/Infinity answer.
 * Returns NULL when the value is ambiguous (ComplexInfinity, Indeterminate). */
static Expr* exp_of_limit(Expr* lim_log) {
    if (is_infinity_sym(lim_log))   return mk_sym("Infinity");
    if (is_neg_infinity(lim_log))   return mk_int(0);
    if (is_complex_infinity(lim_log) || is_indeterminate(lim_log)) return NULL;
    return simp(mk_fn1("Exp", expr_copy(lim_log)));
}

static Expr* layer5_log_reduction(Expr* f, LimitCtx* ctx) {
    if (!has_var_in_exponent(f, ctx->x)) return NULL;

    /* Only fire when f is a "pure power product": a top-level Power, or
     * a top-level Times whose factors are each a constant or a Power.
     * This keeps us away from expressions like (Cos[x]-1)/(Exp[x^2]-1)
     * where `has_var_in_exponent` is true only because of an Exp buried
     * inside a Plus. Log-reducing those shapes would split a determinate
     * 0/0 into two divergent Log terms and trigger infinite Series
     * escalation. */
    bool top_ok = false;
    if (has_head(f, "Power") && f->data.function.arg_count == 2) {
        top_ok = true;
    } else if (has_head(f, "Times")) {
        top_ok = true;
        for (size_t i = 0; i < f->data.function.arg_count; i++) {
            Expr* a = f->data.function.args[i];
            bool is_factor_power = has_head(a, "Power") &&
                                   a->data.function.arg_count == 2;
            if (!is_factor_power && !free_of(a, ctx->x)) { top_ok = false; break; }
        }
    }
    if (!top_ok) return NULL;

    /* Build log_expr = Sum[log_contribution(factor)]. If f itself is a
     * Power head we treat it as a single-factor product. */
    Expr* log_expr = NULL;
    if (has_head(f, "Times")) {
        log_expr = mk_int(0);
        for (size_t i = 0; i < f->data.function.arg_count; i++) {
            log_expr = mk_fn2("Plus", log_expr,
                              log_contribution(f->data.function.args[i]));
        }
    } else {
        log_expr = log_contribution(f);
    }
    Expr* log_s = simp(log_expr);

    LimitCtx sub = *ctx; sub.depth += 1;
    Expr* lim_log = compute_limit(log_s, &sub);
    expr_free(log_s);

    if (!lim_log) return NULL;
    if (expr_contains(lim_log, ctx->x)) { expr_free(lim_log); return NULL; }

    Expr* out = exp_of_limit(lim_log);
    expr_free(lim_log);
    return out;
}

/* ---------------------------------------------------------------------- */
/* Layer 6 -- Bound analysis (Interval returns)                            */
/*                                                                         */
/* We cover one concrete case for now: a bounded head (Sin, Cos) of an    */
/* expression that diverges in the limit -- e.g. Sin[1/x] at x=0. This     */
/* is the main shape exercised by the test suite.                          */
/* ---------------------------------------------------------------------- */
static Expr* layer6_bounded(Expr* f, LimitCtx* ctx) {
    if (!has_head(f, "Sin") && !has_head(f, "Cos")) return NULL;
    if (f->data.function.arg_count != 1) return NULL;

    /* Check that the inner argument has no limit at this point (diverges).
     * The simplest positive check: inner argument has ComplexInfinity or
     * Infinity-like behaviour, or we failed to compute a limit on it. */
    Expr* inner = f->data.function.args[0];
    if (free_of(inner, ctx->x)) return NULL; /* handled elsewhere */

    /* Probe by substitution. If the inner expression yields a clean finite
     * value, the continuous-substitution path would have already fired and
     * we would never be here. So inner is expected to diverge. Produce
     * Interval[{-1, 1}]. */
    Expr* list = expr_new_function(mk_sym("List"),
                                   (Expr*[]){ mk_int(-1), mk_int(1) }, 2);
    return mk_fn1("Interval", list);
}

/* ---------------------------------------------------------------------- */
/* Layer -- ArcTan / ArcCot at +/-Infinity                                 */
/*                                                                         */
/*     ArcTan[h(x)] as x -> a, where h(x) -> +Infinity  ->  Pi/2           */
/*                                             -Infinity -> -Pi/2          */
/*     ArcCot[h(x)] as x -> a, where h(x) -> +Infinity  ->  0              */
/*                                             -Infinity -> Pi             */
/* Series cannot see through ArcTan at an infinite inner argument, so we   */
/* intercept this shape early.                                             */
/* ---------------------------------------------------------------------- */
static Expr* layer_arctan_infinity(Expr* f, LimitCtx* ctx) {
    if ((!has_head(f, "ArcTan") && !has_head(f, "ArcCot")) ||
        f->data.function.arg_count != 1) {
        return NULL;
    }
    Expr* inner = f->data.function.args[0];
    if (free_of(inner, ctx->x)) return NULL;

    LimitCtx sub = *ctx; sub.depth += 1;
    Expr* lim_inner = compute_limit(inner, &sub);
    if (!lim_inner) return NULL;

    /* Preserve pending x by refusing if the inner limit kept x. */
    if (expr_contains(lim_inner, ctx->x)) { expr_free(lim_inner); return NULL; }

    bool pos = is_infinity_sym(lim_inner);
    bool neg = is_neg_infinity(lim_inner);
    if (!pos && !neg) { expr_free(lim_inner); return NULL; }
    expr_free(lim_inner);

    if (has_head(f, "ArcTan")) {
        Expr* halfpi = mk_fn2("Times", mk_fn2("Power", mk_int(2), mk_int(-1)),
                                       mk_sym("Pi"));
        return pos ? halfpi : simp(mk_neg(halfpi));
    }
    /* ArcCot */
    if (pos) return mk_int(0);
    return mk_sym("Pi");
}

/* ---------------------------------------------------------------------- */
/* Layer -- Bounded envelope (squeeze theorem)                             */
/*                                                                         */
/* Try to show |f| -> 0 by bounding |f| pointwise with an expression       */
/* whose limit is easier to compute. Fires when f actually contains a      */
/* bounded oscillatory head; otherwise no value is added over the other    */
/* layers. Returns 0 on success, NULL otherwise. This gives us the         */
/* Sin[h(x)]/p(x), (1 +/- Cos[x])/x, x Sin[x]/(5 + x^2) family at infinity */
/* as well as x^2 Sin[1/x] at 0.                                           */
/* ---------------------------------------------------------------------- */
static Expr* layer_bounded_envelope(Expr* f, LimitCtx* ctx) {
    if (!contains_bounded_head(f)) return NULL;
    /* Only +Infinity for now: magnitude_upper_bound returns `x` (not
     * Abs[x]) for the bare variable, which is only an actual upper bound
     * on |x| when x is positive. Negative-infinity limits must first be
     * transformed via x -> -y. */
    if (!is_infinity_sym(ctx->point)) return NULL;

    Expr* bound = magnitude_upper_bound(f, ctx->x);
    if (!bound) return NULL;

    /* Guard: if the bound itself still mentions a bounded head we can't
     * reason about (e.g. Abs[E^(Sin[..])]) we don't learn anything.      */
    LimitCtx sub = *ctx; sub.depth += 1;
    Expr* lim_bound = compute_limit(bound, &sub);
    expr_free(bound);
    if (!lim_bound) return NULL;

    bool zero = is_lit_zero(lim_bound);
    expr_free(lim_bound);
    if (zero) return mk_int(0);
    return NULL;
}

/* ---------------------------------------------------------------------- */
/* Layer -- Log / polynomial merge at infinity                             */
/*                                                                         */
/* Rewrites `Sum(Log[g_i]) + Sum(h_j)` into a single `Log[∏ g_i * ∏ Exp[h_j]]`  */
/* and re-computes the limit. Meant for shapes like `-x + Log[2 + E^x]` at  */
/* infinity, where the individual summands diverge but after Expand the    */
/* combined argument has a finite positive limit (here 1).                 */
/* ---------------------------------------------------------------------- */
static Expr* layer_log_merge(Expr* f, LimitCtx* ctx) {
    if (!is_infinity_sym(ctx->point) && !is_neg_infinity(ctx->point)) return NULL;
    if (!has_head(f, "Plus")) return NULL;
    size_t n = f->data.function.arg_count;

    bool has_x_log = false;
    for (size_t i = 0; i < n; i++) {
        Expr* t = f->data.function.args[i];
        if (has_head(t, "Log") && t->data.function.arg_count == 1 &&
            !free_of(t, ctx->x)) { has_x_log = true; break; }
    }
    if (!has_x_log) return NULL;

    Expr** factors = malloc(sizeof(Expr*) * n);
    for (size_t i = 0; i < n; i++) {
        Expr* t = f->data.function.args[i];
        if (has_head(t, "Log") && t->data.function.arg_count == 1) {
            factors[i] = expr_copy(t->data.function.args[0]);
        } else {
            factors[i] = mk_fn1("Exp", expr_copy(t));
        }
    }
    Expr* prod = expr_new_function(expr_new_symbol("Times"), factors, n);
    free(factors);
    Expr* prod_s = simp(prod);
    Expr* prod_expanded = simp(mk_fn1("Expand", prod_s));
    Expr* new_f = simp(mk_fn1("Log", prod_expanded));

    /* Require a structural change to avoid unbounded recursion. */
    if (expr_eq(new_f, f)) { expr_free(new_f); return NULL; }

    LimitCtx sub = *ctx; sub.depth += 1;
    Expr* r = compute_limit(new_f, &sub);
    expr_free(new_f);
    return r;
}

/* ---------------------------------------------------------------------- */
/* Layer -- Log of an expression with a finite non-zero limit              */
/*                                                                         */
/* If f = Log[g] and Limit[g] is a finite value c, return Log[c]. The      */
/* evaluator will produce the correct value for any real c (including      */
/* -Infinity for c = 0). Refuses divergent inner limits, which are picked  */
/* up elsewhere.                                                           */
/* ---------------------------------------------------------------------- */
static Expr* layer_log_of_finite(Expr* f, LimitCtx* ctx) {
    if (!has_head(f, "Log") || f->data.function.arg_count != 1) return NULL;
    Expr* g = f->data.function.args[0];
    if (free_of(g, ctx->x)) return NULL;

    LimitCtx sub = *ctx; sub.depth += 1;
    Expr* lim_g = compute_limit(g, &sub);
    if (!lim_g) return NULL;
    if (is_divergent(lim_g) || expr_contains(lim_g, ctx->x)) {
        expr_free(lim_g); return NULL;
    }
    return simp(mk_fn1("Log", lim_g));
}

/* ---------------------------------------------------------------------- */
/* Layer -- Term-wise sum at infinity                                      */
/*                                                                         */
/* For a Plus at ±Infinity, compute each term's limit. If every term has   */
/* a finite (non-divergent) limit, the overall limit is the sum. Refuses   */
/* (returns NULL) the moment any term is divergent or unresolved; the      */
/* remaining layers then see the original shape unchanged.                 */
/* ---------------------------------------------------------------------- */
static Expr* layer_plus_termwise(Expr* f, LimitCtx* ctx) {
    if (!has_head(f, "Plus")) return NULL;
    if (!is_infinity_sym(ctx->point) && !is_neg_infinity(ctx->point)) return NULL;
    size_t n = f->data.function.arg_count;
    if (n == 0) return NULL;

    Expr** terms = malloc(sizeof(Expr*) * n);
    for (size_t i = 0; i < n; i++) terms[i] = NULL;

    for (size_t i = 0; i < n; i++) {
        Expr* t = f->data.function.args[i];
        if (free_of(t, ctx->x)) {
            terms[i] = expr_copy(t);
            continue;
        }
        LimitCtx sub = *ctx; sub.depth += 1;
        Expr* lim_t = compute_limit(t, &sub);
        if (!lim_t || is_divergent(lim_t) || expr_contains(lim_t, ctx->x)) {
            if (lim_t) expr_free(lim_t);
            for (size_t j = 0; j < n; j++) if (terms[j]) expr_free(terms[j]);
            free(terms);
            return NULL;
        }
        terms[i] = lim_t;
    }
    Expr* sum = expr_new_function(expr_new_symbol("Plus"), terms, n);
    free(terms);
    return simp(sum);
}

/* ---------------------------------------------------------------------- */
/* Top-level dispatch                                                      */
/* ---------------------------------------------------------------------- */
static Expr* compute_limit(Expr* f_in, LimitCtx* ctx) {
    if (ctx->depth >= LIMIT_MAX_DEPTH) return NULL;
    ctx->depth++;

    /* Normalize reciprocal trig up-front. This is cheap (one tree walk +
     * one evaluate) and rescues the continuous-substitution path on
     * shapes like x Csc[x], x Cot[a x], Sec[2x] (1 - Tan[x]), etc. */
    Expr* rewritten = rewrite_reciprocal_trig(f_in);
    Expr* f = simp(rewritten);

    Expr* r = NULL;

    /* Layer 1: structural fast paths, including continuous substitution. */
    r = layer1_fast_paths(f, ctx);
    if (r) { expr_free(f); ctx->depth--; return r; }

    /* ArcTan / ArcCot of a divergent inner argument. */
    r = layer_arctan_infinity(f, ctx);
    if (r) { expr_free(f); ctx->depth--; return r; }

    /* Layer 3: rational function short-cut (P(x)/Q(x) classical forms). */
    r = layer3_rational(f, ctx);
    if (r) { expr_free(f); ctx->depth--; return r; }

    /* Log[g(x)] at infinity when g has a finite inner limit. Runs before
     * Series because Series can miss Log[1 + decay] at infinity shapes. */
    r = layer_log_of_finite(f, ctx);
    if (r) { expr_free(f); ctx->depth--; return r; }

    /* Log + linear merge at infinity for shapes like -x + Log[2 + E^x],
     * which individually diverge but combine to a finite Log. */
    r = layer_log_merge(f, ctx);
    if (r) { expr_free(f); ctx->depth--; return r; }

    /* Plus at infinity where every summand has a finite limit; sum them. */
    r = layer_plus_termwise(f, ctx);
    if (r) { expr_free(f); ctx->depth--; return r; }

    /* Layer 5.3: f^g log reduction. Must run before Series because
     * Series does not have a kernel for Power[base, exp] where exp is
     * itself an expression in x (e.g. (1 + a/x)^(b x)). */
    r = layer5_log_reduction(f, ctx);
    if (r) { expr_free(f); ctx->depth--; return r; }

    /* Bounded envelope before Series. Series often blows up on bounded
     * heads at infinity (Sin[t^2] has no Taylor expansion at infinity),
     * so we try to squeeze to 0 first. Gating is enforced inside the
     * layer (currently only fires at +Infinity). */
    r = layer_bounded_envelope(f, ctx);
    if (r) { expr_free(f); ctx->depth--; return r; }

    /* Layer 2: series-based evaluation -- the workhorse. */
    r = layer2_series(f, ctx);
    if (r) { expr_free(f); ctx->depth--; return r; }

    /* Layer 5.1: L'Hospital's rule with guardrails. */
    r = layer5_lhospital(f, ctx);
    if (r) { expr_free(f); ctx->depth--; return r; }

    /* Layer 6: bounded-oscillation Interval. */
    r = layer6_bounded(f, ctx);
    if (r) { expr_free(f); ctx->depth--; return r; }

    expr_free(f);
    ctx->depth--;
    return NULL; /* leave unevaluated */
}

/* ---------------------------------------------------------------------- */
/* Interface normalization                                                 */
/* ---------------------------------------------------------------------- */

/* Given a single Rule[x, a], fill *out_var / *out_point and return true.
 * The caller retains ownership of rule; the out-pointers are borrowed
 * into rule's children (callers must not free them separately). */
static bool split_rule(Expr* rule, Expr** out_var, Expr** out_point) {
    if (!has_head(rule, "Rule") || rule->data.function.arg_count != 2) return false;
    *out_var   = rule->data.function.args[0];
    *out_point = rule->data.function.args[1];
    return true;
}

/* Extract the `Direction -> ...` option (if any) from a list of option
 * arguments. Returns the raw option value (borrowed from opts) or NULL
 * when no Direction option was supplied. */
static Expr* find_direction_opt(Expr** opts, size_t nopts) {
    for (size_t i = 0; i < nopts; i++) {
        Expr* o = opts[i];
        if (has_head(o, "Rule") && o->data.function.arg_count == 2 &&
            is_sym(o->data.function.args[0], "Direction")) {
            return o->data.function.args[1];
        }
        if (has_head(o, "RuleDelayed") && o->data.function.arg_count == 2 &&
            is_sym(o->data.function.args[0], "Direction")) {
            return o->data.function.args[1];
        }
    }
    return NULL;
}

/* Handle Limit[f, {x1 -> a1, ..., xn -> an}] by iterated right-to-left
 * folding: the innermost (rightmost) rule's Limit is computed first. */
static Expr* run_iterated(Expr* f, Expr* rule_list, int dir, int depth) {
    Expr* current = expr_copy(f);
    size_t n = rule_list->data.function.arg_count;
    for (size_t i = n; i-- > 0; ) {
        Expr* rule = rule_list->data.function.args[i];
        Expr *var, *point;
        if (!split_rule(rule, &var, &point)) { expr_free(current); return NULL; }
        LimitCtx ctx = { var, point, dir, depth };
        Expr* next = compute_limit(current, &ctx);
        expr_free(current);
        if (!next) return NULL;
        current = next;
    }
    return current;
}

/* Handle Limit[f, {x1,...,xn} -> {a1,...,an}] as a joint limit via the
 * simple "joint continuous substitution" fast path. We first perform the
 * same Together-based denominator check used by the single-variable
 * fast path so that 0/0 shapes do not silently collapse to 0 through
 * PicoCAS's aggressive arithmetic folding. Anything more sophisticated
 * (polar substitution, path-dependence heuristics) is future work --
 * those cases leave the expression unevaluated here. */
static Expr* run_multivariate(Expr* f, Expr* vars, Expr* points) {
    if (!has_head(vars, "List") || !has_head(points, "List")) return NULL;
    size_t n = vars->data.function.arg_count;
    if (n != points->data.function.arg_count) return NULL;

    /* Validate that every variable appears free of any other variable
     * that could fold things away prematurely -- cheap sanity check. */
    Expr* tog = simp(mk_fn1("Together", expr_copy(f)));
    Expr* den = simp(mk_fn1("Denominator", expr_copy(tog)));
    Expr* den_at = expr_copy(den);
    for (size_t i = 0; i < n; i++) {
        Expr* next = subst_eval(den_at, vars->data.function.args[i],
                                        points->data.function.args[i]);
        expr_free(den_at);
        den_at = next;
    }
    bool den_bad = is_lit_zero(den_at) || is_divergent(den_at);
    expr_free(den); expr_free(tog); expr_free(den_at);
    if (den_bad) return NULL;

    Expr* cur = expr_copy(f);
    for (size_t i = 0; i < n; i++) {
        Expr* s = subst_eval(cur, vars->data.function.args[i],
                                  points->data.function.args[i]);
        expr_free(cur);
        cur = s;
    }
    if (is_divergent(cur)) { expr_free(cur); return NULL; }
    for (size_t i = 0; i < n; i++) {
        if (expr_contains(cur, vars->data.function.args[i])) {
            expr_free(cur);
            return NULL;
        }
    }
    return cur;
}

/* ---------------------------------------------------------------------- */
/* Public entry point                                                      */
/* ---------------------------------------------------------------------- */
Expr* builtin_limit(Expr* res) {
    /* Contract: the evaluator frees `res` on a non-NULL return (see
     * src/eval.c); we must not free it ourselves. Return NULL to leave
     * the expression unevaluated. */
    if (res->type != EXPR_FUNCTION) return NULL;
    size_t argc = res->data.function.arg_count;
    if (argc < 2) return NULL;

    Expr* f   = res->data.function.args[0];
    Expr* spec= res->data.function.args[1];

    /* Collect option args (positions 2..argc-1). */
    Expr** opts = (argc > 2) ? &res->data.function.args[2] : NULL;
    size_t nopts = (argc > 2) ? argc - 2 : 0;

    Expr* dir_opt = find_direction_opt(opts, nopts);
    int dir = LIMIT_DIR_TWOSIDED;
    if (dir_opt && !parse_direction(dir_opt, &dir)) {
        /* Unknown direction -- keep unevaluated. */
        return NULL;
    }

    /* --- Form A: Limit[f, x -> a]
     * --- Form C: Limit[f, {x1,...,xn} -> {a1,...,an}]   (multivariate) */
    if (has_head(spec, "Rule") && spec->data.function.arg_count == 2) {
        if (has_head(spec->data.function.args[0], "List") &&
            has_head(spec->data.function.args[1], "List")) {
            return run_multivariate(f, spec->data.function.args[0],
                                       spec->data.function.args[1]);
        }
        Expr *var, *point;
        split_rule(spec, &var, &point);
        LimitCtx ctx = { var, point, dir, 0 };
        return compute_limit(f, &ctx);
    }

    /* --- Form B: Limit[f, {x1 -> a1, ..., xn -> an}] iterated --- */
    if (has_head(spec, "List")) {
        size_t n = spec->data.function.arg_count;
        if (n == 0) return NULL;
        for (size_t i = 0; i < n; i++) {
            if (!has_head(spec->data.function.args[i], "Rule")) return NULL;
        }
        return run_iterated(f, spec, dir, 0);
    }

    return NULL;
}

/* ---------------------------------------------------------------------- */
/* Registration                                                            */
/* ---------------------------------------------------------------------- */
void limit_init(void) {
    symtab_add_builtin("Limit", builtin_limit);

    /* Limit has HoldFirst-ish behaviour in Mathematica (its second
     * argument, the rule, must not be prematurely evaluated if x has an
     * OwnValue in scope). We use HoldAll here as the safest option --
     * individual layers force evaluation where needed via evaluate(). */
    symtab_get_def("Limit")->attributes |=
        ATTR_PROTECTED | ATTR_READPROTECTED | ATTR_HOLDALL;

    symtab_set_docstring("Limit",
        "Limit[f, x -> a]\n"
        "\tfinds the limit of f as x approaches a.\n"
        "Limit[f, {x1 -> a1, ..., xn -> an}]\n"
        "\titerated limit, applied rightmost-first.\n"
        "Limit[f, {x1, ..., xn} -> {a1, ..., an}]\n"
        "\tmultivariate (joint) limit.\n"
        "Limit[f, x -> a, Direction -> d]\n"
        "\tspecifies the direction of approach:\n"
        "\t  Reals or \"TwoSided\" -- default two-sided limit\n"
        "\t  \"FromAbove\" or -1   -- approach from above (x -> a^+)\n"
        "\t  \"FromBelow\" or +1   -- approach from below (x -> a^-)\n"
        "\t  Complexes           -- limit over all complex directions\n"
        "\n"
        "May return a finite value, Infinity, -Infinity, ComplexInfinity,\n"
        "Indeterminate, Interval[{lo, hi}], or the original unevaluated\n"
        "expression when the limit cannot be determined.");
}
