#include "expr.h"
#include "parse.h"
#include "print.h"
#include "part.h"
#include "eval.h"
#include "symtab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_INPUT_LEN 10240

void process_input(const char* input, int line_number) {
    if (strlen(input) == 0) return;
    
    // Update $Line
    Expr* line_sym = expr_new_symbol("$Line");
    Expr* line_val = expr_new_integer(line_number);
    symtab_add_own_value("$Line", line_sym, line_val);
    expr_free(line_sym);
    expr_free(line_val);

    // Parse the input
    Expr* parsed = parse_expression(input);
    if (!parsed) {
        printf("Parse error\n\n");
        return;
    }
    
    // Evaluate
    Expr* evaluated = evaluate(parsed);
    
    // Store In[line_number] = parsed
    Expr* in_sym = expr_new_symbol("In");
    Expr* in_arg = expr_new_integer(line_number);
    Expr* in_args[] = {in_arg};
    Expr* in_pattern = expr_new_function(in_sym, in_args, 1);
    symtab_add_down_value("In", in_pattern, parsed);
    expr_free(in_pattern);
    
    // Store Out[line_number] = evaluated
    Expr* out_sym = expr_new_symbol("Out");
    Expr* out_arg = expr_new_integer(line_number);
    Expr* out_args[] = {out_arg};
    Expr* out_pattern = expr_new_function(out_sym, out_args, 1);
    symtab_add_down_value("Out", out_pattern, evaluated);
    expr_free(out_pattern);
    
    // Print the result
    printf("Out[%d]= ", line_number);
    expr_print(evaluated);
    printf("\n"); // extra blank line
    
    // Clean up
    expr_free(parsed);
    expr_free(evaluated);
}

void repl_loop() {
    printf("\nPicoCAS - A tiny, LLM-generated, Mathematica-like computer algebra system.\n\n");
    printf("This program is free, open source software and comes with ABSOLUTELY NO WARRANTY.\n\n");
    printf("End a line with '\\' to enter a multiline expression. Press Return to evaluate.\n");
    printf("Exit by evaluating Quit[] or CONTROL-C.\n\n");
    
    int line_number = 1;
    char prompt[64];
    char full_input[MAX_INPUT_LEN] = {0};
    int in_multiline = 0;
    
    while (true) {
        int submit_now = 0;
        if (!in_multiline) {
            snprintf(prompt, sizeof(prompt), "In[%d]:= ", line_number);
        } else {
            snprintf(prompt, sizeof(prompt), "");
        }
        
        char* line = readline(prompt);
        if (!line) {
            printf("\n");
            break; // EOF
        }
        
        size_t line_len = strlen(line);
        
        // Remove trailing whitespace to properly check for backslash
        size_t check_len = line_len;
        while (check_len > 0 && (line[check_len - 1] == ' ' || line[check_len - 1] == '\t')) {
            check_len--;
        }
        
        int has_backslash = 0;
        if (check_len > 0 && line[check_len - 1] == '\\') {
            has_backslash = 1;
            // Remove the backslash from the line
            line[check_len - 1] = '\0';
            line_len = strlen(line);
        }
        
        // Check buffer limits
        if (strlen(full_input) + line_len + 2 >= MAX_INPUT_LEN) {
            printf("Input too long!\n");
            full_input[0] = '\0';
            in_multiline = 0;
            free(line);
            continue;
        }
        
        if (in_multiline) {
            strcat(full_input, "\n");
        }
        strcat(full_input, line);
        
        if (has_backslash) {
            in_multiline = 1;
        } else {
            submit_now = 1;
        }
        
        if (submit_now) {
            if (strlen(full_input) == 0) {
                free(line);
                continue;
            }
            
            add_history(full_input);
            
            if (strcmp(full_input, "Quit[]") == 0) {
                free(line);
                break;
            }
            
            process_input(full_input, line_number);
            
            full_input[0] = '\0';
            in_multiline = 0;
            line_number++;
        }
        
        free(line);
    }
    
    printf("\n");
}

#include "core.h"

int main() {
    symtab_init();
    core_init();
    repl_loop();
    return 0;
}
