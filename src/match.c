#include "match.h"
#include <gmp.h>
#include "part.h"
#include "eval.h"
#include "print.h"
#include "symtab.h"
#include "attr.h"
#include "default_helper.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static bool is_atom(const Expr* e) {
    if (!e) return true;
    if (e->type != EXPR_FUNCTION) return true;
    if (e->data.function.head->type == EXPR_SYMBOL) {
        const char* head_name = e->data.function.head->data.symbol;
        if (strcmp(head_name, "Complex") == 0 || strcmp(head_name, "Rational") == 0) {
            return true;
        }
    }
    return false;
}

MatchEnv* env_new(void) {
    MatchEnv* env = malloc(sizeof(MatchEnv));
    env->count = 0;
    env->capacity = 8;
    env->symbols = malloc(sizeof(char*) * env->capacity);
    env->values = malloc(sizeof(Expr*) * env->capacity);
    return env;
}

void env_free(MatchEnv* env) {
    if (!env) return;
    for (size_t i = 0; i < env->count; i++) {
        free(env->symbols[i]);
        expr_free(env->values[i]);
    }
    free(env->symbols);
    free(env->values);
    free(env);
}

void env_set(MatchEnv* env, const char* symbol, Expr* value) {
    for (size_t i = 0; i < env->count; i++) {
        if (strcmp(env->symbols[i], symbol) == 0) {
            expr_free(env->values[i]);
            env->values[i] = expr_copy(value);
            return;
        }
    }
    if (env->count >= env->capacity) {
        env->capacity *= 2;
        env->symbols = realloc(env->symbols, sizeof(char*) * env->capacity);
        env->values = realloc(env->values, sizeof(Expr*) * env->capacity);
    }
    env->symbols[env->count] = strdup(symbol);
    env->values[env->count] = expr_copy(value);
    env->count++;
}

static void env_rollback(MatchEnv* env, size_t saved_count) {
    while (env->count > saved_count) {
        env->count--;
        free(env->symbols[env->count]);
        expr_free(env->values[env->count]);
    }
}

Expr* env_get(MatchEnv* env, const char* symbol) {
    for (size_t i = 0; i < env->count; i++) {
        if (strcmp(env->symbols[i], symbol) == 0) {
            return env->values[i];
        }
    }
    return NULL;
}

static bool is_pattern(Expr* e, Expr** sym_out, Expr** pat_out) {
    if (e->type == EXPR_FUNCTION && e->data.function.head->type == EXPR_SYMBOL) {
        if (strcmp(e->data.function.head->data.symbol, "Pattern") == 0) {
            if (e->data.function.arg_count == 2) {
                if (sym_out) *sym_out = e->data.function.args[0];
                if (pat_out) *pat_out = e->data.function.args[1];
                return true;
            }
        }
    }
    return false;
}

static bool is_blank(Expr* e, Expr** head_out) {
    if (e->type == EXPR_FUNCTION && e->data.function.head->type == EXPR_SYMBOL) {
        if (strcmp(e->data.function.head->data.symbol, "Blank") == 0) {
            if (e->data.function.arg_count == 0) {
                if (head_out) *head_out = NULL;
                return true;
            } else if (e->data.function.arg_count == 1) {
                if (head_out) *head_out = e->data.function.args[0];
                return true;
            }
        }
    }
    return false;
}

static bool is_sequence_blank(Expr* e, Expr** head_out, int* min_len) {
    if (e->type == EXPR_FUNCTION && e->data.function.head->type == EXPR_SYMBOL) {
        const char* head = e->data.function.head->data.symbol;
        if (strcmp(head, "BlankSequence") == 0) {
            if (min_len) *min_len = 1;
            if (head_out) *head_out = (e->data.function.arg_count == 1) ? e->data.function.args[0] : NULL;
            return true;
        }
        if (strcmp(head, "BlankNullSequence") == 0) {
            if (min_len) *min_len = 0;
            if (head_out) *head_out = (e->data.function.arg_count == 1) ? e->data.function.args[0] : NULL;
            return true;
        }
    }
    return false;
}

static bool is_repeated(Expr* e, Expr** rep_pat, int* min_len, int* max_len) {
    if (e->type != EXPR_FUNCTION || e->data.function.head->type != EXPR_SYMBOL) return false;
    const char* head = e->data.function.head->data.symbol;
    bool is_rep = (strcmp(head, "Repeated") == 0);
    bool is_rep_null = (strcmp(head, "RepeatedNull") == 0);
    if (!is_rep && !is_rep_null) return false;

    *min_len = is_rep ? 1 : 0;
    *max_len = -1; // -1 means infinity
    
    if (e->data.function.arg_count >= 1) {
        *rep_pat = e->data.function.args[0];
    } else {
        return false;
    }
    
    if (e->data.function.arg_count >= 2) {
        Expr* spec = e->data.function.args[1];
        if (spec->type == EXPR_INTEGER) {
            *max_len = (int)spec->data.integer;
        } else if (spec->type == EXPR_FUNCTION && spec->data.function.head->type == EXPR_SYMBOL && strcmp(spec->data.function.head->data.symbol, "List") == 0) {
            if (spec->data.function.arg_count == 1 && spec->data.function.args[0]->type == EXPR_INTEGER) {
                *min_len = (int)spec->data.function.args[0]->data.integer;
                *max_len = *min_len;
            } else if (spec->data.function.arg_count == 2) {
                if (spec->data.function.args[0]->type == EXPR_INTEGER) {
                    *min_len = (int)spec->data.function.args[0]->data.integer;
                }
                if (spec->data.function.args[1]->type == EXPR_INTEGER) {
                    *max_len = (int)spec->data.function.args[1]->data.integer;
                } else if (spec->data.function.args[1]->type == EXPR_SYMBOL && strcmp(spec->data.function.args[1]->data.symbol, "Infinity") == 0) {
                    *max_len = -1;
                }
            }
        }
    }
    return true;
}

static bool match_args(Expr** exprs, size_t n_exprs, Expr** pats, size_t n_pats, MatchEnv* env, Expr* condition, Expr* pat_head, size_t total_pats);

#include "part.h" // for expr_head

/* Helper to get head without allocating */
static Expr* get_expr_head_borrowed(Expr* e) {
    if (is_atom(e)) return NULL; // atoms return symbol head usually, but match handles it
    if (e->type == EXPR_FUNCTION) return e->data.function.head;
    return NULL;
}

static bool next_combination(int* comb, int n, int k) {
    if (k == 0) return false;
    int i = k - 1;
    while (i >= 0 && comb[i] == n - k + i) {
        i--;
    }
    if (i < 0) return false;
    comb[i]++;
    for (int j = i + 1; j < k; j++) {
        comb[j] = comb[j - 1] + 1;
    }
    return true;
}

static void extract_subset(Expr** exprs, size_t n_exprs, int* comb, int k, Expr** subset, Expr** remainder) {
    int c_idx = 0;
    int r_idx = 0;
    for (int i = 0; i < (int)n_exprs; i++) {
        if (c_idx < k && comb[c_idx] == i) {
            subset[c_idx++] = exprs[i];
        } else {
            remainder[r_idx++] = exprs[i];
        }
    }
}

bool match(Expr* expr, Expr* pattern, MatchEnv* env) {
    if (!pattern) return false;
    if (!expr) return false;

    // Handle Condition at the top level of the match
    if (pattern->type == EXPR_FUNCTION && pattern->data.function.head->type == EXPR_SYMBOL &&
        strcmp(pattern->data.function.head->data.symbol, "Condition") == 0) {
        if (pattern->data.function.arg_count != 2) return false;
        
        Expr* inner_pat = pattern->data.function.args[0];
        Expr* cond = pattern->data.function.args[1];
        
        if (inner_pat->type == EXPR_FUNCTION && expr->type == EXPR_FUNCTION) {
            size_t saved_env_count = env->count;
            if (!match(expr->data.function.head, inner_pat->data.function.head, env)) {
                env_rollback(env, saved_env_count);
                return false;
            }
            bool res = match_args(expr->data.function.args, expr->data.function.arg_count,
                                 inner_pat->data.function.args, inner_pat->data.function.arg_count, env, cond, inner_pat->data.function.head, inner_pat->data.function.arg_count);
            if (!res) env_rollback(env, saved_env_count);
            return res;
        }

        // For non-functions or head mismatch, use the simple one-shot check
        size_t saved_env_count = env->count;
        if (!match(expr, inner_pat, env)) {
            env_rollback(env, saved_env_count);
            return false;
        }
        Expr* expanded_test = replace_bindings(cond, env);
        Expr* result = evaluate(expanded_test);
        expr_free(expanded_test);
        bool success = (result->type == EXPR_SYMBOL && strcmp(result->data.symbol, "True") == 0);
        expr_free(result);
        if (!success) env_rollback(env, saved_env_count);
        return success;
    }

    Expr* b_head = NULL;
    if (is_blank(pattern, &b_head)) {
        if (!b_head) return true;
        
        Expr* h = get_expr_head_borrowed(expr);
        if (h) return expr_eq(h, b_head);
        
        // Atom head: check against symbol names
        if (b_head->type == EXPR_SYMBOL) {
            const char* hn = b_head->data.symbol;
            if (expr->type == EXPR_INTEGER && strcmp(hn, "Integer") == 0) return true;
            if (expr->type == EXPR_REAL && strcmp(hn, "Real") == 0) return true;
            if (expr->type == EXPR_SYMBOL && strcmp(hn, "Symbol") == 0) return true;
            if (expr->type == EXPR_STRING && strcmp(hn, "String") == 0) return true;
        }
        return false;
    }

    int min_len = 0;
    if (is_sequence_blank(pattern, &b_head, &min_len)) {
        if (min_len > 1) return false;
        if (!b_head) return true;
        
        Expr* h = get_expr_head_borrowed(expr);
        if (h) return expr_eq(h, b_head);
        
        if (b_head->type == EXPR_SYMBOL) {
            const char* hn = b_head->data.symbol;
            if (expr->type == EXPR_INTEGER && strcmp(hn, "Integer") == 0) return true;
            if (expr->type == EXPR_REAL && strcmp(hn, "Real") == 0) return true;
            if (expr->type == EXPR_SYMBOL && strcmp(hn, "Symbol") == 0) return true;
            if (expr->type == EXPR_STRING && strcmp(hn, "String") == 0) return true;
        }
        return false;
    }

    Expr* p_sym = NULL;
    Expr* p_pat = NULL;
    if (is_pattern(pattern, &p_sym, &p_pat)) {
        if (p_sym->type == EXPR_SYMBOL) {
            size_t saved_env_count = env->count;
            if (!match(expr, p_pat, env)) {
                env_rollback(env, saved_env_count);
                return false;
            }
            Expr* existing = env_get(env, p_sym->data.symbol);
            if (existing) {
                if (expr_eq(expr, existing)) return true;
                env_rollback(env, saved_env_count);
                return false;
            } else {
                env_set(env, p_sym->data.symbol, expr);
                return true;
            }
        }
    }

    // Handle Alternatives
    if (pattern->type == EXPR_FUNCTION && pattern->data.function.head->type == EXPR_SYMBOL &&
        strcmp(pattern->data.function.head->data.symbol, "Alternatives") == 0) {
        size_t saved_env_count = env->count;
        for (size_t i = 0; i < pattern->data.function.arg_count; i++) {
            if (match(expr, pattern->data.function.args[i], env)) return true;
            env_rollback(env, saved_env_count);
        }
        return false;
    }

    // Handle PatternTest
    if (pattern->type == EXPR_FUNCTION && pattern->data.function.head->type == EXPR_SYMBOL &&
        strcmp(pattern->data.function.head->data.symbol, "PatternTest") == 0) {
        if (pattern->data.function.arg_count != 2) return false;
        size_t saved_env_count = env->count;
        if (!match(expr, pattern->data.function.args[0], env)) {
            env_rollback(env, saved_env_count);
            return false;
        }
        
        Expr* test_func = pattern->data.function.args[1];
        Expr* call_args[1] = { expr_copy(expr) };
        Expr* test_call = expr_new_function(expr_copy(test_func), call_args, 1);
        Expr* result = evaluate(test_call);
        expr_free(test_call);
        
        bool success = (result->type == EXPR_SYMBOL && strcmp(result->data.symbol, "True") == 0);
        expr_free(result);
        if (!success) {
            env_rollback(env, saved_env_count);
        }
        return success;
    }

    bool matched_normally = false;
    if (expr->type == pattern->type) {
        switch (expr->type) {
            case EXPR_INTEGER:
                matched_normally = (expr->data.integer == pattern->data.integer); break;
            case EXPR_REAL:
                matched_normally = (expr->data.real == pattern->data.real); break;
            case EXPR_SYMBOL:
                matched_normally = (strcmp(expr->data.symbol, pattern->data.symbol) == 0); break;
            case EXPR_STRING:
                matched_normally = (strcmp(expr->data.string, pattern->data.string) == 0); break;
            case EXPR_BIGINT:
                matched_normally = (mpz_cmp(expr->data.bigint, pattern->data.bigint) == 0); break;
            case EXPR_FUNCTION: {
                size_t saved_env_count = env->count;
                if (match(expr->data.function.head, pattern->data.function.head, env)) {
                    if (match_args(expr->data.function.args, expr->data.function.arg_count,
                                   pattern->data.function.args, pattern->data.function.arg_count, env, NULL, pattern->data.function.head, pattern->data.function.arg_count)) {
                        matched_normally = true;
                    } else {
                        env_rollback(env, saved_env_count);
                    }
                } else {
                    env_rollback(env, saved_env_count);
                }
                break;
            }
        }
    }

    if (matched_normally) return true;

    // Try OneIdentity if pattern is a function
    if (pattern->type == EXPR_FUNCTION && pattern->data.function.head->type == EXPR_SYMBOL) {
        SymbolDef* def = symtab_get_def(pattern->data.function.head->data.symbol);
        if (def && (def->attributes & ATTR_ONEIDENTITY)) {
            size_t saved_env_count = env->count;
            Expr* args[1] = { expr };
            if (match_args(args, 1, pattern->data.function.args, pattern->data.function.arg_count, env, NULL, pattern->data.function.head, pattern->data.function.arg_count)) {
                return true;
            }
            env_rollback(env, saved_env_count);
        }
    }

    return false;
}

static bool match_args(Expr** exprs, size_t n_exprs, Expr** pats, size_t n_pats, MatchEnv* env, Expr* condition, Expr* pat_head, size_t total_pats) {
    if (n_pats == 0) {
        if (n_exprs != 0) return false;
        if (!condition) return true;
        
        Expr* expanded_test = replace_bindings(condition, env);
        Expr* result = evaluate(expanded_test);
        expr_free(expanded_test);
        bool success = (result->type == EXPR_SYMBOL && strcmp(result->data.symbol, "True") == 0);
        expr_free(result);
        return success;
    }

    Expr* p = pats[0];
    
    // Handle Optional, Longest, Shortest
    bool is_optional = false;
    bool is_shortest = false; // For Optional, default is Longest
    bool is_longest = false;  // For Sequence/Repeated, default is Shortest
    Expr* opt_pat = p;
    Expr* opt_container = p;

    if (p->type == EXPR_FUNCTION && p->data.function.head->type == EXPR_SYMBOL) {
        const char* head = p->data.function.head->data.symbol;
        if (strcmp(head, "Shortest") == 0) {
            is_shortest = true;
            if (p->data.function.arg_count >= 1) {
                Expr* inner = p->data.function.args[0];
                if (inner->type == EXPR_FUNCTION && inner->data.function.head->type == EXPR_SYMBOL &&
                    strcmp(inner->data.function.head->data.symbol, "Optional") == 0) {
                    is_optional = true;
                    is_shortest = true;
                    opt_container = inner;
                    if (inner->data.function.arg_count >= 1) {
                        opt_pat = inner->data.function.args[0];
                    }
                } else {
                    opt_pat = inner;
                }
            }
        } else if (strcmp(head, "Longest") == 0) {
            is_longest = true;
            if (p->data.function.arg_count >= 1) {
                Expr* inner = p->data.function.args[0];
                if (inner->type == EXPR_FUNCTION && inner->data.function.head->type == EXPR_SYMBOL &&
                    strcmp(inner->data.function.head->data.symbol, "Optional") == 0) {
                    is_optional = true;
                    is_shortest = false; // Optional defaults to Longest, Longest wrapper enforces it
                    opt_container = inner;
                    if (inner->data.function.arg_count >= 1) {
                        opt_pat = inner->data.function.args[0];
                    }
                } else {
                    opt_pat = inner;
                }
            }
        } else if (strcmp(head, "Optional") == 0) {
            is_optional = true;
            if (p->data.function.arg_count >= 1) {
                opt_pat = p->data.function.args[0];
            }
        }
    }

    // Helper logic to test "without consuming" (i.e. default value)
    // We will inline it below in two places depending on is_shortest.

    if (is_optional && is_shortest) {
        size_t saved_env_count = env->count;
        Expr* def_val = NULL;
        if (opt_container->data.function.arg_count == 2) {
            def_val = expr_copy(opt_container->data.function.args[1]);
        } else {
            def_val = get_default_value(pat_head, total_pats - n_pats + 1, total_pats);
        }

        if (def_val) {
            Expr* inner_p = opt_pat;
            Expr* p_sym = NULL;
            if (is_pattern(opt_pat, &p_sym, &inner_p)) {
                if (p_sym && p_sym->type == EXPR_SYMBOL) {
                    env_set(env, p_sym->data.symbol, def_val);
                }
            }
            expr_free(def_val);

            if (match_args(exprs, n_exprs, pats + 1, n_pats - 1, env, condition, pat_head, total_pats)) {
                return true;
            }
            env_rollback(env, saved_env_count);
        }
    }

    // Attempt to match by consuming exprs
    // size_t saved_env_count = env->count;
    Expr* inner_p = opt_pat;
    Expr* p_sym = NULL;
    int min_len = 0;
    int max_len = -1;
    Expr* b_head = NULL;
    Expr* rep_pat = NULL;

    bool is_seq = false;
    bool is_rep = false;
    if (is_pattern(opt_pat, &p_sym, &inner_p)) {
        is_seq = is_sequence_blank(inner_p, &b_head, &min_len);
        if (!is_seq) is_rep = is_repeated(inner_p, &rep_pat, &min_len, &max_len);
    } else {
        is_seq = is_sequence_blank(opt_pat, &b_head, &min_len);
        if (!is_seq) is_rep = is_repeated(opt_pat, &rep_pat, &min_len, &max_len);
    }

    bool is_orderless = false;
    bool is_flat = false;
    if (pat_head && pat_head->type == EXPR_SYMBOL) {
        SymbolDef* def = symtab_get_def(pat_head->data.symbol);
        if (def) {
            if (def->attributes & ATTR_FLAT) is_flat = true;
            if (def->attributes & ATTR_ORDERLESS) is_orderless = true;
        }
    }

    size_t min_k, max_k;
    if (is_seq) {
        min_k = min_len;
        max_k = n_exprs;
    } else if (is_rep) {
        min_k = min_len;
        max_k = (max_len == -1 || max_len > (int)n_exprs) ? n_exprs : (size_t)max_len;
    } else if (is_flat) {
        min_k = 1;
        max_k = n_exprs;
    } else {
        min_k = 1;
        max_k = (n_exprs > 0) ? 1 : 0;
    }

    if (min_k <= max_k) {
        for (size_t step = 0; step <= max_k - min_k; step++) {
            size_t k = is_longest ? (max_k - step) : (min_k + step);
            if (k > n_exprs) continue;
            
            int* comb = NULL;
            if (k > 0) {
                comb = malloc(k * sizeof(int));
                for (int i = 0; i < (int)k; i++) comb[i] = i;
            }

        do {
            Expr** subset = NULL;
            Expr** remainder = NULL;
            if (k > 0) subset = malloc(k * sizeof(Expr*));
            if (n_exprs - k > 0) remainder = malloc((n_exprs - k) * sizeof(Expr*));
            
            extract_subset(exprs, n_exprs, comb, (int)k, subset, remainder);

            bool matched_this_combo = false;
            size_t saved_env = env->count;

            if (is_seq || is_rep) {
                bool type_ok = true;
                if (is_seq) {
                    if (b_head) {
                        for (size_t i = 0; i < k; i++) {
                            Expr* h = get_expr_head_borrowed(subset[i]);
                            bool ok = false;
                            if (h) {
                                ok = expr_eq(h, b_head);
                            } else if (b_head->type == EXPR_SYMBOL) {
                                const char* hn = b_head->data.symbol;
                                if (subset[i]->type == EXPR_INTEGER && strcmp(hn, "Integer") == 0) ok = true;
                                else if (subset[i]->type == EXPR_REAL && strcmp(hn, "Real") == 0) ok = true;
                                else if (subset[i]->type == EXPR_SYMBOL && strcmp(hn, "Symbol") == 0) ok = true;
                                else if (subset[i]->type == EXPR_STRING && strcmp(hn, "String") == 0) ok = true;
                            }
                            if (!ok) {
                                type_ok = false;
                                break;
                            }
                        }
                    }
                } else if (is_rep) {
                    for (size_t i = 0; i < k; i++) {
                        if (!match(subset[i], rep_pat, env)) {
                            type_ok = false;
                            break;
                        }
                    }
                }

                if (type_ok) {
                    if (p_sym) {
                        Expr* seq_val = expr_new_function(expr_new_symbol("Sequence"), NULL, k);
                        for (size_t i = 0; i < k; i++) seq_val->data.function.args[i] = expr_copy(subset[i]);
                        
                        Expr* existing = env_get(env, p_sym->data.symbol);
                        if (existing) {
                            if (expr_eq(seq_val, existing)) {
                                matched_this_combo = true;
                            }
                            expr_free(seq_val);
                        } else {
                            env_set(env, p_sym->data.symbol, seq_val);
                            expr_free(seq_val);
                            matched_this_combo = true;
                        }
                    } else {
                        matched_this_combo = true;
                    }
                }

                if (matched_this_combo) {
                    if (match_args(remainder, n_exprs - k, pats + 1, n_pats - 1, env, condition, pat_head, total_pats)) {
                        if (subset) free(subset);
                        if (remainder) free(remainder);
                        if (comb) free(comb);
                        return true;
                    }
                }
            } else {
                Expr* matched_val = NULL;
                if (k == 1) {
                    matched_val = expr_copy(subset[0]);
                } else if (k > 1) {
                    matched_val = expr_new_function(expr_copy(pat_head), NULL, k);
                    for (size_t i = 0; i < k; i++) {
                        matched_val->data.function.args[i] = expr_copy(subset[i]);
                    }
                }

                if (matched_val && match(matched_val, opt_pat, env)) {
                    if (match_args(remainder, n_exprs - k, pats + 1, n_pats - 1, env, condition, pat_head, total_pats)) {
                        expr_free(matched_val);
                        if (subset) free(subset);
                        if (remainder) free(remainder);
                        if (comb) free(comb);
                        return true;
                    }
                }
                if (matched_val) expr_free(matched_val);
            }

            env_rollback(env, saved_env);

            if (subset) free(subset);
            if (remainder) free(remainder);

        } while (is_orderless && next_combination(comb, n_exprs, (int)k));
        
        if (comb) free(comb);
        }
    }

    if (is_optional && !is_shortest) {
        size_t saved_env_count = env->count;
        Expr* def_val = NULL;
        if (opt_container->data.function.arg_count == 2) {
            def_val = expr_copy(opt_container->data.function.args[1]);
        } else {
            def_val = get_default_value(pat_head, total_pats - n_pats + 1, total_pats);
        }

        if (def_val) {
            Expr* inner_p = opt_pat;
            Expr* p_sym = NULL;
            if (is_pattern(opt_pat, &p_sym, &inner_p)) {
                if (p_sym && p_sym->type == EXPR_SYMBOL) {
                    env_set(env, p_sym->data.symbol, def_val);
                }
            }
            expr_free(def_val);

            if (match_args(exprs, n_exprs, pats + 1, n_pats - 1, env, condition, pat_head, total_pats)) {
                return true;
            }
            env_rollback(env, saved_env_count);
        }
    }

    return false;
}

Expr* replace_bindings(Expr* expr, MatchEnv* env) {
    if (!expr) return NULL;
    
    if (expr->type == EXPR_SYMBOL) {
        Expr* bound = env_get(env, expr->data.symbol);
        if (bound) {
            return expr_copy(bound);
        }
    }
    
    if (expr->type == EXPR_FUNCTION) {
        Expr* new_head = replace_bindings(expr->data.function.head, env);
        
        bool skip_flattening = false;
        if (new_head->type == EXPR_SYMBOL && (
            strcmp(new_head->data.symbol, "Set") == 0 ||
            strcmp(new_head->data.symbol, "SetDelayed") == 0 ||
            strcmp(new_head->data.symbol, "Rule") == 0 ||
            strcmp(new_head->data.symbol, "RuleDelayed") == 0)) {
            skip_flattening = true;
        }

        // Temporarily store replaced args to check for Sequence expansion
        Expr** temp_args = malloc(sizeof(Expr*) * expr->data.function.arg_count);
        size_t new_count = 0;
        for (size_t i = 0; i < expr->data.function.arg_count; i++) {
            temp_args[i] = replace_bindings(expr->data.function.args[i], env);
            if (!skip_flattening && temp_args[i]->type == EXPR_FUNCTION && 
                temp_args[i]->data.function.head->type == EXPR_SYMBOL &&
                strcmp(temp_args[i]->data.function.head->data.symbol, "Sequence") == 0) {
                new_count += temp_args[i]->data.function.arg_count;
            } else {
                new_count++;
            }
        }
        
        Expr** final_args = malloc(sizeof(Expr*) * new_count);
        size_t idx = 0;
        for (size_t i = 0; i < expr->data.function.arg_count; i++) {
            if (!skip_flattening && temp_args[i]->type == EXPR_FUNCTION && 
                temp_args[i]->data.function.head->type == EXPR_SYMBOL &&
                strcmp(temp_args[i]->data.function.head->data.symbol, "Sequence") == 0) {
                for (size_t j = 0; j < temp_args[i]->data.function.arg_count; j++) {
                    final_args[idx++] = expr_copy(temp_args[i]->data.function.args[j]);
                }
                expr_free(temp_args[i]);
            } else {
                final_args[idx++] = temp_args[i];
            }
        }
        free(temp_args);
        
        Expr* result = expr_new_function(new_head, final_args, new_count);
        free(final_args);
        return result;
    }
    
    return expr_copy(expr);
}

Expr* builtin_matchq(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 2) return NULL;
    
    Expr* expr = evaluate(res->data.function.args[0]);
    Expr* pattern = res->data.function.args[1];
    
    MatchEnv* env = env_new();
    bool result = match(expr, pattern, env);
    env_free(env);
    expr_free(expr);
    
    return expr_new_symbol(result ? "True" : "False");
}
