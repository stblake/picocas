#include "modular.h"
#include "symtab.h"
#include "eval.h"
#include "attr.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "arithmetic.h"

static int64_t ext_gcd(int64_t a, int64_t b, int64_t *x, int64_t *y) {
    if (a == 0) {
        *x = 0; *y = 1;
        return b;
    }
    int64_t x1, y1;
    int64_t d = ext_gcd(b % a, a, &x1, &y1);
    *x = y1 - (b / a) * x1;
    *y = x1;
    return d;
}

static int64_t mod_inverse(int64_t a, int64_t m) {
    if (m == 0) return -1;
    if (m < 0) m = -m;
    if (m == 1) return 0;
    a %= m;
    if (a < 0) a += m;
    int64_t x, y;
    int64_t g = ext_gcd(a, m, &x, &y);
    if (g != 1) return -1;
    return (x % m + m) % m;
}

static int64_t mod_pow(int64_t a, int64_t b, int64_t m) {
    if (m == 0) return 0;
    if (m < 0) m = -m;
    if (m == 1) return 0;
    a %= m;
    if (a < 0) a += m;
    int64_t res = 1;
    while (b > 0) {
        if (b & 1) res = (int64_t)((__int128_t)res * a % m);
        a = (int64_t)((__int128_t)a * a % m);
        b >>= 1;
    }
    return res;
}

Expr* builtin_powermod(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 3) return NULL;
    
    Expr* a_expr = res->data.function.args[0];
    Expr* b_expr = res->data.function.args[1];
    Expr* m_expr = res->data.function.args[2];
    
    if (a_expr->type != EXPR_INTEGER || m_expr->type != EXPR_INTEGER) return NULL;
    int64_t a = a_expr->data.integer;
    int64_t m = m_expr->data.integer;
    if (m == 0) return NULL;
    
    if (b_expr->type == EXPR_INTEGER) {
        int64_t b = b_expr->data.integer;
        if (b < 0) {
            int64_t inv = mod_inverse(a, m);
            if (inv == -1) return expr_copy(res); // Unevaluated if no inverse
            return expr_new_integer(mod_pow(inv, -b, m));
        } else {
            return expr_new_integer(mod_pow(a, b, m));
        }
    } else if (b_expr->type == EXPR_FUNCTION && b_expr->data.function.head->type == EXPR_SYMBOL && strcmp(b_expr->data.function.head->data.symbol, "Rational") == 0 && b_expr->data.function.arg_count == 2) {
        Expr* num = b_expr->data.function.args[0];
        Expr* den = b_expr->data.function.args[1];
        if (num->type == EXPR_INTEGER && num->data.integer == 1 && den->type == EXPR_INTEGER) {
            int64_t r = den->data.integer;
            if (r > 0) {
                int64_t target_a = a % m;
                if (target_a < 0) target_a += m;
                int64_t abs_m = m < 0 ? -m : m;
                
                // Brute force root finding (only up to a reasonable cap to avoid hanging)
                if (abs_m <= 1000000LL) {
                    for (int64_t x = 0; x < abs_m; x++) {
                        if (mod_pow(x, r, abs_m) == target_a) {
                            return expr_new_integer(x);
                        }
                    }
                    return expr_copy(res); // Unevaluated if no root
                }
            }
        }
    }
    
    return NULL;
}

static int64_t module_number = 1;

void modular_init(void) {
    symtab_add_builtin("Module", builtin_module);
    symtab_add_builtin("Block", builtin_block);
    symtab_add_builtin("With", builtin_with);
    symtab_add_builtin("PowerMod", builtin_powermod);
    symtab_get_def("PowerMod")->attributes |= ATTR_LISTABLE | ATTR_PROTECTED;

    // Initial value for $ModuleNumber
    Expr* mn = expr_new_integer(module_number);
    Expr* sym_mn = expr_new_symbol("$ModuleNumber");
    symtab_add_own_value("$ModuleNumber", sym_mn, mn);
    expr_free(mn);
    expr_free(sym_mn);
}

typedef struct ScopingEnv {
    const char* old_name;
    Expr* replacement;
    struct ScopingEnv* next;
} ScopingEnv;

static bool is_scoping_construct(Expr* e) {
    if (e->type != EXPR_FUNCTION || e->data.function.head->type != EXPR_SYMBOL) return false;
    const char* h = e->data.function.head->data.symbol;
    return strcmp(h, "Module") == 0 || strcmp(h, "Block") == 0 || strcmp(h, "With") == 0 || 
           strcmp(h, "Function") == 0 || strcmp(h, "Table") == 0;
}

static Expr* substitute_scoping(Expr* e, ScopingEnv* env) {
    if (!e) return NULL;
    if (e->type == EXPR_SYMBOL) {
        ScopingEnv* curr = env;
        while (curr) {
            if (strcmp(e->data.symbol, curr->old_name) == 0) {
                return expr_copy(curr->replacement);
            }
            curr = curr->next;
        }
        return expr_copy(e);
    }
    if (e->type != EXPR_FUNCTION) return expr_copy(e);

    // Handle shadowing in scoping constructs
    ScopingEnv* filtered_env = env;
    
    if (is_scoping_construct(e) && e->data.function.arg_count >= 1) {
        Expr* vars = e->data.function.args[0];
        if (vars->type == EXPR_FUNCTION && strcmp(vars->data.function.head->data.symbol, "List") == 0) {
            // Create a filtered env that doesn't contain variables redefined here
            for (size_t i = 0; i < vars->data.function.arg_count; i++) {
                Expr* v = vars->data.function.args[i];
                const char* shadowed_name = NULL;
                if (v->type == EXPR_SYMBOL) shadowed_name = v->data.symbol;
                else if (v->type == EXPR_FUNCTION && strcmp(v->data.function.head->data.symbol, "Set") == 0 && v->data.function.arg_count == 2) {
                    if (v->data.function.args[0]->type == EXPR_SYMBOL) shadowed_name = v->data.function.args[0]->data.symbol;
                }

                if (shadowed_name) {
                    // This variable is shadowed, so we remove it from the env we pass down
                    // For simplicity, we just rebuild the env list skipping shadowed names
                    ScopingEnv* new_env = NULL;
                    ScopingEnv* curr = filtered_env;
                    while (curr) {
                        if (strcmp(curr->old_name, shadowed_name) != 0) {
                            ScopingEnv* node = malloc(sizeof(ScopingEnv));
                            node->old_name = curr->old_name;
                            node->replacement = curr->replacement;
                            node->next = new_env;
                            new_env = node;
                        }
                        curr = curr->next;
                    }
                    if (filtered_env != env) {
                        // Free the intermediate filtered env
                        ScopingEnv* tmp = filtered_env;
                        while (tmp) {
                            ScopingEnv* next = tmp->next;
                            free(tmp);
                            tmp = next;
                        }
                    }
                    filtered_env = new_env;
                }
            }
        }
    }
    
    Expr** new_args = malloc(sizeof(Expr*) * e->data.function.arg_count);
    for (size_t i = 0; i < e->data.function.arg_count; i++) {
        // First argument of scoping constructs is the variable list, don't substitute there
        if (i == 0 && is_scoping_construct(e)) {
            new_args[i] = expr_copy(e->data.function.args[i]);
        } else {
            new_args[i] = substitute_scoping(e->data.function.args[i], filtered_env);
        }
    }
    Expr* new_head = substitute_scoping(e->data.function.head, filtered_env);
    Expr* res = expr_new_function(new_head, new_args, e->data.function.arg_count);
    free(new_args);

    if (filtered_env != env) {
        ScopingEnv* tmp = filtered_env;
        while (tmp) {
            ScopingEnv* next = tmp->next;
            free(tmp);
            tmp = next;
        }
    }

    return res;
}

Expr* builtin_module(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 2) return NULL;
    Expr* vars = res->data.function.args[0];
    Expr* body = res->data.function.args[1];

    if (vars->type != EXPR_FUNCTION || strcmp(vars->data.function.head->data.symbol, "List") != 0) return NULL;

    size_t var_count = vars->data.function.arg_count;
    ScopingEnv* env = NULL;

    // Increment $ModuleNumber
    Expr* mn_sym = expr_new_symbol("$ModuleNumber");
    Expr* mn_val_expr = evaluate(mn_sym);
    if (mn_val_expr->type == EXPR_INTEGER) {
        module_number = mn_val_expr->data.integer;
    }
    expr_free(mn_val_expr);
    int64_t current_mn = module_number++;
    Expr* next_mn = expr_new_integer(module_number);
    symtab_add_own_value("$ModuleNumber", mn_sym, next_mn);
    expr_free(mn_sym);
    expr_free(next_mn);

    typedef struct {
        char* new_name;
        Expr* init_val;
    } VarInfo;
    VarInfo* info = calloc(var_count, sizeof(VarInfo));

    for (size_t i = 0; i < var_count; i++) {
        Expr* v = vars->data.function.args[i];
        const char* orig_name = NULL;
        Expr* init_val = NULL;

        if (v->type == EXPR_SYMBOL) {
            orig_name = v->data.symbol;
        } else if (v->type == EXPR_FUNCTION && strcmp(v->data.function.head->data.symbol, "Set") == 0 && v->data.function.arg_count == 2) {
            if (v->data.function.args[0]->type == EXPR_SYMBOL) {
                orig_name = v->data.function.args[0]->data.symbol;
                init_val = evaluate(v->data.function.args[1]);
            }
        }

        if (orig_name) {
            char buf[256];
            snprintf(buf, sizeof(buf), "%s$%lld", orig_name, (long long)current_mn);
            info[i].new_name = strdup(buf);
            info[i].init_val = init_val;

            ScopingEnv* new_node = malloc(sizeof(ScopingEnv));
            new_node->old_name = orig_name;
            new_node->replacement = expr_new_symbol(buf);
            new_node->next = env;
            env = new_node;

            // Register temporary symbol
            symtab_get_def(buf)->attributes |= ATTR_TEMPORARY;
            if (init_val) {
                symtab_add_own_value(buf, new_node->replacement, init_val);
            }
        }
    }

    Expr* substituted_body = substitute_scoping(body, env);
    Expr* final_res = evaluate(substituted_body);
    expr_free(substituted_body);

    // Cleanup env and info
    while (env) {
        ScopingEnv* next = env->next;
        expr_free(env->replacement);
        free(env);
        env = next;
    }
    for (size_t i = 0; i < var_count; i++) {
        if (info[i].new_name) free(info[i].new_name);
        if (info[i].init_val) expr_free(info[i].init_val);
    }
    free(info);

    return final_res;
}

Expr* builtin_block(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 2) return NULL;
    Expr* vars = res->data.function.args[0];
    Expr* body = res->data.function.args[1];

    if (vars->type != EXPR_FUNCTION || strcmp(vars->data.function.head->data.symbol, "List") != 0) return NULL;

    size_t var_count = vars->data.function.arg_count;
    
    typedef struct {
        char* name;
        Rule* old_own;
        uint32_t old_attrs;
    } SavedVar;
    SavedVar* saved = calloc(var_count, sizeof(SavedVar));

    for (size_t i = 0; i < var_count; i++) {
        Expr* v = vars->data.function.args[i];
        const char* name = NULL;
        Expr* init_val = NULL;

        if (v->type == EXPR_SYMBOL) {
            name = v->data.symbol;
        } else if (v->type == EXPR_FUNCTION && strcmp(v->data.function.head->data.symbol, "Set") == 0 && v->data.function.arg_count == 2) {
            if (v->data.function.args[0]->type == EXPR_SYMBOL) {
                name = v->data.function.args[0]->data.symbol;
                init_val = evaluate(v->data.function.args[1]);
            }
        }

        if (name) {
            SymbolDef* def = symtab_get_def(name);
            saved[i].name = strdup(name);
            saved[i].old_own = def->own_values;
            saved[i].old_attrs = def->attributes;
            def->own_values = NULL; // Clear values
            
            if (init_val) {
                symtab_add_own_value(name, (v->type == EXPR_SYMBOL ? v : v->data.function.args[0]), init_val);
                expr_free(init_val);
            }
        }
    }

    Expr* final_res = evaluate(body);

    // Restore
    for (size_t i = 0; i < var_count; i++) {
        if (saved[i].name) {
            SymbolDef* def = symtab_get_def(saved[i].name);
            // Free temporary values assigned in block
            Rule* curr = def->own_values;
            while (curr) {
                Rule* next = curr->next;
                expr_free(curr->pattern);
                expr_free(curr->replacement);
                free(curr);
                curr = next;
            }
            def->own_values = saved[i].old_own;
            def->attributes = saved[i].old_attrs;
            free(saved[i].name);
        }
    }
    free(saved);

    return final_res;
}

Expr* builtin_with(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 2) return NULL;
    Expr* vars = res->data.function.args[0];
    Expr* body = res->data.function.args[1];

    if (vars->type != EXPR_FUNCTION || strcmp(vars->data.function.head->data.symbol, "List") != 0) return NULL;

    size_t var_count = vars->data.function.arg_count;
    ScopingEnv* env = NULL;

    for (size_t i = 0; i < var_count; i++) {
        Expr* v = vars->data.function.args[i];
        const char* name = NULL;
        Expr* val = NULL;

        if (v->type == EXPR_FUNCTION && v->data.function.arg_count == 2) {
            const char* h = v->data.function.head->data.symbol;
            if (strcmp(h, "Set") == 0 || strcmp(h, "SetDelayed") == 0) {
                if (v->data.function.args[0]->type == EXPR_SYMBOL) {
                    name = v->data.function.args[0]->data.symbol;
                    if (strcmp(h, "Set") == 0) {
                        val = evaluate(v->data.function.args[1]);
                    } else {
                        val = expr_copy(v->data.function.args[1]);
                    }
                }
            }
        }

        if (name && val) {
            ScopingEnv* new_node = malloc(sizeof(ScopingEnv));
            new_node->old_name = name;
            new_node->replacement = val;
            new_node->next = env;
            env = new_node;
        }
    }

    Expr* substituted_body = substitute_scoping(body, env);
    Expr* final_res = evaluate(substituted_body);
    expr_free(substituted_body);

    while (env) {
        ScopingEnv* next = env->next;
        expr_free(env->replacement);
        free(env);
        env = next;
    }

    return final_res;
}
