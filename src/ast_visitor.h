#ifndef AST_VISITOR_H
#define AST_VISITOR_H

#include "wasm_ast.h"

#include <cassert>

namespace wasm {

// Visitor-ish pattern; abstracts the data layout of Expression, but driven by
// the implementation. Visit is the entry. The implementation of VisitModule is
// expected to call Visit{Function,Segment,Import}. The VisitFunction impl calls
// VisitExpression.

// The base VisitExpression implementation calls the apropriate derived-class
// function for the expression type and passes in the relevant arguments. The
// VisitFoo implementation is expected to then call VisitExpression the
// expressions therein, but it can do whatever it needs to before and/or after
// that.

// TODO: We could do that same thing with VisitModule (e.g. pass it lists of
// functions, segments, imports, exports) to make it uniform. Realistically it
// might make more sense just to keep the extra data available to the derived
// implementations though.

template <typename ModuleVal, typename ExprVal>
class AstVisitor {
public:
 ModuleVal Visit(const Module& mod) { return VisitModule(mod); }
 ExprVal Visit(TestScriptExpr* script) { return VisitTestScriptExpr(script); }

 protected:
  virtual ModuleVal VisitModule(const Module& mod) {
    for (auto& func : mod.functions)
      VisitFunction(*func);

    if (mod.initial_memory_size) {
      for (auto& seg : mod.segments)
        VisitSegment(*seg);
    }
    for (auto& imp : mod.imports)
      VisitImport(*imp);
    for (auto& ex : mod.exports)
      VisitExport(*ex);
    return ModuleVal();
  }
  virtual void VisitImport(const Import& imp) {}
  virtual void VisitExport(const Export& exp) {}
  virtual void VisitFunction(const Function& func) {
    for (auto& expr : func.body)
      VisitExpression(expr.get());
  }
  virtual void VisitSegment(const Segment& seg) {}

  virtual ExprVal VisitExpression(Expression* expr) {
    switch (expr->opcode) {
      case WASM_OPCODE_NOP:
        return VisitNop(expr);
      case WASM_OPCODE_BLOCK:
        return VisitBlock(expr, &expr->exprs);
      case WASM_OPCODE_IF:
        return VisitIf(expr, expr->exprs[0].get(), expr->exprs[1].get(),
                       expr->exprs.size() > 2 ? expr->exprs[2].get() : nullptr);
      case WASM_OPCODE_CALL:
        return VisitCall(expr, expr->is_import, expr->callee,
                         expr->callee_index, &expr->exprs);
      case WASM_OPCODE_RETURN:
        assert(expr->exprs.size() <= 1);
        // TODO: if multiple returns are really gone, do something better
        return VisitReturn(expr, &expr->exprs);
      case WASM_OPCODE_GET_LOCAL:
        return VisitGetLocal(expr, expr->local_var);
      case WASM_OPCODE_SET_LOCAL:
        return VisitSetLocal(expr, expr->local_var, expr->exprs.front().get());
      case WASM_OPCODE_I8_CONST:
      case WASM_OPCODE_I32_CONST:
      case WASM_OPCODE_I64_CONST:
      case WASM_OPCODE_F32_CONST:
      case WASM_OPCODE_F64_CONST:
        return VisitConst(expr, &expr->literal);
      case WASM_OPCODE_I32_CLZ:
      case WASM_OPCODE_I32_CTZ:
      case WASM_OPCODE_I32_POPCNT:
      case WASM_OPCODE_I64_CLZ:
      case WASM_OPCODE_I64_CTZ:
      case WASM_OPCODE_I64_POPCNT:
      case WASM_OPCODE_F32_NEG:
      case WASM_OPCODE_F32_ABS:
      case WASM_OPCODE_F32_CEIL:
      case WASM_OPCODE_F32_FLOOR:
      case WASM_OPCODE_F32_TRUNC:
      case WASM_OPCODE_F32_NEAREST:
      case WASM_OPCODE_F32_SQRT:
      case WASM_OPCODE_F64_NEG:
      case WASM_OPCODE_F64_ABS:
      case WASM_OPCODE_F64_CEIL:
      case WASM_OPCODE_F64_FLOOR:
      case WASM_OPCODE_F64_TRUNC:
      case WASM_OPCODE_F64_NEAREST:
      case WASM_OPCODE_F64_SQRT:
        return VisitUnop(expr, expr->unop, expr->exprs.front().get());
      case WASM_OPCODE_I32_ADD:
      case WASM_OPCODE_I32_SUB:
      case WASM_OPCODE_I32_MUL:
      case WASM_OPCODE_I32_SDIV:
      case WASM_OPCODE_I32_UDIV:
      case WASM_OPCODE_I32_SREM:
      case WASM_OPCODE_I32_UREM:
      case WASM_OPCODE_I32_AND:
      case WASM_OPCODE_I32_OR:
      case WASM_OPCODE_I32_XOR:
      case WASM_OPCODE_I32_SHL:
      case WASM_OPCODE_I32_SHR:
      case WASM_OPCODE_I32_SAR:
      case WASM_OPCODE_I64_ADD:
      case WASM_OPCODE_I64_SUB:
      case WASM_OPCODE_I64_MUL:
      case WASM_OPCODE_I64_SDIV:
      case WASM_OPCODE_I64_UDIV:
      case WASM_OPCODE_I64_SREM:
      case WASM_OPCODE_I64_UREM:
      case WASM_OPCODE_I64_AND:
      case WASM_OPCODE_I64_OR:
      case WASM_OPCODE_I64_XOR:
      case WASM_OPCODE_I64_SHL:
      case WASM_OPCODE_I64_SHR:
      case WASM_OPCODE_I64_SAR:
      case WASM_OPCODE_F32_ADD:
      case WASM_OPCODE_F32_SUB:
      case WASM_OPCODE_F32_MUL:
      case WASM_OPCODE_F32_DIV:
      case WASM_OPCODE_F32_COPYSIGN:
      case WASM_OPCODE_F32_MIN:
      case WASM_OPCODE_F32_MAX:
      case WASM_OPCODE_F64_ADD:
      case WASM_OPCODE_F64_SUB:
      case WASM_OPCODE_F64_MUL:
      case WASM_OPCODE_F64_DIV:
      case WASM_OPCODE_F64_COPYSIGN:
      case WASM_OPCODE_F64_MIN:
      case WASM_OPCODE_F64_MAX:
        return VisitBinop(expr, expr->binop, expr->exprs[0].get(),
                          expr->exprs[1].get());
      case WASM_OPCODE_I32_EQ:
      case WASM_OPCODE_I32_NE:
      case WASM_OPCODE_I32_SLT:
      case WASM_OPCODE_I32_SLE:
      case WASM_OPCODE_I32_ULT:
      case WASM_OPCODE_I32_ULE:
      case WASM_OPCODE_I32_SGT:
      case WASM_OPCODE_I32_UGT:
      case WASM_OPCODE_I32_SGE:
      case WASM_OPCODE_I32_UGE:
      case WASM_OPCODE_I64_EQ:
      case WASM_OPCODE_I64_NE:
      case WASM_OPCODE_I64_SLT:
      case WASM_OPCODE_I64_SLE:
      case WASM_OPCODE_I64_ULT:
      case WASM_OPCODE_I64_ULE:
      case WASM_OPCODE_I64_SGT:
      case WASM_OPCODE_I64_UGT:
      case WASM_OPCODE_I64_SGE:
      case WASM_OPCODE_I64_UGE:
      case WASM_OPCODE_F32_EQ:
      case WASM_OPCODE_F32_NE:
      case WASM_OPCODE_F32_LT:
      case WASM_OPCODE_F32_LE:
      case WASM_OPCODE_F32_GT:
      case WASM_OPCODE_F32_GE:
      case WASM_OPCODE_F64_EQ:
      case WASM_OPCODE_F64_NE:
      case WASM_OPCODE_F64_LT:
      case WASM_OPCODE_F64_LE:
      case WASM_OPCODE_F64_GT:
      case WASM_OPCODE_F64_GE:
        return VisitCompare(expr, expr->compare_type, expr->relop,
                            expr->exprs[0].get(), expr->exprs[1].get());
      default:
        assert(false);
    }
  }
  virtual ExprVal VisitNop(Expression* expr) { return ExprVal(); }
  virtual ExprVal VisitBlock(Expression* expr,
                             UniquePtrVector<Expression>* exprs) {
    for (auto& e : *exprs)
      VisitExpression(e.get());
    return ExprVal();
  }
  virtual ExprVal VisitIf(Expression* expr,
                          Expression* condition,
                          Expression* then,
                          Expression* els) {
    VisitExpression(condition);
    VisitExpression(then);
    if (els)
      VisitExpression(els);
    return ExprVal();
  }

  virtual ExprVal VisitCall(Expression* expr,
                            bool is_import,
                            Callable* callee,
                            int callee_index,
                            UniquePtrVector<Expression>* args) {
    for (auto& e : *args)
      VisitExpression(e.get());
    return ExprVal();
  }
  virtual ExprVal VisitReturn(Expression* expr,
                              UniquePtrVector<Expression>* value) {
    if (value->size())
      VisitExpression(value->front().get());
    return ExprVal();
  }
  virtual ExprVal VisitGetLocal(Expression* expr, Variable* var) {
    return ExprVal();
  }
  virtual ExprVal VisitSetLocal(Expression* expr,
                                Variable* var,
                                Expression* value) {
    VisitExpression(value);
    return ExprVal();
  }
  virtual ExprVal VisitConst(Expression* expr, Literal* l) { return ExprVal(); }
  virtual ExprVal VisitUnop(Expression* expr,
                            UnaryOperator unop,
                            Expression* operand) {
    VisitExpression(operand);
    return ExprVal();
  }
  virtual ExprVal VisitBinop(Expression* epxr,
                             BinaryOperator binop,
                             Expression* lhs,
                             Expression* rhs) {
    VisitExpression(lhs);
    VisitExpression(rhs);
    return ExprVal();
  }
  virtual ExprVal VisitCompare(Expression* expr,
                               Type compare_type,
                               CompareOperator relop,
                               Expression* lhs,
                               Expression* rhs) {
    VisitExpression(lhs);
    VisitExpression(rhs);
    return ExprVal();
  }

  ExprVal VisitTestScriptExpr(TestScriptExpr* expr) {
    switch (expr->opcode) {
      case TestScriptExpr::kInvoke:
        return VisitInvoke(expr, expr->callee, &expr->exprs);
      case TestScriptExpr::kAssertReturn:
        return VisitAssertReturn(expr, expr->invoke.get(),
                                 expr->exprs[0].get());
      default:
        assert(false);
    }
  }
  virtual ExprVal VisitInvoke(TestScriptExpr* expr,
                              Export* callee,
                              UniquePtrVector<Expression>* args) {
    for (auto& e : *args)
      VisitExpression(e.get());
    return ExprVal();
  }
  virtual ExprVal VisitAssertReturn(TestScriptExpr* expr,
                                    TestScriptExpr* arg,
                                    Expression* expected) {
    Visit(arg);
    VisitExpression(expected);
    return ExprVal();
  }
};
}

#endif // AST_VISITOR_H
