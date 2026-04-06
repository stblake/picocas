#include "load.h"
#include "expr.h"
#include "symtab.h"
#include "parse.h"
#include "eval.h"
#include "print.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Expr* builtin_get(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    
    Expr* file_arg = res->data.function.args[0];
    if (file_arg->type != EXPR_STRING) return NULL;
    
    const char* filename = file_arg->data.string;
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        printf("Get::noopen: Cannot open %s.\n", filename);
        return expr_new_symbol("$Failed");
    }
    
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (fsize < 0) {
        fclose(fp);
        return expr_new_symbol("$Failed");
    }
    
    char* buffer = malloc(fsize + 1);
    if (!buffer) {
        fclose(fp);
        return expr_new_symbol("$Failed");
    }
    
    size_t read_len = fread(buffer, 1, fsize, fp);
    buffer[read_len] = '\0';
    fclose(fp);
    
    Expr* last_eval = expr_new_symbol("Null");
    const char* ptr = buffer;
    
    while (*ptr != '\0') {
        Expr* parsed = parse_next_expression(&ptr);
        if (parsed) {
            Expr* evaluated = evaluate(parsed);
            if (evaluated) {
                expr_free(last_eval);
                last_eval = evaluated;
            }
            expr_free(parsed);
        } else {
            // EOF or unparseable. Since parse_next_expression stops when *state.pos == '\0'
            break;
        }
    }
    
    free(buffer);
    return last_eval;
}

void load_init(void) {
    symtab_add_builtin("Get", builtin_get);
    symtab_get_def("Get")->attributes |= ATTR_PROTECTED;
    symtab_set_docstring("Get", "Get[filename] reads expressions from a file, evaluates them, and returns the last result.");
}
