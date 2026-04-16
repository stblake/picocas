/*
 * test_strings.c - Unit tests for StringLength and Characters builtins
 */

#include "test_utils.h"
#include "expr.h"
#include "eval.h"
#include "core.h"
#include "symtab.h"

/* ===== StringLength tests ===== */

void test_stringlength_basic() {
    assert_eval_eq("StringLength[\"tiger\"]", "5", 0);
}

void test_stringlength_empty() {
    assert_eval_eq("StringLength[\"\"]", "0", 0);
}

void test_stringlength_single_char() {
    assert_eval_eq("StringLength[\"a\"]", "1", 0);
}

void test_stringlength_spaces() {
    assert_eval_eq("StringLength[\"hello world\"]", "11", 0);
}

void test_stringlength_special_chars() {
    assert_eval_eq("StringLength[\"a\\nb\"]", "3", 0);
}

void test_stringlength_digits() {
    assert_eval_eq("StringLength[\"12345\"]", "5", 0);
}

void test_stringlength_mixed() {
    assert_eval_eq("StringLength[\"abc 123!@#\"]", "10", 0);
}

void test_stringlength_symbolic() {
    /* Non-string argument: should remain unevaluated */
    assert_eval_eq("StringLength[x]", "StringLength[x]", 0);
}

void test_stringlength_integer_arg() {
    /* Integer argument: should remain unevaluated */
    assert_eval_eq("StringLength[42]", "StringLength[42]", 0);
}

void test_stringlength_no_args() {
    /* No arguments: should remain unevaluated */
    assert_eval_eq("StringLength[]", "StringLength[]", 0);
}

void test_stringlength_two_args() {
    /* Two arguments: should remain unevaluated */
    assert_eval_eq("StringLength[\"a\", \"b\"]", "StringLength[\"a\", \"b\"]", 0);
}

void test_stringlength_listable() {
    /* Listable: threads over lists */
    assert_eval_eq("StringLength[{\"ABC\", \"DE\", \"F\"}]", "{3, 2, 1}", 0);
}

void test_stringlength_listable_with_empty() {
    assert_eval_eq("StringLength[{\"hello\", \"\", \"world\"}]", "{5, 0, 5}", 0);
}

void test_stringlength_long_string() {
    assert_eval_eq("StringLength[\"abcdefghijklmnopqrstuvwxyz\"]", "26", 0);
}

void test_stringlength_tab() {
    assert_eval_eq("StringLength[\"a\\tb\"]", "3", 0);
}

/* ===== Characters tests ===== */

void test_characters_basic() {
    assert_eval_eq("Characters[\"ABC\"]", "{\"A\", \"B\", \"C\"}", 0);
}

void test_characters_empty() {
    assert_eval_eq("Characters[\"\"]", "{}", 0);
}

void test_characters_single_char() {
    assert_eval_eq("Characters[\"x\"]", "{\"x\"}", 0);
}

void test_characters_with_spaces() {
    assert_eval_eq("Characters[\"A string.\"]", "{\"A\", \" \", \"s\", \"t\", \"r\", \"i\", \"n\", \"g\", \".\"}", 0);
}

void test_characters_digits() {
    assert_eval_eq("Characters[\"123\"]", "{\"1\", \"2\", \"3\"}", 0);
}

void test_characters_symbolic() {
    /* Non-string argument: should remain unevaluated */
    assert_eval_eq("Characters[x]", "Characters[x]", 0);
}

void test_characters_integer_arg() {
    /* Integer argument: should remain unevaluated */
    assert_eval_eq("Characters[42]", "Characters[42]", 0);
}

void test_characters_no_args() {
    assert_eval_eq("Characters[]", "Characters[]", 0);
}

void test_characters_two_args() {
    assert_eval_eq("Characters[\"a\", \"b\"]", "Characters[\"a\", \"b\"]", 0);
}

void test_characters_listable() {
    /* Listable: threads over lists */
    assert_eval_eq("Characters[{\"ABC\", \"DEF\", \"XYZ\"}]",
                   "{{\"A\", \"B\", \"C\"}, {\"D\", \"E\", \"F\"}, {\"X\", \"Y\", \"Z\"}}", 0);
}

void test_characters_listable_mixed_lengths() {
    assert_eval_eq("Characters[{\"ab\", \"c\"}]",
                   "{{\"a\", \"b\"}, {\"c\"}}", 0);
}

void test_characters_special_chars() {
    assert_eval_eq("Characters[\"!@#\"]", "{\"!\", \"@\", \"#\"}", 0);
}

/* ===== StringJoin tests ===== */

void test_stringjoin_basic() {
    assert_eval_eq("StringJoin[\"abcd\", \"ABCD\", \"xyz\"]", "\"abcdABCDxyz\"", 0);
}

void test_stringjoin_two_args() {
    assert_eval_eq("StringJoin[\"hello\", \" world\"]", "\"hello world\"", 0);
}

void test_stringjoin_single_arg() {
    assert_eval_eq("StringJoin[\"abc\"]", "\"abc\"", 0);
}

void test_stringjoin_empty_args() {
    assert_eval_eq("StringJoin[]", "\"\"", 0);
}

void test_stringjoin_empty_strings() {
    assert_eval_eq("StringJoin[\"\", \"\", \"\"]", "\"\"", 0);
}

void test_stringjoin_mixed_empty() {
    assert_eval_eq("StringJoin[\"a\", \"\", \"b\"]", "\"ab\"", 0);
}

void test_stringjoin_flatten_list() {
    /* All lists are recursively flattened */
    assert_eval_eq("StringJoin[{{\"AB\", \"CD\"}, \"XY\"}]", "\"ABCDXY\"", 0);
}

void test_stringjoin_nested_lists() {
    assert_eval_eq("StringJoin[{{{\"a\"}, \"b\"}, \"c\"}]", "\"abc\"", 0);
}

void test_stringjoin_list_with_multiple_args() {
    assert_eval_eq("StringJoin[{\"A\", \"B\"}, \"C\"]", "\"ABC\"", 0);
}

void test_stringjoin_symbolic_unevaluated() {
    /* Non-string argument: remain unevaluated */
    assert_eval_eq("StringJoin[\"a\", x]", "StringJoin[\"a\", x]", 0);
}

void test_stringjoin_integer_unevaluated() {
    assert_eval_eq("StringJoin[\"a\", 42]", "StringJoin[\"a\", 42]", 0);
}

void test_stringjoin_newline() {
    /* Parser treats \n as literal 'n' (no escape interpretation) */
    assert_eval_eq("StringJoin[\"one\", \"\\n\", \"two\"]", "\"onentwo\"", 0);
}

void test_stringjoin_many_args() {
    assert_eval_eq("StringJoin[\"a\", \"b\", \"c\", \"d\", \"e\"]", "\"abcde\"", 0);
}

/* ===== <> infix parser tests ===== */

void test_infix_stringjoin_basic() {
    assert_eval_eq("\"abcd\" <> \"ABCD\" <> \"xyz\"", "\"abcdABCDxyz\"", 0);
}

void test_infix_stringjoin_two() {
    assert_eval_eq("\"hello\" <> \" world\"", "\"hello world\"", 0);
}

void test_infix_stringjoin_empty() {
    assert_eval_eq("\"\" <> \"\"", "\"\"", 0);
}

void test_infix_stringjoin_chain_four() {
    assert_eval_eq("\"a\" <> \"b\" <> \"c\" <> \"d\"", "\"abcd\"", 0);
}

void test_infix_stringjoin_symbolic() {
    assert_eval_eq("\"a\" <> x", "StringJoin[\"a\", x]", 0);
}

void test_infix_stringjoin_newline() {
    /* Parser treats \n as literal 'n' (no escape interpretation) */
    assert_eval_eq("\"one\" <> \"\\n\" <> \"two\"", "\"onentwo\"", 0);
}

void test_infix_stringjoin_fullform() {
    /* FullForm of string result is just the string */
    assert_eval_eq("\"a\" <> \"b\"", "\"ab\"", 1);
}

void test_infix_stringjoin_symbolic_fullform() {
    /* Verify parser produces StringJoin head for unevaluated cases */
    assert_eval_eq("\"a\" <> x", "StringJoin[\"a\", x]", 1);
}

void test_infix_stringjoin_three_fullform() {
    /* Flat attribute should flatten chained <> into one StringJoin */
    assert_eval_eq("\"a\" <> x <> \"c\"", "StringJoin[\"a\", x, \"c\"]", 1);
}

void test_infix_stringjoin_with_parens() {
    assert_eval_eq("(\"a\" <> \"b\") <> \"c\"", "\"abc\"", 0);
}

void test_infix_stringjoin_spaces() {
    assert_eval_eq("\"hello\"  <>  \"world\"", "\"helloworld\"", 0);
}

/* ===== Combined / integration tests ===== */

void test_stringjoin_with_characters() {
    /* Join the characters back together */
    assert_eval_eq("StringJoin[Characters[\"hello\"]]", "\"hello\"", 0);
}

void test_stringlength_of_stringjoin() {
    assert_eval_eq("StringLength[\"abc\" <> \"def\"]", "6", 0);
}

void test_stringlength_of_characters() {
    /* StringLength applied to the result of Characters should give list of 1s */
    assert_eval_eq("StringLength /@ Characters[\"hello\"]", "{1, 1, 1, 1, 1}", 0);
}

void test_length_of_characters() {
    /* Length of Characters result should equal StringLength */
    assert_eval_eq("Length[Characters[\"tiger\"]]", "5", 0);
}

int main() {
    symtab_init();
    core_init();

    /* StringLength tests */
    TEST(test_stringlength_basic);
    TEST(test_stringlength_empty);
    TEST(test_stringlength_single_char);
    TEST(test_stringlength_spaces);
    TEST(test_stringlength_special_chars);
    TEST(test_stringlength_digits);
    TEST(test_stringlength_mixed);
    TEST(test_stringlength_symbolic);
    TEST(test_stringlength_integer_arg);
    TEST(test_stringlength_no_args);
    TEST(test_stringlength_two_args);
    TEST(test_stringlength_listable);
    TEST(test_stringlength_listable_with_empty);
    TEST(test_stringlength_long_string);
    TEST(test_stringlength_tab);

    /* Characters tests */
    TEST(test_characters_basic);
    TEST(test_characters_empty);
    TEST(test_characters_single_char);
    TEST(test_characters_with_spaces);
    TEST(test_characters_digits);
    TEST(test_characters_symbolic);
    TEST(test_characters_integer_arg);
    TEST(test_characters_no_args);
    TEST(test_characters_two_args);
    TEST(test_characters_listable);
    TEST(test_characters_listable_mixed_lengths);
    TEST(test_characters_special_chars);

    /* StringJoin tests */
    TEST(test_stringjoin_basic);
    TEST(test_stringjoin_two_args);
    TEST(test_stringjoin_single_arg);
    TEST(test_stringjoin_empty_args);
    TEST(test_stringjoin_empty_strings);
    TEST(test_stringjoin_mixed_empty);
    TEST(test_stringjoin_flatten_list);
    TEST(test_stringjoin_nested_lists);
    TEST(test_stringjoin_list_with_multiple_args);
    TEST(test_stringjoin_symbolic_unevaluated);
    TEST(test_stringjoin_integer_unevaluated);
    TEST(test_stringjoin_newline);
    TEST(test_stringjoin_many_args);

    /* <> infix parser tests */
    TEST(test_infix_stringjoin_basic);
    TEST(test_infix_stringjoin_two);
    TEST(test_infix_stringjoin_empty);
    TEST(test_infix_stringjoin_chain_four);
    TEST(test_infix_stringjoin_symbolic);
    TEST(test_infix_stringjoin_newline);
    TEST(test_infix_stringjoin_fullform);
    TEST(test_infix_stringjoin_symbolic_fullform);
    TEST(test_infix_stringjoin_three_fullform);
    TEST(test_infix_stringjoin_with_parens);
    TEST(test_infix_stringjoin_spaces);

    /* Combined tests */
    TEST(test_stringjoin_with_characters);
    TEST(test_stringlength_of_stringjoin);
    TEST(test_stringlength_of_characters);
    TEST(test_length_of_characters);

    printf("All string tests passed!\n");
    return 0;
}
