#include "replace.h"
#include "symtab.h"
#include "eval.h"
#include "match.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    Expr* pos_spec;
    Expr* replacement;
    bool delayed;
} RawRule;

static void add_raw_rule(RawRule** rules, size_t* cap, size_t* count, Expr* path, Expr* replacement, bool delayed) {
    if (*count >= *cap) {
        *cap = (*cap == 0) ? 4 : (*cap * 2);
        *rules = realloc(*rules, sizeof(RawRule) * (*cap));
    }
    (*rules)[*count].pos_spec = expr_copy(path);
    (*rules)[*count].replacement = expr_copy(replacement);
    (*rules)[*count].delayed = delayed;
    (*count)++;
}

static void parse_single_rule(Expr* rule_expr, RawRule** rules, size_t* cap, size_t* count) {
    if (rule_expr->type != EXPR_FUNCTION) return;
    bool delayed = false;
    if (strcmp(rule_expr->data.function.head->data.symbol, "Rule") == 0) delayed = false;
    else if (strcmp(rule_expr->data.function.head->data.symbol, "RuleDelayed") == 0) delayed = true;
    else return;

    if (rule_expr->data.function.arg_count != 2) return;
    Expr* lhs = rule_expr->data.function.args[0];
    Expr* rhs = rule_expr->data.function.args[1];

    if (lhs->type == EXPR_FUNCTION && strcmp(lhs->data.function.head->data.symbol, "List") == 0) {
        if (lhs->data.function.arg_count > 0 && 
            lhs->data.function.args[0]->type == EXPR_FUNCTION &&
            strcmp(lhs->data.function.args[0]->data.function.head->data.symbol, "List") == 0) {
            for (size_t i = 0; i < lhs->data.function.arg_count; i++) {
                add_raw_rule(rules, cap, count, lhs->data.function.args[i], rhs, delayed);
            }
        } else {
            add_raw_rule(rules, cap, count, lhs, rhs, delayed);
        }
    } else {
        Expr* lhs_copy = expr_copy(lhs);
        Expr* wrapped = expr_new_function(expr_new_symbol("List"), &lhs_copy, 1);
        add_raw_rule(rules, cap, count, wrapped, rhs, delayed);
        expr_free(wrapped);
    }
}

typedef struct {
    int64_t pos_index;
    int64_t neg_index;
} PathElement;

static bool check_match(RawRule* rules, size_t num_rules, PathElement* path, size_t depth, 
                        Expr** out_replacement, bool* out_delayed, MatchEnv** out_env) {
    size_t num_aliases = 1 << depth;
    for (int64_t r = (int64_t)num_rules - 1; r >= 0; r--) {
        for (size_t i = 0; i < num_aliases; i++) {
            Expr** alias_args = NULL;
            if (depth > 0) alias_args = malloc(sizeof(Expr*) * depth);
            for (size_t d = 0; d < depth; d++) {
                if (path[d].pos_index == 0) {
                    alias_args[d] = expr_new_integer(0);
                } else {
                    if ((i >> d) & 1) {
                        alias_args[d] = expr_new_integer(path[d].neg_index);
                    } else {
                        alias_args[d] = expr_new_integer(path[d].pos_index);
                    }
                }
            }
            Expr* alias_expr = expr_new_function(expr_new_symbol("List"), alias_args, depth);
            
            MatchEnv* env = env_new();
            if (match(alias_expr, rules[r].pos_spec, env)) {
                *out_replacement = rules[r].replacement;
                *out_delayed = rules[r].delayed;
                *out_env = env;
                expr_free(alias_expr);
                if (alias_args) free(alias_args);
                return true;
            }
            env_free(env);
            expr_free(alias_expr);
            if (alias_args) free(alias_args);
        }
    }
    return false;
}

static Expr* replace_part_rec(Expr* expr, RawRule* rules, size_t num_rules, PathElement* current_path, size_t depth) {
    Expr* replacement_expr = NULL;
    bool delayed = false;
    MatchEnv* env = NULL;

    if (check_match(rules, num_rules, current_path, depth, &replacement_expr, &delayed, &env)) {
        Expr* res = replace_bindings(replacement_expr, env);
        if (delayed) {
            Expr* eval_res = evaluate(res);
            expr_free(res);
            res = eval_res;
        }
        env_free(env);
        return res;
    }

    if (expr->type == EXPR_FUNCTION) {
        size_t len = expr->data.function.arg_count;
        Expr** new_args = NULL;
        if (len > 0) new_args = malloc(sizeof(Expr*) * len);
        
        PathElement* next_path = malloc(sizeof(PathElement) * (depth + 1));
        if (depth > 0) memcpy(next_path, current_path, sizeof(PathElement) * depth);
        
        // Check head (index 0)
        next_path[depth].pos_index = 0;
        next_path[depth].neg_index = 0;
        Expr* new_head = replace_part_rec(expr->data.function.head, rules, num_rules, next_path, depth + 1);

        for (size_t i = 0; i < len; i++) {
            next_path[depth].pos_index = i + 1;
            next_path[depth].neg_index = (int64_t)(i + 1) - (int64_t)len - 1;
            new_args[i] = replace_part_rec(expr->data.function.args[i], rules, num_rules, next_path, depth + 1);
        }
        
        free(next_path);
        
        Expr* result = expr_new_function(new_head, new_args, len);
        if (new_args) free(new_args);
        return result;
    }
    
    return expr_copy(expr);
}

Expr* builtin_replace_part(Expr* res) {
    if (res->type != EXPR_FUNCTION) return NULL;
    if (res->data.function.arg_count == 1) {
        // Operator form ReplacePart[rules]
        return NULL; // Stays unevaluated
    } else if (res->data.function.arg_count == 2) {
        Expr* expr = res->data.function.args[0];
        Expr* rules_expr = res->data.function.args[1];

        size_t cap = 4;
        size_t num_rules = 0;
        RawRule* rules = malloc(sizeof(RawRule) * cap);

        if (rules_expr->type == EXPR_FUNCTION && strcmp(rules_expr->data.function.head->data.symbol, "List") == 0) {
            for (size_t i = 0; i < rules_expr->data.function.arg_count; i++) {
                parse_single_rule(rules_expr->data.function.args[i], &rules, &cap, &num_rules);
            }
        } else {
            parse_single_rule(rules_expr, &rules, &cap, &num_rules);
        }

        if (num_rules == 0) {
            free(rules);
            return expr_copy(expr);
        }

        Expr* result = replace_part_rec(expr, rules, num_rules, NULL, 0);

        for (size_t i = 0; i < num_rules; i++) {
            expr_free(rules[i].pos_spec);
            expr_free(rules[i].replacement);
        }
        free(rules);

        return result;
    }
    return NULL;
}

typedef struct {
    Expr* pattern;
    Expr* replacement;
    bool delayed;
} ReplaceRule;

static int64_t get_expr_depth_replace(Expr* e, bool heads) {
    if (e->type != EXPR_FUNCTION) return 1;
    if (e->data.function.head->type == EXPR_SYMBOL) {
        const char* h = e->data.function.head->data.symbol;
        if (strcmp(h, "Rational") == 0 || strcmp(h, "Complex") == 0) return 1;
    }
    int64_t max_d = 0;
    for (size_t i = 0; i < e->data.function.arg_count; i++) {
        int64_t d = get_expr_depth_replace(e->data.function.args[i], heads);
        if (d > max_d) max_d = d;
    }
    if (heads) {
        int64_t d_head = get_expr_depth_replace(e->data.function.head, heads);
        if (d_head > max_d) max_d = d_head;
    }
    return max_d + 1;
}

static Expr* do_replace_at_level(Expr* e, int64_t current_level, int64_t min_l, int64_t max_l, bool heads, ReplaceRule* rules, size_t num_rules) {
    if (max_l >= 0 && current_level > max_l) return expr_copy(e);

    int64_t d = get_expr_depth_replace(e, heads);

    bool match_level = true;
    if (min_l >= 0) {
        if (current_level < min_l) match_level = false;
    } else {
        if (d > -min_l) match_level = false;
    }

    if (max_l >= 0) {
        if (current_level > max_l) match_level = false;
    } else {
        if (d < -max_l) match_level = false;
    }

    Expr* new_e = NULL;
    if (e->type == EXPR_FUNCTION) {
        Expr* new_head = e->data.function.head;
        if (heads) {
            new_head = do_replace_at_level(e->data.function.head, current_level + 1, min_l, max_l, heads, rules, num_rules);
        } else {
            new_head = expr_copy(new_head);
        }

        size_t count = e->data.function.arg_count;
        Expr** new_args = NULL;
        if (count > 0) new_args = malloc(sizeof(Expr*) * count);
        for (size_t i = 0; i < count; i++) {
            new_args[i] = do_replace_at_level(e->data.function.args[i], current_level + 1, min_l, max_l, heads, rules, num_rules);
        }

        new_e = expr_new_function(new_head, new_args, count);
        if (new_args) free(new_args);
    } else {
        new_e = expr_copy(e);
    }

    if (match_level) {
        for (size_t i = 0; i < num_rules; i++) {
            MatchEnv* env = env_new();
            if (match(new_e, rules[i].pattern, env)) {
                Expr* repl = replace_bindings(rules[i].replacement, env);
                env_free(env);
                expr_free(new_e);
                return repl;
            }
            env_free(env);
        }
    }

    return new_e;
}

static bool is_rule(Expr* e) {
    if (e->type != EXPR_FUNCTION) return false;
    if (e->data.function.head->type != EXPR_SYMBOL) return false;
    const char* h = e->data.function.head->data.symbol;
    return (strcmp(h, "Rule") == 0 || strcmp(h, "RuleDelayed") == 0);
}

static Expr* apply_replace_nested(Expr* expr, Expr* rules_expr, int64_t min_l, int64_t max_l, bool heads) {
    if (rules_expr->type == EXPR_FUNCTION && strcmp(rules_expr->data.function.head->data.symbol, "List") == 0) {
        if (rules_expr->data.function.arg_count > 0 && !is_rule(rules_expr->data.function.args[0])) {
            size_t count = rules_expr->data.function.arg_count;
            Expr** args = malloc(sizeof(Expr*) * count);
            for (size_t i = 0; i < count; i++) {
                args[i] = apply_replace_nested(expr, rules_expr->data.function.args[i], min_l, max_l, heads);
            }
            Expr* res = expr_new_function(expr_new_symbol("List"), args, count);
            free(args);
            return res;
        }
    }
    
    size_t cap = 4;
    size_t num_rules = 0;
    ReplaceRule* rules = malloc(sizeof(ReplaceRule) * cap);
    
    if (rules_expr->type == EXPR_FUNCTION && strcmp(rules_expr->data.function.head->data.symbol, "List") == 0) {
        for (size_t i = 0; i < rules_expr->data.function.arg_count; i++) {
            Expr* r = rules_expr->data.function.args[i];
            if (is_rule(r) && r->data.function.arg_count == 2) {
                if (num_rules == cap) { cap *= 2; rules = realloc(rules, sizeof(ReplaceRule) * cap); }
                rules[num_rules].pattern = r->data.function.args[0];
                rules[num_rules].replacement = r->data.function.args[1];
                rules[num_rules].delayed = strcmp(r->data.function.head->data.symbol, "RuleDelayed") == 0;
                num_rules++;
            }
        }
    } else if (is_rule(rules_expr) && rules_expr->data.function.arg_count == 2) {
        rules[num_rules].pattern = rules_expr->data.function.args[0];
        rules[num_rules].replacement = rules_expr->data.function.args[1];
        rules[num_rules].delayed = strcmp(rules_expr->data.function.head->data.symbol, "RuleDelayed") == 0;
        num_rules++;
    }
    
    Expr* result;
    if (num_rules > 0) {
        result = do_replace_at_level(expr, 0, min_l, max_l, heads, rules, num_rules);
    } else {
        result = expr_copy(expr);
    }
    free(rules);
    return result;
}

Expr* builtin_replace(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count < 2) return NULL;
    Expr* expr = res->data.function.args[0];
    Expr* rules_expr = res->data.function.args[1];
    
    int64_t min_l = 0, max_l = 0;
    bool heads = false;
    
    if (res->data.function.arg_count >= 3) {
        Expr* ls = res->data.function.args[2];
        if (ls->type == EXPR_INTEGER) {
            min_l = 1; max_l = ls->data.integer;
        } else if (ls->type == EXPR_SYMBOL && strcmp(ls->data.symbol, "All") == 0) {
            min_l = 0; max_l = 1000000;
        } else if (ls->type == EXPR_SYMBOL && strcmp(ls->data.symbol, "Infinity") == 0) {
            min_l = 1; max_l = 1000000;
        } else if (ls->type == EXPR_FUNCTION && strcmp(ls->data.function.head->data.symbol, "List") == 0) {
            if (ls->data.function.arg_count == 1 && ls->data.function.args[0]->type == EXPR_INTEGER) {
                min_l = max_l = ls->data.function.args[0]->data.integer;
            } else if (ls->data.function.arg_count == 2) {
                if (ls->data.function.args[0]->type == EXPR_INTEGER) min_l = ls->data.function.args[0]->data.integer;
                if (ls->data.function.args[1]->type == EXPR_INTEGER) max_l = ls->data.function.args[1]->data.integer;
                else if (ls->data.function.args[1]->type == EXPR_SYMBOL && strcmp(ls->data.function.args[1]->data.symbol, "Infinity") == 0) max_l = 1000000;
            }
        }
    }
    
    for (size_t i = 2; i < res->data.function.arg_count; i++) {
        Expr* opt = res->data.function.args[i];
        if (opt->type == EXPR_FUNCTION && strcmp(opt->data.function.head->data.symbol, "Rule") == 0 && opt->data.function.arg_count == 2) {
            if (opt->data.function.args[0]->type == EXPR_SYMBOL && strcmp(opt->data.function.args[0]->data.symbol, "Heads") == 0) {
                if (opt->data.function.args[1]->type == EXPR_SYMBOL && strcmp(opt->data.function.args[1]->data.symbol, "True") == 0) heads = true;
                else if (opt->data.function.args[1]->type == EXPR_SYMBOL && strcmp(opt->data.function.args[1]->data.symbol, "False") == 0) heads = false;
            }
        }
    }
    
    return apply_replace_nested(expr, rules_expr, min_l, max_l, heads);
}

static Expr* do_replace_all(Expr* e, ReplaceRule* rules, size_t num_rules) {
    for (size_t i = 0; i < num_rules; i++) {
        MatchEnv* env = env_new();
        if (match(e, rules[i].pattern, env)) {
            Expr* repl = replace_bindings(rules[i].replacement, env);
            env_free(env);
            return repl;
        }
        env_free(env);
    }

    if (e->type == EXPR_FUNCTION) {
        Expr* new_head = do_replace_all(e->data.function.head, rules, num_rules);
        size_t count = e->data.function.arg_count;
        Expr** new_args = NULL;
        if (count > 0) new_args = malloc(sizeof(Expr*) * count);
        for (size_t i = 0; i < count; i++) {
            new_args[i] = do_replace_all(e->data.function.args[i], rules, num_rules);
        }
        Expr* new_e = expr_new_function(new_head, new_args, count);
        if (new_args) free(new_args);
        return new_e;
    }

    return expr_copy(e);
}

static Expr* apply_replace_all_nested(Expr* expr, Expr* rules_expr) {
    if (rules_expr->type == EXPR_FUNCTION && strcmp(rules_expr->data.function.head->data.symbol, "List") == 0) {
        if (rules_expr->data.function.arg_count > 0 && !is_rule(rules_expr->data.function.args[0])) {
            size_t count = rules_expr->data.function.arg_count;
            Expr** args = malloc(sizeof(Expr*) * count);
            for (size_t i = 0; i < count; i++) {
                args[i] = apply_replace_all_nested(expr, rules_expr->data.function.args[i]);
            }
            Expr* res = expr_new_function(expr_new_symbol("List"), args, count);
            free(args);
            return res;
        }
    }
    
    size_t cap = 4;
    size_t num_rules = 0;
    ReplaceRule* rules = malloc(sizeof(ReplaceRule) * cap);
    
    if (rules_expr->type == EXPR_FUNCTION && strcmp(rules_expr->data.function.head->data.symbol, "List") == 0) {
        for (size_t i = 0; i < rules_expr->data.function.arg_count; i++) {
            Expr* r = rules_expr->data.function.args[i];
            if (is_rule(r) && r->data.function.arg_count == 2) {
                if (num_rules == cap) { cap *= 2; rules = realloc(rules, sizeof(ReplaceRule) * cap); }
                rules[num_rules].pattern = r->data.function.args[0];
                rules[num_rules].replacement = r->data.function.args[1];
                rules[num_rules].delayed = strcmp(r->data.function.head->data.symbol, "RuleDelayed") == 0;
                num_rules++;
            }
        }
    } else if (is_rule(rules_expr) && rules_expr->data.function.arg_count == 2) {
        rules[num_rules].pattern = rules_expr->data.function.args[0];
        rules[num_rules].replacement = rules_expr->data.function.args[1];
        rules[num_rules].delayed = strcmp(rules_expr->data.function.head->data.symbol, "RuleDelayed") == 0;
        num_rules++;
    }
    
    Expr* result;
    if (num_rules > 0) {
        result = do_replace_all(expr, rules, num_rules);
    } else {
        result = expr_copy(expr);
    }
    free(rules);
    return result;
}

Expr* builtin_replace_all(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 2) return NULL;
    Expr* expr = res->data.function.args[0];
    Expr* rules_expr = res->data.function.args[1];
    
    return apply_replace_all_nested(expr, rules_expr);
}

static Expr* apply_replace_repeated_nested(Expr* expr, Expr* rules_expr) {
    if (rules_expr->type == EXPR_FUNCTION && strcmp(rules_expr->data.function.head->data.symbol, "List") == 0) {
        if (rules_expr->data.function.arg_count > 0 && !is_rule(rules_expr->data.function.args[0])) {
            size_t count = rules_expr->data.function.arg_count;
            Expr** args = malloc(sizeof(Expr*) * count);
            for (size_t i = 0; i < count; i++) {
                args[i] = apply_replace_repeated_nested(expr, rules_expr->data.function.args[i]);
            }
            Expr* res = expr_new_function(expr_new_symbol("List"), args, count);
            free(args);
            return res;
        }
    }

    Expr* current = expr_copy(expr);
    int iterations = 0;
    while (iterations < 65536) {
        Expr* next_raw = apply_replace_all_nested(current, rules_expr);
        Expr* next = eval_and_free(next_raw);
        if (expr_eq(current, next)) {
            expr_free(next);
            return current;
        }
        expr_free(current);
        current = next;
        iterations++;
    }
    
    return current;
}

Expr* builtin_replace_repeated(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 2) return NULL;
    Expr* expr = res->data.function.args[0];
    Expr* rules_expr = res->data.function.args[1];
    
    return apply_replace_repeated_nested(expr, rules_expr);
}


typedef struct {
    Expr* replacement;
    bool delayed;
    Expr** results;
    size_t count;
    size_t cap;
    int64_t limit;
} ReplaceListState;

static bool replacelist_callback(struct MatchEnv* env, void* user_data) {
    ReplaceListState* state = (ReplaceListState*)user_data;
    
    if (state->limit >= 0 && (int64_t)state->count >= state->limit) return true;
    
    Expr* repl = replace_bindings(state->replacement, env);
    if (state->delayed) {
        repl = eval_and_free(repl);
    }
    
    if (state->count == state->cap) {
        state->cap *= 2;
        state->results = realloc(state->results, sizeof(Expr*) * state->cap);
    }
    state->results[state->count++] = repl;
    
    return false;
}

Expr* builtin_replacelist(Expr* res) {
    if (res->type != EXPR_FUNCTION) return NULL;
    size_t argc = res->data.function.arg_count;
    if (argc < 2 || argc > 3) return NULL;

    Expr* expr = res->data.function.args[0];
    Expr* rules_expr = res->data.function.args[1];
    
    int64_t limit = -1;
    if (argc == 3) {
        Expr* l_expr = res->data.function.args[2];
        if (l_expr->type == EXPR_INTEGER) {
            limit = l_expr->data.integer;
        }
    }
    
    size_t cap = 16;
    size_t num_rules = 0;
    ReplaceRule* rules = malloc(sizeof(ReplaceRule) * cap);
    
    if (rules_expr->type == EXPR_FUNCTION && strcmp(rules_expr->data.function.head->data.symbol, "List") == 0) {
        for (size_t i = 0; i < rules_expr->data.function.arg_count; i++) {
            Expr* r = rules_expr->data.function.args[i];
            if (is_rule(r) && r->data.function.arg_count == 2) {
                if (num_rules == cap) { cap *= 2; rules = realloc(rules, sizeof(ReplaceRule) * cap); }
                rules[num_rules].pattern = r->data.function.args[0];
                rules[num_rules].replacement = r->data.function.args[1];
                rules[num_rules].delayed = strcmp(r->data.function.head->data.symbol, "RuleDelayed") == 0;
                num_rules++;
            }
        }
    } else if (is_rule(rules_expr) && rules_expr->data.function.arg_count == 2) {
        rules[num_rules].pattern = rules_expr->data.function.args[0];
        rules[num_rules].replacement = rules_expr->data.function.args[1];
        rules[num_rules].delayed = strcmp(rules_expr->data.function.head->data.symbol, "RuleDelayed") == 0;
        num_rules++;
    }
    
    ReplaceListState state;
    state.results = malloc(sizeof(Expr*) * 16);
    state.count = 0;
    state.cap = 16;
    state.limit = limit;
    
    for (size_t i = 0; i < num_rules; i++) {
        state.replacement = rules[i].replacement;
        state.delayed = rules[i].delayed;
        
        MatchEnv* env = env_new();
        env->callback = replacelist_callback;
        env->callback_data = &state;
        
        match(expr, rules[i].pattern, env);
        env_free(env);
        
        if (state.limit >= 0 && (int64_t)state.count >= state.limit) {
            break;
        }
    }
    
    free(rules);
    
    Expr* list = expr_new_function(expr_new_symbol("List"), state.results, state.count);
    free(state.results);
    return list;
}

void replace_init(void) {
    symtab_add_builtin("ReplacePart", builtin_replace_part);
    symtab_add_builtin("Replace", builtin_replace);
    symtab_get_def("Replace")->attributes |= ATTR_PROTECTED;
    symtab_add_builtin("ReplaceAll", builtin_replace_all);
    symtab_get_def("ReplaceAll")->attributes |= ATTR_PROTECTED;
    symtab_add_builtin("ReplaceRepeated", builtin_replace_repeated);
    symtab_get_def("ReplaceRepeated")->attributes |= ATTR_PROTECTED;
    symtab_add_builtin("ReplaceList", builtin_replacelist);
    symtab_get_def("ReplaceList")->attributes |= ATTR_PROTECTED;
}
