#ifndef PARSING_AST_VISITOR_H
#define PARSING_AST_VISITOR_H

namespace parsing {

class ast_node_visitor_base;
class const_ast_node_visitor_base;
/// Base class of all ast nodes (for visitor)
struct ast_node {
  virtual void accept(ast_node_visitor_base &v) = 0;
  virtual void accept(const_ast_node_visitor_base &v) const = 0;
};
/// forward declaration of all ast nodes
#define NODE(NAME, CHILD_NODES) class NAME ## _node;
#define DERIVED(NAME, ANCESTORS, CHILD_NODES) class NAME ## _node;
#include "parsing/ast.def"

class ast_node_visitor_base {
  /// make ast nodes friend classes
#define NODE(NAME, CHILD_NODES) friend class NAME ## _node;
#define DERIVED(NAME, ANCESTORS, CHILD_NODES) friend class NAME ## _node;
#include "parsing/ast.def"

#define NODE(NAME, CHILD_NODES) virtual void gen_result(NAME ## _node &node) = 0;
#define DERIVED(NAME, ANCESTORS, CHILD_NODES) virtual void gen_result(NAME ## _node &node) = 0;
#include "parsing/ast.def"
protected:
  void dispatch(ast_node &node) {
    node.accept(*this);
  }
};
class const_ast_node_visitor_base {
  /// make ast nodes friend classes
#define NODE(NAME, CHILD_NODES) friend class NAME ## _node;
#define DERIVED(NAME, ANCESTORS, CHILD_NODES) friend class NAME ## _node;
#include "parsing/ast.def"

#define NODE(NAME, CHILD_NODES) virtual void gen_result(const NAME ## _node &node) = 0;
#define DERIVED(NAME, ANCESTORS, CHILD_NODES) virtual void gen_result(const NAME ## _node &node) = 0;
#include "parsing/ast.def"
protected:
  void dispatch(const ast_node &node) {
    node.accept(*this);
  }
};

template<typename RET_TY>
class ast_node_visitor : public ast_node_visitor_base {
  RET_TY result;
#define NODE(NAME, CHILD_NODES) void gen_result(NAME ## _node &node) override { result = accept(node); }
#define DERIVED(NAME, ANCESTORS, CHILD_NODES) void gen_result(NAME ## _node &node) override { result = accept(node); }
#include "parsing/ast.def"
#define NODE(NAME, CHILD_NODES) virtual RET_TY accept(NAME ## _node &) = 0;
#define DERIVED(NAME, ANCESTORS, CHILD_NODES) virtual RET_TY accept(NAME ## _node &) = 0;
#include "parsing/ast.def"
public:
  RET_TY visit(ast_node &n) {
    dispatch(n);
    return result;
  }
};
template<>
class ast_node_visitor<void> : public ast_node_visitor_base {
#define NODE(NAME, CHILD_NODES) void gen_result(NAME ## _node &node) override { accept(node); }
#define DERIVED(NAME, ANCESTORS, CHILD_NODES) void gen_result(NAME ## _node &node) override { accept(node); }
#include "parsing/ast.def"
#define NODE(NAME, CHILD_NODES) virtual void accept(NAME ## _node &) = 0;
#define DERIVED(NAME, ANCESTORS, CHILD_NODES) virtual void accept(NAME ## _node &) = 0;
#include "parsing/ast.def"
public:
  void visit(ast_node &n) {
    dispatch(n);
  }
};

template<typename RET_TY>
class const_ast_node_visitor : public const_ast_node_visitor_base {
  RET_TY result;
#define NODE(NAME, CHILD_NODES) void gen_result(const NAME ## _node &node) override { result = accept(node); }
#define DERIVED(NAME, ANCESTORS, CHILD_NODES) void gen_result(const NAME ## _node &node) override { result = accept(node); }
#include "parsing/ast.def"
#define NODE(NAME, CHILD_NODES) virtual RET_TY accept(const NAME ## _node &) = 0;
#define DERIVED(NAME, ANCESTORS, CHILD_NODES) virtual RET_TY accept(const NAME ## _node &) = 0;
#include "parsing/ast.def"
public:
  RET_TY visit(const ast_node &n) {
    dispatch(n);
    return result;
  }
};

template<>
class const_ast_node_visitor<void> : public const_ast_node_visitor_base {
#define NODE(NAME, CHILD_NODES) void gen_result(const NAME ## _node &node) override { accept(node); }
#define DERIVED(NAME, ANCESTORS, CHILD_NODES) void gen_result(const NAME ## _node &node) override { accept(node); }
#include "parsing/ast.def"
#define NODE(NAME, CHILD_NODES) virtual void accept(const NAME ## _node &) = 0;
#define DERIVED(NAME, ANCESTORS, CHILD_NODES) virtual void accept(const NAME ## _node &) = 0;
#include "parsing/ast.def"
public:
  void visit(const ast_node &n) {
    dispatch(n);
  }
};

} // namespace parsing

#endif // PARSING_AST_VISITOR_H