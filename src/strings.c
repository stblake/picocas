/*
 * strings.c - String manipulation builtins for PicoCAS
 *
 * StringLength["string"]   - gives the number of characters in a string
 * Characters["string"]     - gives a list of the individual characters in a string
 *
 * Both functions have attribute Listable, so they automatically thread
 * over List arguments.
 */

#include "strings.h"
#include "symtab.h"
#include "attr.h"
#include <string.h>

/*
 * builtin_stringlength:
 * StringLength["string"] returns the number of characters in the string
 * as an integer. Returns NULL (unevaluated) for non-string arguments.
 */
Expr* builtin_stringlength(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1)
        return NULL;

    Expr* arg = res->data.function.args[0];
    if (arg->type != EXPR_STRING)
        return NULL;

    int64_t len = (int64_t)strlen(arg->data.string);
    Expr* result = expr_new_integer(len);

    return result;
}

/*
 * builtin_characters:
 * Characters["string"] returns a List of single-character strings.
 * Each character in the input string becomes a length-1 string element.
 * Returns NULL (unevaluated) for non-string arguments.
 */
Expr* builtin_characters(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1)
        return NULL;

    Expr* arg = res->data.function.args[0];
    if (arg->type != EXPR_STRING)
        return NULL;

    const char* str = arg->data.string;
    size_t len = strlen(str);

    Expr** chars = malloc(sizeof(Expr*) * len);
    if (!chars) {
    
        return expr_new_function(expr_new_symbol("List"), NULL, 0);
    }

    char buf[2];
    buf[1] = '\0';
    for (size_t i = 0; i < len; i++) {
        buf[0] = str[i];
        chars[i] = expr_new_string(buf);
    }

    Expr* result = expr_new_function(expr_new_symbol("List"), chars, len);
    free(chars);

    return result;
}

/*
 * collect_strings:
 * Recursively collects all strings from an expression, flattening any List
 * wrappers encountered. Appends string pointers (borrowed, not copied) to
 * the output array. Returns true if all leaves are strings, false otherwise.
 */
static bool collect_strings(Expr* e, const char*** strs, size_t* count, size_t* cap) {
    if (e->type == EXPR_STRING) {
        if (*count >= *cap) {
            *cap *= 2;
            *strs = realloc(*strs, sizeof(const char*) * (*cap));
        }
        (*strs)[(*count)++] = e->data.string;
        return true;
    }
    if (e->type == EXPR_FUNCTION && e->data.function.head->type == EXPR_SYMBOL
        && strcmp(e->data.function.head->data.symbol, "List") == 0) {
        for (size_t i = 0; i < e->data.function.arg_count; i++) {
            if (!collect_strings(e->data.function.args[i], strs, count, cap))
                return false;
        }
        return true;
    }
    return false;
}

/*
 * builtin_stringjoin:
 * StringJoin[s1, s2, ...] concatenates all string arguments into a single
 * string. Any List wrappers in the arguments are recursively flattened so
 * that all enclosed strings participate in the join.
 *
 * StringJoin[] with no arguments returns the empty string "".
 * Returns NULL (unevaluated) if any non-string, non-list leaf is found.
 */
Expr* builtin_stringjoin(Expr* res) {
    if (res->type != EXPR_FUNCTION)
        return NULL;

    size_t argc = res->data.function.arg_count;

    /* StringJoin[] -> "" */
    if (argc == 0)
        return expr_new_string("");

    /* Collect all strings, flattening lists */
    size_t cap = 16, count = 0;
    const char** strs = malloc(sizeof(const char*) * cap);

    for (size_t i = 0; i < argc; i++) {
        if (!collect_strings(res->data.function.args[i], &strs, &count, &cap)) {
            free(strs);
            return NULL;
        }
    }

    /* Compute total length */
    size_t total_len = 0;
    for (size_t i = 0; i < count; i++) {
        total_len += strlen(strs[i]);
    }

    /* Build concatenated string */
    char* buf = malloc(total_len + 1);
    buf[0] = '\0';
    size_t offset = 0;
    for (size_t i = 0; i < count; i++) {
        size_t slen = strlen(strs[i]);
        memcpy(buf + offset, strs[i], slen);
        offset += slen;
    }
    buf[total_len] = '\0';

    free(strs);

    Expr* result = expr_new_string(buf);
    free(buf);
    return result;
}

/*
 * strings_init:
 * Registers StringLength, Characters, and StringJoin builtins with
 * appropriate attributes and docstrings.
 */
void strings_init(void) {
    symtab_add_builtin("StringLength", builtin_stringlength);
    symtab_get_def("StringLength")->attributes |= ATTR_LISTABLE | ATTR_PROTECTED;
    symtab_set_docstring("StringLength",
        "StringLength[\"string\"]\n"
        "\tGives the number of characters in a string.");

    symtab_add_builtin("Characters", builtin_characters);
    symtab_get_def("Characters")->attributes |= ATTR_LISTABLE | ATTR_PROTECTED;
    symtab_set_docstring("Characters",
        "Characters[\"string\"]\n"
        "\tGives a list of the characters in a string.\n"
        "\tEach character is given as a length-1 string.");

    symtab_add_builtin("StringJoin", builtin_stringjoin);
    symtab_get_def("StringJoin")->attributes |= ATTR_FLAT | ATTR_ONEIDENTITY | ATTR_PROTECTED;
    symtab_set_docstring("StringJoin",
        "StringJoin[\"s1\", \"s2\", ...]\n"
        "\tConcatenates strings together.\n"
        "\tStringJoin[{\"s1\", \"s2\", ...}] flattens all lists.\n"
        "\tThe infix form is \"s1\" <> \"s2\" <> ...");
}
