#include "jnsn/js/ast_ops.h"
#include <sstream>

using namespace jnsn;
using namespace std;

struct json_escape {
  std::string_view str;
  json_escape(std::string_view str) : str(str) {}
};
static std::ostream &operator<<(std::ostream &stream,
                                const json_escape escape) {
  for (auto c : escape.str) {
    if (c == '"') {
      stream << '\\';
    }
    stream << c;
  }
  return stream;
}

/// Returns the number of children that were printed for the top-level
/// node
struct parent_json_printer : public const_ast_node_visitor<void> {
  std::ostream &stream;
  const_ast_node_visitor<void> &printer;
  parent_json_printer(std::ostream &stream,
                      const_ast_node_visitor<void> &printer)
      : stream(stream), printer(printer) {}

#define CHILDREN(...)                                                          \
  do {                                                                         \
    __VA_ARGS__                                                                \
  } while (false)

#define MANY(OF, NAME)                                                         \
  stream << ", \"" #NAME "\": [";                                              \
  for (size_t i = 0; i < node.NAME.size(); ++i) {                              \
    if (i != 0) {                                                              \
      stream << ", ";                                                          \
    }                                                                          \
    node.NAME[i]->accept(printer);                                             \
  }                                                                            \
  stream << "]";

#define ONE(OF, NAME)                                                          \
  stream << ", \"" #NAME "\": ";                                               \
  node.NAME->accept(printer);

#define MAYBE(OF, NAME)                                                        \
  stream << ", \"" #NAME "\": ";                                               \
  if (node.NAME) {                                                             \
    (*node.NAME)->accept(printer);                                             \
  } else {                                                                     \
    stream << "null";                                                          \
  }

#define MAYBE_STR(NAME)                                                        \
  stream << ", \"" #NAME << "\": ";                                            \
  if (node.NAME) {                                                             \
    stream << "\"" << json_escape{*node.NAME} << "\"";                         \
  } else {                                                                     \
    stream << "null";                                                          \
  }

#define STRINGS(NAME)                                                          \
  stream << ", \"" #NAME "\": [";                                              \
  for (size_t i = 0; i < node.NAME.size(); ++i) {                              \
    if (i != 0) {                                                              \
      stream << ", ";                                                          \
    }                                                                          \
    stream << "\"" << json_escape{node.NAME[i]} << "\"";                       \
  }                                                                            \
  stream << "]";

#define STRING(NAME)                                                           \
  stream << ", \"" #NAME "\": \"" << json_escape{node.NAME} << "\"";

#define NODE(NAME, CHILD_NODES)                                                \
  void accept(const NAME##_node &node) override { CHILD_NODES; }
#define EXTENDS(NAME) NAME##_node
#define DERIVED(NAME, ANCESTOR, CHILD_NODES)                                   \
  void accept(const NAME##_node &node) override {                              \
    accept(static_cast<const ANCESTOR &>(node));                               \
    CHILD_NODES;                                                               \
  }

#include "jnsn/js/ast.def"
};

struct json_printer : public const_ast_node_visitor<void> {
  std::ostream &stream;
  parent_json_printer parent_printer;
  json_printer(std::ostream &stream)
      : stream(stream), parent_printer(stream, *this) {}

#define CHILDREN(...)                                                          \
  do {                                                                         \
    __VA_ARGS__                                                                \
  } while (false)

#define MANY(OF, NAME)                                                         \
  stream << ", \"" #NAME "\": [";                                              \
  for (size_t i = 0; i < node.NAME.size(); ++i) {                              \
    if (i != 0) {                                                              \
      stream << ", ";                                                          \
    }                                                                          \
    node.NAME[i]->accept(*this);                                               \
  }                                                                            \
  stream << "]";

#define ONE(OF, NAME)                                                          \
  stream << ", \"" #NAME "\": ";                                               \
  node.NAME->accept(*this);

#define MAYBE(OF, NAME)                                                        \
  stream << ", \"" #NAME "\": ";                                               \
  if (node.NAME) {                                                             \
    visit(**node.NAME);                                                        \
  } else {                                                                     \
    stream << "null";                                                          \
  }

#define MAYBE_STR(NAME)                                                        \
  stream << ", \"" #NAME << "\": ";                                            \
  if (node.NAME) {                                                             \
    stream << "\"" << json_escape{*node.NAME} << "\"";                         \
  } else {                                                                     \
    stream << "null";                                                          \
  }

#define STRINGS(NAME)                                                          \
  stream << ", \"" #NAME "\": [";                                              \
  for (size_t i = 0; i < node.NAME.size(); ++i) {                              \
    if (i != 0) {                                                              \
      stream << ", ";                                                          \
    }                                                                          \
    stream << "\"" << json_escape{node.NAME[i]} << "\"";                       \
  }                                                                            \
  stream << "]";

#define STRING(NAME)                                                           \
  stream << ", \"" #NAME "\": \"" << json_escape{node.NAME} << "\"";

#define NODE(NAME, CHILD_NODES)                                                \
  void accept(const NAME##_node &node) override {                              \
    stream << "{";                                                             \
    stream << "\"type\": "                                                     \
           << "\"" #NAME << "\"";                                              \
    CHILD_NODES;                                                               \
    stream << "}";                                                             \
  }
#define EXTENDS(NAME) NAME##_node
#define DERIVED(NAME, ANCESTOR, CHILD_NODES)                                   \
  void accept(const NAME##_node &node) override {                              \
    stream << "{";                                                             \
    stream << "\"type\": "                                                     \
           << "\"" #NAME << "\"";                                              \
    parent_printer.accept(static_cast<const ANCESTOR &>(node));                \
    CHILD_NODES;                                                               \
    stream << "}";                                                             \
  }
#include "jnsn/js/ast.def"
};

namespace jnsn {
std::ostream &operator<<(std::ostream &stream, const ast_to_json &wrapper) {
  json_printer printer{stream};
  printer.visit(*wrapper.ast);
  return stream;
}
} // namespace jnsn

/// isa<> impl
template <class nodety>
struct isa_checker : public const_ast_node_visitor<bool> {
#define NODE(NAME, CHILDREN)                                                   \
  bool accept(const NAME##_node &) override {                                  \
    return std::is_same_v<NAME##_node, nodety>;                                \
  }
#define EXTENDS(BASE) BASE##_node
#define DERIVED(NAME, BASE, CHILDREN)                                          \
  bool accept(const NAME##_node &node) {                                       \
    return std::is_same_v<NAME##_node, nodety> ||                              \
           accept(static_cast<const BASE &>(node));                            \
  }
#include "jnsn/js/ast.def"
};

namespace jnsn {
#define NODE(NAME, CHILDREN)                                                   \
  template <> bool isa<NAME##_node>(const ast_node *node) {                    \
    assert(node);                                                              \
    return isa_checker<NAME##_node>().visit(*node);                            \
  }
#define DERIVED(NAME, EXTENDS, CHILDREN) NODE(NAME, CHILDREN)
#include "jnsn/js/ast.def"

#define NODE(NAME, CHILD_NODES) static const char *NAME##_name = #NAME;
#define DERIVED(NAME, ANCESTOR, CHILD_NODES) NODE(NAME, CHILD_NODES)
#include "jnsn/js/ast.def"
const char *get_ast_node_typename(const ast_node &node) {
  struct ast_node_name_generator : public const_ast_node_visitor<const char *>{
#define NODE(NAME, CHILD_NODES)                                                \
  const char *accept(const NAME##_node &node) override { return NAME##_name; }
#define DERIVED(NAME, ANCESTOR, CHILD_NODES) NODE(NAME, CHILD_NODES)
#include "jnsn/js/ast.def"
                                   } name_gen;
  return name_gen.visit(node);
}
} // namespace jnsn
