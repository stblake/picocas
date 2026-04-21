#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "symtab.h"
#include "match.h"
#include "print.h"
#include <string.h>
#include <stdlib.h>

#define SYMTAB_SIZE 65535

typedef struct SymEntry {
    SymbolDef* def;
    struct SymEntry* next;
} SymEntry;

static SymEntry* symtab[SYMTAB_SIZE] = {0};

static unsigned int hash(const char* s) {
    unsigned int h = 5381;
    while (*s) h = ((h << 5) + h) + *s++;
    return h % SYMTAB_SIZE;
}

void symtab_init(void) {
    memset(symtab, 0, sizeof(symtab));
}

SymbolDef* symtab_get_def(const char* symbol_name) {
    unsigned int idx = hash(symbol_name);
    SymEntry* entry = symtab[idx];
    while (entry) {
        if (strcmp(entry->def->symbol_name, symbol_name) == 0) {
            return entry->def;
        }
        entry = entry->next;
    }
    
    // Create new symbol definition
    SymbolDef* def = malloc(sizeof(SymbolDef));
    def->symbol_name = strdup(symbol_name);
    def->own_values = NULL;
    def->down_values = NULL;
    def->builtin_func = NULL;
    def->attributes = 0;
    def->docstring = NULL;
    
    SymEntry* new_entry = malloc(sizeof(SymEntry));
    new_entry->def = def;
    new_entry->next = symtab[idx];
    symtab[idx] = new_entry;
    
    return def;
}

SymbolDef* symtab_lookup(const char* symbol_name) {
    unsigned int idx = hash(symbol_name);
    SymEntry* entry = symtab[idx];
    while (entry) {
        if (strcmp(entry->def->symbol_name, symbol_name) == 0) {
            return entry->def;
        }
        entry = entry->next;
    }
    return NULL;
}

void symtab_add_builtin(const char* symbol_name, BuiltinFunc func) {
    SymbolDef* def = symtab_get_def(symbol_name);
    def->builtin_func = func;
}

void symtab_set_docstring(const char* symbol_name, const char* doc) {
    SymbolDef* def = symtab_get_def(symbol_name);
    if (def->docstring) free(def->docstring);
    def->docstring = doc ? strdup(doc) : NULL;
}

const char* symtab_get_docstring(const char* symbol_name) {
    SymbolDef* def = symtab_get_def(symbol_name);
    return def->docstring;
}

static bool has_patterns(Expr* e) {
    if (!e) return false;
    if (e->type == EXPR_SYMBOL) {
        return strcmp(e->data.symbol, "Blank") == 0 ||
               strcmp(e->data.symbol, "BlankSequence") == 0 ||
               strcmp(e->data.symbol, "BlankNullSequence") == 0;
    }
    if (e->type != EXPR_FUNCTION) return false;
    if (e->data.function.head->type == EXPR_SYMBOL) {
        const char* h = e->data.function.head->data.symbol;
        if (strcmp(h, "Pattern") == 0 || strcmp(h, "Blank") == 0 || 
            strcmp(h, "BlankSequence") == 0 || strcmp(h, "BlankNullSequence") == 0) return true;
    }
    for (size_t i = 0; i < e->data.function.arg_count; i++) {
        if (has_patterns(e->data.function.args[i])) return true;
    }
    return false;
}

// Helper to add a rule to a list
static void add_rule(Rule** list, Expr* pattern, Expr* replacement) {
    // Check if rule with same pattern already exists
    Rule* curr = *list;
    Rule* prev = NULL;
    while (curr) {
        if (expr_eq(curr->pattern, pattern)) {
            // Replace replacement
            expr_free(curr->replacement);
            curr->replacement = expr_copy(replacement);
            return;
        }
        prev = curr;
        curr = curr->next;
    }

    Rule* new_rule = malloc(sizeof(Rule));
    new_rule->pattern = expr_copy(pattern);
    new_rule->replacement = expr_copy(replacement);

    // Heuristic: specific rules (no patterns) go to the front
    if (!has_patterns(pattern)) {
        new_rule->next = *list;
        *list = new_rule;
    } else {
        new_rule->next = NULL;
        if (prev == NULL) {
            *list = new_rule;
        } else {
            prev->next = new_rule;
        }
    }
}
void symtab_add_own_value(const char* symbol_name, Expr* pattern, Expr* replacement) {
    SymbolDef* def = symtab_get_def(symbol_name);
    add_rule(&def->own_values, pattern, replacement);
}

void symtab_add_down_value(const char* symbol_name, Expr* pattern, Expr* replacement) {
    SymbolDef* def = symtab_get_def(symbol_name);
    add_rule(&def->down_values, pattern, replacement);
}

void symtab_clear_symbol(const char* symbol_name) {
    SymbolDef* def = symtab_get_def(symbol_name);
    Rule* curr = def->own_values;
    while (curr) {
        Rule* next = curr->next;
        expr_free(curr->pattern);
        expr_free(curr->replacement);
        free(curr);
        curr = next;
    }
    def->own_values = NULL;

    curr = def->down_values;
    while (curr) {
        Rule* next = curr->next;
        expr_free(curr->pattern);
        expr_free(curr->replacement);
        free(curr);
        curr = next;
    }
    def->down_values = NULL;
}

void symtab_clear(void) {
    for (int i = 0; i < SYMTAB_SIZE; i++) {
        SymEntry* entry = symtab[i];
        while (entry) {
            SymEntry* next = entry->next;
            
            // Inline symtab_clear_symbol logic without lookup
            Rule* curr = entry->def->own_values;
            while (curr) {
                Rule* r_next = curr->next;
                expr_free(curr->pattern);
                expr_free(curr->replacement);
                free(curr);
                curr = r_next;
            }
            entry->def->own_values = NULL;

            curr = entry->def->down_values;
            while (curr) {
                Rule* r_next = curr->next;
                expr_free(curr->pattern);
                expr_free(curr->replacement);
                free(curr);
                curr = r_next;
            }
            entry->def->down_values = NULL;

            if (entry->def->docstring) free(entry->def->docstring);
            free(entry->def->symbol_name);
            free(entry->def);
            free(entry);
            entry = next;
        }
        symtab[i] = NULL;
    }
}


Rule* symtab_get_own_values(const char* symbol_name) {
    unsigned int idx = hash(symbol_name);
    SymEntry* entry = symtab[idx];
    while (entry) {
        if (strcmp(entry->def->symbol_name, symbol_name) == 0) {
            return entry->def->own_values;
        }
        entry = entry->next;
    }
    return NULL;
}

Rule* symtab_get_down_values(const char* symbol_name) {
    unsigned int idx = hash(symbol_name);
    SymEntry* entry = symtab[idx];
    while (entry) {
        if (strcmp(entry->def->symbol_name, symbol_name) == 0) {
            return entry->def->down_values;
        }
        entry = entry->next;
    }
    return NULL;
}

Expr* apply_down_values(Expr* expr) {
    if (!expr || expr->type != EXPR_FUNCTION) return NULL;
    
    Expr* head = expr->data.function.head;
    if (head->type != EXPR_SYMBOL) return NULL;
    
    SymbolDef* def = symtab_get_def(head->data.symbol);
    Rule* rule = def->down_values;
    while (rule) {
        MatchEnv* env = env_new();
        if (match(expr, rule->pattern, env)) {
            Expr* result = replace_bindings(rule->replacement, env);
            env_free(env);
            return result;
        }
        env_free(env);
        rule = rule->next;
    }
    return NULL;
}

Expr* apply_own_values(Expr* expr) {
    if (!expr || expr->type != EXPR_SYMBOL) return NULL;
    
    SymbolDef* def = symtab_get_def(expr->data.symbol);
    Rule* rule = def->own_values;
    while (rule) {
        MatchEnv* env = env_new();
        if (match(expr, rule->pattern, env)) {
            Expr* result = replace_bindings(rule->replacement, env);
            env_free(env);
            return result;
        }
        env_free(env);
        rule = rule->next;
    }
    return NULL;
}
