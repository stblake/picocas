#include "expand.h"
#include "eval.h"
#include "symtab.h"
#include "arithmetic.h"
#include "match.h"
#include <string.h>
#include <stdlib.h>

static bool expr_contains_patt(Expr* e, Expr* patt) {
    if (!patt) return true; // NULL pattern matches everything
    
    MatchEnv* env = env_new();
    bool is_match = match(e, patt, env);
    env_free(env);
    if (is_match) return true;
    
    if (e->type == EXPR_FUNCTION) {
        if (expr_contains_patt(e->data.function.head, patt)) return true;
        for (size_t i = 0; i < e->data.function.arg_count; i++) {
            if (expr_contains_patt(e->data.function.args[i], patt)) return true;
        }
    }
    return false;
}

static Expr* multiply_two(Expr* a, Expr* b) {
    bool a_is_plus = (a->type == EXPR_FUNCTION && a->data.function.head->type == EXPR_SYMBOL && strcmp(a->data.function.head->data.symbol, "Plus") == 0);
    bool b_is_plus = (b->type == EXPR_FUNCTION && b->data.function.head->type == EXPR_SYMBOL && strcmp(b->data.function.head->data.symbol, "Plus") == 0);

    if (a_is_plus && b_is_plus) {
        size_t count = a->data.function.arg_count * b->data.function.arg_count;
        Expr** args = malloc(sizeof(Expr*) * count);
        size_t k = 0;
        for (size_t i = 0; i < a->data.function.arg_count; i++) {
            for (size_t j = 0; j < b->data.function.arg_count; j++) {
                Expr* t_args[2] = { expr_copy(a->data.function.args[i]), expr_copy(b->data.function.args[j]) };
                args[k++] = eval_and_free(expr_new_function(expr_new_symbol("Times"), t_args, 2));
            }
        }
        Expr* res = eval_and_free(expr_new_function(expr_new_symbol("Plus"), args, count));
        free(args);
        return res;
    } else if (a_is_plus) {
        size_t count = a->data.function.arg_count;
        Expr** args = malloc(sizeof(Expr*) * count);
        for (size_t i = 0; i < count; i++) {
            Expr* t_args[2] = { expr_copy(a->data.function.args[i]), expr_copy(b) };
            args[i] = eval_and_free(expr_new_function(expr_new_symbol("Times"), t_args, 2));
        }
        Expr* res = eval_and_free(expr_new_function(expr_new_symbol("Plus"), args, count));
        free(args);
        return res;
    } else if (b_is_plus) {
        size_t count = b->data.function.arg_count;
        Expr** args = malloc(sizeof(Expr*) * count);
        for (size_t i = 0; i < count; i++) {
            Expr* t_args[2] = { expr_copy(a), expr_copy(b->data.function.args[i]) };
            args[i] = eval_and_free(expr_new_function(expr_new_symbol("Times"), t_args, 2));
        }
        Expr* res = eval_and_free(expr_new_function(expr_new_symbol("Plus"), args, count));
        free(args);
        return res;
    } else {
        Expr* t_args[2] = { expr_copy(a), expr_copy(b) };
        return eval_and_free(expr_new_function(expr_new_symbol("Times"), t_args, 2));
    }
}

static Expr* multiply_all(Expr** args, size_t start, size_t end) {
    if (start == end) return expr_copy(args[start]);
    if (start + 1 == end) return multiply_two(args[start], args[end]);
    size_t mid = start + (end - start) / 2;
    Expr* left = multiply_all(args, start, mid);
    Expr* right = multiply_all(args, mid + 1, end);
    Expr* res = multiply_two(left, right);
    expr_free(left);
    expr_free(right);
    return res;
}

static Expr* power_expand(Expr* base, int64_t exp) {
    if (exp == 0) return expr_new_integer(1);
    if (exp == 1) return expr_copy(base);
    
    Expr* res = expr_new_integer(1);
    Expr* b = expr_copy(base);
    int64_t e = exp;
    
    while (e > 0) {
        if (e % 2 == 1) {
            Expr* next_res = multiply_two(res, b);
            expr_free(res);
            res = next_res;
        }
        e /= 2;
        if (e > 0) {
            Expr* next_b = multiply_two(b, b);
            expr_free(b);
            b = next_b;
        }
    }
    expr_free(b);
    return res;
}

Expr* expr_expand_patt(Expr* e, Expr* patt) {
    if (!e) return NULL;
    if (patt && !expr_contains_patt(e, patt)) return expr_copy(e);

    if (e->type != EXPR_FUNCTION) return expr_copy(e);

    const char* head = e->data.function.head->type == EXPR_SYMBOL ? e->data.function.head->data.symbol : "";

    // Thread over lists, equations, inequalities, logic
    if (strcmp(head, "List") == 0 || strcmp(head, "Equal") == 0 || strcmp(head, "Less") == 0 || 
        strcmp(head, "LessEqual") == 0 || strcmp(head, "Greater") == 0 || strcmp(head, "GreaterEqual") == 0 ||
        strcmp(head, "And") == 0 || strcmp(head, "Or") == 0 || strcmp(head, "Not") == 0) {
        Expr** args = malloc(sizeof(Expr*) * e->data.function.arg_count);
        for (size_t i = 0; i < e->data.function.arg_count; i++) {
            args[i] = expr_expand_patt(e->data.function.args[i], patt);
        }
        Expr* res = eval_and_free(expr_new_function(expr_copy(e->data.function.head), args, e->data.function.arg_count));
        free(args);
        return res;
    }

    if (strcmp(head, "Plus") == 0) {
        Expr** args = malloc(sizeof(Expr*) * e->data.function.arg_count);
        for (size_t i = 0; i < e->data.function.arg_count; i++) {
            args[i] = expr_expand_patt(e->data.function.args[i], patt);
        }
        Expr* res = eval_and_free(expr_new_function(expr_new_symbol("Plus"), args, e->data.function.arg_count));
        free(args);
        return res;
    }

    if (strcmp(head, "Times") == 0) {
        size_t count = e->data.function.arg_count;
        if (count == 0) return expr_new_integer(1);
        Expr** args = malloc(sizeof(Expr*) * count);
        for (size_t i = 0; i < count; i++) {
            args[i] = expr_expand_patt(e->data.function.args[i], patt);
        }
        Expr* res = multiply_all(args, 0, count - 1);
        for (size_t i = 0; i < count; i++) expr_free(args[i]);
        free(args);
        return res;
    }

    if (strcmp(head, "Power") == 0 && e->data.function.arg_count == 2) {
        Expr* base = e->data.function.args[0];
        Expr* exp = e->data.function.args[1];
        if (exp->type == EXPR_INTEGER && exp->data.integer > 0 && exp->data.integer < 100) {
            Expr* exp_base = expr_expand_patt(base, patt);
            Expr* res = power_expand(exp_base, exp->data.integer);
            expr_free(exp_base);
            return res;
        }
        // For negative integer or non-integer power, we still don't go into subexpressions
        return expr_copy(e);
    }

    // Default: do not go into subexpressions
    return expr_copy(e);
}

Expr* expr_expand(Expr* e) {
    return expr_expand_patt(e, NULL);
}

Expr* builtin_expand(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count < 1 || res->data.function.arg_count > 2) return NULL;
    Expr* patt = NULL;
    if (res->data.function.arg_count == 2) patt = res->data.function.args[1];
    Expr* ret = expr_expand_patt(res->data.function.args[0], patt);
    return ret;
}

void expand_init(void) {
    symtab_add_builtin("Expand", builtin_expand);
    symtab_get_def("Expand")->attributes |= ATTR_PROTECTED;
}
