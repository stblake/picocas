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
