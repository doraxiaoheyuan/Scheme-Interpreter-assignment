#ifndef EXPR_HPP
#define EXPR_HPP

#include "Def.hpp"
#include "syntax.hpp"
#include "RE.hpp"
#include <memory>
#include <vector>
#include <string>

struct Value;

// Base class for all expressions
struct ExprBase {
    ExprType e_type;
    ExprBase(ExprType et);
    virtual Value eval(Assoc &env) = 0;
    virtual ~ExprBase() = default;
};

// Smart holder for expressions
struct Expr {
    std::shared_ptr<ExprBase> ptr;
    Expr(ExprBase *eb = nullptr);
    ExprBase* operator->() const;
    ExprBase& operator*();
    ExprBase* get() const;
};

// BASIC TYPES AND LITERALS
struct Fixnum : ExprBase {
    int n;
    Fixnum(int);
    Value eval(Assoc &env) override;
};

struct RationalNum : ExprBase {
    int numerator, denominator;
    RationalNum(int num, int den);
    Value eval(Assoc &env) override;
};

struct StringExpr : ExprBase {
    std::string s;
    StringExpr(const std::string &);
    Value eval(Assoc &env) override;
};

struct True : ExprBase {
    True();
    Value eval(Assoc &env) override;
};

struct False : ExprBase {
    False();
    Value eval(Assoc &env) override;
};

struct MakeVoid : ExprBase {
    MakeVoid();
    Value eval(Assoc &env) override;
};

struct Exit : ExprBase {
    Exit();
    Value eval(Assoc &env) override;
};

// ABSTRACT OPERATOR BASES
struct Unary : ExprBase {
    Expr rand;
    Unary(ExprType, const Expr &);
    virtual Value evalRator(const Value &v) = 0;
    Value eval(Assoc &env) override;
};

struct Binary : ExprBase {
    Expr rand1, rand2;
    Binary(ExprType, const Expr &, const Expr &);
    virtual Value evalRator(const Value &v1, const Value &v2) = 0;
    Value eval(Assoc &env) override;
};

struct Variadic : ExprBase {
    std::vector<Expr> rands;
    Variadic(ExprType, const std::vector<Expr> &);
    virtual Value evalRator(const std::vector<Value> &args) = 0;
    Value eval(Assoc &env) override;
};

// ARITHMETIC / COMPARISONS
struct Mult : Binary { Mult(const Expr &, const Expr &); Value evalRator(const Value &, const Value &) override; };
struct Plus : Binary { Plus(const Expr &, const Expr &); Value evalRator(const Value &, const Value &) override; };
struct Minus : Binary { Minus(const Expr &, const Expr &); Value evalRator(const Value &, const Value &) override; };
struct Less : Binary { Less(const Expr &, const Expr &); Value evalRator(const Value &, const Value &) override; };
struct LessEq : Binary { LessEq(const Expr &, const Expr &); Value evalRator(const Value &, const Value &) override; };
struct Equal : Binary { Equal(const Expr &, const Expr &); Value evalRator(const Value &, const Value &) override; };
struct GreaterEq : Binary { GreaterEq(const Expr &, const Expr &); Value evalRator(const Value &, const Value &) override; };
struct Greater : Binary { Greater(const Expr &, const Expr &); Value evalRator(const Value &, const Value &) override; };
struct IsEq : Binary { IsEq(const Expr &, const Expr &); Value evalRator(const Value &, const Value &) override; };

struct Modulo : Binary { Modulo(const Expr &, const Expr &); Value evalRator(const Value &, const Value &) override; };
struct Expt   : Binary { Expt(const Expr &, const Expr &);   Value evalRator(const Value &, const Value &) override; };

// Variadic primitives
struct PlusVar      : Variadic { PlusVar(const std::vector<Expr> &);      Value evalRator(const std::vector<Value> &) override; };
struct MinusVar     : Variadic { MinusVar(const std::vector<Expr> &);     Value evalRator(const std::vector<Value> &) override; };
struct MultVar      : Variadic { MultVar(const std::vector<Expr> &);      Value evalRator(const std::vector<Value> &) override; };
struct Div          : Binary   { Div(const Expr &, const Expr &);         Value evalRator(const Value &, const Value &) override; };
struct DivVar       : Variadic { DivVar(const std::vector<Expr> &);       Value evalRator(const std::vector<Value> &) override; };

struct LessVar      : Variadic { LessVar(const std::vector<Expr> &);      Value evalRator(const std::vector<Value> &) override; };
struct LessEqVar    : Variadic { LessEqVar(const std::vector<Expr> &);    Value evalRator(const std::vector<Value> &) override; };
struct EqualVar     : Variadic { EqualVar(const std::vector<Expr> &);     Value evalRator(const std::vector<Value> &) override; };
struct GreaterEqVar : Variadic { GreaterEqVar(const std::vector<Expr> &); Value evalRator(const std::vector<Value> &) override; };
struct GreaterVar   : Variadic { GreaterVar(const std::vector<Expr> &);   Value evalRator(const std::vector<Value> &) override; };

// LIST OPERATIONS
struct Cons : Binary { Cons(const Expr &, const Expr &); Value evalRator(const Value &, const Value &) override; };
struct Car : Unary { Car(const Expr &); Value evalRator(const Value &) override; };
struct Cdr : Unary { Cdr(const Expr &); Value evalRator(const Value &) override; };
struct ListFunc : Variadic { ListFunc(const std::vector<Expr> &); Value evalRator(const std::vector<Value> &) override; };
struct SetCar : Binary { SetCar(const Expr &, const Expr &); Value evalRator(const Value &, const Value &) override; };
struct SetCdr : Binary { SetCdr(const Expr &, const Expr &); Value evalRator(const Value &, const Value &) override; };

struct IsList : Unary { IsList(const Expr &); Value evalRator(const Value &) override; };
struct IsString : Unary { IsString(const Expr &); Value evalRator(const Value &) override; };
struct Display : Unary { Display(const Expr &); Value evalRator(const Value &) override; };

// TYPE PREDICATES
struct IsBoolean   : Unary { IsBoolean(const Expr &);   Value evalRator(const Value &) override; };
struct IsFixnum    : Unary { IsFixnum(const Expr &);    Value evalRator(const Value &) override; };
struct IsNull      : Unary { IsNull(const Expr &);      Value evalRator(const Value &) override; };
struct IsPair      : Unary { IsPair(const Expr &);      Value evalRator(const Value &) override; };
struct IsProcedure : Unary { IsProcedure(const Expr &); Value evalRator(const Value &) override; };
struct IsSymbol    : Unary { IsSymbol(const Expr &);    Value evalRator(const Value &) override; };

// LOGIC
struct AndVar : ExprBase { std::vector<Expr> rands; AndVar(const std::vector<Expr> &); Value eval(Assoc &env) override; };
struct OrVar  : ExprBase { std::vector<Expr> rands; OrVar(const std::vector<Expr> &);  Value eval(Assoc &env) override; };
struct Not : Unary { Not(const Expr &); Value evalRator(const Value &) override; };

// CONTROL FLOW / QUOTE
struct Begin : ExprBase { std::vector<Expr> es; Begin(const std::vector<Expr> &); Value eval(Assoc &env) override; };
struct Quote : ExprBase { Syntax s; Quote(const Syntax &); Value eval(Assoc &env) override; };

// CONDITIONAL
struct If : ExprBase { Expr cond, conseq, alter; If(const Expr &, const Expr &, const Expr &); Value eval(Assoc &env) override; };
struct Cond : ExprBase { std::vector<std::vector<Expr>> clauses; Cond(const std::vector<std::vector<Expr>> &); Value eval(Assoc &env) override; };

// VARIABLE/FUNCTION
struct Var : ExprBase { std::string x; Var(const std::string &); Value eval(Assoc &env) override; };
struct Apply : ExprBase { Expr rator; std::vector<Expr> rand; Apply(const Expr &, const std::vector<Expr> &); Value eval(Assoc &env) override; };
struct Lambda : ExprBase { std::vector<std::string> x; Expr e; Lambda(const std::vector<std::string> &, const Expr &); Value eval(Assoc &env) override; };
struct Define : ExprBase { std::string var; Expr e; Define(const std::string &, const Expr &); Value eval(Assoc &env) override; };

// BINDINGS
struct Let : ExprBase { std::vector<std::pair<std::string, Expr>> bind; Expr body; Let(const std::vector<std::pair<std::string, Expr>> &, const Expr &); Value eval(Assoc &env) override; };
struct Letrec : ExprBase { std::vector<std::pair<std::string, Expr>> bind; Expr body; Letrec(const std::vector<std::pair<std::string, Expr>> &, const Expr &); Value eval(Assoc &env) override; };
struct Set : ExprBase { std::string var; Expr e; Set(const std::string &, const Expr &); Value eval(Assoc &env) override; };

// Utilities (optional compatibility)
Value syntax_to_value(const Syntax &stx);

#endif // EXPR_HPP