#include "print.h"

#include "funcprog.h"
#include "eval.h"
#include "arithmetic.h"
#include "match.h"
#include <string.h>
#include <stdlib.h>

typedef struct {
    int64_t min;
    int64_t max;
    bool heads;
} LevelSpec;

static int64_t get_depth(Expr* e) {
    if (e->type != EXPR_FUNCTION) return 1;
    if (e->data.function.head->type == EXPR_SYMBOL) {
        const char* h = e->data.function.head->data.symbol;
        if (strcmp(h, "Rational") == 0 || strcmp(h, "Complex") == 0) return 1;
    }
    int64_t max_d = 0;
    for (size_t i = 0; i < e->data.function.arg_count; i++) {
        int64_t d = get_depth(e->data.function.args[i]);
        if (d > max_d) max_d = d;
    }
    return max_d + 1;
}

static LevelSpec parse_level_spec(Expr* ls, int64_t default_min, int64_t default_max) {
    LevelSpec spec = {default_min, default_max, false};
    if (!ls) return spec;

    if (ls->type == EXPR_INTEGER) {
        spec.min = 1;
        spec.max = ls->data.integer;
    } else if (ls->type == EXPR_FUNCTION && strcmp(ls->data.function.head->data.symbol, "List") == 0) {
        if (ls->data.function.arg_count == 1 && ls->data.function.args[0]->type == EXPR_INTEGER) {
            spec.min = spec.max = ls->data.function.args[0]->data.integer;
        } else if (ls->data.function.arg_count == 2 && 
                   ls->data.function.args[0]->type == EXPR_INTEGER &&
                   ls->data.function.args[1]->type == EXPR_INTEGER) {
            spec.min = ls->data.function.args[0]->data.integer;
            spec.max = ls->data.function.args[1]->data.integer;
        }
    } else if (ls->type == EXPR_SYMBOL && strcmp(ls->data.symbol, "Infinity") == 0) {
        spec.min = 1;
        spec.max = 1000000; 
    }
    return spec;
}

static void parse_options(Expr* res, size_t start_idx, LevelSpec* spec) {
    for (size_t i = start_idx; i < res->data.function.arg_count; i++) {
        Expr* opt = res->data.function.args[i];
        if (opt->type == EXPR_FUNCTION && strcmp(opt->data.function.head->data.symbol, "Rule") == 0) {
            if (opt->data.function.arg_count == 2 && 
                opt->data.function.args[0]->type == EXPR_SYMBOL &&
                strcmp(opt->data.function.args[0]->data.symbol, "Heads") == 0) {
                if (opt->data.function.args[1]->type == EXPR_SYMBOL &&
                    strcmp(opt->data.function.args[1]->data.symbol, "True") == 0) {
                    spec->heads = true;
                }
            }
        }
    }
}

/* ------------------- Apply ------------------- */

static Expr* apply_at_level(Expr* f, Expr* expr, int64_t current_level, LevelSpec spec) {
    if (expr->type != EXPR_FUNCTION) {
        return expr_copy(expr);
    }

    int64_t d = get_depth(expr);
    bool should_apply = (current_level >= spec.min && current_level <= spec.max) ||
                        (-d >= spec.min && -d <= spec.max);

    if (should_apply) {
        size_t count = expr->data.function.arg_count;
        Expr** args = malloc(sizeof(Expr*) * count);
        for (size_t i = 0; i < count; i++) {
            args[i] = apply_at_level(f, expr->data.function.args[i], current_level + 1, spec);
        }
        Expr* new_func = expr_new_function(expr_copy(f), args, count);
        free(args);
        Expr* eval_res = evaluate(new_func);
        expr_free(new_func);
        return eval_res;
    }

    size_t count = expr->data.function.arg_count;
    Expr** new_args = malloc(sizeof(Expr*) * count);
    for (size_t i = 0; i < count; i++) {
        new_args[i] = apply_at_level(f, expr->data.function.args[i], current_level + 1, spec);
    }
    
    Expr* new_head = NULL;
    if (spec.heads) {
        new_head = apply_at_level(f, expr->data.function.head, current_level + 1, spec);
    } else {
        new_head = expr_copy(expr->data.function.head);
    }
    
    Expr* result = expr_new_function(new_head, new_args, count);
    free(new_args);
    return result;
}

Expr* builtin_apply(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count < 2) return NULL;
    
    Expr* f = res->data.function.args[0];
    Expr* expr = res->data.function.args[1];
    
    Expr* ls = (res->data.function.arg_count >= 3) ? res->data.function.args[2] : NULL;
    if (ls && ls->type == EXPR_FUNCTION && strcmp(ls->data.function.head->data.symbol, "Rule") == 0) ls = NULL;

    LevelSpec spec = parse_level_spec(ls, 0, 0);
    parse_options(res, ls ? 3 : 2, &spec);

    return apply_at_level(f, expr, 0, spec);
}

/* ------------------- Map ------------------- */

static Expr* map_at_level(Expr* f, Expr* expr, int64_t current_level, LevelSpec spec) {
    Expr* intermediate = NULL;
    if (expr->type == EXPR_FUNCTION) {
        // Recurse first (Bottom-up)
        size_t count = expr->data.function.arg_count;
        Expr** new_args = malloc(sizeof(Expr*) * count);
        for (size_t i = 0; i < count; i++) {
            new_args[i] = map_at_level(f, expr->data.function.args[i], current_level + 1, spec);
        }
        
        Expr* new_head = NULL;
        if (spec.heads) {
            new_head = map_at_level(f, expr->data.function.head, current_level + 1, spec);
        } else {
            new_head = expr_copy(expr->data.function.head);
        }
        
        intermediate = expr_new_function(new_head, new_args, count);
        free(new_args);
    } else {
        intermediate = expr_copy(expr);
    }

    int64_t d = get_depth(intermediate);
    bool should_map = (current_level >= spec.min && current_level <= spec.max) ||
                      (-d >= spec.min && -d <= spec.max);

    if (should_map) {
        Expr* f_copy = expr_copy(f);
        Expr* call = expr_new_function(f_copy, &intermediate, 1);
        Expr* result = evaluate(call);
        expr_free(call);
        return result;
    }

    return intermediate;
}

Expr* builtin_map(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count < 2) return NULL;

    Expr* f = res->data.function.args[0];
    Expr* expr = res->data.function.args[1];
    
    Expr* ls = (res->data.function.arg_count >= 3) ? res->data.function.args[2] : NULL;
    if (ls && ls->type == EXPR_FUNCTION && strcmp(ls->data.function.head->data.symbol, "Rule") == 0) ls = NULL;

    LevelSpec spec = parse_level_spec(ls, 1, 1);
    parse_options(res, ls ? 3 : 2, &spec);

    return map_at_level(f, expr, 0, spec);
}

/* ------------------- MapAt ------------------- */

/* Build evaluate(f[arg]) returning a new owned Expr*. */
static Expr* mapat_apply_f(Expr* f, Expr* arg) {
    Expr* arg_copy = expr_copy(arg);
    Expr* call = expr_new_function(expr_copy(f), &arg_copy, 1);
    Expr* result = evaluate(call);
    expr_free(call);
    return result;
}

/*
 * mapat_at_path: apply f at the given position path in expr.
 *
 * Arguments:
 *   f     : function/symbol to apply (borrowed)
 *   expr  : source expression (borrowed)
 *   path  : array of position indices (each index is an Expr*, borrowed)
 *   plen  : number of elements in path
 *
 * Returns a freshly-allocated expression (caller owns).
 *
 * A path element may be:
 *   - integer k (>0 counts from start, <0 counts from end, 0 targets head)
 *   - the symbol All (apply at all children at this level)
 *   - Span[a, b] or Span[a, b, step] (apply to the spanned range)
 *
 * If the path runs past a non-compound sub-expression the remaining path
 * is ignored and the leaf is returned unchanged.  Out-of-range integers
 * are silently ignored, matching Mathematica's permissive behaviour.
 */
static Expr* mapat_at_path(Expr* f, Expr* expr, Expr** path, size_t plen) {
    if (plen == 0) {
        return mapat_apply_f(f, expr);
    }
    if (expr->type != EXPR_FUNCTION) {
        return expr_copy(expr);
    }

    Expr* idx = path[0];
    size_t len = expr->data.function.arg_count;

    Expr** new_args = NULL;
    if (len > 0) {
        new_args = malloc(sizeof(Expr*) * len);
        for (size_t i = 0; i < len; i++) {
            new_args[i] = expr_copy(expr->data.function.args[i]);
        }
    }
    Expr* new_head = expr_copy(expr->data.function.head);

    if (idx->type == EXPR_INTEGER) {
        int64_t k = idx->data.integer;
        if (k == 0) {
            Expr* r = mapat_at_path(f, expr->data.function.head, path + 1, plen - 1);
            expr_free(new_head);
            new_head = r;
        } else {
            if (k < 0) k = (int64_t)len + k + 1;
            if (k >= 1 && k <= (int64_t)len) {
                Expr* r = mapat_at_path(f, expr->data.function.args[k - 1], path + 1, plen - 1);
                expr_free(new_args[k - 1]);
                new_args[k - 1] = r;
            }
        }
    } else if (idx->type == EXPR_SYMBOL && strcmp(idx->data.symbol, "All") == 0) {
        for (size_t i = 0; i < len; i++) {
            Expr* r = mapat_at_path(f, expr->data.function.args[i], path + 1, plen - 1);
            expr_free(new_args[i]);
            new_args[i] = r;
        }
    } else if (idx->type == EXPR_FUNCTION &&
               idx->data.function.head->type == EXPR_SYMBOL &&
               strcmp(idx->data.function.head->data.symbol, "Span") == 0) {
        int64_t start = 1, end = (int64_t)len, step = 1;
        size_t span_argc = idx->data.function.arg_count;
        if (span_argc >= 1) {
            Expr* a1 = idx->data.function.args[0];
            if (a1->type == EXPR_INTEGER) {
                start = a1->data.integer;
                if (start < 0) start = (int64_t)len + start + 1;
            } else if (a1->type == EXPR_SYMBOL && strcmp(a1->data.symbol, "All") == 0) {
                start = 1;
            }
        }
        if (span_argc >= 2) {
            Expr* a2 = idx->data.function.args[1];
            if (a2->type == EXPR_INTEGER) {
                end = a2->data.integer;
                if (end < 0) end = (int64_t)len + end + 1;
            } else if (a2->type == EXPR_SYMBOL && strcmp(a2->data.symbol, "All") == 0) {
                end = (int64_t)len;
            }
        }
        if (span_argc >= 3) {
            Expr* a3 = idx->data.function.args[2];
            if (a3->type == EXPR_INTEGER) step = a3->data.integer;
        }

        if (step > 0) {
            for (int64_t i = start; i <= end && i >= 1 && i <= (int64_t)len; i += step) {
                Expr* r = mapat_at_path(f, expr->data.function.args[i - 1], path + 1, plen - 1);
                expr_free(new_args[i - 1]);
                new_args[i - 1] = r;
            }
        } else if (step < 0) {
            for (int64_t i = start; i >= end && i >= 1 && i <= (int64_t)len; i += step) {
                Expr* r = mapat_at_path(f, expr->data.function.args[i - 1], path + 1, plen - 1);
                expr_free(new_args[i - 1]);
                new_args[i - 1] = r;
            }
        }
    }

    Expr* result = expr_new_function(new_head, new_args, len);
    if (new_args) free(new_args);
    return result;
}

/* True iff e is a List expression. */
static bool mapat_is_list(Expr* e) {
    return e->type == EXPR_FUNCTION &&
           e->data.function.head->type == EXPR_SYMBOL &&
           strcmp(e->data.function.head->data.symbol, "List") == 0;
}

Expr* builtin_map_at(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 3) return NULL;

    Expr* f = res->data.function.args[0];
    Expr* expr = res->data.function.args[1];
    Expr* pos = res->data.function.args[2];

    /* Disambiguate single-path vs. multiple-paths:
     *   - multiple-paths iff pos is a non-empty List whose first element is itself a List
     *   - otherwise, single path (possibly wrapped in a List of indices) */
    bool multi = false;
    if (mapat_is_list(pos) && pos->data.function.arg_count > 0 &&
        mapat_is_list(pos->data.function.args[0])) {
        multi = true;
    }

    if (!multi) {
        Expr** path;
        size_t plen;
        if (mapat_is_list(pos)) {
            plen = pos->data.function.arg_count;
            path = pos->data.function.args;
        } else {
            plen = 1;
            path = &pos;
        }
        return mapat_at_path(f, expr, path, plen);
    }

    /* Multiple positions: apply sequentially. Repeated positions apply f repeatedly. */
    Expr* current = expr_copy(expr);
    for (size_t i = 0; i < pos->data.function.arg_count; i++) {
        Expr* sub = pos->data.function.args[i];
        Expr** path;
        size_t plen;
        if (mapat_is_list(sub)) {
            plen = sub->data.function.arg_count;
            path = sub->data.function.args;
        } else {
            plen = 1;
            path = &sub;
        }
        Expr* next = mapat_at_path(f, current, path, plen);
        expr_free(current);
        current = next;
    }
    return current;
}

/* ------------------- MapAll ------------------- */

Expr* builtin_map_all(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count < 2) return NULL;

    Expr* f = res->data.function.args[0];
    Expr* expr = res->data.function.args[1];
    
    LevelSpec spec = {0, 1000000, false}; // {0, Infinity}
    parse_options(res, 2, &spec);

    return map_at_level(f, expr, 0, spec);
}

/* ------------------- Select ------------------- */

Expr* builtin_select(Expr* res) {
    if (res->type != EXPR_FUNCTION || (res->data.function.arg_count != 2 && res->data.function.arg_count != 3)) {
        return NULL;
    }
    
    Expr* list = res->data.function.args[0];
    Expr* crit = res->data.function.args[1];
    
    if (list->type != EXPR_FUNCTION) return NULL; // Can only select from compound expressions
    
    int64_t n_max = -1; // -1 means all
    if (res->data.function.arg_count == 3) {
        Expr* n_expr = res->data.function.args[2];
        if (n_expr->type == EXPR_INTEGER) {
            n_max = n_expr->data.integer;
        } else {
            return NULL; // Invalid n
        }
    }
    
    size_t count = list->data.function.arg_count;
    Expr** kept_args = NULL;
    if (count > 0) kept_args = malloc(sizeof(Expr*) * count);
    size_t kept_count = 0;
    
    for (size_t i = 0; i < count; i++) {
        if (n_max >= 0 && (int64_t)kept_count >= n_max) break;
        
        Expr* elem = list->data.function.args[i];
        
        Expr* call_args[1] = { expr_copy(elem) };
        Expr* call = expr_new_function(expr_copy(crit), call_args, 1);
        
        Expr* eval_res = evaluate(call);
        
        if (eval_res->type == EXPR_SYMBOL && strcmp(eval_res->data.symbol, "True") == 0) {
            kept_args[kept_count++] = expr_copy(elem);
        }
        
        expr_free(eval_res);
        expr_free(call);
    }
    
    Expr* result = expr_new_function(expr_copy(list->data.function.head), kept_args, kept_count);
    if (kept_args) free(kept_args);
    
    return result;
}

/* ------------------- Through ------------------- */

static Expr* transform_head(Expr* head, Expr* h_spec, Expr** args, size_t arg_count, bool* transformed) {
    if (head->type != EXPR_FUNCTION) return expr_copy(head);
    
    Expr* P = head->data.function.head;
    bool match = false;
    
    if (h_spec == NULL) {
        match = true;
    } else {
        match = expr_eq(P, h_spec);
    }
    
    if (match) {
        *transformed = true;
        size_t n = head->data.function.arg_count;
        Expr** new_args = malloc(sizeof(Expr*) * n);
        
        for (size_t i = 0; i < n; i++) {
            Expr* fi = head->data.function.args[i];
            
            Expr** call_args = malloc(sizeof(Expr*) * arg_count);
            for (size_t j = 0; j < arg_count; j++) {
                call_args[j] = expr_copy(args[j]);
            }
            Expr* call = expr_new_function(expr_copy(fi), call_args, arg_count);
            free(call_args);
            
            new_args[i] = evaluate(call);
            expr_free(call);
        }
        
        Expr* ret = expr_new_function(expr_copy(P), new_args, n);
        free(new_args);
        return ret;
    } else {
        size_t n = head->data.function.arg_count;
        Expr** new_args = malloc(sizeof(Expr*) * n);
        for (size_t i = 0; i < n; i++) {
            new_args[i] = transform_head(head->data.function.args[i], h_spec, args, arg_count, transformed);
        }
        Expr* ret = expr_new_function(expr_copy(P), new_args, n);
        free(new_args);
        return ret;
    }
}

Expr* builtin_through(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count < 1 || res->data.function.arg_count > 2) return NULL;
    
    Expr* expr = res->data.function.args[0];
    Expr* h_spec = (res->data.function.arg_count == 2) ? res->data.function.args[1] : NULL;
    
    if (expr->type != EXPR_FUNCTION) return expr_copy(expr);
    
    Expr* head = expr->data.function.head;
    Expr** args = expr->data.function.args;
    size_t arg_count = expr->data.function.arg_count;
    
    bool transformed = false;
    Expr* new_expr = NULL;
    
    if (h_spec == NULL) {
        if (head->type == EXPR_FUNCTION) {
            Expr* P = head->data.function.head;
            size_t n = head->data.function.arg_count;
            Expr** new_H_args = malloc(sizeof(Expr*) * n);
            for (size_t i = 0; i < n; i++) {
                Expr* fi = head->data.function.args[i];
                
                Expr** call_args = malloc(sizeof(Expr*) * arg_count);
                for (size_t j = 0; j < arg_count; j++) {
                    call_args[j] = expr_copy(args[j]);
                }
                Expr* call = expr_new_function(expr_copy(fi), call_args, arg_count);
                free(call_args);
                
                new_H_args[i] = evaluate(call);
                expr_free(call);
            }
            new_expr = expr_new_function(expr_copy(P), new_H_args, n);
            free(new_H_args);
            transformed = true;
        }
    } else {
        new_expr = transform_head(head, h_spec, args, arg_count, &transformed);
    }
    
    if (transformed) {
        Expr* eval_res = evaluate(new_expr);
        expr_free(new_expr);
        return eval_res;
    }
    
    if (new_expr) expr_free(new_expr);
    return expr_copy(expr);
}

/* ------------------- FreeQ ------------------- */

static bool freeq_at_level(Expr* expr, Expr* form, int64_t current_level, LevelSpec spec) {
    int64_t d = get_depth(expr);
    bool should_check = false;
    
    if (spec.min == -1 && spec.max == -1) {
        if (expr->type != EXPR_FUNCTION || (expr->type == EXPR_FUNCTION && expr->data.function.head->type == EXPR_SYMBOL && (strcmp(expr->data.function.head->data.symbol, "Rational") == 0 || strcmp(expr->data.function.head->data.symbol, "Complex") == 0))) {
            should_check = true;
        }
    } else {
        should_check = (current_level >= spec.min && current_level <= spec.max) ||
                       (-d >= spec.min && -d <= spec.max);
    }

    if (should_check) {
        MatchEnv* env = env_new();
        if (match(expr, form, env)) {
            env_free(env);
            return false; // Not free!
        }
        env_free(env);
    }

    if (expr->type == EXPR_FUNCTION && !(expr->data.function.head->type == EXPR_SYMBOL && (strcmp(expr->data.function.head->data.symbol, "Rational") == 0 || strcmp(expr->data.function.head->data.symbol, "Complex") == 0))) {
        if (spec.heads) {
            if (!freeq_at_level(expr->data.function.head, form, current_level + 1, spec)) {
                return false;
            }
        }
        for (size_t i = 0; i < expr->data.function.arg_count; i++) {
            if (!freeq_at_level(expr->data.function.args[i], form, current_level + 1, spec)) {
                return false;
            }
        }
    }

    return true;
}

Expr* builtin_freeq(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count < 2) return NULL;
    Expr* expr = res->data.function.args[0];
    Expr* form = res->data.function.args[1];

    LevelSpec spec = {0, 1000000, true}; // Default: {0, Infinity}, Heads -> True
    if (res->data.function.arg_count >= 3) {
        Expr* ls = res->data.function.args[2];
        spec = parse_level_spec(ls, 1, 1);
        if (ls->type == EXPR_FUNCTION && strcmp(ls->data.function.head->data.symbol, "List") == 0) {
            // Already parsed by parse_level_spec
        } else if (ls->type == EXPR_SYMBOL && strcmp(ls->data.symbol, "All") == 0) {
            spec.min = 0; spec.max = 1000000;
        } else if (ls->type == EXPR_SYMBOL && strcmp(ls->data.symbol, "Infinity") == 0) {
            spec.min = 1; spec.max = 1000000;
        } else if (ls->type == EXPR_INTEGER) {
            spec.min = 1; spec.max = ls->data.integer;
        }
    }

    parse_options(res, res->data.function.arg_count >= 3 ? 3 : 2, &spec);
    
    // Check options anywhere for FreeQ
    for (size_t i = 2; i < res->data.function.arg_count; i++) {
        Expr* opt = res->data.function.args[i];
        if (opt->type == EXPR_FUNCTION && strcmp(opt->data.function.head->data.symbol, "Rule") == 0) {
            if (opt->data.function.arg_count == 2 && 
                opt->data.function.args[0]->type == EXPR_SYMBOL &&
                strcmp(opt->data.function.args[0]->data.symbol, "Heads") == 0) {
                if (opt->data.function.args[1]->type == EXPR_SYMBOL &&
                    strcmp(opt->data.function.args[1]->data.symbol, "False") == 0) {
                    spec.heads = false;
                }
            }
        }
    }

    if (freeq_at_level(expr, form, 0, spec)) {
        return expr_new_symbol("True");
    } else {
        return expr_new_symbol("False");
    }
}

/* ------------------- Distribute ------------------- */

static void distribute_recursive(Expr*** components, size_t* component_counts, size_t n_args, size_t current_arg, Expr** current_tuple, Expr*** results, size_t* res_count, size_t* res_cap, Expr* fp_head) {
    if (current_arg == n_args) {
        Expr** tuple_copy = malloc(sizeof(Expr*) * n_args);
        for (size_t i = 0; i < n_args; i++) tuple_copy[i] = expr_copy(current_tuple[i]);
        Expr* term = expr_new_function(expr_copy(fp_head), tuple_copy, n_args);
        free(tuple_copy);
        
        if (*res_count >= *res_cap) {
            *res_cap = (*res_cap == 0) ? 16 : (*res_cap * 2);
            *results = realloc(*results, sizeof(Expr*) * (*res_cap));
        }
        (*results)[(*res_count)++] = term;
        return;
    }
    
    for (size_t i = 0; i < component_counts[current_arg]; i++) {
        current_tuple[current_arg] = components[current_arg][i];
        distribute_recursive(components, component_counts, n_args, current_arg + 1, current_tuple, results, res_count, res_cap, fp_head);
    }
}

Expr* builtin_distribute(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count < 1 || res->data.function.arg_count > 5) {
        return NULL;
    }
    
    Expr* expr = res->data.function.args[0];
    if (expr->type != EXPR_FUNCTION) return expr_copy(expr);

    Expr* g = (res->data.function.arg_count >= 2) ? res->data.function.args[1] : expr_new_symbol("Plus");
    Expr* f = (res->data.function.arg_count >= 3) ? res->data.function.args[2] : expr_copy(expr->data.function.head);
    Expr* gp = (res->data.function.arg_count >= 4) ? res->data.function.args[3] : expr_copy(g);
    Expr* fp = (res->data.function.arg_count >= 5) ? res->data.function.args[4] : expr_copy(f);

    // If head(expr) != f, return expr
    if (!expr_eq(expr->data.function.head, f)) {
        if (res->data.function.arg_count < 2) expr_free(g);
        if (res->data.function.arg_count < 3) expr_free(f);
        if (res->data.function.arg_count < 4) expr_free(gp);
        if (res->data.function.arg_count < 5) expr_free(fp);
        return expr_copy(expr);
    }

    size_t n_args = expr->data.function.arg_count;
    Expr*** components = malloc(sizeof(Expr**) * n_args);
    size_t* component_counts = malloc(sizeof(size_t) * n_args);
    bool any_g = false;

    for (size_t i = 0; i < n_args; i++) {
        Expr* arg = expr->data.function.args[i];
        if (arg->type == EXPR_FUNCTION && expr_eq(arg->data.function.head, g)) {
            components[i] = arg->data.function.args;
            component_counts[i] = arg->data.function.arg_count;
            any_g = true;
        } else {
            components[i] = &expr->data.function.args[i];
            component_counts[i] = 1;
        }
    }

    if (!any_g) {
        // No distribution possible, but we still apply fp if it's different?
        // Actually Distribute[f[x], g, f, gp, fp] -> gp[fp[x]] if no g is found?
        // Let's re-read: "Distribute explicitly constructs the complete result of a distribution"
        // If no g is found, it usually returns fp[args] wrapped in gp? No, wait.
        // Distribute[f[a], g] -> f[a]
        // Distribute[f[g[a]], g] -> g[f[a]]
        
        // Cleanup and return original if no g found
        free(components);
        free(component_counts);
        if (res->data.function.arg_count < 2) expr_free(g);
        if (res->data.function.arg_count < 3) expr_free(f);
        if (res->data.function.arg_count < 4) expr_free(gp);
        if (res->data.function.arg_count < 5) expr_free(fp);
        return expr_copy(expr);
    }

    Expr** results = NULL;
    size_t res_count = 0;
    size_t res_cap = 0;
    Expr** current_tuple = malloc(sizeof(Expr*) * n_args);

    distribute_recursive(components, component_counts, n_args, 0, current_tuple, &results, &res_count, &res_cap, fp);

    Expr* final_res = expr_new_function(expr_copy(gp), results, res_count);
    if (results) free(results);
    
    // results array itself is consumed by expr_new_function, but only if we don't free it.
    // wait, expr_new_function usually COPIES the array if we pass it, OR it takes ownership?
    // In picocas, expr_new_function takes Expr** args.
    // Let's check expr_new_function implementation.
    
    free(current_tuple);
    free(components);
    free(component_counts);
    if (res->data.function.arg_count < 2) expr_free(g);
    if (res->data.function.arg_count < 3) expr_free(f);
    if (res->data.function.arg_count < 4) expr_free(gp);
    if (res->data.function.arg_count < 5) expr_free(fp);
    
    // We need to evaluate the result because gp or fp might be builtins
    Expr* eval_res = evaluate(final_res);
    expr_free(final_res);
    return eval_res;
}

/* ------------------- Inner ------------------- */

/* --- Inner with n=1: contract first index of A with first index of B --- */

static Expr* inner_n1_B(Expr* f, Expr** A_leaves, Expr** B_slices, size_t L, Expr* g, Expr* head) {
    Expr* first_b = B_slices[0];

    if (first_b->type == EXPR_FUNCTION && expr_eq(first_b->data.function.head, head)) {
        /* B has remaining structure — iterate over its sub-elements ("columns") */
        size_t M = first_b->data.function.arg_count;
        Expr** res_args = malloc(sizeof(Expr*) * M);
        Expr** new_B = malloc(sizeof(Expr*) * L);
        for (size_t j = 0; j < M; j++) {
            for (size_t k = 0; k < L; k++) {
                Expr* bk = B_slices[k];
                if (bk->type == EXPR_FUNCTION && j < bk->data.function.arg_count) {
                    new_B[k] = bk->data.function.args[j];
                } else {
                    new_B[k] = bk; /* fallback for ragged */
                }
            }
            res_args[j] = inner_n1_B(f, A_leaves, new_B, L, g, head);
        }
        free(new_B);
        Expr* ret = expr_new_function(expr_copy(head), res_args, M);
        free(res_args);
        return ret;
    } else {
        /* Base case: both A and B at leaf level — form g[f[a0,b0], f[a1,b1], ...] */
        Expr** g_args = malloc(sizeof(Expr*) * L);
        for (size_t k = 0; k < L; k++) {
            Expr* f_args_arr[2];
            f_args_arr[0] = expr_copy(A_leaves[k]);
            f_args_arr[1] = expr_copy(B_slices[k]);
            g_args[k] = expr_new_function(expr_copy(f), f_args_arr, 2);
        }
        Expr* ret = expr_new_function(expr_copy(g), g_args, L);
        free(g_args);
        return ret;
    }
}

static Expr* inner_n1_A(Expr* f, Expr** A_slices, Expr** B_slices, size_t L, Expr* g, Expr* head) {
    Expr* first_a = A_slices[0];

    /* If first A slice is not a list, go to B-side */
    if (first_a->type != EXPR_FUNCTION || !expr_eq(first_a->data.function.head, head)) {
        return inner_n1_B(f, A_slices, B_slices, L, g, head);
    }

    /* Check if first A slice is a "matrix" (its elements are also lists) */
    bool a_is_matrix = false;
    if (first_a->data.function.arg_count > 0 &&
        first_a->data.function.args[0]->type == EXPR_FUNCTION &&
        expr_eq(first_a->data.function.args[0]->data.function.head, head)) {
        a_is_matrix = true;
    }

    if (a_is_matrix) {
        /* A has deeper remaining structure — descend one level */
        size_t M = first_a->data.function.arg_count;
        Expr** res_args = malloc(sizeof(Expr*) * M);
        Expr** new_slices = malloc(sizeof(Expr*) * L);
        for (size_t i = 0; i < M; i++) {
            for (size_t k = 0; k < L; k++) {
                Expr* ak = A_slices[k];
                if (ak->type == EXPR_FUNCTION && i < ak->data.function.arg_count) {
                    new_slices[k] = ak->data.function.args[i];
                } else {
                    new_slices[k] = ak; /* fallback for ragged */
                }
            }
            res_args[i] = inner_n1_A(f, new_slices, B_slices, L, g, head);
        }
        free(new_slices);
        Expr* ret = expr_new_function(expr_copy(head), res_args, M);
        free(res_args);
        return ret;
    } else {
        /* A slices are vectors or atoms — go to B-side */
        return inner_n1_B(f, A_slices, B_slices, L, g, head);
    }
}

/* --- Standard Inner: contract last index of A with first index of B --- */

static Expr* contract_V_B(Expr* f, Expr* V, Expr* B, Expr* g, Expr* head) {
    if (V->type != EXPR_FUNCTION || B->type != EXPR_FUNCTION) return NULL;
    size_t N = V->data.function.arg_count;
    
    bool b_is_matrix = false;
    if (B->data.function.arg_count > 0 && B->data.function.args[0]->type == EXPR_FUNCTION &&
        expr_eq(B->data.function.args[0]->data.function.head, head)) {
        b_is_matrix = true;
    }
    
    if (b_is_matrix) {
        size_t M = B->data.function.args[0]->data.function.arg_count;
        Expr** res_args = malloc(sizeof(Expr*) * M);
        for (size_t j = 0; j < M; j++) {
            Expr** col_args = malloc(sizeof(Expr*) * N);
            for (size_t i = 0; i < N; i++) {
                if (i < B->data.function.arg_count) {
                    Expr* B_i = B->data.function.args[i];
                    if (B_i->type == EXPR_FUNCTION && j < B_i->data.function.arg_count) {
                        col_args[i] = expr_copy(B_i->data.function.args[j]);
                    } else {
                        col_args[i] = expr_new_symbol("Null");
                    }
                } else {
                    col_args[i] = expr_new_symbol("Null");
                }
            }
            Expr* B_col = expr_new_function(expr_copy(head), col_args, N);
            Expr* contracted = contract_V_B(f, V, B_col, g, head);
            expr_free(B_col);
            if (!contracted) {
                for (size_t k = 0; k < j; k++) expr_free(res_args[k]);
                free(res_args);
                return NULL;
            }
            res_args[j] = contracted;
        }
        Expr* ret = expr_new_function(expr_copy(head), res_args, M);
        free(res_args);
        return ret;
    } else {
        size_t B_len = B->data.function.arg_count;
        size_t min_len = N < B_len ? N : B_len;
        if (N != B_len) {
            // Technically lengths should match. We'll evaluate up to the min_len.
        }
        Expr** g_args = malloc(sizeof(Expr*) * min_len);
        for (size_t i = 0; i < min_len; i++) {
            Expr* f_args[2] = { expr_copy(V->data.function.args[i]), expr_copy(B->data.function.args[i]) };
            g_args[i] = expr_new_function(expr_copy(f), f_args, 2);
        }
        Expr* ret = expr_new_function(expr_copy(g), g_args, min_len);
        free(g_args);
        return ret;
    }
}

static Expr* inner_A(Expr* f, Expr* A, Expr* B, Expr* g, Expr* head) {
    if (A->type != EXPR_FUNCTION) return NULL;
    bool a_is_matrix = false;
    if (A->data.function.arg_count > 0 && A->data.function.args[0]->type == EXPR_FUNCTION &&
        expr_eq(A->data.function.args[0]->data.function.head, head)) {
        a_is_matrix = true;
    }
    
    if (a_is_matrix) {
        size_t N = A->data.function.arg_count;
        Expr** res_args = malloc(sizeof(Expr*) * N);
        for (size_t i = 0; i < N; i++) {
            Expr* inner_res = inner_A(f, A->data.function.args[i], B, g, head);
            if (!inner_res) {
                for (size_t j = 0; j < i; j++) expr_free(res_args[j]);
                free(res_args);
                return NULL;
            }
            res_args[i] = inner_res;
        }
        Expr* ret = expr_new_function(expr_copy(head), res_args, N);
        free(res_args);
        return ret;
    } else {
        return contract_V_B(f, A, B, g, head);
    }
}

Expr* builtin_inner(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count < 3) return NULL;
    
    Expr* f = res->data.function.args[0];
    Expr* A = res->data.function.args[1];
    Expr* B = res->data.function.args[2];
    Expr* g = (res->data.function.arg_count >= 4) ? res->data.function.args[3] : expr_new_symbol("Plus");
    
    Expr* n_expr = (res->data.function.arg_count >= 5) ? res->data.function.args[4] : NULL;
    
    if (A->type != EXPR_FUNCTION || B->type != EXPR_FUNCTION) {
        if (res->data.function.arg_count < 4) expr_free(g);
        return NULL;
    }
    
    Expr* head = A->data.function.head;

    Expr* inner_res;
    if (n_expr && n_expr->type == EXPR_INTEGER && n_expr->data.integer == 1) {
        /* Contract first index of A with first index of B directly */
        size_t L = A->data.function.arg_count;
        if (L != B->data.function.arg_count) {
            if (res->data.function.arg_count < 4) expr_free(g);
            return NULL;
        }
        inner_res = inner_n1_A(f, A->data.function.args, B->data.function.args, L, g, head);
    } else {
        Expr* A_used = expr_copy(A);
        inner_res = inner_A(f, A_used, B, g, head);
        expr_free(A_used);
    }

    if (res->data.function.arg_count < 4) expr_free(g);

    if (!inner_res) return NULL;

    Expr* final_eval = evaluate(inner_res);
    expr_free(inner_res);
    return final_eval;
}

/* ------------------- Outer ------------------- */

static Expr* outer_rec(Expr* f, Expr** orig_tensors, size_t num_tensors, int64_t* target_depths, 
                size_t arg_idx, Expr* curr_subtensor, int64_t curr_depth, Expr** current_atoms, Expr* head) {
    
    bool treat_as_atom = false;
    if (curr_depth >= target_depths[arg_idx]) {
        treat_as_atom = true;
    } else if (curr_subtensor->type != EXPR_FUNCTION || (head && !expr_eq(curr_subtensor->data.function.head, head))) {
        treat_as_atom = true;
    }
    
    if (treat_as_atom) {
        current_atoms[arg_idx] = curr_subtensor;
if (arg_idx + 1 == num_tensors) {
            Expr** f_args = malloc(sizeof(Expr*) * num_tensors);
            for(size_t i=0; i<num_tensors; i++) f_args[i] = expr_copy(current_atoms[i]);
            Expr* ret = expr_new_function(expr_copy(f), f_args, num_tensors);
            free(f_args);
            return ret;
        } else {
            return outer_rec(f, orig_tensors, num_tensors, target_depths, 
                             arg_idx + 1, orig_tensors[arg_idx + 1], 0, current_atoms, head);
        }
    }
    
    size_t count = curr_subtensor->data.function.arg_count;
    Expr** new_args = malloc(sizeof(Expr*) * count);
    for (size_t i = 0; i < count; i++) {
        new_args[i] = outer_rec(f, orig_tensors, num_tensors, target_depths,
                                arg_idx, curr_subtensor->data.function.args[i], curr_depth + 1, current_atoms, head);
    }
    Expr* ret = expr_new_function(expr_copy(head), new_args, count);
    free(new_args);
    return ret;
}

Expr* builtin_outer(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count < 1) return NULL;
    
    Expr* f = res->data.function.args[0];
    
    size_t num_depths = 0;
    for (int64_t i = res->data.function.arg_count - 1; i >= 1; i--) {
        Expr* a = res->data.function.args[i];
        if (a->type == EXPR_INTEGER || (a->type == EXPR_SYMBOL && strcmp(a->data.symbol, "Infinity") == 0)) {
            num_depths++;
        } else {
            break;
        }
    }
    
    size_t num_tensors = res->data.function.arg_count - 1 - num_depths;
    if (num_tensors == 0) {
        Expr* ret = expr_new_function(expr_copy(f), NULL, 0);
        Expr* evaluated = evaluate(ret);
        expr_free(ret);
        return evaluated;
    }
    
    Expr** tensors = &res->data.function.args[1];
    
    int64_t* target_depths = malloc(sizeof(int64_t) * num_tensors);
    for (size_t i = 0; i < num_tensors; i++) target_depths[i] = INT64_MAX;

    if (num_depths == 1) {
        Expr* d = res->data.function.args[1 + num_tensors];
        int64_t val = (d->type == EXPR_INTEGER) ? d->data.integer : INT64_MAX;
        for (size_t i = 0; i < num_tensors; i++) target_depths[i] = val;
    } else if (num_depths > 1) {
        for (size_t i = 0; i < num_depths && i < num_tensors; i++) {
            Expr* d = res->data.function.args[1 + num_tensors + i];
            target_depths[i] = (d->type == EXPR_INTEGER) ? d->data.integer : INT64_MAX;
        }
    }
    
    Expr* head = NULL;
    for (size_t i = 0; i < num_tensors; i++) {
        if (tensors[i]->type == EXPR_FUNCTION) {
            head = tensors[i]->data.function.head;
            break;
        }
    }
    
    Expr** current_atoms = malloc(sizeof(Expr*) * num_tensors);
    Expr* raw_res = outer_rec(f, tensors, num_tensors, target_depths, 0, tensors[0], 0, current_atoms, head);
    
    free(current_atoms);
    free(target_depths);
    
    Expr* final_eval = evaluate(raw_res);
    expr_free(raw_res);
    return final_eval;
}

/* ------------------- Tuples ------------------- */

static void tuples_rec(Expr** lists, size_t num_lists, size_t curr_list, Expr** current_tuple, Expr* head, Expr*** results, size_t* res_count, size_t* res_cap) {
    if (curr_list == num_lists) {
        Expr** t_args = malloc(sizeof(Expr*) * num_lists);
        for(size_t i=0; i<num_lists; i++) t_args[i] = expr_copy(current_tuple[i]);
        Expr* t = expr_new_function(expr_copy(head), t_args, num_lists);
        if (*res_count >= *res_cap) {
            *res_cap = (*res_cap == 0) ? 16 : (*res_cap * 2);
            *results = realloc(*results, sizeof(Expr*) * (*res_cap));
        }
        (*results)[(*res_count)++] = t;
        return;
    }
    
    Expr* lst = lists[curr_list];
    size_t count = lst->type == EXPR_FUNCTION ? lst->data.function.arg_count : 0;
    if (count == 0) return;
    
    for (size_t i = 0; i < count; i++) {
        current_tuple[curr_list] = lst->data.function.args[i];
        tuples_rec(lists, num_lists, curr_list + 1, current_tuple, head, results, res_count, res_cap);
    }
}

static Expr* reshape_rec(Expr** flat_args, size_t* offset, int64_t* dims, size_t dims_count, size_t current_dim, Expr* head) {
    if (current_dim == dims_count - 1) {
        size_t n = dims[current_dim];
        Expr** args = malloc(sizeof(Expr*) * n);
        for (size_t i = 0; i < n; i++) {
            args[i] = expr_copy(flat_args[(*offset)++]);
        }
        Expr* ret = expr_new_function(expr_copy(head), args, n);
        free(args);
        return ret;
    } else {
        size_t n = dims[current_dim];
        Expr** args = malloc(sizeof(Expr*) * n);
        for (size_t i = 0; i < n; i++) {
            args[i] = reshape_rec(flat_args, offset, dims, dims_count, current_dim + 1, head);
        }
        Expr* ret = expr_new_function(expr_copy(head), args, n);
        free(args);
        return ret;
    }
}

Expr* builtin_tuples(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count < 1 || res->data.function.arg_count > 2) return NULL;
    
    if (res->data.function.arg_count == 1) {
        Expr* arg = res->data.function.args[0];
        if (arg->type != EXPR_FUNCTION) return expr_copy(res);
        Expr* head = arg->data.function.head;
        
        size_t num_lists = arg->data.function.arg_count;
        Expr** lists = arg->data.function.args;
        
        if (num_lists == 0) {
            Expr* t = expr_new_function(expr_copy(head), NULL, 0);
            Expr* final_res = expr_new_function(expr_copy(head), (Expr*[]){t}, 1);
            return final_res;
        }

        Expr** results = NULL;
        size_t res_count = 0, res_cap = 0;
        Expr** current_tuple = malloc(sizeof(Expr*) * num_lists);
        
        tuples_rec(lists, num_lists, 0, current_tuple, head, &results, &res_count, &res_cap);
        free(current_tuple);
        
        Expr* final_res = expr_new_function(expr_copy(head), results, res_count);
        return final_res;
    } else {
        Expr* list = res->data.function.args[0];
        Expr* n_expr = res->data.function.args[1];
        if (list->type != EXPR_FUNCTION) return expr_copy(res);
        Expr* head = list->data.function.head;
        
        if (n_expr->type == EXPR_INTEGER) {
            int64_t n = n_expr->data.integer;
            if (n < 0) n = 0;
            if (n == 0) {
                Expr* t = expr_new_function(expr_copy(head), NULL, 0);
                Expr* final_res = expr_new_function(expr_copy(head), (Expr*[]){t}, 1);
                return final_res;
            }

            Expr** lists = malloc(sizeof(Expr*) * n);
            for (int64_t i = 0; i < n; i++) lists[i] = list;
            
            Expr** results = NULL;
            size_t res_count = 0, res_cap = 0;
            Expr** current_tuple = malloc(sizeof(Expr*) * n);
            
            tuples_rec(lists, n, 0, current_tuple, head, &results, &res_count, &res_cap);
            free(current_tuple);
            free(lists);
            
            return expr_new_function(expr_new_symbol("List"), results, res_count);
        } else if (n_expr->type == EXPR_FUNCTION && expr_eq(n_expr->data.function.head, head)) {
            size_t dims_count = n_expr->data.function.arg_count;
            int64_t total_elements = 1;
            int64_t* dims = malloc(sizeof(int64_t) * dims_count);
            for (size_t i = 0; i < dims_count; i++) {
                if (n_expr->data.function.args[i]->type != EXPR_INTEGER) {
                    free(dims);
                    return expr_copy(res);
                }
                dims[i] = n_expr->data.function.args[i]->data.integer;
                if (dims[i] < 0) dims[i] = 0;
                total_elements *= dims[i];
            }
            
            if (total_elements == 0) {
                free(dims);
                return expr_new_function(expr_copy(head), NULL, 0);
            }

            Expr** lists = malloc(sizeof(Expr*) * total_elements);
            for (int64_t i = 0; i < total_elements; i++) lists[i] = list;
            
            Expr** results = NULL;
            size_t res_count = 0, res_cap = 0;
            Expr** current_tuple = malloc(sizeof(Expr*) * total_elements);
            
            tuples_rec(lists, total_elements, 0, current_tuple, head, &results, &res_count, &res_cap);
            free(current_tuple);
            free(lists);
            
            for (size_t i = 0; i < res_count; i++) {
                size_t offset = 0;
                Expr* reshaped = reshape_rec(results[i]->data.function.args, &offset, dims, dims_count, 0, head);
                expr_free(results[i]);
                results[i] = reshaped;
            }
            
            free(dims);
            return expr_new_function(expr_new_symbol("List"), results, res_count);
        }
    }
    return expr_copy(res);
}

/* ------------------- Permutations ------------------- */

typedef struct {
    Expr* expr;
    int count;
} UniqueElement;

static void permutations_rec(UniqueElement* elements, size_t num_unique, size_t target_len, size_t current_len, 
                             Expr** current_perm, Expr* head, Expr*** results, size_t* res_count, size_t* res_cap) {
    if (current_len == target_len) {
        Expr** p_args = NULL;
        if (target_len > 0) {
            p_args = malloc(sizeof(Expr*) * target_len);
            for (size_t i = 0; i < target_len; i++) p_args[i] = expr_copy(current_perm[i]);
        }
        Expr* p = expr_new_function(expr_copy(head), p_args, target_len);
        if (p_args) free(p_args);
        if (*res_count >= *res_cap) {
            *res_cap = (*res_cap == 0) ? 16 : (*res_cap * 2);
            *results = realloc(*results, sizeof(Expr*) * (*res_cap));
        }
        (*results)[(*res_count)++] = p;
        return;
    }
    
    for (size_t i = 0; i < num_unique; i++) {
        if (elements[i].count > 0) {
            elements[i].count--;
            current_perm[current_len] = elements[i].expr;
            permutations_rec(elements, num_unique, target_len, current_len + 1, current_perm, head, results, res_count, res_cap);
            elements[i].count++;
        }
    }
}

Expr* builtin_permutations(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count < 1 || res->data.function.arg_count > 2) return NULL;
    
    Expr* list = res->data.function.args[0];
    if (list->type != EXPR_FUNCTION) return expr_copy(res); 
    
    Expr* head = list->data.function.head;
    size_t len = list->data.function.arg_count;
    
    int64_t n_min = len;
    int64_t n_max = len;
    int64_t dn = 1;
    
    if (res->data.function.arg_count == 2) {
        Expr* spec = res->data.function.args[1];
        if (spec->type == EXPR_INTEGER) {
            int64_t n = spec->data.integer;
            if (n > (int64_t)len) n = len;
            if (n < 0) n = 0;
            n_min = n;
            n_max = n;
            dn = 1;
        } else if (spec->type == EXPR_SYMBOL && strcmp(spec->data.symbol, "All") == 0) {
            n_min = 0;
            n_max = len;
            dn = 1;
        } else if (spec->type == EXPR_FUNCTION && strcmp(spec->data.function.head->data.symbol, "List") == 0) {
            size_t s_len = spec->data.function.arg_count;
            if (s_len == 1 && spec->data.function.args[0]->type == EXPR_INTEGER) {
                n_min = n_max = spec->data.function.args[0]->data.integer;
            } else if (s_len == 2 && spec->data.function.args[0]->type == EXPR_INTEGER && spec->data.function.args[1]->type == EXPR_INTEGER) {
                n_min = spec->data.function.args[0]->data.integer;
                n_max = spec->data.function.args[1]->data.integer;
                dn = (n_min <= n_max) ? 1 : -1;
            } else if (s_len == 3 && spec->data.function.args[0]->type == EXPR_INTEGER && spec->data.function.args[1]->type == EXPR_INTEGER && spec->data.function.args[2]->type == EXPR_INTEGER) {
                n_min = spec->data.function.args[0]->data.integer;
                n_max = spec->data.function.args[1]->data.integer;
                dn = spec->data.function.args[2]->data.integer;
            }
        }
    }
    
    UniqueElement* elements = malloc(sizeof(UniqueElement) * len);
    size_t num_unique = 0;
    for (size_t i = 0; i < len; i++) {
        Expr* e = list->data.function.args[i];
        bool found = false;
        for (size_t j = 0; j < num_unique; j++) {
            if (expr_eq(e, elements[j].expr)) {
                elements[j].count++;
                found = true;
                break;
            }
        }
        if (!found) {
            elements[num_unique].expr = e;
            elements[num_unique].count = 1;
            num_unique++;
        }
    }
    
    Expr** results = NULL;
    size_t res_count = 0, res_cap = 0;
    Expr** current_perm = malloc(sizeof(Expr*) * (n_min > n_max ? n_min : n_max) + sizeof(Expr*) * len); 
    
    if (dn != 0) {
        if (dn > 0) {
            for (int64_t l = n_min; l <= n_max; l += dn) {
                if (l >= 0 && l <= (int64_t)len) {
                    permutations_rec(elements, num_unique, l, 0, current_perm, head, &results, &res_count, &res_cap);
                }
            }
        } else {
            for (int64_t l = n_min; l >= n_max; l += dn) {
                if (l >= 0 && l <= (int64_t)len) {
                    permutations_rec(elements, num_unique, l, 0, current_perm, head, &results, &res_count, &res_cap);
                }
            }
        }
    }
    
    free(current_perm);
    free(elements);
    
    Expr* final_res = expr_new_function(expr_new_symbol("List"), results, res_count);
    if (results) free(results);
    return final_res;
}

/* ------------------- Nest ------------------- */

/*
 * Nest[f, expr, n] returns the result of applying f to expr n times.
 *
 * n must be a non-negative integer. If n is 0, a copy of expr is returned.
 * Each iteration constructs f[current] and evaluates it, taking the result
 * as the new current value.
 *
 * Returns NULL (leaving Nest unevaluated) for:
 *   - wrong arg count
 *   - n that is not an integer
 *   - n < 0
 */
Expr* builtin_nest(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 3) return NULL;

    Expr* f = res->data.function.args[0];
    Expr* expr = res->data.function.args[1];
    Expr* n_expr = res->data.function.args[2];

    if (n_expr->type != EXPR_INTEGER) return NULL;
    int64_t n = n_expr->data.integer;
    if (n < 0) return NULL;

    Expr* current = expr_copy(expr);
    for (int64_t i = 0; i < n; i++) {
        Expr* arg_copy = expr_copy(current);
        Expr* call = expr_new_function(expr_copy(f), &arg_copy, 1);
        Expr* next = eval_and_free(call);
        expr_free(current);
        current = next;
    }
    return current;
}

/* ------------------- NestList ------------------- */

/*
 * NestList[f, expr, n] returns a list of length n+1 containing expr and the
 * results of applying f to expr 1, 2, ..., n times.
 *
 * Returns NULL (leaving NestList unevaluated) for:
 *   - wrong arg count
 *   - n that is not an integer
 *   - n < 0
 */
Expr* builtin_nestlist(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 3) return NULL;

    Expr* f = res->data.function.args[0];
    Expr* expr = res->data.function.args[1];
    Expr* n_expr = res->data.function.args[2];

    if (n_expr->type != EXPR_INTEGER) return NULL;
    int64_t n = n_expr->data.integer;
    if (n < 0) return NULL;

    size_t count = (size_t)n + 1;
    Expr** items = malloc(sizeof(Expr*) * count);
    items[0] = expr_copy(expr);
    for (int64_t i = 0; i < n; i++) {
        Expr* arg_copy = expr_copy(items[i]);
        Expr* call = expr_new_function(expr_copy(f), &arg_copy, 1);
        items[i + 1] = eval_and_free(call);
    }
    Expr* list = expr_new_function(expr_new_symbol("List"), items, count);
    free(items);
    return list;
}

/* ------------------- NestWhile ------------------- */

/*
 * NestWhile[f, expr, test]                    -- apply f while test yields True
 * NestWhile[f, expr, test, m]                 -- pass last m results to test (m = All means all)
 * NestWhile[f, expr, test, {mmin, mmax}]      -- start testing once mmin results exist, pass up to mmax
 * NestWhile[f, expr, test, m, max]            -- cap the number of f-applications at max
 * NestWhile[f, expr, test, m, max, n]         -- apply f n extra times after the loop ends
 * NestWhile[f, expr, test, m, max, -n]        -- return the result n applications before the end
 *
 * Returns NULL (leaving NestWhile unevaluated) for malformed argument specs.
 */

static bool nw_is_true(Expr* e) {
    return e && e->type == EXPR_SYMBOL && strcmp(e->data.symbol, "True") == 0;
}

Expr* builtin_nestwhile(Expr* res) {
    if (res->type != EXPR_FUNCTION) return NULL;
    size_t argc = res->data.function.arg_count;
    if (argc < 3 || argc > 6) return NULL;

    Expr* f = res->data.function.args[0];
    Expr* expr = res->data.function.args[1];
    Expr* test = res->data.function.args[2];

    /* Parse m: positive integer, All, or {mmin, mmax|Infinity} */
    int64_t m_min = 1, m_max = 1;
    bool m_max_inf = false;
    if (argc >= 4) {
        Expr* m_arg = res->data.function.args[3];
        if (m_arg->type == EXPR_INTEGER) {
            if (m_arg->data.integer < 1) return NULL;
            m_min = m_max = m_arg->data.integer;
        } else if (m_arg->type == EXPR_SYMBOL && strcmp(m_arg->data.symbol, "All") == 0) {
            m_min = 1;
            m_max_inf = true;
        } else if (m_arg->type == EXPR_FUNCTION &&
                   m_arg->data.function.head->type == EXPR_SYMBOL &&
                   strcmp(m_arg->data.function.head->data.symbol, "List") == 0 &&
                   m_arg->data.function.arg_count == 2) {
            Expr* a0 = m_arg->data.function.args[0];
            Expr* a1 = m_arg->data.function.args[1];
            if (a0->type != EXPR_INTEGER || a0->data.integer < 1) return NULL;
            m_min = a0->data.integer;
            if (a1->type == EXPR_INTEGER) {
                if (a1->data.integer < m_min) return NULL;
                m_max = a1->data.integer;
            } else if (a1->type == EXPR_SYMBOL && strcmp(a1->data.symbol, "Infinity") == 0) {
                m_max_inf = true;
            } else {
                return NULL;
            }
        } else {
            return NULL;
        }
    }

    /* Parse max: non-negative integer or Infinity */
    int64_t max_apps = 0;
    bool max_inf = true;
    if (argc >= 5) {
        Expr* max_arg = res->data.function.args[4];
        if (max_arg->type == EXPR_INTEGER) {
            if (max_arg->data.integer < 0) return NULL;
            max_apps = max_arg->data.integer;
            max_inf = false;
        } else if (max_arg->type == EXPR_SYMBOL && strcmp(max_arg->data.symbol, "Infinity") == 0) {
            max_inf = true;
        } else {
            return NULL;
        }
    }

    /* Parse n_extra: any integer */
    int64_t n_extra = 0;
    if (argc >= 6) {
        Expr* n_arg = res->data.function.args[5];
        if (n_arg->type != EXPR_INTEGER) return NULL;
        n_extra = n_arg->data.integer;
    }

    /* Safety cap for effectively-infinite iteration */
    const int64_t SAFETY_CAP = 1000000;

    size_t cap = 16;
    size_t count = 0;
    Expr** items = malloc(sizeof(Expr*) * cap);
    items[count++] = expr_copy(expr);

    int64_t apps = 0;
    while (1) {
        if (!max_inf && apps >= max_apps) break;
        if (max_inf && apps >= SAFETY_CAP) {
            for (size_t i = 0; i < count; i++) expr_free(items[i]);
            free(items);
            return NULL;
        }

        if ((int64_t)count >= m_min) {
            int64_t k = m_max_inf ? (int64_t)count
                                  : ((int64_t)count < m_max ? (int64_t)count : m_max);
            Expr** test_args = malloc(sizeof(Expr*) * (size_t)k);
            for (int64_t i = 0; i < k; i++) {
                test_args[i] = expr_copy(items[count - (size_t)k + (size_t)i]);
            }
            Expr* test_call = expr_new_function(expr_copy(test), test_args, (size_t)k);
            free(test_args);
            Expr* test_result = eval_and_free(test_call);
            bool is_true = nw_is_true(test_result);
            expr_free(test_result);
            if (!is_true) break;
        }

        Expr* arg_copy = expr_copy(items[count - 1]);
        Expr* call = expr_new_function(expr_copy(f), &arg_copy, 1);
        Expr* next = eval_and_free(call);
        if (count >= cap) {
            cap *= 2;
            items = realloc(items, sizeof(Expr*) * cap);
        }
        items[count++] = next;
        apps++;
    }

    /* Positive n_extra: apply f n_extra more times */
    if (n_extra > 0) {
        for (int64_t i = 0; i < n_extra; i++) {
            Expr* arg_copy = expr_copy(items[count - 1]);
            Expr* call = expr_new_function(expr_copy(f), &arg_copy, 1);
            Expr* next = eval_and_free(call);
            if (count >= cap) {
                cap *= 2;
                items = realloc(items, sizeof(Expr*) * cap);
            }
            items[count++] = next;
        }
    }

    /* Determine return index: Part[NestWhileList, -n_extra-1] when n_extra < 0 */
    int64_t ret_idx = (n_extra >= 0) ? (int64_t)count - 1
                                     : (int64_t)count - 1 + n_extra;
    if (ret_idx < 0 || ret_idx >= (int64_t)count) {
        for (size_t i = 0; i < count; i++) expr_free(items[i]);
        free(items);
        return NULL;
    }

    Expr* result = items[ret_idx];
    items[ret_idx] = NULL;
    for (size_t i = 0; i < count; i++) {
        if (items[i]) expr_free(items[i]);
    }
    free(items);
    return result;
}

/* ------------------- NestWhileList ------------------- */

/*
 * NestWhileList[f, expr, test]                    -- list results while test yields True
 * NestWhileList[f, expr, test, m]                 -- pass last m results to test
 * NestWhileList[f, expr, test, {mmin, mmax}]      -- start testing once mmin exist, pass up to mmax
 * NestWhileList[f, expr, test, m, max]            -- cap f-applications at max
 * NestWhileList[f, expr, test, m, max, n]         -- append n extra applications
 * NestWhileList[f, expr, test, m, max, -n]        -- drop last n elements
 *
 * Returns NULL (leaving unevaluated) for malformed argument specs.
 */
Expr* builtin_nestwhilelist(Expr* res) {
    if (res->type != EXPR_FUNCTION) return NULL;
    size_t argc = res->data.function.arg_count;
    if (argc < 3 || argc > 6) return NULL;

    Expr* f = res->data.function.args[0];
    Expr* expr = res->data.function.args[1];
    Expr* test = res->data.function.args[2];

    /* Parse m: positive integer, All, or {mmin, mmax|Infinity} */
    int64_t m_min = 1, m_max = 1;
    bool m_max_inf = false;
    if (argc >= 4) {
        Expr* m_arg = res->data.function.args[3];
        if (m_arg->type == EXPR_INTEGER) {
            if (m_arg->data.integer < 1) return NULL;
            m_min = m_max = m_arg->data.integer;
        } else if (m_arg->type == EXPR_SYMBOL && strcmp(m_arg->data.symbol, "All") == 0) {
            m_min = 1;
            m_max_inf = true;
        } else if (m_arg->type == EXPR_FUNCTION &&
                   m_arg->data.function.head->type == EXPR_SYMBOL &&
                   strcmp(m_arg->data.function.head->data.symbol, "List") == 0 &&
                   m_arg->data.function.arg_count == 2) {
            Expr* a0 = m_arg->data.function.args[0];
            Expr* a1 = m_arg->data.function.args[1];
            if (a0->type != EXPR_INTEGER || a0->data.integer < 1) return NULL;
            m_min = a0->data.integer;
            if (a1->type == EXPR_INTEGER) {
                if (a1->data.integer < m_min) return NULL;
                m_max = a1->data.integer;
            } else if (a1->type == EXPR_SYMBOL && strcmp(a1->data.symbol, "Infinity") == 0) {
                m_max_inf = true;
            } else {
                return NULL;
            }
        } else {
            return NULL;
        }
    }

    /* Parse max: non-negative integer or Infinity */
    int64_t max_apps = 0;
    bool max_inf = true;
    if (argc >= 5) {
        Expr* max_arg = res->data.function.args[4];
        if (max_arg->type == EXPR_INTEGER) {
            if (max_arg->data.integer < 0) return NULL;
            max_apps = max_arg->data.integer;
            max_inf = false;
        } else if (max_arg->type == EXPR_SYMBOL && strcmp(max_arg->data.symbol, "Infinity") == 0) {
            max_inf = true;
        } else {
            return NULL;
        }
    }

    /* Parse n_extra: any integer */
    int64_t n_extra = 0;
    if (argc >= 6) {
        Expr* n_arg = res->data.function.args[5];
        if (n_arg->type != EXPR_INTEGER) return NULL;
        n_extra = n_arg->data.integer;
    }

    const int64_t SAFETY_CAP = 1000000;

    size_t cap = 16;
    size_t count = 0;
    Expr** items = malloc(sizeof(Expr*) * cap);
    items[count++] = expr_copy(expr);

    int64_t apps = 0;
    while (1) {
        if (!max_inf && apps >= max_apps) break;
        if (max_inf && apps >= SAFETY_CAP) {
            for (size_t i = 0; i < count; i++) expr_free(items[i]);
            free(items);
            return NULL;
        }

        if ((int64_t)count >= m_min) {
            int64_t k = m_max_inf ? (int64_t)count
                                  : ((int64_t)count < m_max ? (int64_t)count : m_max);
            Expr** test_args = malloc(sizeof(Expr*) * (size_t)k);
            for (int64_t i = 0; i < k; i++) {
                test_args[i] = expr_copy(items[count - (size_t)k + (size_t)i]);
            }
            Expr* test_call = expr_new_function(expr_copy(test), test_args, (size_t)k);
            free(test_args);
            Expr* test_result = eval_and_free(test_call);
            bool is_true = nw_is_true(test_result);
            expr_free(test_result);
            if (!is_true) break;
        }

        Expr* arg_copy = expr_copy(items[count - 1]);
        Expr* call = expr_new_function(expr_copy(f), &arg_copy, 1);
        Expr* next = eval_and_free(call);
        if (count >= cap) {
            cap *= 2;
            items = realloc(items, sizeof(Expr*) * cap);
        }
        items[count++] = next;
        apps++;
    }

    /* Positive n_extra: apply f n_extra more times, appending */
    if (n_extra > 0) {
        for (int64_t i = 0; i < n_extra; i++) {
            Expr* arg_copy = expr_copy(items[count - 1]);
            Expr* call = expr_new_function(expr_copy(f), &arg_copy, 1);
            Expr* next = eval_and_free(call);
            if (count >= cap) {
                cap *= 2;
                items = realloc(items, sizeof(Expr*) * cap);
            }
            items[count++] = next;
        }
    }

    /* Negative n_extra: drop last |n_extra| elements */
    size_t final_count = count;
    if (n_extra < 0) {
        int64_t drop = -n_extra;
        if (drop >= (int64_t)count) {
            final_count = 0;
        } else {
            final_count = count - (size_t)drop;
        }
        for (size_t i = final_count; i < count; i++) {
            expr_free(items[i]);
            items[i] = NULL;
        }
    }

    /* Build the List result */
    Expr** list_args = malloc(sizeof(Expr*) * (final_count > 0 ? final_count : 1));
    for (size_t i = 0; i < final_count; i++) {
        list_args[i] = items[i];
    }
    Expr* list = expr_new_function(expr_new_symbol("List"), list_args, final_count);
    free(list_args);
    free(items);
    return list;
}

/* ------------------- FixedPointList ------------------- */

/*
 * FixedPointList[f, expr]                     -- list expr, f[expr], f[f[expr]], ...
 *                                                until two successive results are SameQ.
 *                                                The fixed point appears as the last two elements.
 * FixedPointList[f, expr, n]                  -- stop after at most n applications of f.
 *                                                If n applications occur without convergence,
 *                                                the last two elements may not be equal.
 * FixedPointList[f, expr, SameTest -> s]      -- use s instead of SameQ to compare pairs.
 * FixedPointList[f, expr, n, SameTest -> s]   -- combine bounded iteration and custom test.
 *
 * Returns NULL (leaving FixedPointList unevaluated) for malformed argument specs.
 */
Expr* builtin_fixedpointlist(Expr* res) {
    if (res->type != EXPR_FUNCTION) return NULL;
    size_t argc = res->data.function.arg_count;
    if (argc < 2) return NULL;

    Expr* f = res->data.function.args[0];
    Expr* expr = res->data.function.args[1];

    /* Parse remaining args: optional n (integer/Infinity), optional SameTest -> s */
    Expr* same_test = NULL;
    bool has_max = false;
    int64_t max_apps = 0;
    bool max_inf = true;

    for (size_t i = 2; i < argc; i++) {
        Expr* a = res->data.function.args[i];
        if (a->type == EXPR_FUNCTION &&
            a->data.function.head->type == EXPR_SYMBOL &&
            (strcmp(a->data.function.head->data.symbol, "Rule") == 0 ||
             strcmp(a->data.function.head->data.symbol, "RuleDelayed") == 0) &&
            a->data.function.arg_count == 2 &&
            a->data.function.args[0]->type == EXPR_SYMBOL &&
            strcmp(a->data.function.args[0]->data.symbol, "SameTest") == 0) {
            if (same_test != NULL) return NULL;
            same_test = a->data.function.args[1];
        } else if (!has_max && a->type == EXPR_INTEGER) {
            if (a->data.integer < 0) return NULL;
            max_apps = a->data.integer;
            max_inf = false;
            has_max = true;
        } else if (!has_max && a->type == EXPR_SYMBOL &&
                   strcmp(a->data.symbol, "Infinity") == 0) {
            has_max = true;
            max_inf = true;
        } else {
            return NULL;
        }
    }

    const int64_t SAFETY_CAP = 1000000;

    size_t cap = 16;
    size_t count = 0;
    Expr** items = malloc(sizeof(Expr*) * cap);
    items[count++] = expr_copy(expr);

    int64_t apps = 0;
    while (1) {
        if (!max_inf && apps >= max_apps) break;
        if (max_inf && apps >= SAFETY_CAP) {
            for (size_t i = 0; i < count; i++) expr_free(items[i]);
            free(items);
            return NULL;
        }

        Expr* arg_copy = expr_copy(items[count - 1]);
        Expr* call = expr_new_function(expr_copy(f), &arg_copy, 1);
        Expr* next = eval_and_free(call);

        if (count >= cap) {
            cap *= 2;
            items = realloc(items, sizeof(Expr*) * cap);
        }
        items[count++] = next;
        apps++;

        bool same;
        if (same_test == NULL) {
            same = expr_eq(items[count - 2], items[count - 1]);
        } else {
            Expr* test_args[2];
            test_args[0] = expr_copy(items[count - 2]);
            test_args[1] = expr_copy(items[count - 1]);
            Expr* test_call = expr_new_function(expr_copy(same_test), test_args, 2);
            Expr* test_result = eval_and_free(test_call);
            same = (test_result && test_result->type == EXPR_SYMBOL &&
                    strcmp(test_result->data.symbol, "True") == 0);
            expr_free(test_result);
        }
        if (same) break;
    }

    Expr** list_args = malloc(sizeof(Expr*) * (count > 0 ? count : 1));
    for (size_t i = 0; i < count; i++) {
        list_args[i] = items[i];
    }
    Expr* list = expr_new_function(expr_new_symbol("List"), list_args, count);
    free(list_args);
    free(items);
    return list;
}

Expr* builtin_fixedpoint(Expr* res) {
    if (res->type != EXPR_FUNCTION) return NULL;
    size_t argc = res->data.function.arg_count;
    if (argc < 2) return NULL;

    Expr* f = res->data.function.args[0];
    Expr* expr = res->data.function.args[1];

    Expr* same_test = NULL;
    bool has_max = false;
    int64_t max_apps = 0;
    bool max_inf = true;

    for (size_t i = 2; i < argc; i++) {
        Expr* a = res->data.function.args[i];
        if (a->type == EXPR_FUNCTION &&
            a->data.function.head->type == EXPR_SYMBOL &&
            (strcmp(a->data.function.head->data.symbol, "Rule") == 0 ||
             strcmp(a->data.function.head->data.symbol, "RuleDelayed") == 0) &&
            a->data.function.arg_count == 2 &&
            a->data.function.args[0]->type == EXPR_SYMBOL &&
            strcmp(a->data.function.args[0]->data.symbol, "SameTest") == 0) {
            if (same_test != NULL) return NULL;
            same_test = a->data.function.args[1];
        } else if (!has_max && a->type == EXPR_INTEGER) {
            if (a->data.integer < 0) return NULL;
            max_apps = a->data.integer;
            max_inf = false;
            has_max = true;
        } else if (!has_max && a->type == EXPR_SYMBOL &&
                   strcmp(a->data.symbol, "Infinity") == 0) {
            has_max = true;
            max_inf = true;
        } else {
            return NULL;
        }
    }

    const int64_t SAFETY_CAP = 1000000;

    Expr* prev = expr_copy(expr);
    int64_t apps = 0;
    while (1) {
        if (!max_inf && apps >= max_apps) break;
        if (max_inf && apps >= SAFETY_CAP) {
            expr_free(prev);
            return NULL;
        }

        Expr* arg_copy = expr_copy(prev);
        Expr* call = expr_new_function(expr_copy(f), &arg_copy, 1);
        Expr* next = eval_and_free(call);
        apps++;

        if (next && next->type == EXPR_FUNCTION &&
            next->data.function.head->type == EXPR_SYMBOL) {
            const char* h = next->data.function.head->data.symbol;
            if (strcmp(h, "Throw") == 0 || strcmp(h, "Abort") == 0 ||
                strcmp(h, "Quit") == 0 || strcmp(h, "Return") == 0) {
                expr_free(prev);
                return next;
            }
        }

        bool same;
        if (same_test == NULL) {
            same = expr_eq(prev, next);
        } else {
            Expr* test_args[2];
            test_args[0] = expr_copy(prev);
            test_args[1] = expr_copy(next);
            Expr* test_call = expr_new_function(expr_copy(same_test), test_args, 2);
            Expr* test_result = eval_and_free(test_call);
            same = (test_result && test_result->type == EXPR_SYMBOL &&
                    strcmp(test_result->data.symbol, "True") == 0);
            expr_free(test_result);
        }

        expr_free(prev);
        prev = next;
        if (same) break;
    }

    return prev;
}
