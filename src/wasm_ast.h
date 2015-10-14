#ifndef WASM_AST_H
#define WASM_AST_H

#include "wasm.h"

#include <cassert>
#include <memory>
#include <string>
#include <vector>

namespace wasm {

class Callable;
class Module;
template <typename T>
using UniquePtrVector = std::vector<std::unique_ptr<T>>;

static_assert(WASM_TYPE_ALL == 15, "wasm::Type enum needs to be adjusted");
// Type is basically just an enum, but implicitly convertible from WasmType.
// It also includes an "unknown" state used during construction/type fixup.
class Type {
 public:
  typedef enum Type_ {
    kVoid = WASM_TYPE_VOID,
    kI32 = WASM_TYPE_I32,
    kI64 = WASM_TYPE_I64,
    kF32 = WASM_TYPE_F32,
    kF64 = WASM_TYPE_F64,
    kAny = WASM_TYPE_ALL,  // TODO: Is this useful? could just void
    kUnknown,
  } Type_;
  Type(Type_ t) : value_(t) {}
  Type(WasmType t) : value_(static_cast<Type_>(t)) {
    assert(t < WASM_TYPE_ALL && "Bad Type initializer");
  }
  operator Type_() const { return value_; }
  explicit operator WasmType() const { return static_cast<WasmType>(value_); }

 private:
  Type_ value_ = kUnknown;
};

// TODO: do we need a hierarchy of operators like the spec?
enum UnaryOperator {
  // Int
  kClz,
  kCtz,
  kPopcnt,
  // FP
  kNeg,
  kAbs,
  kCeil,
  kFloor,
  kTrunc,
  kNearest,
  kSqrt,
};

enum BinaryOperator {
  kAdd,
  kSub,
  kMul,
  // Int
  kDivS,
  kDivU,
  kRemS,
  kRemU,
  kAnd,
  kOr,
  kXor,
  kShl,
  kShrU,
  kShrS,
  // FP
  kDiv,
  kCopySign,
  kMin,
  kMax,
};

enum CompareOperator {
  kEq,
  kNE,
  // Int
  kLtS,
  kLtU,
  kLeS,
  kLeU,
  kGtS,
  kGtU,
  kGeS,
  kGeU,
  // FP
  kLt,
  kLe,
  kGt,
  kGe,
};

enum ConversionOperator {
  // Int
  kExtendSInt32,
  kExtendUInt32,
  kWrapInt64,
  kTruncSFloat32,
  kTruncUFloat32,
  kTruncSFloat64,
  kTruncUFloat64,
  kReinterpretFloat,
  // FP
  kConvertSInt32,
  kConvertUInt32,
  kConvertSInt64,
  kConvertUInt64,
  kPromoteFloat32,
  kDemoteFloat64,
  kReinterpretInt,
};

class Literal {
 public:
  Type type = Type::kUnknown;
  union {
    uint32_t i32;
    uint64_t i64;
    float f32;
    double f64;
  } value;
};

class Variable {
 public:
  Variable(Type t) : type(t) {}
  Type type = Type::kUnknown;
  int index;
  std::string local_name;  // Empty if none bound
};

// There's only one Expression class, which has all the possible data members,
// some of which are common. This avoids the need to make it virtual or use any
// kind of RTTI. Only AstVisitor::VisitExpression and the parser actually need
// to care about the kind, and the use of these members by the kinds.
class Expression {
 public:
  enum ExpressionKind {
    kNop,
    kBlock,
    kIf,
    kCallDirect,
    kReturn,
    kGetLocal,
    kSetLocal,
    kConst,
    kUnary,
    kBinary,
    kCompare,
    kConvert,
  };
  Expression(ExpressionKind k) : kind(k) {}
  // Common
  ExpressionKind kind;
  Type expr_type = Type::kUnknown;
  Type expected_type = Type::kUnknown;
  // Const
  Literal literal = {};
  // Call, CallImport
  int callee_index = 0;
  bool is_import = false;
  Callable* callee;
  // get_local, set_local variable
  Variable* local_var;
  // Unops
  UnaryOperator unop;
  // Binops
  BinaryOperator binop;
  // Compare
  Type compare_type = Type::kUnknown;
  CompareOperator relop;
  // Convert
  ConversionOperator cvt;
  //   Technically redundant with cvt, but handy to have.
  Type operand_type = Type::kUnknown;
  // Common (block, call args, return/set_local vals, compare operands,
  // conversion operand)
  UniquePtrVector<Expression> exprs;
};

class Callable {
 public:
  Callable(Type t) : result_type(t) {}
  Type result_type = Type::kVoid;
  std::string local_name;  // Empty if none bound
  UniquePtrVector<Variable> locals;  // Includes the args at the front
  std::vector<Variable*> args;       // Convenience pointers to the args
};

class Function : public Callable {
 public:
  Function(Type t, int idx) : Callable(t), index_in_module(idx) {}
  UniquePtrVector<Expression> body;
  int index_in_module = 0;
};

class Export {
 public:
  Export(Function* f, const std::string& n, Module* m)
      : function(f), name(n), module(m) {}
  Function* function;
  std::string name;
  Module* module;
};

class Import : public Callable {
 public:
  Import(Type t, const std::string& m, const std::string& f)
      : Callable(t), module_name(m), func_name(f) {}
  std::string module_name;
  std::string func_name;
};

class Segment {
 public:
  Segment(size_t sz, size_t addr) : size(sz), address(addr) {}
  std::string as_string() const;
  size_t size = 0;
  size_t address = 0;
  std::vector<char> initial_data;
};

class Module {
 public:
  UniquePtrVector<Segment> segments;
  UniquePtrVector<Function> functions;
  UniquePtrVector<Export> exports;
  UniquePtrVector<Import> imports;
  uint32_t initial_memory_size = 0;
  uint32_t max_memory_size = 0;
  std::string name;
};

class SourceLocation {
 public:
  SourceLocation(const WasmSourceLocation& loc)
      : filename(loc.source->filename), line(loc.line), col(loc.col) {}
  // For now there's only ever one file per run, but it's simpler to keep the
  // filename here.  If there gets to be SourceLocs for every expr, we probably
  // want to dedup the filename info.
  std::string filename = "";
  int line = 0;
  int col = 0;
};

// For spec repo test scripts. The spec test script operations are basically
// expressions but have no wasm opcodes and they exist outside modules, so at
// the top level they have their own classes, which contain Expressions.  Each
// script expression refers to the module that immediately preceeds it in the
// file (the parser checks this and sets up the mapping when creating the AST).
class TestScriptExpr {
 public:
  typedef enum {
    kAssertInvalid,
    kInvoke,
    kAssertReturn,
    kAssertReturnNaN,
    kAssertTrap
  } Opcode;
  TestScriptExpr(Module* mod, Opcode op, const WasmSourceLocation loc)
      : module(mod), opcode(op), source_loc(loc), type(Type::kUnknown) {}

  Module* module;
  Opcode opcode;
  SourceLocation source_loc;
  Export* callee;                          // Invoke
  Type type;                               // AssertReturn, Invoke
  std::unique_ptr<TestScriptExpr> invoke;  // AssertReturn, AssertTrap
  UniquePtrVector<Expression> exprs;       // Invoke args, AR expectation
};

}  // namespace wasm
#endif  // WASM_AST_H
