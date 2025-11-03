#include "Def.hpp"
#include "expr.hpp"
#include <vector>
using std::vector;
using std::string;
using std::pair;

ExprBase::ExprBase(ExprType et) : e_type(et) {}

Expr::Expr(ExprBase * eb) : ptr(eb) {}
ExprBase* Expr::operator->() const { return ptr.get(); }
ExprBase& Expr::operator*() { return *ptr; }
ExprBase* Expr::get() const { return ptr.get(); }

// BASIC TYPES AND LITERALS
Fixnum::Fixnum(int x) : ExprBase(E_FIXNUM), n(x) {}
RationalNum::RationalNum(int num, int den) : ExprBase(E_RATIONAL), numerator(num), denominator(den) {}
StringExpr::StringExpr(const std::string &str) : ExprBase(E_STRING), s(str) {}
True::True() : ExprBase(E_TRUE) {}
False::False() : ExprBase(E_FALSE) {}
MakeVoid::MakeVoid() : ExprBase(E_VOID) {}
Exit::Exit() : ExprBase(E_EXIT) {}

// ABSTRACT OPERATOR BASES
Unary::Unary(ExprType et, const Expr &expr) : ExprBase(et), rand(expr) {}
Binary::Binary(ExprType et, const Expr &r1, const Expr &r2) : ExprBase(et), rand1(r1), rand2(r2) {}
Variadic::Variadic(ExprType et, const std::vector<Expr> &rands) : ExprBase(et), rands(rands) {}

// ARITHMETIC / COMPARISONS
Mult::Mult(const Expr &r1, const Expr &r2) : Binary(E_MUL, r1, r2) {}
Plus::Plus(const Expr &r1, const Expr &r2) : Binary(E_PLUS, r1, r2) {}
Minus::Minus(const Expr &r1, const Expr &r2) : Binary(E_MINUS, r1, r2) {}
Less::Less(const Expr &r1, const Expr &r2) : Binary(E_LT, r1, r2) {}
LessEq::LessEq(const Expr &r1, const Expr &r2) : Binary(E_LE, r1, r2) {}
Equal::Equal(const Expr &r1, const Expr &r2) : Binary(E_EQ, r1, r2) {}
GreaterEq::GreaterEq(const Expr &r1, const Expr &r2) : Binary(E_GE, r1, r2) {}
Greater::Greater(const Expr &r1, const Expr &r2) : Binary(E_GT, r1, r2) {}
IsEq::IsEq(const Expr &r1, const Expr &r2) : Binary(E_EQQ, r1, r2) {}

Modulo::Modulo(const Expr &r1, const Expr &r2) : Binary(E_MODULO, r1, r2) {}
Expt::Expt(const Expr &r1, const Expr &r2) : Binary(E_EXPT, r1, r2) {}

PlusVar::PlusVar(const std::vector<Expr> &rands) : Variadic(E_PLUS, rands) {}
MinusVar::MinusVar(const std::vector<Expr> &rands) : Variadic(E_MINUS, rands) {}
MultVar::MultVar(const std::vector<Expr> &rands) : Variadic(E_MUL, rands) {}
Div::Div(const Expr &r1, const Expr &r2) : Binary(E_DIV, r1, r2) {}
DivVar::DivVar(const std::vector<Expr> &rands) : Variadic(E_DIV, rands) {}

LessVar::LessVar(const std::vector<Expr> &rands) : Variadic(E_LT, rands) {}
LessEqVar::LessEqVar(const std::vector<Expr> &rands) : Variadic(E_LE, rands) {}
EqualVar::EqualVar(const std::vector<Expr> &rands) : Variadic(E_EQ, rands) {}
GreaterEqVar::GreaterEqVar(const std::vector<Expr> &rands) : Variadic(E_GE, rands) {}
GreaterVar::GreaterVar(const std::vector<Expr> &rands) : Variadic(E_GT, rands) {}

// LIST OPERATIONS
Cons::Cons(const Expr &r1, const Expr &r2) : Binary(E_CONS, r1, r2) {}
Car::Car(const Expr &r1) : Unary(E_CAR, r1) {}
Cdr::Cdr(const Expr &r1) : Unary(E_CDR, r1) {}
ListFunc::ListFunc(const std::vector<Expr> &rands) : Variadic(E_LIST, rands) {}
SetCar::SetCar(const Expr &r1, const Expr &r2) : Binary(E_SETCAR, r1, r2) {}
SetCdr::SetCdr(const Expr &r1, const Expr &r2) : Binary(E_SETCDR, r1, r2) {}

// LOGIC
AndVar::AndVar(const std::vector<Expr> &rands) : ExprBase(E_AND), rands(rands) {}
OrVar::OrVar(const std::vector<Expr> &rands) : ExprBase(E_OR), rands(rands) {}
Not::Not(const Expr &r1) : Unary(E_NOT, r1) {}

// TYPE PREDICATES
IsList::IsList(const Expr &r1) : Unary(E_LISTQ, r1) {}
IsString::IsString(const Expr &r1) : Unary(E_STRINGQ, r1) {}
Display::Display(const Expr &r1) : Unary(E_DISPLAY, r1) {}

IsBoolean::IsBoolean(const Expr &r1) : Unary(E_BOOLQ, r1) {}
IsFixnum::IsFixnum(const Expr &r1) : Unary(E_INTQ, r1) {}
IsNull::IsNull(const Expr &r1) : Unary(E_NULLQ, r1) {}
IsPair::IsPair(const Expr &r1) : Unary(E_PAIRQ, r1) {}
IsProcedure::IsProcedure(const Expr &r1) : Unary(E_PROCQ, r1) {}
IsSymbol::IsSymbol(const Expr &r1) : Unary(E_SYMBOLQ, r1) {}

// CONTROL FLOW / QUOTE
Begin::Begin(const vector<Expr> &vec) : ExprBase(E_BEGIN), es(vec) {}
Quote::Quote(const Syntax &t) : ExprBase(E_QUOTE), s(t) {}

// CONDITIONAL
If::If(const Expr &c, const Expr &c_t, const Expr &c_e) : ExprBase(E_IF), cond(c), conseq(c_t), alter(c_e) {}
Cond::Cond(const std::vector<std::vector<Expr>> &cls) : ExprBase(E_COND), clauses(cls) {}

// VARIABLE/FUNCTION
Var::Var(const string &s) : ExprBase(E_VAR), x(s) {}
Apply::Apply(const Expr &expr, const vector<Expr> &vec) : ExprBase(E_APPLY), rator(expr), rand(vec) {}
Lambda::Lambda(const vector<string> &vec, const Expr &expr) : ExprBase(E_LAMBDA), x(vec), e(expr) {}
Define::Define(const string &variable, const Expr &expr) : ExprBase(E_DEFINE), var(variable), e(expr) {}

// BINDINGS
Let::Let(const vector<pair<string, Expr>> &vec, const Expr &expr) : ExprBase(E_LET), bind(vec), body(expr) {}
Letrec::Letrec(const vector<pair<string, Expr>> &vec, const Expr &expr) : ExprBase(E_LETREC), bind(vec), body(expr) {}
Set::Set(const std::string &var, const Expr &expr) : ExprBase(E_SET), var(var), e(expr) {}