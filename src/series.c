/* ============================================================================
 * series.c - Series and SeriesData
 * ============================================================================
 *
 * This module implements the power-series machinery for PicoCAS.
 *
 * SeriesData[x, x0, {a0, ..., a_{k-1}}, nmin, nmax, den] is the data head
 * that represents a truncated power series. The i-th coefficient multiplies
 * (x - x0)^((nmin + i)/den) and an O[x - x0]^(nmax/den) term captures the
 * dropped higher-order terms.
 *
 * Series[f, {x, x0, n}]  expands f as a power series in (x - x0) up to
 * order n. Series also accepts the leading-term form
 * Series[f, x -> x0] and the iterated multivariate form
 * Series[f, {x, x0, nx}, {y, y0, ny}, ...]. The algorithm is a recursive
 * "series algebra": primitive subexpressions become SeriesObj's, algebraic
 * heads (Plus, Times, Power) combine them, and elementary heads
 * (Exp, Log, Sin, Cos, Sinh, Cosh, Tan, Tanh) apply their known series
 * kernels. Unknown heads fall back to naive Taylor via D[...]. Expansion
 * about Infinity is handled by substituting x -> 1/u internally and
 * presenting the result with Power[x, -1] as the series variable.
 *
 * Normal[s] drops the O-term from a SeriesData and returns an ordinary sum.
 */

#include "series.h"
#include "expr.h"
#include "symtab.h"
#include "attr.h"
#include "eval.h"
#include "arithmetic.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ----------------------------------------------------------------------------
 * Tiny Expr helpers
 * -------------------------------------------------------------------------- */

static int64_t abs_i64(int64_t v) { return v < 0 ? -v : v; }

static Expr* mk_symbol(const char* s) { return expr_new_symbol(s); }

/* Build a fresh function expression from owned pieces. The returned node
 * takes ownership of head and of each arg; the caller passes ownership in. */
static Expr* mk_fn(Expr* head, Expr** args, size_t n) {
    return expr_new_function(head, args, n);
}

static Expr* mk_fn1(const char* name, Expr* a) {
    Expr* args[1] = { a };
    return mk_fn(mk_symbol(name), args, 1);
}

static Expr* mk_fn2(const char* name, Expr* a, Expr* b) {
    Expr* args[2] = { a, b };
    return mk_fn(mk_symbol(name), args, 2);
}

static Expr* mk_plus(Expr* a, Expr* b)  { return mk_fn2("Plus",  a, b); }
static Expr* mk_times(Expr* a, Expr* b) { return mk_fn2("Times", a, b); }
static Expr* mk_power(Expr* a, Expr* b) { return mk_fn2("Power", a, b); }

/* Simplify via the evaluator; takes ownership, returns owned. */
static Expr* simp(Expr* e) { return evaluate(e); }

static bool is_lit_zero(Expr* e) {
    if (!e) return false;
    if (e->type == EXPR_INTEGER) return e->data.integer == 0;
    if (e->type == EXPR_REAL)    return e->data.real == 0.0;
    if (e->type == EXPR_BIGINT)  return mpz_sgn(e->data.bigint) == 0;
    return false;
}

static bool has_symbol_head(Expr* e, const char* name) {
    return e && e->type == EXPR_FUNCTION &&
           e->data.function.head->type == EXPR_SYMBOL &&
           strcmp(e->data.function.head->data.symbol, name) == 0;
}

/* Structural containment: true iff target does not appear anywhere in e. */
static bool expr_free_of(Expr* e, Expr* target) {
    if (!e) return true;
    if (expr_eq(e, target)) return false;
    if (e->type == EXPR_FUNCTION) {
        if (!expr_free_of(e->data.function.head, target)) return false;
        for (size_t i = 0; i < e->data.function.arg_count; i++) {
            if (!expr_free_of(e->data.function.args[i], target)) return false;
        }
    }
    return true;
}

/* ReplaceAll helper: substitute `from` with `to` everywhere in `e`.
 * Returns a fresh expression (caller owns); does not free inputs. */
static Expr* replace_all_of(Expr* e, Expr* from, Expr* to) {
    Expr* rule_args[2] = { expr_copy(from), expr_copy(to) };
    Expr* rule = mk_fn(mk_symbol("Rule"), rule_args, 2);
    Expr* ra_args[2] = { expr_copy(e), rule };
    Expr* ra = mk_fn(mk_symbol("ReplaceAll"), ra_args, 2);
    return simp(ra);
}

/* ----------------------------------------------------------------------------
 * SeriesObj -- internal representation during computation
 *
 * A series is sum_{i=0..coef_count-1} coefs[i] * (x - x0)^((nmin+i)/den),
 * with an implied O[x - x0]^(order/den) term. Coefficients are ordinary
 * Expr* nodes (may contain symbolic expressions such as Log[x] or
 * Derivative[k][f][x0]) and are kept in canonical (evaluated) form.
 * -------------------------------------------------------------------------- */

typedef struct {
    Expr*   x;          /* expansion variable, owned */
    Expr*   x0;         /* expansion point, owned */
    Expr**  coefs;      /* owned coefficients, length == coef_count */
    size_t  coef_count;
    int64_t nmin;       /* leading exponent numerator */
    int64_t order;      /* O-term exponent numerator (exclusive) */
    int64_t den;        /* common denominator for exponents (>= 1) */
} SeriesObj;

static SeriesObj* so_alloc(Expr* x, Expr* x0, int64_t nmin, int64_t order, int64_t den) {
    SeriesObj* s = calloc(1, sizeof(SeriesObj));
    s->x = expr_copy(x);
    s->x0 = expr_copy(x0);
    s->nmin = nmin;
    s->order = order < nmin ? nmin : order;
    s->den = den <= 0 ? 1 : den;
    int64_t count = s->order - s->nmin;
    if (count < 0) count = 0;
    s->coef_count = (size_t)count;
    if (count > 0) {
        s->coefs = calloc((size_t)count, sizeof(Expr*));
        for (int64_t i = 0; i < count; i++) s->coefs[i] = expr_new_integer(0);
    }
    return s;
}

static void so_free(SeriesObj* s) {
    if (!s) return;
    if (s->x) expr_free(s->x);
    if (s->x0) expr_free(s->x0);
    if (s->coefs) {
        for (size_t i = 0; i < s->coef_count; i++) {
            if (s->coefs[i]) expr_free(s->coefs[i]);
        }
        free(s->coefs);
    }
    free(s);
}

/* Replace coefficient at index i, freeing the previous value. */
static void so_set_coef(SeriesObj* s, size_t i, Expr* v) {
    if (i >= s->coef_count) { expr_free(v); return; }
    expr_free(s->coefs[i]);
    s->coefs[i] = v;
}

/* Drop leading zeros (advance nmin) and trailing zeros never happen to
 * affect `order` in the series algebra, so we only trim leading zeros. */
static void so_trim_leading(SeriesObj* s) {
    size_t drop = 0;
    while (drop < s->coef_count && is_lit_zero(s->coefs[drop])) drop++;
    if (drop == 0) return;
    if (drop == s->coef_count) {
        for (size_t i = 0; i < s->coef_count; i++) expr_free(s->coefs[i]);
        free(s->coefs);
        s->coefs = NULL;
        s->coef_count = 0;
        s->nmin = s->order; /* empty series, purely O-term */
        return;
    }
    size_t new_count = s->coef_count - drop;
    Expr** new_coefs = calloc(new_count, sizeof(Expr*));
    for (size_t i = 0; i < drop; i++) expr_free(s->coefs[i]);
    for (size_t i = 0; i < new_count; i++) new_coefs[i] = s->coefs[drop + i];
    free(s->coefs);
    s->coefs = new_coefs;
    s->coef_count = new_count;
    s->nmin += (int64_t)drop;
}

/* Rescale to a new (larger) denominator. The current denominator must
 * divide the new one. Exponents expand by factor k = new_den/den. */
static SeriesObj* so_rescale(SeriesObj* s, int64_t new_den) {
    if (new_den == s->den) {
        /* Return a copy so the caller can always free the result. */
        SeriesObj* c = so_alloc(s->x, s->x0, s->nmin, s->order, s->den);
        for (size_t i = 0; i < s->coef_count; i++) so_set_coef(c, i, expr_copy(s->coefs[i]));
        return c;
    }
    int64_t k = new_den / s->den;
    int64_t new_nmin = s->nmin * k;
    int64_t new_order = s->order * k;
    SeriesObj* r = so_alloc(s->x, s->x0, new_nmin, new_order, new_den);
    /* Only multiples-of-k exponents carry coefficients. */
    for (size_t i = 0; i < s->coef_count; i++) {
        so_set_coef(r, (size_t)(i * k), expr_copy(s->coefs[i]));
    }
    return r;
}

static int64_t gcd_i64(int64_t a, int64_t b) {
    a = abs_i64(a); b = abs_i64(b);
    while (b) { int64_t t = a % b; a = b; b = t; }
    return a ? a : 1;
}

static int64_t lcm_i64(int64_t a, int64_t b) {
    if (a == 0 || b == 0) return 0;
    return (a / gcd_i64(a, b)) * b;
}

/* Align two series to a common denominator. Returns fresh objects; the
 * caller owns both. */
static void so_align_den(SeriesObj* a, SeriesObj* b, SeriesObj** ra, SeriesObj** rb) {
    int64_t d = lcm_i64(a->den, b->den);
    *ra = so_rescale(a, d);
    *rb = so_rescale(b, d);
}

/* Constant series c + O[(x-x0)^order_over_den]. */
static SeriesObj* so_from_constant(Expr* c, Expr* x, Expr* x0, int64_t order, int64_t den) {
    SeriesObj* s = so_alloc(x, x0, 0, order, den);
    if (s->coef_count > 0) {
        so_set_coef(s, 0, expr_copy(c));
    }
    return s;
}

/* Identity series: x expressed around x0, i.e. x = x0 + 1 * (x - x0). */
static SeriesObj* so_from_variable(Expr* x, Expr* x0, int64_t order, int64_t den) {
    SeriesObj* s = so_alloc(x, x0, 0, order, den);
    if (s->coef_count > 0) so_set_coef(s, 0, expr_copy(x0));
    if (s->coef_count > 1) so_set_coef(s, 1, expr_new_integer(1));
    return s;
}

/* ----------------------------------------------------------------------------
 * Conversion between SeriesObj and SeriesData Expr trees
 * -------------------------------------------------------------------------- */

static Expr* so_to_expr(const SeriesObj* s) {
    Expr** list_args = NULL;
    if (s->coef_count > 0) {
        list_args = calloc(s->coef_count, sizeof(Expr*));
        for (size_t i = 0; i < s->coef_count; i++) list_args[i] = expr_copy(s->coefs[i]);
    }
    Expr* coefs_list = expr_new_function(mk_symbol("List"), list_args, s->coef_count);
    if (list_args) free(list_args);
    Expr* args[6] = {
        expr_copy(s->x),
        expr_copy(s->x0),
        coefs_list,
        expr_new_integer(s->nmin),
        expr_new_integer(s->order),
        expr_new_integer(s->den)
    };
    return mk_fn(mk_symbol("SeriesData"), args, 6);
}

/* ----------------------------------------------------------------------------
 * Basic series arithmetic
 * -------------------------------------------------------------------------- */

static SeriesObj* so_add(SeriesObj* a, SeriesObj* b) {
    SeriesObj *aa, *bb;
    so_align_den(a, b, &aa, &bb);
    int64_t new_nmin  = aa->nmin  < bb->nmin  ? aa->nmin  : bb->nmin;
    int64_t new_order = aa->order < bb->order ? aa->order : bb->order;
    SeriesObj* r = so_alloc(aa->x, aa->x0, new_nmin, new_order, aa->den);
    for (int64_t k = new_nmin; k < new_order; k++) {
        Expr* sum = expr_new_integer(0);
        int64_t ia = k - aa->nmin;
        int64_t ib = k - bb->nmin;
        if (ia >= 0 && (size_t)ia < aa->coef_count) sum = simp(mk_plus(sum, expr_copy(aa->coefs[ia])));
        if (ib >= 0 && (size_t)ib < bb->coef_count) sum = simp(mk_plus(sum, expr_copy(bb->coefs[ib])));
        so_set_coef(r, (size_t)(k - new_nmin), sum);
    }
    so_free(aa); so_free(bb);
    return r;
}

static SeriesObj* so_scalar_mul(Expr* c, SeriesObj* a) {
    SeriesObj* r = so_alloc(a->x, a->x0, a->nmin, a->order, a->den);
    for (size_t i = 0; i < a->coef_count; i++) {
        so_set_coef(r, i, simp(mk_times(expr_copy(c), expr_copy(a->coefs[i]))));
    }
    return r;
}

static SeriesObj* so_neg(SeriesObj* a) {
    Expr* m1 = expr_new_integer(-1);
    SeriesObj* r = so_scalar_mul(m1, a);
    expr_free(m1);
    return r;
}

static SeriesObj* so_sub(SeriesObj* a, SeriesObj* b) {
    SeriesObj* nb = so_neg(b);
    SeriesObj* r = so_add(a, nb);
    so_free(nb);
    return r;
}

static SeriesObj* so_mul(SeriesObj* a, SeriesObj* b) {
    SeriesObj *aa, *bb;
    so_align_den(a, b, &aa, &bb);
    int64_t result_nmin = aa->nmin + bb->nmin;
    /* result_order is min(a.order + b.nmin, b.order + a.nmin): the lowest
     * exponent at which we lose precision from either O-term contribution. */
    int64_t oa = aa->order + bb->nmin;
    int64_t ob = bb->order + aa->nmin;
    int64_t result_order = oa < ob ? oa : ob;
    SeriesObj* r = so_alloc(aa->x, aa->x0, result_nmin, result_order, aa->den);
    for (size_t i = 0; i < aa->coef_count; i++) {
        if (is_lit_zero(aa->coefs[i])) continue;
        for (size_t j = 0; j < bb->coef_count; j++) {
            if (is_lit_zero(bb->coefs[j])) continue;
            int64_t k = aa->nmin + (int64_t)i + bb->nmin + (int64_t)j;
            if (k < result_nmin || k >= result_order) continue;
            size_t idx = (size_t)(k - result_nmin);
            Expr* prod = simp(mk_times(expr_copy(aa->coefs[i]), expr_copy(bb->coefs[j])));
            Expr* sum = simp(mk_plus(expr_copy(r->coefs[idx]), prod));
            so_set_coef(r, idx, sum);
        }
    }
    so_free(aa); so_free(bb);
    return r;
}

/* Multiply a series by (x - x0)^(p/q). In the rescaled (common-den) form
 * this is just a shift in nmin by p*den/q -- which requires q to divide
 * den (or we rescale). Returns a fresh series. */
static SeriesObj* so_shift_by_rational(SeriesObj* s, int64_t p, int64_t q) {
    int64_t g = gcd_i64(abs_i64(p), abs_i64(q));
    if (g) { p /= g; q /= g; }
    if (q < 0) { q = -q; p = -p; }
    int64_t needed_den = lcm_i64(s->den, q);
    SeriesObj* a = so_rescale(s, needed_den);
    int64_t shift = p * (needed_den / q);
    SeriesObj* r = so_alloc(a->x, a->x0, a->nmin + shift, a->order + shift, a->den);
    for (size_t i = 0; i < a->coef_count; i++) {
        so_set_coef(r, i, expr_copy(a->coefs[i]));
    }
    so_free(a);
    return r;
}

/* Make a copy of a with leading zeros trimmed. */
static SeriesObj* so_copy_trimmed(SeriesObj* a) {
    SeriesObj* c = so_alloc(a->x, a->x0, a->nmin, a->order, a->den);
    for (size_t i = 0; i < a->coef_count; i++) so_set_coef(c, i, expr_copy(a->coefs[i]));
    so_trim_leading(c);
    return c;
}

/* Count non-zero coefficients (cheap check for "single-term" exactness). */
static size_t so_nonzero_count(SeriesObj* s) {
    size_t c = 0;
    for (size_t i = 0; i < s->coef_count; i++) if (!is_lit_zero(s->coefs[i])) c++;
    return c;
}

/* Series inverse: 1/a. Requires the leading coefficient to be non-zero
 * after trimming. Handles the exact single-term case without the usual
 * order reduction; otherwise the correction expansion loses 2*nmin of
 * precision against the O-term exponent. */
static SeriesObj* so_inv(SeriesObj* a_in) {
    SeriesObj* a = so_copy_trimmed(a_in);
    if (a->coef_count == 0) { so_free(a); return NULL; }

    /* Exact single-term inversion: 1/(c * (x-x0)^nmin) = (1/c) * (x-x0)^{-nmin}
     * and its accuracy is whatever the input's was. */
    if (so_nonzero_count(a) == 1) {
        Expr* inv_c = simp(mk_power(expr_copy(a->coefs[0]), expr_new_integer(-1)));
        SeriesObj* r = so_alloc(a->x, a->x0, -a->nmin, a->order, a->den);
        if (r->coef_count > 0) so_set_coef(r, 0, inv_c);
        else expr_free(inv_c);
        so_free(a);
        return r;
    }

    int64_t ord_rel = a->order - a->nmin;
    size_t N = (size_t)ord_rel;
    if (N == 0) {
        SeriesObj* r = so_alloc(a->x, a->x0, -a->nmin, -a->nmin, a->den);
        so_free(a);
        return r;
    }
    Expr** b = calloc(N, sizeof(Expr*));
    Expr* inv_a0 = simp(mk_power(expr_copy(a->coefs[0]), expr_new_integer(-1)));
    b[0] = expr_copy(inv_a0);
    for (size_t k = 1; k < N; k++) {
        Expr* sum = expr_new_integer(0);
        for (size_t i = 1; i <= k; i++) {
            if (i >= a->coef_count) break;
            if (is_lit_zero(a->coefs[i])) continue;
            if (is_lit_zero(b[k - i])) continue;
            Expr* prod = simp(mk_times(expr_copy(a->coefs[i]), expr_copy(b[k - i])));
            sum = simp(mk_plus(sum, prod));
        }
        Expr* neg = simp(mk_times(expr_new_integer(-1), sum));
        b[k] = simp(mk_times(expr_copy(inv_a0), neg));
    }
    expr_free(inv_a0);

    SeriesObj* r = so_alloc(a->x, a->x0, -a->nmin, a->order - 2 * a->nmin, a->den);
    for (size_t i = 0; i < N && i < r->coef_count; i++) so_set_coef(r, i, b[i]);
    free(b);
    so_free(a);
    return r;
}

static SeriesObj* so_div(SeriesObj* a, SeriesObj* b) {
    SeriesObj* binv = so_inv(b);
    if (!binv) return NULL;
    SeriesObj* r = so_mul(a, binv);
    so_free(binv);
    return r;
}

static SeriesObj* so_pow_int(SeriesObj* a, int64_t n) {
    if (n == 0) {
        SeriesObj* one = so_alloc(a->x, a->x0, 0, a->order - a->nmin, a->den);
        if (one->coef_count > 0) so_set_coef(one, 0, expr_new_integer(1));
        return one;
    }
    if (n < 0) {
        SeriesObj* inv = so_inv(a);
        if (!inv) return NULL;
        SeriesObj* p = so_pow_int(inv, -n);
        so_free(inv);
        return p;
    }
    /* Binary exponentiation. */
    SeriesObj* result = NULL;
    SeriesObj* base = so_alloc(a->x, a->x0, a->nmin, a->order, a->den);
    for (size_t i = 0; i < a->coef_count; i++) so_set_coef(base, i, expr_copy(a->coefs[i]));
    int64_t e = n;
    while (e > 0) {
        if (e & 1) {
            if (!result) {
                result = so_alloc(base->x, base->x0, base->nmin, base->order, base->den);
                for (size_t i = 0; i < base->coef_count; i++) so_set_coef(result, i, expr_copy(base->coefs[i]));
            } else {
                SeriesObj* nr = so_mul(result, base);
                so_free(result);
                result = nr;
            }
        }
        e >>= 1;
        if (e > 0) {
            SeriesObj* sq = so_mul(base, base);
            so_free(base);
            base = sq;
        }
    }
    so_free(base);
    return result;
}

/* Compute (1 + u)^alpha, given u with u.nmin >= 1 (u has no constant term),
 * using the recurrence b_{k+1} = (alpha - k)/(k+1) * b_k applied
 * symbolically. alpha can be an arbitrary Expr (rational, symbolic).
 * Returns a series whose exponents are multiples of u.nmin up to
 * u.order (in u.den), i.e. in the original common denominator. */
static SeriesObj* so_pow_1plus_alpha(SeriesObj* u, Expr* alpha) {
    /* Determine how many terms of the 1+u kernel we need: u.nmin is the
     * minimum power contributed, and we need kernel-index k such that
     * k * u.nmin < u.order. */
    int64_t u_nmin = u->nmin;
    int64_t u_order = u->order;
    if (u_nmin <= 0) {
        /* u must have no constant term. Caller's responsibility. */
        return NULL;
    }
    int64_t N = 0;
    while ((N + 1) * u_nmin < u_order) N++;
    /* Build binomial coefficients b_0..b_N symbolically. */
    Expr** bcoef = calloc((size_t)(N + 1), sizeof(Expr*));
    bcoef[0] = expr_new_integer(1);
    for (int64_t k = 0; k < N; k++) {
        /* b_{k+1} = b_k * (alpha - k) / (k+1) */
        Expr* diff = simp(mk_plus(expr_copy(alpha), expr_new_integer(-k)));
        Expr* num = simp(mk_times(expr_copy(bcoef[k]), diff));
        Expr* inv = expr_new_integer(k + 1);
        Expr* inv_rat = simp(mk_power(inv, expr_new_integer(-1)));
        bcoef[k + 1] = simp(mk_times(num, inv_rat));
    }
    /* Compose: sum_k b_k * u^k, via Horner from high-degree to low. */
    SeriesObj* result = so_alloc(u->x, u->x0, 0, u->order, u->den);
    if (result->coef_count > 0) so_set_coef(result, 0, expr_copy(bcoef[N]));
    for (int64_t k = N - 1; k >= 0; k--) {
        SeriesObj* mul = so_mul(result, u);
        so_free(result);
        result = mul;
        /* Add b_k * (x-x0)^0 constant. */
        SeriesObj* c = so_from_constant(bcoef[k], u->x, u->x0, u->order, u->den);
        SeriesObj* sum = so_add(result, c);
        so_free(result); so_free(c);
        result = sum;
    }
    for (int64_t i = 0; i <= N; i++) expr_free(bcoef[i]);
    free(bcoef);
    return result;
}

/* s^alpha for alpha an arbitrary Expr. Factor out the leading monomial:
 *   s = a0 * (x-x0)^(nmin/den) * (1 + u)
 * with u = s/(a0*(x-x0)^(nmin/den)) - 1 having u.nmin >= 1.
 * Then s^alpha = a0^alpha * (x-x0)^(nmin*alpha/den) * (1+u)^alpha. */
static SeriesObj* so_pow_expr(SeriesObj* s_in, Expr* alpha) {
    if (alpha->type == EXPR_INTEGER) {
        return so_pow_int(s_in, alpha->data.integer);
    }

    SeriesObj* s = so_copy_trimmed(s_in);
    if (s->coef_count == 0) { so_free(s); return NULL; }
    Expr* a0 = s->coefs[0];

    /* Build u = s / (a0 * (x-x0)^(nmin/den)) - 1, a series with nmin >= 1. */
    /* Step 1: scalar-divide by a0 (so leading coef becomes 1). */
    Expr* inv_a0 = simp(mk_power(expr_copy(a0), expr_new_integer(-1)));
    SeriesObj* s_scaled = so_scalar_mul(inv_a0, s);
    expr_free(inv_a0);
    /* Step 2: shift so leading exponent is 0 (i.e. subtract nmin from every
     * exponent, then drop the leading 1 and keep u = the rest). */
    int64_t nmin = s->nmin;
    SeriesObj* shifted = so_shift_by_rational(s_scaled, -nmin, s->den);
    so_free(s_scaled);
    /* Step 3: u = shifted - 1 (remove the constant term, should be 1). */
    Expr* one = expr_new_integer(1);
    SeriesObj* cone = so_from_constant(one, shifted->x, shifted->x0, shifted->order, shifted->den);
    SeriesObj* u = so_sub(shifted, cone);
    so_free(cone); so_free(shifted); expr_free(one);
    so_trim_leading(u);
    if (u->coef_count == 0) {
        /* Exact single-term input: s = a0*(x-x0)^(nmin/den), so
         * s^alpha = a0^alpha * (x-x0)^(nmin*alpha/den). If alpha is rational,
         * we can express the shift exactly; otherwise we give up. */
        so_free(u);
        Expr* a0_pow = simp(mk_power(expr_copy(a0), expr_copy(alpha)));
        if (nmin == 0) {
            SeriesObj* r = so_alloc(s->x, s->x0, 0, s->order, s->den);
            if (r->coef_count > 0) so_set_coef(r, 0, a0_pow);
            else expr_free(a0_pow);
            so_free(s);
            return r;
        }
        int64_t p, q;
        if (alpha->type == EXPR_INTEGER) { p = alpha->data.integer; q = 1; }
        else if (is_rational(alpha, &p, &q)) { /* ok */ }
        else { expr_free(a0_pow); so_free(s); return NULL; }
        /* Build SeriesData with the single coefficient a0_pow at exponent
         * nmin*p/(den*q). */
        int64_t out_p = nmin * p;
        int64_t out_q = s->den * q;
        int64_t g = gcd_i64(abs_i64(out_p), abs_i64(out_q));
        if (g) { out_p /= g; out_q /= g; }
        if (out_q < 0) { out_q = -out_q; out_p = -out_p; }
        SeriesObj* r = so_alloc(s->x, s->x0, out_p, s->order, out_q);
        if (r->coef_count > 0) so_set_coef(r, 0, a0_pow);
        else expr_free(a0_pow);
        so_free(s);
        return r;
    }
    /* Step 4: (1+u)^alpha. */
    SeriesObj* oneplus_pow = so_pow_1plus_alpha(u, alpha);
    so_free(u);
    if (!oneplus_pow) { so_free(s); return NULL; }

    /* Step 5: scalar multiply by a0^alpha and shift by nmin*alpha/den. */
    Expr* a0_pow_alpha = simp(mk_power(expr_copy(a0), expr_copy(alpha)));
    SeriesObj* scaled = so_scalar_mul(a0_pow_alpha, oneplus_pow);
    expr_free(a0_pow_alpha);
    so_free(oneplus_pow);

    SeriesObj* final_result = scaled;
    if (nmin != 0) {
        int64_t p, q;
        if (alpha->type == EXPR_INTEGER) { p = alpha->data.integer; q = 1; }
        else if (is_rational(alpha, &p, &q)) { /* ok */ }
        else { so_free(scaled); so_free(s); return NULL; }
        final_result = so_shift_by_rational(scaled, nmin * p, s->den * q);
        so_free(scaled);
    }
    so_free(s);
    return final_result;
}

/* ----------------------------------------------------------------------------
 * Elementary kernels and function application
 * -------------------------------------------------------------------------- */

/* Compose sum_k kernel[k] * u^k, where kernel[] is a length-N array of
 * Expr* scalar coefficients and u is a series with u.nmin >= 1. */
static SeriesObj* so_compose_scalar_kernel(Expr** kernel, size_t N, SeriesObj* u) {
    if (N == 0) {
        return so_from_constant(expr_new_integer(0), u->x, u->x0, u->order, u->den);
    }
    SeriesObj* result = so_from_constant(kernel[N - 1], u->x, u->x0, u->order, u->den);
    for (ssize_t k = (ssize_t)N - 2; k >= 0; k--) {
        SeriesObj* mul = so_mul(result, u);
        so_free(result);
        result = mul;
        SeriesObj* c = so_from_constant(kernel[k], u->x, u->x0, u->order, u->den);
        SeriesObj* sum = so_add(result, c);
        so_free(result); so_free(c);
        result = sum;
    }
    return result;
}

/* Build the Taylor series coefficients of an elementary function f(u) at
 * u = 0, as scalar Expr* values, for u-powers 0..N-1.
 *   Exp:   1, 1, 1/2, 1/6, 1/24, ...          (1/k!)
 *   Log1p: 0, 1, -1/2, 1/3, -1/4, ...         ((-1)^(k-1)/k for k>=1)
 *   Sin:   0, 1, 0, -1/6, 0, 1/120, ...
 *   Cos:   1, 0, -1/2, 0, 1/24, ...
 *   Sinh:  0, 1, 0,  1/6, 0,  1/120, ...
 *   Cosh:  1, 0,  1/2, 0,  1/24, ...
 * Returns a newly allocated array of owned Expr*. */
static Expr** kernel_coefs(const char* name, size_t N) {
    Expr** c = calloc(N, sizeof(Expr*));
    if (strcmp(name, "Exp") == 0) {
        /* c[k] = 1/k! */
        Expr* fact = expr_new_integer(1);
        for (size_t k = 0; k < N; k++) {
            if (k > 0) fact = simp(mk_times(fact, expr_new_integer((int64_t)k)));
            c[k] = simp(mk_power(expr_copy(fact), expr_new_integer(-1)));
        }
        expr_free(fact);
    } else if (strcmp(name, "Log1p") == 0) {
        /* Log[1+u] = sum_{k>=1} (-1)^(k-1) u^k / k */
        c[0] = expr_new_integer(0);
        for (size_t k = 1; k < N; k++) {
            int64_t sign = (k & 1) ? 1 : -1;
            c[k] = make_rational(sign, (int64_t)k);
        }
    } else if (strcmp(name, "Sin") == 0) {
        Expr* fact = expr_new_integer(1);
        for (size_t k = 0; k < N; k++) {
            if (k > 0) fact = simp(mk_times(fact, expr_new_integer((int64_t)k)));
            if (k % 2 == 0) c[k] = expr_new_integer(0);
            else {
                int64_t sign = ((k / 2) & 1) ? -1 : 1;
                Expr* inv = simp(mk_power(expr_copy(fact), expr_new_integer(-1)));
                c[k] = simp(mk_times(expr_new_integer(sign), inv));
            }
        }
        expr_free(fact);
    } else if (strcmp(name, "Cos") == 0) {
        Expr* fact = expr_new_integer(1);
        for (size_t k = 0; k < N; k++) {
            if (k > 0) fact = simp(mk_times(fact, expr_new_integer((int64_t)k)));
            if (k % 2 != 0) c[k] = expr_new_integer(0);
            else {
                int64_t sign = ((k / 2) & 1) ? -1 : 1;
                Expr* inv = simp(mk_power(expr_copy(fact), expr_new_integer(-1)));
                c[k] = simp(mk_times(expr_new_integer(sign), inv));
            }
        }
        expr_free(fact);
    } else if (strcmp(name, "Sinh") == 0) {
        Expr* fact = expr_new_integer(1);
        for (size_t k = 0; k < N; k++) {
            if (k > 0) fact = simp(mk_times(fact, expr_new_integer((int64_t)k)));
            if (k % 2 == 0) c[k] = expr_new_integer(0);
            else c[k] = simp(mk_power(expr_copy(fact), expr_new_integer(-1)));
        }
        expr_free(fact);
    } else if (strcmp(name, "Cosh") == 0) {
        Expr* fact = expr_new_integer(1);
        for (size_t k = 0; k < N; k++) {
            if (k > 0) fact = simp(mk_times(fact, expr_new_integer((int64_t)k)));
            if (k % 2 != 0) c[k] = expr_new_integer(0);
            else c[k] = simp(mk_power(expr_copy(fact), expr_new_integer(-1)));
        }
        expr_free(fact);
    } else if (strcmp(name, "ArcTan") == 0) {
        /* ArcTan[u] = sum_{k odd, k>=1} (-1)^((k-1)/2) u^k / k */
        for (size_t k = 0; k < N; k++) {
            if ((k & 1) == 0) { c[k] = expr_new_integer(0); continue; }
            int64_t sign = (((k - 1) / 2) & 1) ? -1 : 1;
            c[k] = make_rational(sign, (int64_t)k);
        }
    } else if (strcmp(name, "ArcTanh") == 0) {
        /* ArcTanh[u] = sum_{k odd, k>=1} u^k / k */
        for (size_t k = 0; k < N; k++) {
            if ((k & 1) == 0) c[k] = expr_new_integer(0);
            else c[k] = make_rational(1, (int64_t)k);
        }
    } else if (strcmp(name, "ArcSin") == 0) {
        /* ArcSin[u] coefficients via recurrence
         * c_{2m+1} = c_{2m-1} * (2m-1)^2 / (2m * (2m+1)), c_1 = 1. */
        c[0] = expr_new_integer(0);
        if (N > 1) c[1] = expr_new_integer(1);
        for (size_t k = 2; k < N; k++) {
            if ((k & 1) == 0) { c[k] = expr_new_integer(0); continue; }
            int64_t m = (int64_t)(k - 1) / 2;
            int64_t num = (2*m - 1) * (2*m - 1);
            int64_t den = 2 * m * (2*m + 1);
            Expr* ratio = make_rational(num, den);
            c[k] = simp(mk_times(expr_copy(c[k - 2]), ratio));
        }
    } else if (strcmp(name, "ArcSinh") == 0) {
        /* Same magnitudes as ArcSin with alternating sign on odd k. */
        c[0] = expr_new_integer(0);
        if (N > 1) c[1] = expr_new_integer(1);
        for (size_t k = 2; k < N; k++) {
            if ((k & 1) == 0) { c[k] = expr_new_integer(0); continue; }
            int64_t m = (int64_t)(k - 1) / 2;
            int64_t num = -(2*m - 1) * (2*m - 1);  /* flip sign */
            int64_t den = 2 * m * (2*m + 1);
            Expr* ratio = make_rational(num, den);
            c[k] = simp(mk_times(expr_copy(c[k - 2]), ratio));
        }
    } else {
        for (size_t i = 0; i < N; i++) c[i] = expr_new_integer(0);
    }
    return c;
}

static void kernel_coefs_free(Expr** c, size_t N) {
    if (!c) return;
    for (size_t i = 0; i < N; i++) if (c[i]) expr_free(c[i]);
    free(c);
}

/* Split s into constant part and non-constant-part.
 * On return, *c is the coefficient at (x-x0)^0 (or 0 if nmin > 0), and
 * *u is a new series equal to s minus that constant (nmin >= 1 after). */
static void so_split_constant(SeriesObj* s, Expr** c_out, SeriesObj** u_out) {
    Expr* c = expr_new_integer(0);
    SeriesObj* u;
    if (s->nmin > 0 || s->coef_count == 0) {
        /* No constant term; u = s. */
        u = so_alloc(s->x, s->x0, s->nmin, s->order, s->den);
        for (size_t i = 0; i < s->coef_count; i++) so_set_coef(u, i, expr_copy(s->coefs[i]));
    } else if (s->nmin < 0) {
        /* Series has terms below constant (Laurent). Splitting constant
         * this way isn't meaningful; return s as u, c = 0. The caller
         * should check whether this path is reachable. */
        u = so_alloc(s->x, s->x0, s->nmin, s->order, s->den);
        for (size_t i = 0; i < s->coef_count; i++) so_set_coef(u, i, expr_copy(s->coefs[i]));
    } else {
        /* nmin == 0. */
        expr_free(c);
        c = expr_copy(s->coefs[0]);
        u = so_alloc(s->x, s->x0, s->nmin, s->order, s->den);
        for (size_t i = 0; i < s->coef_count; i++) {
            if (i == 0) so_set_coef(u, i, expr_new_integer(0));
            else        so_set_coef(u, i, expr_copy(s->coefs[i]));
        }
        so_trim_leading(u);
    }
    *c_out = c;
    *u_out = u;
}

/* Apply Exp to a series. Exp[c + u] = Exp[c] * Exp[u]. */
static SeriesObj* so_apply_exp(SeriesObj* s) {
    Expr* c; SeriesObj* u;
    so_split_constant(s, &c, &u);
    int64_t N = (u->order - u->nmin) / (u->nmin > 0 ? u->nmin : 1) + 1;
    if (N < 1) N = 1;
    if (u->nmin < 1) { expr_free(c); so_free(u); return NULL; }
    Expr** k = kernel_coefs("Exp", (size_t)N);
    SeriesObj* ex_u = so_compose_scalar_kernel(k, (size_t)N, u);
    kernel_coefs_free(k, (size_t)N);
    Expr* ex_c = simp(mk_fn1("Exp", c));
    SeriesObj* r = so_scalar_mul(ex_c, ex_u);
    expr_free(ex_c); so_free(u); so_free(ex_u);
    return r;
}

/* Apply Log to a series. Log[c + u] = Log[c] + Log[1 + u/c] when c != 0. */
static SeriesObj* so_apply_log(SeriesObj* s) {
    Expr* c; SeriesObj* u;
    so_split_constant(s, &c, &u);
    if (is_lit_zero(c)) {
        /* Handled by the recursion above via the Power rewrite; here we
         * simply reject. */
        expr_free(c); so_free(u); return NULL;
    }
    if (u->nmin < 1) { expr_free(c); so_free(u); return NULL; }
    Expr* inv_c = simp(mk_power(expr_copy(c), expr_new_integer(-1)));
    SeriesObj* u_over_c = so_scalar_mul(inv_c, u);
    expr_free(inv_c);
    int64_t N = (u_over_c->order - u_over_c->nmin) / (u_over_c->nmin > 0 ? u_over_c->nmin : 1) + 2;
    if (N < 2) N = 2;
    Expr** k = kernel_coefs("Log1p", (size_t)N);
    SeriesObj* log_term = so_compose_scalar_kernel(k, (size_t)N, u_over_c);
    kernel_coefs_free(k, (size_t)N);
    so_free(u_over_c); so_free(u);
    Expr* log_c = simp(mk_fn1("Log", c));
    SeriesObj* c_series = so_from_constant(log_c, log_term->x, log_term->x0, log_term->order, log_term->den);
    expr_free(log_c);
    SeriesObj* r = so_add(c_series, log_term);
    so_free(c_series); so_free(log_term);
    return r;
}

/* Sin[c + u] = Sin[c] Cos[u] + Cos[c] Sin[u]. */
static SeriesObj* so_apply_sin_or_cos(SeriesObj* s, bool is_sin) {
    Expr* c; SeriesObj* u;
    so_split_constant(s, &c, &u);
    if (u->nmin < 1) { expr_free(c); so_free(u); return NULL; }
    int64_t N = (u->order - u->nmin) / (u->nmin > 0 ? u->nmin : 1) + 2;
    if (N < 2) N = 2;
    Expr** ks = kernel_coefs("Sin", (size_t)N);
    Expr** kc = kernel_coefs("Cos", (size_t)N);
    SeriesObj* su = so_compose_scalar_kernel(ks, (size_t)N, u);
    SeriesObj* cu = so_compose_scalar_kernel(kc, (size_t)N, u);
    kernel_coefs_free(ks, (size_t)N);
    kernel_coefs_free(kc, (size_t)N);
    Expr* sinc = simp(mk_fn1("Sin", expr_copy(c)));
    Expr* cosc = simp(mk_fn1("Cos", c));
    SeriesObj* t1, *t2;
    if (is_sin) { t1 = so_scalar_mul(sinc, cu); t2 = so_scalar_mul(cosc, su); }
    else        { t1 = so_scalar_mul(cosc, cu); t2 = so_scalar_mul(sinc, su); }
    expr_free(sinc); expr_free(cosc); so_free(u); so_free(su); so_free(cu);
    SeriesObj* r;
    if (is_sin) { r = so_add(t1, t2); }
    else        { r = so_sub(t1, t2); }
    so_free(t1); so_free(t2);
    return r;
}

/* Apply any odd-series kernel whose series at 0 is known analytically
 * (ArcTan, ArcTanh, ArcSin, ArcSinh) to a series s with zero constant term.
 * These are all analytic at u=0 with the elementary function's value 0
 * there, and their series in u at c = s(x0) requires computing the value
 * and derivatives of f at c. For c != 0 we fall back to evaluating
 * f at c symbolically and using the chain rule via the D-based approach
 * (which the caller can do). Here we only handle c = 0 directly; for
 * c != 0 we return NULL so the dispatcher can pick another path.
 */
static SeriesObj* so_apply_kernel_at_zero(const char* name, SeriesObj* s) {
    Expr* c; SeriesObj* u;
    so_split_constant(s, &c, &u);
    bool c_is_zero = is_lit_zero(c);
    expr_free(c);
    if (!c_is_zero) { so_free(u); return NULL; }
    if (u->nmin < 1) { so_free(u); return NULL; }
    int64_t N = (u->order - u->nmin) / (u->nmin > 0 ? u->nmin : 1) + 2;
    if (N < 2) N = 2;
    Expr** k = kernel_coefs(name, (size_t)N);
    SeriesObj* r = so_compose_scalar_kernel(k, (size_t)N, u);
    kernel_coefs_free(k, (size_t)N);
    so_free(u);
    return r;
}

/* Apply ArcCos[s] = Pi/2 - ArcSin[s]. Requires ArcSin expansion at s(x0). */
static SeriesObj* so_apply_arccos(SeriesObj* s) {
    SeriesObj* as = so_apply_kernel_at_zero("ArcSin", s);
    if (!as) return NULL;
    /* Negate and add Pi/2 constant. */
    SeriesObj* neg = so_neg(as);
    so_free(as);
    Expr* halfpi = simp(mk_times(make_rational(1, 2), mk_symbol("Pi")));
    SeriesObj* k = so_from_constant(halfpi, neg->x, neg->x0, neg->order, neg->den);
    expr_free(halfpi);
    SeriesObj* r = so_add(k, neg);
    so_free(k); so_free(neg);
    return r;
}

/* Apply ArcCot[s] = Pi/2 - ArcTan[s]. */
static SeriesObj* so_apply_arccot(SeriesObj* s) {
    SeriesObj* at = so_apply_kernel_at_zero("ArcTan", s);
    if (!at) return NULL;
    SeriesObj* neg = so_neg(at);
    so_free(at);
    Expr* halfpi = simp(mk_times(make_rational(1, 2), mk_symbol("Pi")));
    SeriesObj* k = so_from_constant(halfpi, neg->x, neg->x0, neg->order, neg->den);
    expr_free(halfpi);
    SeriesObj* r = so_add(k, neg);
    so_free(k); so_free(neg);
    return r;
}

/* Apply ArcCoth[s] = I*Pi/2 + ArcTanh[s] (principal-branch convention that
 * matches Mathematica's Series[ArcCoth[x], {x, 0, n}] output). */
static SeriesObj* so_apply_arccoth(SeriesObj* s) {
    SeriesObj* at = so_apply_kernel_at_zero("ArcTanh", s);
    if (!at) return NULL;
    /* I*Pi/2 = Complex[0, 1/2] * Pi  -- build as Times[Complex[0, 1/2], Pi]. */
    Expr* half = make_rational(1, 2);
    Expr* iconst = expr_new_function(mk_symbol("Complex"),
                        (Expr*[]){ expr_new_integer(0), half }, 2);
    Expr* ihp = simp(mk_times(iconst, mk_symbol("Pi")));
    SeriesObj* k = so_from_constant(ihp, at->x, at->x0, at->order, at->den);
    expr_free(ihp);
    SeriesObj* r = so_add(k, at);
    so_free(k); so_free(at);
    return r;
}

/* Apply ArcCosh[s] = I*ArcCos[s] (principal-branch identity that holds in a
 * neighbourhood of s(x0) = 0 and gives the series matching Mathematica's
 * Series[ArcCosh[x], {x, 0, n}] output: I*Pi/2 - I*x - I*x^3/6 - ...). */
static SeriesObj* so_apply_arccosh(SeriesObj* s) {
    SeriesObj* acs = so_apply_arccos(s);
    if (!acs) return NULL;
    Expr* iconst = expr_new_function(mk_symbol("Complex"),
                        (Expr*[]){ expr_new_integer(0), expr_new_integer(1) }, 2);
    SeriesObj* r = so_scalar_mul(iconst, acs);
    expr_free(iconst); so_free(acs);
    return r;
}

/* Build the series representing I*Pi/2 as a SeriesObj-compatible constant. */
static SeriesObj* so_const_half_i_pi(SeriesObj* tmpl) {
    Expr* half = make_rational(1, 2);
    Expr* iconst = expr_new_function(mk_symbol("Complex"),
                        (Expr*[]){ expr_new_integer(0), half }, 2);
    Expr* ihp = simp(mk_times(iconst, mk_symbol("Pi")));
    SeriesObj* k = so_from_constant(ihp, tmpl->x, tmpl->x0, tmpl->order, tmpl->den);
    expr_free(ihp);
    return k;
}

/* Sinh[c + u] = Sinh[c] Cosh[u] + Cosh[c] Sinh[u].
 * Cosh[c + u] = Cosh[c] Cosh[u] + Sinh[c] Sinh[u]. */
static SeriesObj* so_apply_sinh_or_cosh(SeriesObj* s, bool is_sinh) {
    Expr* c; SeriesObj* u;
    so_split_constant(s, &c, &u);
    if (u->nmin < 1) { expr_free(c); so_free(u); return NULL; }
    int64_t N = (u->order - u->nmin) / (u->nmin > 0 ? u->nmin : 1) + 2;
    if (N < 2) N = 2;
    Expr** ks = kernel_coefs("Sinh", (size_t)N);
    Expr** kc = kernel_coefs("Cosh", (size_t)N);
    SeriesObj* su = so_compose_scalar_kernel(ks, (size_t)N, u);
    SeriesObj* cu = so_compose_scalar_kernel(kc, (size_t)N, u);
    kernel_coefs_free(ks, (size_t)N);
    kernel_coefs_free(kc, (size_t)N);
    Expr* sinc = simp(mk_fn1("Sinh", expr_copy(c)));
    Expr* cosc = simp(mk_fn1("Cosh", c));
    SeriesObj* t1, *t2;
    if (is_sinh) { t1 = so_scalar_mul(sinc, cu); t2 = so_scalar_mul(cosc, su); }
    else         { t1 = so_scalar_mul(cosc, cu); t2 = so_scalar_mul(sinc, su); }
    expr_free(sinc); expr_free(cosc); so_free(u); so_free(su); so_free(cu);
    SeriesObj* r = so_add(t1, t2);
    so_free(t1); so_free(t2);
    return r;
}

/* ----------------------------------------------------------------------------
 * series_expand: recursive descent
 * -------------------------------------------------------------------------- */

typedef struct {
    Expr*   x;      /* expansion variable (borrowed) */
    Expr*   x0;     /* expansion point (borrowed) */
    int64_t order;  /* target order (numerator); den is always 1 entering */
} SeriesCtx;

static SeriesObj* series_expand(Expr* e, const SeriesCtx* ctx);

/* Detect Infinity / ComplexInfinity / Indeterminate / DirectedInfinity
 * anywhere inside e. Used to bail out of naive Taylor before it spins. */
static bool has_infinity(Expr* e) {
    if (!e) return false;
    if (e->type == EXPR_SYMBOL) {
        const char* s = e->data.symbol;
        return (strcmp(s, "Infinity") == 0 ||
                strcmp(s, "ComplexInfinity") == 0 ||
                strcmp(s, "Indeterminate") == 0);
    }
    if (e->type == EXPR_FUNCTION) {
        if (e->data.function.head->type == EXPR_SYMBOL) {
            const char* h = e->data.function.head->data.symbol;
            if (strcmp(h, "DirectedInfinity") == 0 ||
                strcmp(h, "Indeterminate") == 0) return true;
        }
        if (has_infinity(e->data.function.head)) return true;
        for (size_t i = 0; i < e->data.function.arg_count; i++) {
            if (has_infinity(e->data.function.args[i])) return true;
        }
    }
    return false;
}

/* Naive Taylor via D[]: used for heads we don't recognise and for the
 * canonical Series[f[x], {x, a, n}] output shape. Computes coefficients
 * a_k = (D^k f at x0) / k! for k = 0..order-1.
 *
 * Bails out (returns NULL) if the evaluated f(x0) -- or any of its
 * derivatives -- contains Infinity, ComplexInfinity or Indeterminate, so
 * we do not spin when naive Taylor has hit a singularity of f. Also caps
 * the number of D iterations at MAX_NAIVE_ORDER to avoid the exponential
 * expression-size blow-up that hits non-trivial heads like ArcCos; tests
 * that truly need more terms should go through a direct kernel path. */
#define MAX_NAIVE_ORDER 20
static SeriesObj* series_taylor_via_D(Expr* e, const SeriesCtx* ctx) {
    int64_t n_iter = ctx->order;
    if (n_iter > MAX_NAIVE_ORDER) n_iter = MAX_NAIVE_ORDER;
    /* Quick singularity check before starting. */
    Expr* probe = replace_all_of(e, ctx->x, ctx->x0);
    bool bad = has_infinity(probe);
    expr_free(probe);
    if (bad) return NULL;

    SeriesObj* s = so_alloc(ctx->x, ctx->x0, 0, ctx->order, 1);
    Expr* current = expr_copy(e);
    Expr* factorial = expr_new_integer(1);
    for (int64_t k = 0; k < n_iter; k++) {
        if (k > 0) factorial = simp(mk_times(factorial, expr_new_integer(k)));
        Expr* at_x0 = replace_all_of(current, ctx->x, ctx->x0);
        if (has_infinity(at_x0)) {
            expr_free(at_x0); expr_free(current); expr_free(factorial);
            so_free(s); return NULL;
        }
        Expr* inv_fact = simp(mk_power(expr_copy(factorial), expr_new_integer(-1)));
        Expr* coef = simp(mk_times(at_x0, inv_fact));
        so_set_coef(s, (size_t)k, coef);
        if (k + 1 < n_iter) {
            Expr* next = simp(mk_fn2("D", current, expr_copy(ctx->x)));
            current = next;
        }
    }
    expr_free(current);
    expr_free(factorial);
    return s;
}
#undef MAX_NAIVE_ORDER

/* True iff `e` is an elementary head we have a direct kernel for. */
static bool is_known_elementary(Expr* e) {
    if (!e || e->type != EXPR_FUNCTION) return false;
    if (e->data.function.arg_count != 1) return false;
    const char* names[] = {
        "Sin", "Cos", "Tan", "Exp", "Log",
        "Sinh", "Cosh", "Tanh",
        "ArcSin", "ArcCos", "ArcTan", "ArcCot",
        "ArcSinh", "ArcCosh", "ArcTanh", "ArcCoth",
        NULL
    };
    for (int i = 0; names[i]; i++) if (has_symbol_head(e, names[i])) return true;
    return false;
}

/* Reciprocal rewrites. Covers:
 *   Forward-reciprocal trig/hyperbolic heads (Sec, Csc, Cot, Sech, Csch, Coth)
 *   which rewrite as 1/f[z] so the kernel path can handle them.
 *   Inverse-reciprocal heads (ArcSec, ArcCsc, ArcSech, ArcCsch) which rewrite
 *   via the identities
 *     ArcSec[z]  = ArcCos[1/z]
 *     ArcCsc[z]  = ArcSin[1/z]
 *     ArcSech[z] = ArcCosh[1/z]
 *     ArcCsch[z] = ArcSinh[1/z]
 *   so that composition with a blowing-up inner series (e.g. z = 1/x)
 *   collapses to a convergent kernel case rather than triggering naive
 *   Taylor's x=x0 probe (which would emit spurious `Power::infy: 1/0`
 *   warnings before the guard bails out).
 *
 * Returns a new expression (caller owns) or NULL if not applicable. */
static Expr* rewrite_reciprocal_head(Expr* e) {
    if (!e || e->type != EXPR_FUNCTION || e->data.function.arg_count != 1) return NULL;
    if (e->data.function.head->type != EXPR_SYMBOL) return NULL;
    const char* h = e->data.function.head->data.symbol;
    Expr* arg = expr_copy(e->data.function.args[0]);
    if (strcmp(h, "Sec")  == 0) return mk_power(mk_fn1("Cos",  arg), expr_new_integer(-1));
    if (strcmp(h, "Csc")  == 0) return mk_power(mk_fn1("Sin",  arg), expr_new_integer(-1));
    if (strcmp(h, "Cot")  == 0) return mk_times(mk_fn1("Cos",  expr_copy(arg)),
                                                mk_power(mk_fn1("Sin",  arg), expr_new_integer(-1)));
    if (strcmp(h, "Sech") == 0) return mk_power(mk_fn1("Cosh", arg), expr_new_integer(-1));
    if (strcmp(h, "Csch") == 0) return mk_power(mk_fn1("Sinh", arg), expr_new_integer(-1));
    if (strcmp(h, "Coth") == 0) return mk_times(mk_fn1("Cosh", expr_copy(arg)),
                                                mk_power(mk_fn1("Sinh", arg), expr_new_integer(-1)));
    /* Inverse-reciprocal identities. simp() collapses 1/(1/x) back to x, so
     * e.g. ArcSec[1/x] becomes ArcCos[x] and dispatches to the ArcCos kernel. */
    if (strcmp(h, "ArcSec")  == 0) return mk_fn1("ArcCos",  simp(mk_power(arg, expr_new_integer(-1))));
    if (strcmp(h, "ArcCsc")  == 0) return mk_fn1("ArcSin",  simp(mk_power(arg, expr_new_integer(-1))));
    if (strcmp(h, "ArcSech") == 0) return mk_fn1("ArcCosh", simp(mk_power(arg, expr_new_integer(-1))));
    if (strcmp(h, "ArcCsch") == 0) return mk_fn1("ArcSinh", simp(mk_power(arg, expr_new_integer(-1))));
    expr_free(arg);
    return NULL;
}

static SeriesObj* series_expand(Expr* e, const SeriesCtx* ctx) {
    /* Early out: free of x => constant series. */
    if (expr_free_of(e, ctx->x)) {
        return so_from_constant(e, ctx->x, ctx->x0, ctx->order, 1);
    }
    /* e == x => identity series. */
    if (expr_eq(e, ctx->x)) {
        return so_from_variable(ctx->x, ctx->x0, ctx->order, 1);
    }

    if (e->type == EXPR_FUNCTION) {
        const char* head = (e->data.function.head->type == EXPR_SYMBOL)
                               ? e->data.function.head->data.symbol : NULL;
        /* ---- Plus ---- */
        if (head && strcmp(head, "Plus") == 0) {
            SeriesObj* acc = NULL;
            for (size_t i = 0; i < e->data.function.arg_count; i++) {
                SeriesObj* t = series_expand(e->data.function.args[i], ctx);
                if (!t) { if (acc) so_free(acc); return NULL; }
                if (!acc) acc = t;
                else { SeriesObj* sum = so_add(acc, t); so_free(acc); so_free(t); acc = sum; }
            }
            if (!acc) acc = so_from_constant(expr_new_integer(0), ctx->x, ctx->x0, ctx->order, 1);
            return acc;
        }
        /* ---- Times ---- */
        if (head && strcmp(head, "Times") == 0) {
            SeriesObj* acc = NULL;
            for (size_t i = 0; i < e->data.function.arg_count; i++) {
                SeriesObj* t = series_expand(e->data.function.args[i], ctx);
                if (!t) { if (acc) so_free(acc); return NULL; }
                if (!acc) acc = t;
                else { SeriesObj* mul = so_mul(acc, t); so_free(acc); so_free(t); acc = mul; }
            }
            if (!acc) acc = so_from_constant(expr_new_integer(1), ctx->x, ctx->x0, ctx->order, 1);
            return acc;
        }
        /* ---- Power ---- */
        if (head && strcmp(head, "Power") == 0 && e->data.function.arg_count == 2) {
            Expr* base = e->data.function.args[0];
            Expr* exp  = e->data.function.args[1];
            bool base_has_x = !expr_free_of(base, ctx->x);
            bool exp_has_x  = !expr_free_of(exp, ctx->x);
            if (!base_has_x && !exp_has_x) {
                return so_from_constant(e, ctx->x, ctx->x0, ctx->order, 1);
            }
            if (base_has_x && !exp_has_x) {
                SeriesObj* bs = series_expand(base, ctx);
                if (!bs) return NULL;
                SeriesObj* r = so_pow_expr(bs, exp);
                so_free(bs);
                return r;
            }
            /* Exponent depends on x: rewrite as Exp[exp * Log[base]] and recurse. */
            Expr* log_base = mk_fn1("Log", expr_copy(base));
            Expr* prod = mk_times(expr_copy(exp), log_base);
            Expr* rew = mk_fn1("Exp", prod);
            SeriesObj* r = series_expand(rew, ctx);
            expr_free(rew);
            return r;
        }
        /* ---- Log[x]-at-x=0 symbolic coefficient ---- */
        if (head && strcmp(head, "Log") == 0 && e->data.function.arg_count == 1) {
            Expr* arg = e->data.function.args[0];
            /* Log[c*(x-x0)^r * (1+u)] = r*Log[x-x0] + Log[c] + Log[1+u].
             * For x0 = 0 and arg = x, this is Log[x] -- keep symbolic. */
            if (expr_eq(arg, ctx->x) && is_lit_zero(ctx->x0)) {
                return so_from_constant(e, ctx->x, ctx->x0, ctx->order, 1);
            }
            /* General: expand arg, split leading monomial if vanishing. */
            SeriesObj* as = series_expand(arg, ctx);
            if (!as) return NULL;
            so_trim_leading(as);
            if (as->coef_count == 0) { so_free(as); return NULL; }
            int64_t nmin = as->nmin;
            if (nmin == 0) {
                SeriesObj* r = so_apply_log(as);
                so_free(as);
                return r;
            }
            /* nmin != 0: arg vanishes (or blows up) at x0. Peel off
             * (x-x0)^(nmin/den) and treat its log as a symbolic constant. */
            Expr* a0 = expr_copy(as->coefs[0]);
            Expr* inv_a0 = simp(mk_power(expr_copy(a0), expr_new_integer(-1)));
            SeriesObj* a_scaled = so_scalar_mul(inv_a0, as);
            expr_free(inv_a0);
            SeriesObj* shifted = so_shift_by_rational(a_scaled, -nmin, as->den);
            so_free(a_scaled);
            SeriesObj* log_rest = so_apply_log(shifted);
            so_free(shifted); so_free(as);
            if (!log_rest) { expr_free(a0); return NULL; }
            /* Add symbolic Log[a0] + (nmin/den) * Log[x - x0]. */
            Expr* log_a0 = simp(mk_fn1("Log", a0));
            Expr* base_sym;
            if (is_lit_zero(ctx->x0)) base_sym = expr_copy(ctx->x);
            else                      base_sym = simp(mk_plus(expr_copy(ctx->x),
                                                              simp(mk_times(expr_new_integer(-1),
                                                                            expr_copy(ctx->x0)))));
            Expr* log_base = simp(mk_fn1("Log", base_sym));
            Expr* n_over_d = make_rational(nmin, log_rest->den);
            Expr* extra = simp(mk_plus(log_a0, simp(mk_times(n_over_d, log_base))));
            SeriesObj* extras = so_from_constant(extra, log_rest->x, log_rest->x0, log_rest->order, log_rest->den);
            expr_free(extra);
            SeriesObj* r = so_add(extras, log_rest);
            so_free(extras); so_free(log_rest);
            return r;
        }
        /* ---- Reciprocal heads (Sec, Csc, Cot, Sech, Csch, Coth) ---- */
        {
            Expr* rewrite = rewrite_reciprocal_head(e);
            if (rewrite) {
                SeriesObj* r = series_expand(rewrite, ctx);
                expr_free(rewrite);
                return r;
            }
        }
        /* ---- Known elementary functions ---- */
        if (is_known_elementary(e)) {
            SeriesObj* inner = series_expand(e->data.function.args[0], ctx);
            if (!inner) return NULL;
            SeriesObj* r = NULL;
            if (strcmp(head, "Exp") == 0)       r = so_apply_exp(inner);
            else if (strcmp(head, "Sin") == 0)  r = so_apply_sin_or_cos(inner, true);
            else if (strcmp(head, "Cos") == 0)  r = so_apply_sin_or_cos(inner, false);
            else if (strcmp(head, "Sinh") == 0) r = so_apply_sinh_or_cosh(inner, true);
            else if (strcmp(head, "Cosh") == 0) r = so_apply_sinh_or_cosh(inner, false);
            else if (strcmp(head, "Tan") == 0) {
                SeriesObj* sn = so_apply_sin_or_cos(inner, true);
                SeriesObj* cs = so_apply_sin_or_cos(inner, false);
                if (sn && cs) r = so_div(sn, cs);
                if (sn) so_free(sn);
                if (cs) so_free(cs);
            } else if (strcmp(head, "Tanh") == 0) {
                SeriesObj* sn = so_apply_sinh_or_cosh(inner, true);
                SeriesObj* cs = so_apply_sinh_or_cosh(inner, false);
                if (sn && cs) r = so_div(sn, cs);
                if (sn) so_free(sn);
                if (cs) so_free(cs);
            } else if (strcmp(head, "Log") == 0) {
                r = so_apply_log(inner);
            } else if (strcmp(head, "ArcTan")  == 0) r = so_apply_kernel_at_zero("ArcTan",   inner);
            else if (strcmp(head, "ArcTanh") == 0) {
                r = so_apply_kernel_at_zero("ArcTanh", inner);
                /* If arg blows up at x0, use principal-branch identity
                 * ArcTanh[1/u] = I*Pi/2 + ArcTanh[u]. */
                if (!r && inner->nmin < 0) {
                    SeriesObj* inv = so_inv(inner);
                    if (inv) {
                        SeriesObj* at = so_apply_kernel_at_zero("ArcTanh", inv);
                        so_free(inv);
                        if (at) {
                            SeriesObj* k = so_const_half_i_pi(at);
                            r = so_add(k, at);
                            so_free(k); so_free(at);
                        }
                    }
                }
            }
            else if (strcmp(head, "ArcSin")  == 0) r = so_apply_kernel_at_zero("ArcSin",   inner);
            else if (strcmp(head, "ArcSinh") == 0) {
                r = so_apply_kernel_at_zero("ArcSinh", inner);
                /* Identity at infinity: ArcSinh[1/v] = -Log[v] + Log[1 + Sqrt[1 + v^2]].
                 * Fire when inner actually blows up (nmin < 0); series_expand's
                 * Log-at-x branch absorbs the symbolic -Log[v]. */
                if (!r && inner->nmin < 0) {
                    Expr* arg = e->data.function.args[0];
                    Expr* v = simp(mk_power(expr_copy(arg), expr_new_integer(-1)));
                    Expr* v_sq = simp(mk_power(expr_copy(v), expr_new_integer(2)));
                    Expr* sqrt_term = simp(mk_power(
                        simp(mk_plus(expr_new_integer(1), v_sq)),
                        make_rational(1, 2)));
                    Expr* log2 = simp(mk_fn1("Log",
                                    simp(mk_plus(expr_new_integer(1), sqrt_term))));
                    Expr* neg_log_v = simp(mk_times(expr_new_integer(-1),
                                       simp(mk_fn1("Log", v))));
                    Expr* rewrite = simp(mk_plus(neg_log_v, log2));
                    r = series_expand(rewrite, ctx);
                    expr_free(rewrite);
                }
            }
            else if (strcmp(head, "ArcCos")  == 0) r = so_apply_arccos(inner);
            else if (strcmp(head, "ArcCosh") == 0) {
                r = so_apply_arccosh(inner);
                /* Identity at infinity: ArcCosh[1/v] = -Log[v] + Log[1 + Sqrt[1 - v^2]]. */
                if (!r && inner->nmin < 0) {
                    Expr* arg = e->data.function.args[0];
                    Expr* v = simp(mk_power(expr_copy(arg), expr_new_integer(-1)));
                    Expr* v_sq = simp(mk_power(expr_copy(v), expr_new_integer(2)));
                    Expr* one_minus_v2 = simp(mk_plus(expr_new_integer(1),
                                          simp(mk_times(expr_new_integer(-1), v_sq))));
                    Expr* sqrt_term = simp(mk_power(one_minus_v2, make_rational(1, 2)));
                    Expr* log2 = simp(mk_fn1("Log",
                                    simp(mk_plus(expr_new_integer(1), sqrt_term))));
                    Expr* neg_log_v = simp(mk_times(expr_new_integer(-1),
                                       simp(mk_fn1("Log", v))));
                    Expr* rewrite = simp(mk_plus(neg_log_v, log2));
                    r = series_expand(rewrite, ctx);
                    expr_free(rewrite);
                }
            }
            else if (strcmp(head, "ArcCot")  == 0) {
                r = so_apply_arccot(inner);
                /* If arg blows up at x0 (e.g. 1/x), fall back to the
                 * at-infinity branch: ArcCot[u] = ArcTan[1/u]. */
                if (!r && inner->nmin < 0) {
                    SeriesObj* inv = so_inv(inner);
                    if (inv) { r = so_apply_kernel_at_zero("ArcTan", inv); so_free(inv); }
                }
            }
            else if (strcmp(head, "ArcCoth") == 0) {
                r = so_apply_arccoth(inner);
                /* If arg blows up at x0 (e.g. 1/x), use ArcCoth[u] = ArcTanh[1/u]. */
                if (!r && inner->nmin < 0) {
                    SeriesObj* inv = so_inv(inner);
                    if (inv) { r = so_apply_kernel_at_zero("ArcTanh", inv); so_free(inv); }
                }
            }
            so_free(inner);
            return r;
        }
    }

    /* Fallback: naive Taylor via D. */
    return series_taylor_via_D(e, ctx);
}

/* ----------------------------------------------------------------------------
 * Normal
 * -------------------------------------------------------------------------- */

static Expr* series_build_xmx0(Expr* x, Expr* x0) {
    if ((x0->type == EXPR_INTEGER && x0->data.integer == 0) ||
        (x0->type == EXPR_REAL && x0->data.real == 0.0)) {
        return expr_copy(x);
    }
    Expr* neg = simp(mk_times(expr_new_integer(-1), expr_copy(x0)));
    return simp(mk_plus(expr_copy(x), neg));
}

Expr* builtin_normal(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* arg = res->data.function.args[0];

    if (!has_symbol_head(arg, "SeriesData") || arg->data.function.arg_count != 6) {
        /* Pass-through for any non-SeriesData argument. */
        return expr_copy(arg);
    }

    Expr* x      = arg->data.function.args[0];
    Expr* x0     = arg->data.function.args[1];
    Expr* coefs  = arg->data.function.args[2];
    Expr* nmin_e = arg->data.function.args[3];
    Expr* den_e  = arg->data.function.args[5];

    if (!has_symbol_head(coefs, "List") ||
        nmin_e->type != EXPR_INTEGER ||
        den_e->type  != EXPR_INTEGER ||
        den_e->data.integer == 0) return NULL;

    int64_t nmin = nmin_e->data.integer;
    int64_t den  = den_e->data.integer;
    size_t k     = coefs->data.function.arg_count;

    Expr* base = series_build_xmx0(x, x0);
    Expr** terms = calloc(k + 1, sizeof(Expr*));
    size_t tc = 0;
    for (size_t i = 0; i < k; i++) {
        Expr* coef = coefs->data.function.args[i];
        if (is_lit_zero(coef)) continue;
        int64_t num = nmin + (int64_t)i;
        Expr* exp_e = (den == 1) ? expr_new_integer(num) : make_rational(num, den);
        Expr* term;
        if (is_lit_zero(exp_e)) { expr_free(exp_e); term = expr_copy(coef); }
        else {
            Expr* p = simp(mk_power(expr_copy(base), exp_e));
            term = simp(mk_times(expr_copy(coef), p));
        }
        terms[tc++] = term;
    }
    expr_free(base);

    Expr* sum;
    if (tc == 0)      sum = expr_new_integer(0);
    else if (tc == 1) sum = terms[0];
    else {
        sum = expr_new_function(mk_symbol("Plus"), terms, tc);
        terms = NULL;
    }
    if (terms) free(terms);

    return simp(sum);
}

/* ----------------------------------------------------------------------------
 * Series builtin
 * -------------------------------------------------------------------------- */

/* Parse a single spec argument, accepting either `{x, x0, n}` (full form)
 * or `x -> x0` (leading-term form). Returns true on success and populates
 * *x_out / *x0_out / *n_out (borrowed Expr pointers into the spec) and
 * sets *leading_only accordingly. */
static bool parse_series_spec(Expr* spec, Expr** x_out, Expr** x0_out,
                              int64_t* n_out, bool* leading_only) {
    if (has_symbol_head(spec, "List") && spec->data.function.arg_count == 3) {
        Expr* n_e = spec->data.function.args[2];
        if (n_e->type != EXPR_INTEGER) {
            /* Evaluate in case the user passed a computable expression. */
            Expr* ev = evaluate(expr_copy(n_e));
            if (ev->type != EXPR_INTEGER) { expr_free(ev); return false; }
            /* Replace into the spec's own slot for the caller to read back;
             * but we just populate outputs and leak the eval? Easier: we
             * return the integer value directly. */
            *x_out         = spec->data.function.args[0];
            *x0_out        = spec->data.function.args[1];
            *n_out         = ev->data.integer;
            *leading_only  = false;
            expr_free(ev);
            return true;
        }
        *x_out         = spec->data.function.args[0];
        *x0_out        = spec->data.function.args[1];
        *n_out         = n_e->data.integer;
        *leading_only  = false;
        return true;
    }
    if (has_symbol_head(spec, "Rule") && spec->data.function.arg_count == 2) {
        *x_out         = spec->data.function.args[0];
        *x0_out        = spec->data.function.args[1];
        *n_out         = 0;
        *leading_only  = true;
        return true;
    }
    return false;
}

/* Expand f around x=x0 to order n and return SeriesData expression. */
static Expr* do_series_single(Expr* f, Expr* x, Expr* x0, int64_t n, bool leading_only) {
    /* Evaluate f with the series context implicit. Since Series has
     * HoldAll, f has not been evaluated yet; we evaluate now. */
    Expr* f_eval = evaluate(expr_copy(f));
    Expr* x0_eval = evaluate(expr_copy(x0));

    /* Handle expansion at Infinity by substituting x -> 1/u. */
    bool at_infinity = (x0_eval->type == EXPR_SYMBOL &&
                        (strcmp(x0_eval->data.symbol, "Infinity") == 0));
    Expr* f_use  = f_eval;
    Expr* x_use  = x;
    Expr* x0_use = x0_eval;
    Expr* u_sym  = NULL;
    if (at_infinity) {
        u_sym = mk_symbol("$SeriesInvVar$");
        Expr* inv_u = mk_power(expr_copy(u_sym), expr_new_integer(-1));
        Expr* f_sub = replace_all_of(f_eval, x, inv_u);
        expr_free(inv_u);
        f_use = f_sub;
        x_use = u_sym;
        expr_free(x0_eval);
        x0_use = expr_new_integer(0);
    }

    /* target_exp_n is the user-facing O-term exponent, i.e. x^target_exp_n
     * is the smallest power that has been dropped. Series[f,{x,x0,n}]
     * sets this to n; Series[f, x->x0] emits O[x-x0]^2 per Mathematica's
     * observed behaviour (first-order approximation), so target_exp_n = 1. */
    int64_t target_exp_n = leading_only ? 1 : n;
    int64_t order = target_exp_n + 1;
    if (order < 1) order = 1;

    /* Laurent and compound expressions lose O-term accuracy when negative
     * exponents flow through multiplication; we compensate by expanding
     * internally to a padded order, then truncating the result back down
     * to the user-requested O-term. Padding = 15 is chosen to survive
     * deep Laurent expansions like 1/Sin[x]^10 while keeping big-rational
     * coefficient arithmetic (Puiseux, logarithmic) tractable. */
    int64_t pad = 12;
    int64_t internal_order = order + pad;

    SeriesCtx ctx = { x_use, x0_use, internal_order };
    SeriesObj* s = series_expand(f_use, &ctx);
    Expr* result = NULL;
    if (s) {
        /* Truncate to the user-facing O-term exponent.
         * User asked for x^target_exp_n to be the first dropped term.
         * In the series's internal denominator this is target_exp_n*den
         * plus one step: terms at exponents strictly less than
         * target_exp_n + 1/den are kept, and the O-term is placed at
         * the next valid step beyond target_exp_n. */
        int64_t target_num = target_exp_n * s->den + 1;
        if (s->order > target_num) {
            SeriesObj* t = so_alloc(s->x, s->x0, s->nmin, target_num, s->den);
            for (size_t i = 0; i < t->coef_count && i < s->coef_count; i++) {
                so_set_coef(t, i, expr_copy(s->coefs[i]));
            }
            so_free(s);
            s = t;
        }
        if (at_infinity) {
            Expr* inv_x = mk_power(expr_copy(x), expr_new_integer(-1));
            expr_free(s->x);
            s->x = inv_x;
        }
        result = so_to_expr(s);
        so_free(s);
    }
    if (at_infinity) { expr_free(f_use); expr_free(u_sym); }
    else             expr_free(x0_eval);
    expr_free(f_eval);
    if (at_infinity) expr_free(x0_use);
    return result;
}

Expr* builtin_series(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count < 2) return NULL;
    Expr* f = res->data.function.args[0];

    /* List threading on the first argument (not ATTR_LISTABLE since the
     * specs must not thread). */
    if (has_symbol_head(f, "List")) {
        size_t n = f->data.function.arg_count;
        Expr** threaded = calloc(n, sizeof(Expr*));
        for (size_t i = 0; i < n; i++) {
            Expr** new_args = calloc(res->data.function.arg_count, sizeof(Expr*));
            new_args[0] = expr_copy(f->data.function.args[i]);
            for (size_t j = 1; j < res->data.function.arg_count; j++) {
                new_args[j] = expr_copy(res->data.function.args[j]);
            }
            Expr* call = expr_new_function(mk_symbol("Series"), new_args,
                                           res->data.function.arg_count);
            threaded[i] = evaluate(call);
        }
        Expr* lst = expr_new_function(mk_symbol("List"), threaded, n);
        free(threaded);
        return lst;
    }

    /* Parse specs. */
    size_t spec_count = res->data.function.arg_count - 1;
    Expr** specs = res->data.function.args + 1;

    /* Multivariate: process right-to-left. Expand f first in the rightmost
     * variable, which produces a series whose coefficients are themselves
     * expressions in the remaining variables. Then expand each coefficient
     * in the next variable. */
    if (spec_count == 1) {
        Expr* x_arg; Expr* x0_arg; int64_t n_val; bool leading;
        if (!parse_series_spec(specs[0], &x_arg, &x0_arg, &n_val, &leading)) return NULL;
        return do_series_single(f, x_arg, x0_arg, n_val, leading);
    }

    /* For multivariate: expand outermost (leftmost) first -- the natural
     * reading matches Mathematica: Series[f, spec_x, spec_y] means
     * expand in x first, getting coefs that are expressions in y, then
     * expand each coef in y. */
    Expr* x_arg; Expr* x0_arg; int64_t n_val; bool leading;
    if (!parse_series_spec(specs[0], &x_arg, &x0_arg, &n_val, &leading)) return NULL;
    Expr* outer = do_series_single(f, x_arg, x0_arg, n_val, leading);
    if (!outer) return NULL;

    /* outer is SeriesData[x, x0, {a0, a1, ...}, 0, n+1, 1]. Recurse on
     * each coefficient with the remaining specs. */
    if (!has_symbol_head(outer, "SeriesData") || outer->data.function.arg_count != 6) {
        return outer;
    }
    Expr* coefs = outer->data.function.args[2];
    for (size_t i = 0; i < coefs->data.function.arg_count; i++) {
        /* Build Series[ai, spec_y, spec_z, ...] and evaluate. */
        size_t rest = spec_count - 1;
        Expr** new_args = calloc(rest + 1, sizeof(Expr*));
        new_args[0] = expr_copy(coefs->data.function.args[i]);
        for (size_t j = 0; j < rest; j++) new_args[j + 1] = expr_copy(specs[j + 1]);
        Expr* call = expr_new_function(mk_symbol("Series"), new_args, rest + 1);
        free(new_args);
        Expr* expanded = evaluate(call);
        expr_free(coefs->data.function.args[i]);
        coefs->data.function.args[i] = expanded;
    }
    return outer;
}

/* ----------------------------------------------------------------------------
 * Initialization
 * -------------------------------------------------------------------------- */

void series_init(void) {
    symtab_get_def("SeriesData")->attributes |= ATTR_PROTECTED;
    symtab_add_builtin("Series", builtin_series);
    symtab_get_def("Series")->attributes |= ATTR_HOLDALL | ATTR_PROTECTED;
    symtab_add_builtin("Normal", builtin_normal);
    symtab_get_def("Normal")->attributes |= ATTR_PROTECTED;
}
