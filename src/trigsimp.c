#include "trigsimp.h"
#include "attr.h"
#include "eval.h"
#include "parse.h"
#include "symtab.h"
#include <stdlib.h>
#include <string.h>

/*
 * trigsimp.c
 *
 * Trigonometric simplification builtins:
 *   TrigToExp  -- rewrite circular/hyperbolic (and their inverses) as Exp/Log.
 *   ExpToTrig  -- rewrite Exp/Log forms back into circular/hyperbolic functions.
 *   TrigExpand -- expand sums and integer multiples that appear in arguments of
 *                 trigonometric (circular and hyperbolic) functions.
 */

static Expr* trig_to_exp_rules = NULL;

static Expr* exp_to_trig_rules = NULL;
static Expr* exp_to_trig_simp = NULL;

static Expr* trig_expand_rules = NULL;
static Expr* trig_expand_pythag = NULL;

Expr* builtin_exptotrig(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* arg = res->data.function.args[0];

    Expr* replace_args[2] = { expr_copy(arg), expr_copy(exp_to_trig_rules) };
    Expr* replace_expr = expr_new_function(expr_new_symbol("ReplaceRepeated"), replace_args, 2);
    Expr* replaced = evaluate(replace_expr);
    expr_free(replace_expr);

    Expr* together_args[1] = { replaced };
    Expr* together_expr = expr_new_function(expr_new_symbol("Together"), together_args, 1);
    Expr* togethered = evaluate(together_expr);
    expr_free(together_expr);

    Expr* cancel_args[1] = { togethered };
    Expr* cancel_expr = expr_new_function(expr_new_symbol("Cancel"), cancel_args, 1);
    Expr* cancelled = evaluate(cancel_expr);
    expr_free(cancel_expr);

    Expr* rep_args[2] = { cancelled, expr_copy(exp_to_trig_simp) };
    Expr* rep_simp = expr_new_function(expr_new_symbol("ReplaceRepeated"), rep_args, 2);
    Expr* result = evaluate(rep_simp);
    expr_free(rep_simp);

    return result;
}

Expr* builtin_trigtoexp(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* arg = res->data.function.args[0];

    Expr* replace_args[2] = { expr_copy(arg), expr_copy(trig_to_exp_rules) };
    Expr* replace_expr = expr_new_function(expr_new_symbol("ReplaceAll"), replace_args, 2);
    Expr* replaced = evaluate(replace_expr);
    expr_free(replace_expr);

    Expr* expand_args[1] = { replaced };
    Expr* expand_expr = expr_new_function(expr_new_symbol("Expand"), expand_args, 1);
    Expr* result = evaluate(expand_expr);
    expr_free(expand_expr);

    return result;
}

/* Heads over which TrigExpand threads (in addition to List, which is handled
 * via the ATTR_LISTABLE attribute). Mathematica's TrigExpand threads over
 * equations, inequalities, and logic functions.
 */
static int trigexpand_threads_over(const char* name) {
    return strcmp(name, "Equal") == 0 ||
           strcmp(name, "Unequal") == 0 ||
           strcmp(name, "Less") == 0 ||
           strcmp(name, "LessEqual") == 0 ||
           strcmp(name, "Greater") == 0 ||
           strcmp(name, "GreaterEqual") == 0 ||
           strcmp(name, "SameQ") == 0 ||
           strcmp(name, "UnsameQ") == 0 ||
           strcmp(name, "And") == 0 ||
           strcmp(name, "Or") == 0 ||
           strcmp(name, "Not") == 0 ||
           strcmp(name, "Xor") == 0 ||
           strcmp(name, "Implies") == 0;
}

/*
 * Test whether an expression tree contains any Power[_, n] subterm with a
 * negative-integer exponent. This is how picocas represents reciprocals
 * (e.g. 1/x = Power[x, -1], a/b = Times[a, Power[b, -1]]). Expand does not
 * distribute across negative powers, so the Factor-based Pythagorean
 * collapse must not be applied to expressions that hide structure inside a
 * denominator -- otherwise we would permanently replace a canonical form
 * such as "Cos[x]^2 - Sin[x]^2" with its factored denominator form
 * "(Cos[x] + Sin[x])(Cos[x] - Sin[x])".
 */
static int has_reciprocal_power(const Expr* e) {
    if (!e || e->type != EXPR_FUNCTION) return 0;
    if (e->data.function.head &&
        e->data.function.head->type == EXPR_SYMBOL &&
        strcmp(e->data.function.head->data.symbol, "Power") == 0 &&
        e->data.function.arg_count == 2) {
        const Expr* exp = e->data.function.args[1];
        if (exp->type == EXPR_INTEGER && exp->data.integer < 0) return 1;
    }
    if (has_reciprocal_power(e->data.function.head)) return 1;
    for (size_t i = 0; i < e->data.function.arg_count; i++) {
        if (has_reciprocal_power(e->data.function.args[i])) return 1;
    }
    return 0;
}

Expr* builtin_trigexpand(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* arg = res->data.function.args[0];

    /* Thread over equations, inequalities, and logic heads. */
    if (arg->type == EXPR_FUNCTION &&
        arg->data.function.head &&
        arg->data.function.head->type == EXPR_SYMBOL &&
        trigexpand_threads_over(arg->data.function.head->data.symbol)) {
        size_t n = arg->data.function.arg_count;
        Expr** new_args = (Expr**)calloc(n, sizeof(Expr*));
        if (!new_args) return NULL;
        for (size_t i = 0; i < n; i++) {
            Expr* inner[1] = { expr_copy(arg->data.function.args[i]) };
            Expr* wrap = expr_new_function(expr_new_symbol("TrigExpand"), inner, 1);
            new_args[i] = evaluate(wrap);
            expr_free(wrap);
        }
        return expr_new_function(expr_copy(arg->data.function.head), new_args, n);
    }

    /* Apply the angle-addition and multiple-angle rules to a fixed point. */
    Expr* replace_args[2] = { expr_copy(arg), expr_copy(trig_expand_rules) };
    Expr* replace_expr = expr_new_function(expr_new_symbol("ReplaceRepeated"), replace_args, 2);
    Expr* replaced = evaluate(replace_expr);
    expr_free(replace_expr);

    /* Distribute products of sums so the result is a flat sum of monomials. */
    Expr* expand_args[1] = { replaced };
    Expr* expand_expr = expr_new_function(expr_new_symbol("Expand"), expand_args, 1);
    Expr* expanded = evaluate(expand_expr);
    expr_free(expand_expr);

    /*
     * Factor-based Pythagorean collapse.
     *
     * Expanding Sin[n*x]^2 + Cos[n*x]^2 produces (Sin[x]^2 + Cos[x]^2)^n in
     * distributed form -- a polynomial Factor recognises. Applying the
     * (Sin[x]^2 + Cos[x]^2)^n -> 1 rule after Factor collapses it, and the
     * final Expand restores the canonical monomial form for any non-collapsing
     * subexpressions (so e.g. "Cos[x]^2 - Sin[x]^2" survives unchanged).
     *
     * Expand does not distribute across negative powers, so applying Factor
     * to an expression with a denominator would leave the denominator in
     * factored form permanently (e.g. Tan[2 x] -> 2 Cos Sin / ((Cos+Sin)(Cos-Sin))
     * instead of 2 Cos Sin / (Cos^2 - Sin^2)). Skip the Factor-based pass for
     * such expressions and only run the direct-sum Pythagorean rules.
     */
    Expr* reduced;
    if (has_reciprocal_power(expanded)) {
        Expr* pyth_args[2] = { expanded, expr_copy(trig_expand_pythag) };
        Expr* pyth_expr = expr_new_function(expr_new_symbol("ReplaceRepeated"), pyth_args, 2);
        reduced = evaluate(pyth_expr);
        expr_free(pyth_expr);
    } else {
        Expr* factor_args[1] = { expanded };
        Expr* factor_expr = expr_new_function(expr_new_symbol("Factor"), factor_args, 1);
        Expr* factored = evaluate(factor_expr);
        expr_free(factor_expr);

        Expr* pyth_args[2] = { factored, expr_copy(trig_expand_pythag) };
        Expr* pyth_expr = expr_new_function(expr_new_symbol("ReplaceRepeated"), pyth_args, 2);
        reduced = evaluate(pyth_expr);
        expr_free(pyth_expr);
    }

    /* Re-expand so that any residual factored products are distributed back
     * into the canonical TrigExpand monomial form. */
    Expr* final_args[1] = { reduced };
    Expr* final_expr = expr_new_function(expr_new_symbol("Expand"), final_args, 1);
    Expr* result = evaluate(final_expr);
    expr_free(final_expr);

    return result;
}

void trigsimp_init(void) {
    const char* rules_str = "{ "
        "Sin[x_] :> 1/2 I E^(-I x) - 1/2 I E^(I x), "
        "Cos[x_] :> E^(-I x)/2 + E^(I x)/2, "
        "Tan[x_] :> (I (E^(-I x) - E^(I x)))/(E^(-I x) + E^(I x)), "
        "Cot[x_] :> (-I (E^(-I x) + E^(I x)))/(E^(-I x) - E^(I x)), "
        "Sec[x_] :> 2/(E^(-I x) + E^(I x)), "
        "Csc[x_] :> (2 I)/(E^(I x) - E^(-I x)), "
        "Sinh[x_] :> -(E^-x/2) + E^x/2, "
        "Cosh[x_] :> E^-x/2 + E^x/2, "
        "Tanh[x_] :> -(E^-x/(E^-x+E^x)) + E^x/(E^-x+E^x), "
        "Coth[x_] :> E^-x/(E^-x-E^x) + E^x/(E^-x-E^x), "
        "Sech[x_] :> 2/(E^-x + E^x), "
        "Csch[x_] :> 2/(-E^-x + E^x), "
        "ArcSin[x_] :> -I Log[I x + Sqrt[1 - x^2]], "
        "ArcCos[x_] :> Pi/2 + I Log[I x + Sqrt[1 - x^2]], "
        "ArcTan[x_] :> 1/2 I Log[1 - I x] - 1/2 I Log[1 + I x], "
        "ArcTan[x_, y_] :> -I Log[(x + I y)/Sqrt[x^2 + y^2]], "
        "ArcCot[x_] :> 1/2 I Log[1 - I/x] - 1/2 I Log[1 + I/x], "
        "ArcSec[x_] :> Pi/2 + I Log[I/x + Sqrt[1 - 1/x^2]], "
        "ArcCsc[x_] :> -I Log[I/x + Sqrt[1 - 1/x^2]], "
        "ArcSinh[x_] :> Log[x + Sqrt[1 + x^2]], "
        "ArcCosh[x_] :> Log[x + Sqrt[-1 + x] Sqrt[1 + x]], "
        "ArcTanh[x_] :> -(1/2) Log[1 - x] + 1/2 Log[1 + x], "
        "ArcCoth[x_] :> -(1/2) Log[1 - 1/x] + 1/2 Log[1 + 1/x], "
        "ArcSech[x_] :> Log[1/x + Sqrt[-1 + 1/x] Sqrt[1 + 1/x]], "
        "ArcCsch[x_] :> Log[1/x + Sqrt[1 + 1/x^2]] "
    "}";

    trig_to_exp_rules = parse_expression(rules_str);

    const char* exp_to_trig_rules_str = "{ "
        "E^((I) x_.) :> Cos[x] + I Sin[x], "
        "E^((-I) x_.) :> Cos[x] - I Sin[x], "
        "E^(-x_.) :> Cosh[x] - Sinh[x], "
        "E^x_. :> Cosh[x] + Sinh[x], "
        "Log[a_ + (I) x_] - Log[a_ - (I) x_] :> 2 I ArcTan[x/a], "
        "Log[a_ - (I) x_] - Log[a_ + (I) x_] :> -2 I ArcTan[x/a], "
        "Log[a_ + x_] - Log[a_ - x_] :> 2 ArcTanh[x/a], "
        "Log[a_ - x_] - Log[a_ + x_] :> -2 ArcTanh[x/a], "
        "Log[x_ + Sqrt[1 + x_^2]] :> ArcSinh[x], "
        "Log[x_ + Sqrt[-1 + x_] Sqrt[1 + x_]] :> ArcCosh[x], "
        "Log[(I) x_ + Sqrt[1 - x_^2]] :> (I) ArcSin[x], "
        "Log[1/x_ + Sqrt[1 + 1/x_^2]] :> ArcCsch[x], "
        "Log[1/x_ + Sqrt[-1 + 1/x_] Sqrt[1 + 1/x_]] :> ArcSech[x], "
        "Log[(I)/x_ + Sqrt[1 - 1/x_^2]] :> (I) ArcCsc[x] "
    "}";

    const char* exp_to_trig_simp_str = "{ "
        "Sin[x_] * Cos[x_]^-1 :> Tan[x], "
        "Cos[x_] * Sin[x_]^-1 :> Cot[x], "
        "Sinh[x_] * Cosh[x_]^-1 :> Tanh[x], "
        "Cosh[x_] * Sinh[x_]^-1 :> Coth[x], "
        "Cos[x_]^-1 :> Sec[x], "
        "Sin[x_]^-1 :> Csc[x], "
        "Cosh[x_]^-1 :> Sech[x], "
        "Sinh[x_]^-1 :> Csch[x] "
    "}";

    exp_to_trig_rules = parse_expression(exp_to_trig_rules_str);
    exp_to_trig_simp = parse_expression(exp_to_trig_simp_str);

    /*
     * TrigExpand rules.
     *
     * 1. Angle-addition: split Sin[a + b + ...] into the binary form by
     *    matching the first summand and grouping the rest back up with Plus.
     *    Multi-term arguments are handled by iterated application of the
     *    binary formula via ReplaceRepeated.
     * 2. Multiple-angle: rewrite Sin[n x] in terms of Sin[(n-1) x] and
     *    Cos[(n-1) x] for integer n >= 2, so ReplaceRepeated reduces the
     *    argument down to Sin[x] and Cos[x]. Negative-integer arguments are
     *    folded out by the existing parity simplifications in trig.c / hyperbolic.c
     *    (e.g. Sin[-2 x] auto-evaluates to -Sin[2 x]).
     * 3. Tan/Cot/Sec/Csc (and their hyperbolic analogs) with a sum argument
     *    or an integer-multiple argument are rewritten in terms of Sin/Cos
     *    (resp. Sinh/Cosh) so the base rules can expand them.
     */
    const char* expand_rules_str = "{ "
        "Sin[x_ + y__] :> Sin[x] Cos[Plus[y]] + Cos[x] Sin[Plus[y]], "
        "Cos[x_ + y__] :> Cos[x] Cos[Plus[y]] - Sin[x] Sin[Plus[y]], "
        "Sinh[x_ + y__] :> Sinh[x] Cosh[Plus[y]] + Cosh[x] Sinh[Plus[y]], "
        "Cosh[x_ + y__] :> Cosh[x] Cosh[Plus[y]] + Sinh[x] Sinh[Plus[y]], "
        "Sin[n_Integer x_] /; n > 1 :> Sin[(n-1) x] Cos[x] + Cos[(n-1) x] Sin[x], "
        "Cos[n_Integer x_] /; n > 1 :> Cos[(n-1) x] Cos[x] - Sin[(n-1) x] Sin[x], "
        "Sinh[n_Integer x_] /; n > 1 :> Sinh[(n-1) x] Cosh[x] + Cosh[(n-1) x] Sinh[x], "
        "Cosh[n_Integer x_] /; n > 1 :> Cosh[(n-1) x] Cosh[x] + Sinh[(n-1) x] Sinh[x], "
        "Tan[x_ + y__] :> Sin[x + y] / Cos[x + y], "
        "Cot[x_ + y__] :> Cos[x + y] / Sin[x + y], "
        "Sec[x_ + y__] :> 1 / Cos[x + y], "
        "Csc[x_ + y__] :> 1 / Sin[x + y], "
        "Tanh[x_ + y__] :> Sinh[x + y] / Cosh[x + y], "
        "Coth[x_ + y__] :> Cosh[x + y] / Sinh[x + y], "
        "Sech[x_ + y__] :> 1 / Cosh[x + y], "
        "Csch[x_ + y__] :> 1 / Sinh[x + y], "
        "Tan[n_Integer x_] /; n > 1 :> Sin[n x] / Cos[n x], "
        "Cot[n_Integer x_] /; n > 1 :> Cos[n x] / Sin[n x], "
        "Sec[n_Integer x_] /; n > 1 :> 1 / Cos[n x], "
        "Csc[n_Integer x_] /; n > 1 :> 1 / Sin[n x], "
        "Tanh[n_Integer x_] /; n > 1 :> Sinh[n x] / Cosh[n x], "
        "Coth[n_Integer x_] /; n > 1 :> Cosh[n x] / Sinh[n x], "
        "Sech[n_Integer x_] /; n > 1 :> 1 / Cosh[n x], "
        "Csch[n_Integer x_] /; n > 1 :> 1 / Sinh[n x] "
    "}";

    trig_expand_rules = parse_expression(expand_rules_str);

    /*
     * Pythagorean identities applied after Factor.
     *
     * Factor does not produce a canonical sign for these expressions: depending
     * on the input, Factor[Expand[Sin[n*x]^2 + Cos[n*x]^2]] may emerge as
     * (Sin^2 + Cos^2)^n, or (-Sin^2 - Cos^2)^n wrapped in an outer -1, and the
     * hyperbolic analog likewise may factor with the minus attached to either
     * Sinh or Cosh. We enumerate both signs; each rule fires once and the
     * remaining constant factors are picked up by Expand.
     *
     *   (Sin[x]^2 + Cos[x]^2)^n           circular Pythagorean identity,
     *   (-Sin[x]^2 - Cos[x]^2)^n          same with negated base;
     *   (Cosh+Sinh)^n (Cosh-Sinh)^n       hyperbolic Pythagorean via real
     *                                     factorization,
     *   (Cosh+Sinh)^n (-Cosh+Sinh)^n      same with negated second factor;
     *   r___ tails                        allow arbitrary extra factors in the
     *                                     surrounding Times (e.g. a leading
     *                                     constant that Factor pulled out),
     *   <base>+r___ sum rules             catch the identity when it appears as
     *                                     part of a larger sum that Factor left
     *                                     in the original summed form.
     */
    const char* pythag_str = "{ "
        "(Sin[x_]^2 + Cos[x_]^2)^n_. r___ :> 1 r, "
        "(-Sin[x_]^2 - Cos[x_]^2)^n_. r___ :> (-1)^n r, "
        "(Cosh[x_] + Sinh[x_])^n_. (Cosh[x_] - Sinh[x_])^n_. r___ :> 1 r, "
        "(Cosh[x_] + Sinh[x_])^n_. (-Cosh[x_] + Sinh[x_])^n_. r___ :> (-1)^n r, "
        "Sin[x_]^2 + Cos[x_]^2 + r___ :> 1 + r, "
        "Cosh[x_]^2 - Sinh[x_]^2 + r___ :> 1 + r, "
        "Sinh[x_]^2 - Cosh[x_]^2 + r___ :> -1 + r "
    "}";

    trig_expand_pythag = parse_expression(pythag_str);

    symtab_add_builtin("ExpToTrig", builtin_exptotrig);
    symtab_get_def("ExpToTrig")->attributes |= (ATTR_LISTABLE | ATTR_PROTECTED);

    symtab_add_builtin("TrigToExp", builtin_trigtoexp);
    symtab_get_def("TrigToExp")->attributes |= (ATTR_LISTABLE | ATTR_PROTECTED);

    symtab_add_builtin("TrigExpand", builtin_trigexpand);
    symtab_get_def("TrigExpand")->attributes |= (ATTR_LISTABLE | ATTR_PROTECTED);
}
