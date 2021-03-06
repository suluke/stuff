#include "gtest_utils.h"
#include "parse_utils.h"
#include "gtest/gtest.h"
#include <iostream>
#include <sstream>

using namespace jnsn;
using namespace std;
using namespace testing::internal;

class parser_test : public ::testing::Test {
protected:
  constant_string_parser parser;
  stringstream str;
};

using ast_root = parser_base::ast_root;

#define MOD_WRAP(JSON) "{\"type\": \"module\", \"stmts\": [" JSON "]}"

#define ASSERT_PARSED_MATCHES_JSON(INPUT, JSON)                                \
  do {                                                                         \
    str.str("");                                                               \
    parser.lexer.set_text(INPUT);                                              \
    auto res = parser.parse();                                                 \
    ASSERT_TRUE(holds_alternative<ast_root *>(res))                            \
        << std::get<parser_error>(res);                                        \
    auto mod = get<ast_root *>(res);                                           \
                                                                               \
    str << mod;                                                                \
    ASSERT_EQ(str.str(), JSON "\n");                                           \
  } while (false)

#define PARSER_SUCCESS(INPUT)                                                  \
  do {                                                                         \
    str.str("");                                                               \
    parser.lexer.set_text(INPUT);                                              \
    auto res = parser.parse();                                                 \
    ASSERT_TRUE(holds_alternative<ast_root *>(res));                           \
  } while (false)
#define PARSER_ERROR(INPUT)                                                    \
  do {                                                                         \
    str.str("");                                                               \
    parser.lexer.set_text(INPUT);                                              \
    auto res = parser.parse();                                                 \
    ASSERT_TRUE(holds_alternative<parser_error>(res));                         \
  } while (false)

#define XFAIL(INPUT)                                                           \
  do {                                                                         \
    str.str("");                                                               \
    parser.lexer.set_text(INPUT);                                              \
    auto res = parser.parse();                                                 \
    ASSERT_TRUE(holds_alternative<parser_error>(res));                         \
    ColoredPrintf(COLOR_YELLOW, "[   XFAIL  ] ");                              \
    ColoredPrintf(COLOR_DEFAULT, INPUT " (error)\n");                          \
  } while (false)

#define XWRONG(INPUT, EXPECTED)                                                \
  do {                                                                         \
    str.str("");                                                               \
    parser.lexer.set_text(INPUT);                                              \
    auto res = parser.parse();                                                 \
    ASSERT_FALSE(holds_alternative<parser_error>(res));                        \
    str << std::get<ast_root *>(res);                                          \
    ASSERT_NE(str.str(), EXPECTED);                                            \
    ColoredPrintf(COLOR_YELLOW, "[   XFAIL  ] ");                              \
    ColoredPrintf(COLOR_DEFAULT, INPUT " (wrong result)\n");                   \
  } while (false)

TEST_F(parser_test, empty) {
  ASSERT_PARSED_MATCHES_JSON("", "{\"type\": \"module\", \"stmts\": []}");
}
TEST_F(parser_test, number_literals) {
  ASSERT_PARSED_MATCHES_JSON("1", "{\"type\": \"module\", \"stmts\": "
                                  "[{\"type\": \"int_literal\", \"val\": "
                                  "\"1\"}]}");
  PARSER_ERROR("1.window");
}
TEST_F(parser_test, string_literals) {
  ASSERT_PARSED_MATCHES_JSON(
      "'use strict'",
      MOD_WRAP("{\"type\": \"string_literal\", \"val\": \"'use strict'\"}"));
  ASSERT_PARSED_MATCHES_JSON(
      "let s = \"text\"",
      MOD_WRAP("{\"type\": \"var_decl\", \"keyword\": \"let\", \"parts\": "
               "[{\"type\": \"var_decl_part\", \"name\": \"s\", \"init\": "
               "{\"type\": \"string_literal\", \"val\": \"\\\"text\\\"\"}}]}"));
}
TEST_F(parser_test, template_literals) {
  ASSERT_PARSED_MATCHES_JSON(
      "`1${2}3${4}5`",
      MOD_WRAP("{\"type\": \"template_literal\", \"strs\": [\"`1${\", "
               "\"}3${\", \"}5`\"], \"exprs\": [{\"type\": \"int_literal\", "
               "\"val\": \"2\"}, {\"type\": \"int_literal\", \"val\": "
               "\"4\"}]}"));
  PARSER_ERROR("let o = {``: 0}");
}
TEST_F(parser_test, regex_literals) {
  ASSERT_PARSED_MATCHES_JSON(
      "/.*/.test('abc')",
      MOD_WRAP("{\"type\": \"call_expr\", \"callee\": {\"type\": "
               "\"member_access\", \"base\": {\"type\": \"regex_literal\", "
               "\"val\": \"/.*/\"}, \"member\": \"test\"}, \"args\": "
               "{\"type\": \"argument_list\", \"values\": [{\"type\": "
               "\"string_literal\", \"val\": \"'abc'\"}]}}"));
}
TEST_F(parser_test, array_literals) {
  ASSERT_PARSED_MATCHES_JSON(
      "let arr = [1, ...a, 3, ...b]",
      "{\"type\": \"module\", \"stmts\": [{\"type\": \"var_decl\", "
      "\"keyword\": \"let\", \"parts\": [{\"type\": \"var_decl_part\", "
      "\"name\": \"arr\", \"init\": {\"type\": "
      "\"array_literal\", \"values\": [{\"type\": \"int_literal\", \"val\": "
      "\"1\"}, {\"type\": \"spread_expr\", \"list\": {\"type\": "
      "\"identifier_expr\", \"str\": \"a\"}}, {\"type\": \"int_literal\", "
      "\"val\": \"3\"}, {\"type\": \"spread_expr\", \"list\": {\"type\": "
      "\"identifier_expr\", \"str\": \"b\"}}]}}]}]}");
  XFAIL("[a,b,]");
}
TEST_F(parser_test, object_literals) {
  ASSERT_PARSED_MATCHES_JSON(
      "let x = {a, b, ...c, i: 5}",
      "{\"type\": \"module\", \"stmts\": [{\"type\": \"var_decl\", "
      "\"keyword\": \"let\", \"parts\": [{\"type\": \"var_decl_part\", "
      "\"name\": \"x\", \"init\": {\"type\": "
      "\"object_literal\", \"entries\": [{\"type\": \"identifier_expr\", "
      "\"str\": \"a\"}, {\"type\": \"identifier_expr\", \"str\": \"b\"}, "
      "{\"type\": \"spread_expr\", \"list\": {\"type\": \"identifier_expr\", "
      "\"str\": \"c\"}}, {\"type\": \"object_entry\", \"key\": "
      "\"i\", \"val\": {\"type\": \"int_literal\", \"val\": \"5\"}}]}}]}]}");
  // Trailing comma
  ASSERT_PARSED_MATCHES_JSON(
      "let x = {a,}",
      MOD_WRAP("{\"type\": \"var_decl\", \"keyword\": \"let\", \"parts\": "
               "[{\"type\": \"var_decl_part\", \"name\": \"x\", \"init\": "
               "{\"type\": \"object_literal\", \"entries\": [{\"type\": "
               "\"identifier_expr\", \"str\": \"a\"}]}}]}"));
}
TEST_F(parser_test, block_vs_objs) {
  ASSERT_PARSED_MATCHES_JSON("{}", "{\"type\": \"module\", \"stmts\": "
                                   "[{\"type\": \"block\", "
                                   "\"stmts\": []}]}");
  ASSERT_PARSED_MATCHES_JSON(
      "{ label: window, console }",
      MOD_WRAP("{\"type\": \"block\", \"stmts\": [{\"type\": \"label_stmt\", "
               "\"label\": \"label\", \"stmt\": {\"type\": \"comma_operator\", "
               "\"lhs\": {\"type\": \"identifier_expr\", \"str\": \"window\"}, "
               "\"rhs\": {\"type\": \"identifier_expr\", \"str\": "
               "\"console\"}}}]}"));
  ASSERT_PARSED_MATCHES_JSON(
      "{ label: window, console.log(1) }",
      MOD_WRAP("{\"type\": \"block\", \"stmts\": [{\"type\": \"label_stmt\", "
               "\"label\": \"label\", \"stmt\": {\"type\": \"comma_operator\", "
               "\"lhs\": {\"type\": \"identifier_expr\", \"str\": \"window\"}, "
               "\"rhs\": {\"type\": \"call_expr\", \"callee\": {\"type\": "
               "\"member_access\", \"base\": {\"type\": \"identifier_expr\", "
               "\"str\": \"console\"}, \"member\": \"log\"}, \"args\": "
               "{\"type\": \"argument_list\", \"values\": [{\"type\": "
               "\"int_literal\", \"val\": \"1\"}]}}}}]}"));
}
TEST_F(parser_test, parenthesis) {
  ASSERT_PARSED_MATCHES_JSON("(((1)))", "{\"type\": \"module\", \"stmts\": "
                                        "[{\"type\": \"int_literal\", \"val\": "
                                        "\"1\"}]}");
  PARSER_ERROR("(((1))");
}
TEST_F(parser_test, postfix_ops) {
  ASSERT_PARSED_MATCHES_JSON(
      "i++", MOD_WRAP("{\"type\": \"postfix_increment\", \"value\": {\"type\": "
                      "\"identifier_expr\", \"str\": \"i\"}}"));
  ASSERT_PARSED_MATCHES_JSON(
      "i--", MOD_WRAP("{\"type\": \"postfix_decrement\", \"value\": {\"type\": "
                      "\"identifier_expr\", \"str\": \"i\"}}"));
  PARSER_ERROR("i----");
  PARSER_ERROR("i++++");
}
TEST_F(parser_test, prefix_ops) {
  ASSERT_PARSED_MATCHES_JSON(
      "++i", MOD_WRAP("{\"type\": \"prefix_increment\", \"value\": {\"type\": "
                      "\"identifier_expr\", \"str\": \"i\"}}"));
  ASSERT_PARSED_MATCHES_JSON(
      "--i", MOD_WRAP("{\"type\": \"prefix_decrement\", \"value\": {\"type\": "
                      "\"identifier_expr\", \"str\": \"i\"}}"));
  ASSERT_PARSED_MATCHES_JSON(
      "+i", MOD_WRAP("{\"type\": \"prefix_plus\", \"value\": {\"type\": "
                     "\"identifier_expr\", \"str\": \"i\"}}"));
  ASSERT_PARSED_MATCHES_JSON(
      "-i", MOD_WRAP("{\"type\": \"prefix_minus\", \"value\": {\"type\": "
                     "\"identifier_expr\", \"str\": \"i\"}}"));
  ASSERT_PARSED_MATCHES_JSON(
      "~i", MOD_WRAP("{\"type\": \"binverse_expr\", \"value\": {\"type\": "
                     "\"identifier_expr\", \"str\": \"i\"}}"));
  ASSERT_PARSED_MATCHES_JSON(
      "!i", MOD_WRAP("{\"type\": \"not_expr\", \"value\": {\"type\": "
                     "\"identifier_expr\", \"str\": \"i\"}}"));
  ASSERT_PARSED_MATCHES_JSON(
      "typeof i", MOD_WRAP("{\"type\": \"typeof_expr\", \"value\": {\"type\": "
                           "\"identifier_expr\", \"str\": \"i\"}}"));
  ASSERT_PARSED_MATCHES_JSON(
      "void i", MOD_WRAP("{\"type\": \"void_expr\", \"value\": {\"type\": "
                         "\"identifier_expr\", \"str\": \"i\"}}"));
  ASSERT_PARSED_MATCHES_JSON(
      "delete i", MOD_WRAP("{\"type\": \"delete_expr\", \"value\": {\"type\": "
                           "\"identifier_expr\", \"str\": \"i\"}}"));
  ASSERT_PARSED_MATCHES_JSON(
      "+i++", MOD_WRAP("{\"type\": \"prefix_plus\", \"value\": {\"type\": "
                       "\"postfix_increment\", \"value\": {\"type\": "
                       "\"identifier_expr\", \"str\": \"i\"}}}"));

  PARSER_ERROR("----i");
  PARSER_ERROR("++++i");
}
TEST_F(parser_test, decl) {
  ASSERT_PARSED_MATCHES_JSON("let x;",
                             "{\"type\": \"module\", \"stmts\": [{\"type\": "
                             "\"var_decl\", \"keyword\": \"let\", \"parts\": "
                             "[{\"type\": \"var_decl_part\", \"name\": \"x\", "
                             "\"init\": null}]}]}");
  ASSERT_PARSED_MATCHES_JSON(
      "{let i = 0;}",
      "{\"type\": \"module\", \"stmts\": [{\"type\": "
      "\"block\", \"stmts\": [{\"type\": "
      "\"var_decl\", \"keyword\": \"let\", \"parts\": [{\"type\": "
      "\"var_decl_part\", \"name\": \"i\", "
      "\"init\": {\"type\": \"int_literal\", \"val\": \"0\"}}]}]}]}");
  ASSERT_PARSED_MATCHES_JSON(
      "let i, j, k",
      "{\"type\": \"module\", \"stmts\": [{\"type\": \"var_decl\", "
      "\"keyword\": \"let\", \"parts\": [{\"type\": \"var_decl_part\", "
      "\"name\": \"i\", \"init\": null}, {\"type\": \"var_decl_part\", "
      "\"name\": \"j\", \"init\": "
      "null}, {\"type\": "
      "\"var_decl_part\", \"name\": \"k\", \"init\": null}]}]}");
  ASSERT_PARSED_MATCHES_JSON(
      "let i = 0, j = 1, k = 2",
      "{\"type\": \"module\", \"stmts\": [{\"type\": \"var_decl\", "
      "\"keyword\": \"let\", \"parts\": [{\"type\": \"var_decl_part\", "
      "\"name\": \"i\", \"init\": {\"type\": \"int_literal\", \"val\": "
      "\"0\"}}, {\"type\": \"var_decl_part\", \"name\": \"j\", \"init\": "
      "{\"type\": \"int_literal\", \"val\": \"1\"}}, {\"type\": "
      "\"var_decl_part\", \"name\": \"k\", \"init\": {\"type\": "
      "\"int_literal\", \"val\": \"2\"}}]}]}");
  PARSER_ERROR("var");
  PARSER_ERROR("var i = var j = var k");
}
TEST_F(parser_test, binary_ops) {
  ASSERT_PARSED_MATCHES_JSON(
      "1 + 1", "{\"type\": \"module\", \"stmts\": [{\"type\": \"add\", "
               "\"lhs\": {\"type\": \"int_literal\", \"val\": \"1\"}, \"rhs\": "
               "{\"type\": \"int_literal\", \"val\": \"1\"}}]}");
  ASSERT_PARSED_MATCHES_JSON(
      "1 + 4 / 2", "{\"type\": \"module\", \"stmts\": [{\"type\": \"add\", "
                   "\"lhs\": {\"type\": \"int_literal\", \"val\": \"1\"}, "
                   "\"rhs\": {\"type\": \"divide\", \"lhs\": {\"type\": "
                   "\"int_literal\", \"val\": \"4\"}, \"rhs\": {\"type\": "
                   "\"int_literal\", \"val\": \"2\"}}}]}");
  ASSERT_PARSED_MATCHES_JSON(
      "1 + 4 / 2; 6 + 7",
      "{\"type\": \"module\", \"stmts\": [{\"type\": \"add\", "
      "\"lhs\": {\"type\": \"int_literal\", \"val\": \"1\"}, "
      "\"rhs\": {\"type\": \"divide\", \"lhs\": {\"type\": "
      "\"int_literal\", \"val\": \"4\"}, \"rhs\": {\"type\": "
      "\"int_literal\", \"val\": \"2\"}}}, {\"type\": \"add\", \"lhs\": "
      "{\"type\": \"int_literal\", \"val\": \"6\"}, \"rhs\": {\"type\": "
      "\"int_literal\", \"val\": \"7\"}}]}");
  ASSERT_PARSED_MATCHES_JSON(
      "a instanceof A",
      MOD_WRAP("{\"type\": \"instanceof_expr\", \"lhs\": {\"type\": "
               "\"identifier_expr\", \"str\": \"a\"}, \"rhs\": {\"type\": "
               "\"identifier_expr\", \"str\": \"A\"}}"));
  ASSERT_PARSED_MATCHES_JSON(
      "a in A", MOD_WRAP("{\"type\": \"in_expr\", \"lhs\": {\"type\": "
                         "\"identifier_expr\", \"str\": \"a\"}, \"rhs\": "
                         "{\"type\": \"identifier_expr\", \"str\": \"A\"}}"));
  PARSER_SUCCESS("1=1");
  PARSER_SUCCESS("1==1");
  PARSER_SUCCESS("1===1");
  PARSER_SUCCESS("1!=1");
  PARSER_SUCCESS("1!==1");
  PARSER_SUCCESS("1<<1");
  PARSER_SUCCESS("1>>1");
  PARSER_SUCCESS("1>>>1");
  PARSER_SUCCESS("1**1");
  PARSER_SUCCESS("1%1");
  PARSER_SUCCESS("1<1");
  PARSER_SUCCESS("1<=1");
  PARSER_SUCCESS("1>1");
  PARSER_SUCCESS("1>=1");
  PARSER_SUCCESS("1&1");
  PARSER_SUCCESS("1&&1");
  PARSER_SUCCESS("1|1");
  PARSER_SUCCESS("1^1");
  PARSER_SUCCESS("1+=1");
  PARSER_SUCCESS("1-=1");
  PARSER_SUCCESS("1*=1");
  PARSER_SUCCESS("1/=1");
  PARSER_SUCCESS("1%=1");
  PARSER_SUCCESS("1|=1");
  PARSER_SUCCESS("1&=1");
  PARSER_SUCCESS("1^=1");
  PARSER_SUCCESS("1,1");
}
TEST_F(parser_test, ternary_op) {
  ASSERT_PARSED_MATCHES_JSON(
      "a ? b ? c ? 1 : 2 : 3 : 4",
      MOD_WRAP("{\"type\": \"ternary_operator\", \"lhs\": {\"type\": "
               "\"identifier_expr\", \"str\": \"a\"}, \"rhs\": {\"type\": "
               "\"int_literal\", \"val\": \"4\"}, \"mid\": {\"type\": "
               "\"ternary_operator\", \"lhs\": {\"type\": \"identifier_expr\", "
               "\"str\": \"b\"}, \"rhs\": {\"type\": \"int_literal\", \"val\": "
               "\"3\"}, \"mid\": {\"type\": \"ternary_operator\", \"lhs\": "
               "{\"type\": \"identifier_expr\", \"str\": \"c\"}, \"rhs\": "
               "{\"type\": \"int_literal\", \"val\": \"2\"}, \"mid\": "
               "{\"type\": \"int_literal\", \"val\": \"1\"}}}}"));
}
TEST_F(parser_test, function) {
  ASSERT_PARSED_MATCHES_JSON(
      "function test(arg1, arg2) { return arg1 + arg2; }",
      "{\"type\": \"module\", \"stmts\": [{\"type\": \"function_stmt\", "
      "\"name\": "
      "\"test\", \"params\": {\"type\": \"param_list\", \"names\": [\"arg1\", "
      "\"arg2\"], \"rest\": null}, \"body\": {\"type\": \"block\", \"stmts\": "
      "[{\"type\": \"return_stmt\", \"value\": {\"type\": \"add\", \"lhs\": "
      "{\"type\": \"identifier_expr\", \"str\": \"arg1\"}, \"rhs\": {\"type\": "
      "\"identifier_expr\", \"str\": \"arg2\"}}}]}}]}");
  ASSERT_PARSED_MATCHES_JSON(
      "(function() {})",
      "{\"type\": \"module\", \"stmts\": [{\"type\": "
      "\"function_expr\", \"name\": null, \"params\": {\"type\": "
      "\"param_list\", \"names\": [], \"rest\": null}, \"body\": "
      "{\"type\": \"block\", \"stmts\": []}}]}");
  PARSER_ERROR("function() {}");
  XFAIL("function test(arg1, arg2 = 5) {}");
}
TEST_F(parser_test, member_access) {
  ASSERT_PARSED_MATCHES_JSON(
      "a.b.c.d", "{\"type\": \"module\", \"stmts\": [{\"type\": "
                 "\"member_access\", \"base\": {\"type\": \"member_access\", "
                 "\"base\": {\"type\": \"member_access\", \"base\": {\"type\": "
                 "\"identifier_expr\", \"str\": \"a\"}, \"member\": \"b\"}, "
                 "\"member\": \"c\"}, \"member\": \"d\"}]}");
}
TEST_F(parser_test, call) {
  ASSERT_PARSED_MATCHES_JSON("console.log(1 + 2);",
                             "{\"type\": \"module\", \"stmts\": [{\"type\": "
                             "\"call_expr\", \"callee\": {\"type\": "
                             "\"member_access\", \"base\": {\"type\": "
                             "\"identifier_expr\", \"str\": \"console\"}, "
                             "\"member\": \"log\"}, \"args\": {\"type\": "
                             "\"argument_list\", \"values\": [{\"type\": "
                             "\"add\", \"lhs\": {\"type\": \"int_literal\", "
                             "\"val\": \"1\"}, \"rhs\": {\"type\": "
                             "\"int_literal\", \"val\": \"2\"}}]}}]}");
  XFAIL("console.log(...[1,2,3])");
}

TEST_F(parser_test, computed_member_access) {
  ASSERT_PARSED_MATCHES_JSON(
      "a[1]", "{\"type\": \"module\", \"stmts\": [{\"type\": "
              "\"computed_member_access\", \"base\": {\"type\": "
              "\"identifier_expr\", \"str\": \"a\"}, \"member\": {\"type\": "
              "\"int_literal\", \"val\": \"1\"}}]}");
  ASSERT_PARSED_MATCHES_JSON(
      "a[1+4/2]", "{\"type\": \"module\", \"stmts\": [{\"type\": "
                  "\"computed_member_access\", \"base\": {\"type\": "
                  "\"identifier_expr\", \"str\": \"a\"}, \"member\": "
                  "{\"type\": \"add\", \"lhs\": {\"type\": \"int_literal\", "
                  "\"val\": \"1\"}, \"rhs\": {\"type\": \"divide\", \"lhs\": "
                  "{\"type\": \"int_literal\", \"val\": \"4\"}, \"rhs\": "
                  "{\"type\": \"int_literal\", \"val\": \"2\"}}}}]}");
}

TEST_F(parser_test, comma_operator) {
  ASSERT_PARSED_MATCHES_JSON(
      "window[1,'console'].log(4)",
      "{\"type\": \"module\", \"stmts\": [{\"type\": \"call_expr\", "
      "\"callee\": {\"type\": \"member_access\", \"base\": {\"type\": "
      "\"computed_member_access\", \"base\": {\"type\": \"identifier_expr\", "
      "\"str\": \"window\"}, \"member\": {\"type\": \"comma_operator\", "
      "\"lhs\": {\"type\": \"int_literal\", \"val\": \"1\"}, \"rhs\": "
      "{\"type\": \"string_literal\", \"val\": \"'console'\"}}}, \"member\": "
      "\"log\"}, \"args\": "
      "{\"type\": \"argument_list\", \"values\": [{\"type\": \"int_literal\", "
      "\"val\": \"4\"}]}}]}");
}

TEST_F(parser_test, arrow_function) {
  ASSERT_PARSED_MATCHES_JSON(
      "() => {}", "{\"type\": \"module\", \"stmts\": [{\"type\": "
                  "\"arrow_function\", \"params\": {\"type\": \"param_list\", "
                  "\"names\": [], \"rest\": null}, \"body\": {\"type\": "
                  "\"block\", \"stmts\": []}}]}");
  ASSERT_PARSED_MATCHES_JSON(
      "(test) => console.log(test)",
      "{\"type\": \"module\", \"stmts\": [{\"type\": "
      "\"arrow_function\", \"params\": {\"type\": \"param_list\", "
      "\"names\": [\"test\"], \"rest\": null}, \"body\": {\"type\": "
      "\"call_expr\", \"callee\": {\"type\": \"member_access\", \"base\": "
      "{\"type\": \"identifier_expr\", \"str\": \"console\"}, \"member\": "
      "\"log\"}, \"args\": {\"type\": \"argument_list\", \"values\": "
      "[{\"type\": \"identifier_expr\", \"str\": \"test\"}]}}}]}");
  ASSERT_PARSED_MATCHES_JSON(
      "() => ({})",
      MOD_WRAP("{\"type\": "
               "\"arrow_function\", \"params\": {\"type\": \"param_list\", "
               "\"names\": [], \"rest\": null}, \"body\": {\"type\": "
               "\"object_literal\", \"entries\": []}}"));
  // rest
  ASSERT_PARSED_MATCHES_JSON(
      "(...args) => null",
      MOD_WRAP("{\"type\": \"arrow_function\", \"params\": {\"type\": "
               "\"param_list\", \"names\": [], \"rest\": \"args\"}, \"body\": "
               "{\"type\": \"null_literal\"}}"));
}

TEST_F(parser_test, assignment) {
  ASSERT_PARSED_MATCHES_JSON(
      "a = b = c = 1 * 3",
      "{\"type\": \"module\", \"stmts\": [{\"type\": \"assign\", \"lhs\": "
      "{\"type\": \"identifier_expr\", \"str\": \"a\"}, \"rhs\": {\"type\": "
      "\"assign\", \"lhs\": {\"type\": \"identifier_expr\", \"str\": \"b\"}, "
      "\"rhs\": {\"type\": \"assign\", \"lhs\": {\"type\": "
      "\"identifier_expr\", \"str\": \"c\"}, \"rhs\": {\"type\": \"multiply\", "
      "\"lhs\": {\"type\": \"int_literal\", \"val\": \"1\"}, \"rhs\": "
      "{\"type\": \"int_literal\", \"val\": \"3\"}}}}}]}");
  ASSERT_PARSED_MATCHES_JSON(
      "a[i].x = b[j].y",
      MOD_WRAP("{\"type\": \"assign\", \"lhs\": {\"type\": \"member_access\", "
               "\"base\": {\"type\": \"computed_member_access\", \"base\": "
               "{\"type\": \"identifier_expr\", \"str\": \"a\"}, \"member\": "
               "{\"type\": \"identifier_expr\", \"str\": \"i\"}}, \"member\": "
               "\"x\"}, \"rhs\": {\"type\": \"member_access\", \"base\": "
               "{\"type\": \"computed_member_access\", \"base\": {\"type\": "
               "\"identifier_expr\", \"str\": \"b\"}, \"member\": {\"type\": "
               "\"identifier_expr\", \"str\": \"j\"}}, \"member\": \"y\"}}"));
  XWRONG("[a] = arr",
         MOD_WRAP("{\"type\": \"array_destruct\", \"lhs\": {\"type\": "
                  "\"array_destruct_keys\", \"keys\": [{\"type\": "
                  "\"array_destruct_key\", \"key\": \"a\", \"init\": null}], "
                  "\"rest\": null}, \"rhs\": {\"type\": \"identifier\", "
                  "\"str\": \"arr\"}}"));
  XFAIL("let [a] = arr");
  XFAIL("let {a} = obj");
}
TEST_F(parser_test, if_stmt) {
  ASSERT_PARSED_MATCHES_JSON(
      "if (false) if (false) 1; else 2;",
      MOD_WRAP("{\"type\": \"if_stmt\", \"condition\": {\"type\": "
               "\"false_literal\"}, \"body\": {\"type\": "
               "\"if_stmt\", \"condition\": {\"type\": \"false_literal\"}, "
               "\"body\": {\"type\": \"int_literal\", "
               "\"val\": \"1\"}, \"else_stmt\": {\"type\": \"int_literal\", "
               "\"val\": \"2\"}}, \"else_stmt\": null}"));
}
TEST_F(parser_test, do_while) {
  ASSERT_PARSED_MATCHES_JSON(
      "do 1; while (false);",
      MOD_WRAP("{\"type\": \"do_while\", \"condition\": {\"type\": "
               "\"false_literal\"}, \"body\": {\"type\": "
               "\"int_literal\", \"val\": \"1\"}}"));
}
TEST_F(parser_test, while_stmt) {
  ASSERT_PARSED_MATCHES_JSON(
      "while(false) { 1; }",
      MOD_WRAP("{\"type\": \"while_stmt\", \"condition\": {\"type\": "
               "\"false_literal\"}, \"body\": {\"type\": "
               "\"block\", \"stmts\": [{\"type\": \"int_literal\", \"val\": "
               "\"1\"}]}}"));
}
TEST_F(parser_test, for_stmts) {
  ASSERT_PARSED_MATCHES_JSON(
      "for (var i = 0; i < 10; ++i) 1;",
      MOD_WRAP("{\"type\": \"for_stmt\", \"pre_stmt\": {\"type\": "
               "\"var_decl\", \"keyword\": \"var\", \"parts\": [{\"type\": "
               "\"var_decl_part\", \"name\": \"i\", \"init\": {\"type\": "
               "\"int_literal\", \"val\": \"0\"}}]}, \"condition\": {\"type\": "
               "\"less_expr\", \"lhs\": {\"type\": \"identifier_expr\", "
               "\"str\": \"i\"}, \"rhs\": {\"type\": \"int_literal\", \"val\": "
               "\"10\"}}, \"latch_stmt\": {\"type\": \"prefix_increment\", "
               "\"value\": {\"type\": \"identifier_expr\", \"str\": \"i\"}}, "
               "\"body\": {\"type\": \"int_literal\", \"val\": \"1\"}}"));
  ASSERT_PARSED_MATCHES_JSON(
      "for (let i in [1, 2, 3]) 1;",
      MOD_WRAP("{\"type\": \"for_in\", \"keyword\": \"let\", \"var\": \"i\", "
               "\"iterable\": {\"type\": \"array_literal\", \"values\": "
               "[{\"type\": \"int_literal\", \"val\": \"1\"}, {\"type\": "
               "\"int_literal\", \"val\": \"2\"}, {\"type\": \"int_literal\", "
               "\"val\": \"3\"}]}, \"body\": {\"type\": \"int_literal\", "
               "\"val\": \"1\"}}"));
  ASSERT_PARSED_MATCHES_JSON(
      "for (let i of [1, 2, 3]) 1;",
      MOD_WRAP("{\"type\": \"for_of\", \"keyword\": \"let\", \"var\": \"i\", "
               "\"iterable\": {\"type\": \"array_literal\", \"values\": "
               "[{\"type\": \"int_literal\", \"val\": \"1\"}, {\"type\": "
               "\"int_literal\", \"val\": \"2\"}, {\"type\": \"int_literal\", "
               "\"val\": \"3\"}]}, \"body\": {\"type\": \"int_literal\", "
               "\"val\": \"1\"}}"));
  ASSERT_PARSED_MATCHES_JSON(
      "for (i of [1, 2, 3]) 1;",
      MOD_WRAP("{\"type\": \"for_of\", \"keyword\": null, \"var\": \"i\", "
               "\"iterable\": {\"type\": \"array_literal\", \"values\": "
               "[{\"type\": \"int_literal\", \"val\": \"1\"}, {\"type\": "
               "\"int_literal\", \"val\": \"2\"}, {\"type\": \"int_literal\", "
               "\"val\": \"3\"}]}, \"body\": {\"type\": \"int_literal\", "
               "\"val\": \"1\"}}"));
}
TEST_F(parser_test, throw_stmt) {
  ASSERT_PARSED_MATCHES_JSON(
      "throw {a}", MOD_WRAP("{\"type\": \"throw_stmt\", \"value\": {\"type\": "
                            "\"object_literal\", \"entries\": [{\"type\": "
                            "\"identifier_expr\", \"str\": \"a\"}]}}"));
}
TEST_F(parser_test, new_test) {
  ASSERT_PARSED_MATCHES_JSON("new.target",
                             MOD_WRAP("{\"type\": \"new_target\"}"));
  ASSERT_PARSED_MATCHES_JSON(
      "new target",
      MOD_WRAP("{\"type\": \"new_expr\", \"constructor\": {\"type\": "
               "\"identifier_expr\", \"str\": \"target\"}, \"args\": null}"));
  ASSERT_PARSED_MATCHES_JSON(
      "new target()",
      MOD_WRAP("{\"type\": \"new_expr\", \"constructor\": {\"type\": "
               "\"identifier_expr\", \"str\": \"target\"}, \"args\": "
               "{\"type\": \"argument_list\", \"values\": []}}"));
  ASSERT_PARSED_MATCHES_JSON(
      "new target(1, 2)",
      MOD_WRAP("{\"type\": \"new_expr\", \"constructor\": {\"type\": "
               "\"identifier_expr\", \"str\": \"target\"}, \"args\": "
               "{\"type\": \"argument_list\", \"values\": [{\"type\": "
               "\"int_literal\", \"val\": \"1\"}, {\"type\": \"int_literal\", "
               "\"val\": \"2\"}]}}"));
}
TEST_F(parser_test, try_catch) {
  ASSERT_PARSED_MATCHES_JSON(
      "try {} catch(e) {} finally {}",
      MOD_WRAP("{\"type\": \"try_stmt\", \"body\": {\"type\": \"block\", "
               "\"stmts\": []}, \"catch_block\": {\"type\": \"catch\", "
               "\"var\": \"e\", \"body\": {\"type\": \"block\", \"stmts\": "
               "[]}}, \"finally\": {\"type\": \"block\", \"stmts\": []}}"));
  ASSERT_PARSED_MATCHES_JSON(
      "try {} catch(e) {}",
      MOD_WRAP("{\"type\": \"try_stmt\", \"body\": {\"type\": \"block\", "
               "\"stmts\": []}, \"catch_block\": {\"type\": \"catch\", "
               "\"var\": \"e\", \"body\": {\"type\": \"block\", \"stmts\": "
               "[]}}, \"finally\": null}"));
  ASSERT_PARSED_MATCHES_JSON(
      "try {} finally {}",
      MOD_WRAP("{\"type\": \"try_stmt\", \"body\": {\"type\": \"block\", "
               "\"stmts\": []}, \"catch_block\": null, \"finally\": {\"type\": "
               "\"block\", \"stmts\": []}}"));
  PARSER_ERROR("try {}");
  PARSER_ERROR("try {} catch(e1) {} catch(e2) {}");
  PARSER_ERROR("try {} finally {} catch(e) {}");
  PARSER_ERROR("try {} finally {} finally {}");
}
TEST_F(parser_test, switch_stmt) {
  ASSERT_PARSED_MATCHES_JSON(
      "switch(1) {}",
      MOD_WRAP("{\"type\": \"switch_stmt\", \"value\": {\"type\": "
               "\"int_literal\", \"val\": \"1\"}, \"clauses\": []}"));
  ASSERT_PARSED_MATCHES_JSON(
      "switch(1) {case 2: 3; break; 4; break; default: 5;}",
      MOD_WRAP("{\"type\": \"switch_stmt\", \"value\": {\"type\": "
               "\"int_literal\", \"val\": \"1\"}, \"clauses\": [{\"type\": "
               "\"case\", \"stmts\": [{\"type\": \"int_literal\", \"val\": "
               "\"3\"}, {\"type\": \"break_stmt\", \"label\": null}, "
               "{\"type\": \"int_literal\", \"val\": \"4\"}, {\"type\": "
               "\"break_stmt\", \"label\": null}], \"condition\": {\"type\": "
               "\"int_literal\", \"val\": \"2\"}}, {\"type\": "
               "\"switch_clause\", \"stmts\": [{\"type\": \"int_literal\", "
               "\"val\": \"5\"}]}]}"));
  ASSERT_PARSED_MATCHES_JSON(
      "switch(val) {case void function() {return 1;}(): console.log(1)}",
      MOD_WRAP("{\"type\": \"switch_stmt\", \"value\": {\"type\": "
               "\"identifier_expr\", \"str\": \"val\"}, \"clauses\": "
               "[{\"type\": \"case\", \"stmts\": [{\"type\": \"call_expr\", "
               "\"callee\": {\"type\": \"member_access\", \"base\": {\"type\": "
               "\"identifier_expr\", \"str\": \"console\"}, \"member\": "
               "\"log\"}, \"args\": {\"type\": \"argument_list\", \"values\": "
               "[{\"type\": \"int_literal\", \"val\": \"1\"}]}}], "
               "\"condition\": {\"type\": \"void_expr\", \"value\": {\"type\": "
               "\"call_expr\", \"callee\": {\"type\": \"function_expr\", "
               "\"name\": null, \"params\": {\"type\": \"param_list\", "
               "\"names\": [], \"rest\": null}, \"body\": {\"type\": "
               "\"block\", \"stmts\": [{\"type\": \"return_stmt\", \"value\": "
               "{\"type\": \"int_literal\", \"val\": \"1\"}}]}}, \"args\": "
               "{\"type\": \"argument_list\", \"values\": []}}}}]}"));
  PARSER_ERROR("switch(){}");
  PARSER_ERROR("switch(1){default: 2; default: 3;}");
}
TEST_F(parser_test, classes) {
  XFAIL("class test {}");
  XFAIL("class test { constructor() {} }");
  XFAIL("class test { static foo() {} }");
}
TEST_F(parser_test, import_stmt) {
  XFAIL("import * as Test from 'test'");
  XFAIL("import Test from 'test'");
  XFAIL("import { Test } from 'test'");
  XFAIL("import { Test as tseT } from 'test'");
}
TEST_F(parser_test, export_stmt) {
  XFAIL("export * from 'test'");
  XFAIL("export { Test as tseT }");
  XFAIL("export { Test } from 'test'");
  XFAIL("export var i = 0");
  XFAIL("export default class test {}");
}
