#include "parsing/ast.h"
#include "parsing/ast_ops.h"
#include <cassert>
#include <iostream>

using namespace parsing;
/// ast_node_store impl
void ast_node_store::clear() {
#define NODE(NAME, CHILD_NODES) NAME##_vec.clear();
#define DERIVED(NAME, ANCESTORS, CHILD_NODES) NAME##_vec.clear();
#include "parsing/ast.def"
}

#define NODE(NAME, CHILD_NODES)                                                \
  NAME##_node *ast_node_store::make_##NAME() {                                 \
    if (NAME##_vec.empty()) {                                                  \
      NAME##_vec.emplace_back(std::make_unique<no_reloc_buf<NAME##_node>>());  \
      NAME##_vec.back()->reserve(1);                                           \
    }                                                                          \
    auto *storage = NAME##_vec.back().get();                                   \
    /* prevent relocation */                                                   \
    auto cap = storage->capacity();                                            \
    if (storage->size() == cap) {                                              \
      NAME##_vec.emplace_back(std::make_unique<no_reloc_buf<NAME##_node>>());  \
      storage = NAME##_vec.back().get();                                       \
      storage->reserve(2 * cap);                                               \
    }                                                                          \
    storage->emplace_back();                                                   \
    return &storage->back();                                                   \
  }
#define DERIVED(NAME, ANCESTORS, CHILD_NODES) NODE(NAME, CHILD_NODES)
#include "parsing/ast.def"

namespace parsing {
std::ostream &operator<<(std::ostream &stream, const ast_node *ref) {
  stream << ast_to_json(ref) << "\n";
  return stream;
}
} // namespace parsing