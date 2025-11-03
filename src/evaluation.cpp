/**
 * @file evaluation.cpp
 * @brief Expression evaluation implementation for the Scheme interpreter
 * 
 * This file implements evaluation methods for all expression types in the Scheme
 * interpreter, including primitives, special forms, closures, and environment ops.
 */

#include "value.hpp"
#include "expr.hpp" 
#include "RE.hpp"
#include "syntax.hpp"
#include <vector>
#include <map>
#include <climits>
#include <iostream>

using std::string;
using std::vector;

extern std::map<std::string, ExprType> primitives;
extern std::map<std::string, ExprType> reserved_words;

// ----------------- literals -----------------

Value Fixnum::eval(Assoc &e) { return IntegerV(n); }
Value RationalNum::eval(Assoc &e) { return RationalV(numerator, denominator); }
Value StringExpr::eval(Assoc &e) { return StringV(s); }
Value True::eval(Assoc &e) { return BooleanV(true); }
Value False::eval(Assoc &e) { return BooleanV(false); }
Value MakeVoid::eval(Assoc &e) { return VoidV(); }
Value Exit::eval(Assoc &e) { return TerminateV(); }

// ----------------- abstract eval shells -----------------

Value Unary::eval(Assoc &e) { return evalRator(rand->eval(e)); }
Value Binary::eval(Assoc &e) { return evalRator(rand1->eval(e), rand2->eval(e)); }
Value Variadic::eval(Assoc &e) {
    vector<Value> vals;
    for (auto &x : rands) vals.push_back(x->eval(e));
    return evalRator(vals);
}

// ----------------- numeric helpers -----------------

static Rational asRational(const Value &v) {
    if (v->v_type == V_RATIONAL) return *dynamic_cast<Rational*>(v.get());
    if (v->v_type == V_INT)      return Rational(dynamic_cast<Integer*>(v.get())->n, 1);
    throw RuntimeError("Numeric operand required");
}
static Value makeNumber(const Rational &r) { return RationalV(r.numerator, r.denominator); }

// numeric compare provided here to support ints and rationals
int compareNumericValues(const Value &v1, const Value &v2) {
    if (v1->v_type == V_INT && v2->v_type == V_INT) {
        int n1 = dynamic_cast<Integer*>(v1.get())->n;
        int n2 = dynamic_cast<Integer*>(v2.get())->n;
        return (n1 < n2) ? -1 : (n1 > n2) ? 1 : 0;
    } else if (v1->v_type == V_RATIONAL && v2->v_type == V_INT) {
        Rational* r1 = dynamic_cast<Rational*>(v1.get());
        int n2 = dynamic_cast<Integer*>(v2.get())->n;
        int left = r1->numerator;
        int right = n2 * r1->denominator;
        return (left < right) ? -1 : (left > right) ? 1 : 0;
    } else if (v1->v_type == V_INT && v2->v_type == V_RATIONAL) {
        int n1 = dynamic_cast<Integer*>(v1.get())->n;
        Rational* r2 = dynamic_cast<Rational*>(v2.get());
        int left = n1 * r2->denominator;
        int right = r2->numerator;
        return (left < right) ? -1 : (left > right) ? 1 : 0;
    } else if (v1->v_type == V_RATIONAL && v2->v_type == V_RATIONAL) {
        Rational* r1 = dynamic_cast<Rational*>(v1.get());
        Rational* r2 = dynamic_cast<Rational*>(v2.get());
        int left = r1->numerator * r2->denominator;
        int right = r2->numerator * r1->denominator;
        return (left < right) ? -1 : (left > right) ? 1 : 0;
    }
    throw RuntimeError("Wrong typename in numeric comparison");
}

// ----------------- Var (variables and primitive closures) -----------------

static Value makePrimitiveClosure(ExprType et, Assoc &env) {
    switch (et) {
        case E_VOID:   return ProcedureV({}, Expr(new MakeVoid()), env);
        case E_EXIT:   return ProcedureV({}, Expr(new Exit()), env);

        case E_BOOLQ:    return ProcedureV({"x"}, Expr(new IsBoolean(Expr(new Var("x")))), env);
        case E_INTQ:     return ProcedureV({"x"}, Expr(new IsFixnum(Expr(new Var("x")))), env);
        case E_NULLQ:    return ProcedureV({"x"}, Expr(new IsNull(Expr(new Var("x")))), env);
        case E_PAIRQ:    return ProcedureV({"x"}, Expr(new IsPair(Expr(new Var("x")))), env);
        case E_PROCQ:    return ProcedureV({"x"}, Expr(new IsProcedure(Expr(new Var("x")))), env);
        case E_SYMBOLQ:  return ProcedureV({"x"}, Expr(new IsSymbol(Expr(new Var("x")))), env);
        case E_STRINGQ:  return ProcedureV({"x"}, Expr(new IsString(Expr(new Var("x")))), env);
        case E_LISTQ:    return ProcedureV({"x"}, Expr(new IsList(Expr(new Var("x")))), env);
        case E_NOT:      return ProcedureV({"x"}, Expr(new Not(Expr(new Var("x")))), env);
        case E_DISPLAY:  return ProcedureV({"x"}, Expr(new Display(Expr(new Var("x")))), env);

        case E_MODULO: return ProcedureV({"a","b"}, Expr(new Modulo(Expr(new Var("a")), Expr(new Var("b")))), env);
        case E_EXPT:   return ProcedureV({"a","b"}, Expr(new Expt(Expr(new Var("a")), Expr(new Var("b")))), env);
        case E_CONS:   return ProcedureV({"a","b"}, Expr(new Cons(Expr(new Var("a")), Expr(new Var("b")))), env);
        case E_CAR:    return ProcedureV({"p"}, Expr(new Car(Expr(new Var("p")))), env);
        case E_CDR:    return ProcedureV({"p"}, Expr(new Cdr(Expr(new Var("p")))), env);
        case E_SETCAR: return ProcedureV({"p","v"}, Expr(new SetCar(Expr(new Var("p")), Expr(new Var("v")))), env);
        case E_SETCDR: return ProcedureV({"p","v"}, Expr(new SetCdr(Expr(new Var("p")), Expr(new Var("v")))), env);
        case E_EQQ:    return ProcedureV({"a","b"}, Expr(new IsEq(Expr(new Var("a")), Expr(new Var("b")))), env);

        case E_PLUS:    return ProcedureV({}, Expr(new PlusVar({})), env);
        case E_MINUS:   return ProcedureV({}, Expr(new MinusVar({})), env);
        case E_MUL:     return ProcedureV({}, Expr(new MultVar({})), env);
        case E_DIV:     return ProcedureV({}, Expr(new DivVar({})), env);
        case E_EQ:      return ProcedureV({}, Expr(new EqualVar({})), env);
        case E_LT:      return ProcedureV({}, Expr(new LessVar({})), env);
        case E_LE:      return ProcedureV({}, Expr(new LessEqVar({})), env);
        case E_GE:      return ProcedureV({}, Expr(new GreaterEqVar({})), env);
        case E_GT:      return ProcedureV({}, Expr(new GreaterVar({})), env);
        case E_LIST:    return ProcedureV({}, Expr(new ListFunc({})), env);
        case E_AND:     return ProcedureV({}, Expr(new AndVar({})), env);
        case E_OR:      return ProcedureV({}, Expr(new OrVar({})), env);
        default: break;
    }
    throw RuntimeError("Unsupported primitive closure");
}

Value Var::eval(Assoc &e) {
    Value matched_value = find(x, e);
    if (matched_value.get() != nullptr) return matched_value;

    auto it = primitives.find(x);
    if (it != primitives.end()) return makePrimitiveClosure(it->second, e);

    throw RuntimeError("Invalid variable: " + x);
}

// ----------------- numeric primitives -----------------

Value Plus::evalRator(const Value &a, const Value &b) {
    Rational ra = asRational(a), rb = asRational(b);
    Rational sum(ra.numerator * rb.denominator + rb.numerator * ra.denominator,
                 ra.denominator * rb.denominator);
    return makeNumber(sum);
}
Value Minus::evalRator(const Value &a, const Value &b) {
    Rational ra = asRational(a), rb = asRational(b);
    Rational diff(ra.numerator * rb.denominator - rb.numerator * ra.denominator,
                  ra.denominator * rb.denominator);
    return makeNumber(diff);
}
Value Mult::evalRator(const Value &a, const Value &b) {
    Rational ra = asRational(a), rb = asRational(b);
    Rational prod(ra.numerator * rb.numerator, ra.denominator * rb.denominator);
    return makeNumber(prod);
}
Value Div::evalRator(const Value &a, const Value &b) {
    Rational ra = asRational(a), rb = asRational(b);
    if (rb.numerator == 0) throw RuntimeError("Division by zero");
    Rational q(ra.numerator * rb.denominator, ra.denominator * rb.numerator);
    return makeNumber(q);
}
Value Modulo::evalRator(const Value &a, const Value &b) {
    int lhs, rhs;
    if (a->v_type == V_INT) lhs = dynamic_cast<Integer*>(a.get())->n;
    else if (a->v_type == V_RATIONAL && dynamic_cast<Rational*>(a.get())->denominator == 1)
        lhs = dynamic_cast<Rational*>(a.get())->numerator;
    else throw RuntimeError("modulo is only defined for integers");

    if (b->v_type == V_INT) rhs = dynamic_cast<Integer*>(b.get())->n;
    else if (b->v_type == V_RATIONAL && dynamic_cast<Rational*>(b.get())->denominator == 1)
        rhs = dynamic_cast<Rational*>(b.get())->numerator;
    else throw RuntimeError("modulo is only defined for integers");

    if (rhs == 0) throw RuntimeError("Division by zero");
    return IntegerV(lhs % rhs);
}

Value Expt::evalRator(const Value &rand1, const Value &rand2) { // expt
    if (((rand1->v_type == V_INT) || (rand1->v_type == V_RATIONAL && dynamic_cast<Rational*>(rand1.get())->denominator == 1)) 
     && ((rand2->v_type == V_INT) || (rand2->v_type == V_RATIONAL && dynamic_cast<Rational*>(rand2.get())->denominator == 1))) {
        int base = (rand1->v_type == V_INT) ? dynamic_cast<Integer*>(rand1.get())->n : dynamic_cast<Rational*>(rand1.get())->numerator;
        int exponent = (rand2->v_type == V_INT) ?dynamic_cast<Integer*>(rand2.get())->n : dynamic_cast<Rational*>(rand2.get())->numerator;
        
        if (exponent < 0) {
            throw(RuntimeError("Negative exponent not supported for integers"));
        }
        if (base == 0 && exponent == 0) {
            throw(RuntimeError("0^0 is undefined"));
        }
        
        long long result = 1;
        long long b = base;
        int exp = exponent;
        
        while (exp > 0) {
            if (exp % 2 == 1) {
                result *= b;
                if (result > INT_MAX || result < INT_MIN) {
                    throw(RuntimeError("Integer overflow in expt"));
                }
            }
            b *= b;
            if (b > INT_MAX || b < INT_MIN) {
                if (exp > 1) {
                    throw(RuntimeError("Integer overflow in expt"));
                }
            }
            exp /= 2;
        }
        
        return IntegerV((int)result);
    }
    throw(RuntimeError("Wrong typename in expt"));
}

// Variadic arithmetic
Value PlusVar::evalRator(const std::vector<Value> &args) {
    if (args.empty()) return IntegerV(0);
    Rational acc = asRational(args[0]);
    for (size_t i = 1; i < args.size(); ++i) {
        Rational rv = asRational(args[i]);
        acc = Rational(acc.numerator * rv.denominator + rv.numerator * acc.denominator,
                       acc.denominator * rv.denominator);
    }
    return makeNumber(acc);
}
Value MinusVar::evalRator(const std::vector<Value> &args) {
    if (args.empty()) throw RuntimeError("Wrong number of arguments for -");
    if (args.size() == 1) {
        Rational r = asRational(args[0]);
        return makeNumber(Rational(-r.numerator, r.denominator));
    }
    Rational acc = asRational(args[0]);
    for (size_t i = 1; i < args.size(); ++i) {
        Rational rv = asRational(args[i]);
        acc = Rational(acc.numerator * rv.denominator - rv.numerator * acc.denominator,
                       acc.denominator * rv.denominator);
    }
    return makeNumber(acc);
}
Value MultVar::evalRator(const std::vector<Value> &args) {
    if (args.empty()) return IntegerV(1);
    Rational acc(1,1);
    for (auto &v : args) {
        Rational rv = asRational(v);
        acc = Rational(acc.numerator * rv.numerator, acc.denominator * rv.denominator);
    }
    return makeNumber(acc);
}
Value DivVar::evalRator(const std::vector<Value> &args) {
    if (args.empty()) throw RuntimeError("Wrong number of arguments for /");
    if (args.size() == 1) {
        Rational r = asRational(args[0]);
        if (r.numerator == 0) throw RuntimeError("Division by zero");
        return makeNumber(Rational(r.denominator, r.numerator));
    }
    Rational acc = asRational(args[0]);
    for (size_t i = 1; i < args.size(); ++i) {
        Rational rv = asRational(args[i]);
        if (rv.numerator == 0) throw RuntimeError("Division by zero");
        acc = Rational(acc.numerator * rv.denominator, acc.denominator * rv.numerator);
    }
    return makeNumber(acc);
}

// Comparisons (binary)
Value Less::evalRator(const Value &a, const Value &b)       { return BooleanV(compareNumericValues(a,b) < 0); }
Value LessEq::evalRator(const Value &a, const Value &b)     { return BooleanV(compareNumericValues(a,b) <= 0); }
Value Equal::evalRator(const Value &a, const Value &b)      { return BooleanV(compareNumericValues(a,b) == 0); }
Value GreaterEq::evalRator(const Value &a, const Value &b)  { return BooleanV(compareNumericValues(a,b) >= 0); }
Value Greater::evalRator(const Value &a, const Value &b)    { return BooleanV(compareNumericValues(a,b) > 0); }

// Comparisons (variadic)
Value LessVar::evalRator(const std::vector<Value> &args) {
    if (args.size() < 2) return BooleanV(true);
    for (size_t i = 1; i < args.size(); ++i)
        if (!(compareNumericValues(args[i-1], args[i]) < 0)) return BooleanV(false);
    return BooleanV(true);
}
Value LessEqVar::evalRator(const std::vector<Value> &args) {
    if (args.size() < 2) return BooleanV(true);
    for (size_t i = 1; i < args.size(); ++i)
        if (!(compareNumericValues(args[i-1], args[i]) <= 0)) return BooleanV(false);
    return BooleanV(true);
}
Value EqualVar::evalRator(const std::vector<Value> &args) {
    if (args.size() < 2) return BooleanV(true);
    for (size_t i = 1; i < args.size(); ++i)
        if (!(compareNumericValues(args[i-1], args[i]) == 0)) return BooleanV(false);
    return BooleanV(true);
}
Value GreaterEqVar::evalRator(const std::vector<Value> &args) {
    if (args.size() < 2) return BooleanV(true);
    for (size_t i = 1; i < args.size(); ++i)
        if (!(compareNumericValues(args[i-1], args[i]) >= 0)) return BooleanV(false);
    return BooleanV(true);
}
Value GreaterVar::evalRator(const std::vector<Value> &args) {
    if (args.size() < 2) return BooleanV(true);
    for (size_t i = 1; i < args.size(); ++i)
        if (!(compareNumericValues(args[i-1], args[i]) > 0)) return BooleanV(false);
    return BooleanV(true);
}

// List operations
Value Cons::evalRator(const Value &v1, const Value &v2) { return PairV(v1, v2); }

Value ListFunc::evalRator(const std::vector<Value> &args) {
    Value lst = NullV();
    for (auto it = args.rbegin(); it != args.rend(); ++it) lst = PairV(*it, lst);
    return lst;
}

static bool isProperList(const Value &v) {
    if (v->v_type == V_NULL) return true;
    if (v->v_type != V_PAIR) return false;
    return isProperList(dynamic_cast<Pair*>(v.get())->cdr);
}
Value IsList::evalRator(const Value &v) { return BooleanV(isProperList(v)); }

Value Car::evalRator(const Value &v) {
    if (v->v_type != V_PAIR) throw RuntimeError("car on non-pair");
    return dynamic_cast<Pair*>(v.get())->car;
}
Value Cdr::evalRator(const Value &v) {
    if (v->v_type != V_PAIR) throw RuntimeError("cdr on non-pair");
    return dynamic_cast<Pair*>(v.get())->cdr;
}
Value SetCar::evalRator(const Value &p, const Value &nv) {
    if (p->v_type != V_PAIR) throw RuntimeError("set-car! on non-pair");
    dynamic_cast<Pair*>(p.get())->car = nv;
    return VoidV();
}
Value SetCdr::evalRator(const Value &p, const Value &nv) {
    if (p->v_type != V_PAIR) throw RuntimeError("set-cdr! on non-pair");
    dynamic_cast<Pair*>(p.get())->cdr = nv;
    return VoidV();
}

// display
Value Display::evalRator(const Value &v) {
    v->show(std::cout);
    return VoidV();
}

// eq?
Value IsEq::evalRator(const Value &a, const Value &b) {
    if ((a->v_type == V_INT || a->v_type == V_RATIONAL) &&
        (b->v_type == V_INT || b->v_type == V_RATIONAL)) {
        return BooleanV(compareNumericValues(a,b) == 0);
    }
    if (a->v_type == V_BOOL && b->v_type == V_BOOL) {
        return BooleanV(dynamic_cast<Boolean*>(a.get())->b == dynamic_cast<Boolean*>(b.get())->b);
    }
    if (a->v_type == V_SYM && b->v_type == V_SYM) {
        return BooleanV(dynamic_cast<Symbol*>(a.get())->s == dynamic_cast<Symbol*>(b.get())->s);
    }
    if ((a->v_type == V_NULL && b->v_type == V_NULL) || (a->v_type == V_VOID && b->v_type == V_VOID)) {
        return BooleanV(true);
    }
    return BooleanV(a.get() == b.get());
}

// type predicates
Value IsBoolean::evalRator(const Value &v) { return BooleanV(v->v_type == V_BOOL); }
Value IsFixnum::evalRator(const Value &v) { return BooleanV(v->v_type == V_INT || v->v_type == V_RATIONAL); }
Value IsNull::evalRator(const Value &v)   { return BooleanV(v->v_type == V_NULL); }
Value IsPair::evalRator(const Value &v)   { return BooleanV(v->v_type == V_PAIR); }
Value IsProcedure::evalRator(const Value &v) { return BooleanV(v->v_type == V_PROC); }
Value IsSymbol::evalRator(const Value &v) { return BooleanV(v->v_type == V_SYM); }
Value IsString::evalRator(const Value &v) { return BooleanV(v->v_type == V_STRING); }

// ----------------- Begin, Quote -----------------

Value Begin::eval(Assoc &e) {
    if (es.empty()) return VoidV();

    Value last = VoidV();
    std::vector<std::pair<string, Expr>> pending;

    auto flush = [&](Assoc &env) {
        if (pending.empty()) return;
        for (auto &kv : pending) env = extend(kv.first, VoidV(), env);
        for (auto &kv : pending) {
            Value rhs = kv.second->eval(env);
            modify(kv.first, rhs, env);
        }
        pending.clear();
    };

    for (auto &ex : es) {
        if (auto d = dynamic_cast<Define*>(ex.get())) {
            pending.push_back({d->var, d->e});
            continue;
        }
        flush(e);
        last = ex->eval(e);
        if (last->v_type == V_TERMINATE) return last;
    }
    flush(e);
    return last;
}

// quote helpers
static Value quoteToValue(const Syntax &s);
static Value listFrom(const std::vector<Syntax> &elems, size_t lo, size_t hi) {
    Value tail = NullV();
    for (size_t i = hi; i > lo; --i) {
        tail = PairV(quoteToValue(elems[i-1]), tail);
    }
    return tail;
}
static Value spliceDotted(const std::vector<Syntax> &elems) {
    size_t dot = elems.size();
    for (size_t i = 0; i < elems.size(); ++i) {
        if (auto sym = dynamic_cast<SymbolSyntax*>(elems[i].get())) {
            if (sym->s == ".") { dot = i; break; }
        }
    }
    if (dot == elems.size()) return listFrom(elems, 0, elems.size());
    if (dot + 1 >= elems.size()) throw RuntimeError("Malformed dotted list");
    Value left = listFrom(elems, 0, dot);
    Value right = quoteToValue(elems[dot + 1]);
    if (left->v_type == V_NULL) return right;
    Value cur = left;
    Pair* lastp = nullptr;
    while (cur->v_type == V_PAIR) {
        lastp = dynamic_cast<Pair*>(cur.get());
        cur = lastp->cdr;
    }
    if (lastp) lastp->cdr = right;
    return left;
}
static Value quoteToValue(const Syntax &s) {
    if (auto n = dynamic_cast<Number*>(s.get()))          return IntegerV(n->n);
    if (auto r = dynamic_cast<RationalSyntax*>(s.get()))  return RationalV(r->numerator, r->denominator);
    if (auto t = dynamic_cast<TrueSyntax*>(s.get()))      return BooleanV(true);
    if (auto f = dynamic_cast<FalseSyntax*>(s.get()))     return BooleanV(false);
    if (auto str = dynamic_cast<StringSyntax*>(s.get()))  return StringV(str->s);
    if (auto sym = dynamic_cast<SymbolSyntax*>(s.get()))  return SymbolV(sym->s);
    if (auto lst = dynamic_cast<List*>(s.get()))          return spliceDotted(lst->stxs);
    throw RuntimeError("Bad quoted form");
}
Value Quote::eval(Assoc &e) {
    return quoteToValue(s);
}

// ----------------- logic / conditionals -----------------

Value AndVar::eval(Assoc &e) {
    if (rands.empty()) return BooleanV(true);
    Value last = BooleanV(true);
    for (auto &ex : rands) {
        last = ex->eval(e);
        if (last->v_type == V_BOOL && !dynamic_cast<Boolean*>(last.get())->b) return BooleanV(false);
    }
    return last;
}
Value OrVar::eval(Assoc &e) {
    if (rands.empty()) return BooleanV(false);
    for (auto &ex : rands) {
        Value v = ex->eval(e);
        if (!(v->v_type == V_BOOL && !dynamic_cast<Boolean*>(v.get())->b)) return v;
    }
    return BooleanV(false);
}
Value Not::evalRator(const Value &v) {
    if (v->v_type == V_BOOL && !dynamic_cast<Boolean*>(v.get())->b) return BooleanV(true);
    return BooleanV(false);
}

Value If::eval(Assoc &e) {
    Value c = cond->eval(e);
    bool isFalse = (c->v_type == V_BOOL && !dynamic_cast<Boolean*>(c.get())->b);
    return isFalse ? alter->eval(e) : conseq->eval(e);
}

Value Cond::eval(Assoc &env) {
    for (auto &cl : clauses) {
        if (cl.empty()) continue;
        if (auto v = dynamic_cast<Var*>(cl[0].get())) {
            if (v->x == "else") {
                if (cl.size() == 1) return VoidV();
                Value last = VoidV();
                for (size_t i = 1; i < cl.size(); ++i) last = cl[i]->eval(env);
                return last;
            }
        }
        Value pred = cl[0]->eval(env);
        bool pass = !(pred->v_type == V_BOOL && !dynamic_cast<Boolean*>(pred.get())->b);
        if (pass) {
            if (cl.size() == 1) return pred;
            Value last = VoidV();
            for (size_t i = 1; i < cl.size(); ++i) last = cl[i]->eval(env);
            return last;
        }
    }
    return VoidV();
}

// ----------------- closures, apply, define, let/letrec, set! -----------------

Value Lambda::eval(Assoc &env) { 
    return ProcedureV(x, e, env);
}

Value Apply::eval(Assoc &e) {
    Value fun = rator->eval(e);
    if (fun->v_type != V_PROC) throw RuntimeError("Attempt to apply a non-procedure");
    auto proc = dynamic_cast<Procedure*>(fun.get());

    vector<Value> argv;
    for (auto &ex : rand) argv.push_back(ex->eval(e));

    if (auto varBody = dynamic_cast<Variadic*>(proc->e.get())) {
        return varBody->evalRator(argv);
    }

    if (argv.size() != proc->parameters.size()) throw RuntimeError("Wrong number of arguments");

    Assoc penv = proc->env;
    for (size_t i = 0; i < argv.size(); ++i) {
        penv = extend(proc->parameters[i], argv[i], penv);
    }
    if (auto bg = dynamic_cast<Begin*>(proc->e.get())) return bg->eval(penv);
    return proc->e->eval(penv);
}

Value Define::eval(Assoc &env) {
    Value existing = find(var, env);
    if (existing.get() == nullptr) {
        env = extend(var, VoidV(), env);
    }
    Value rhs = e->eval(env);
    modify(var, rhs, env);
    return VoidV();
}

Value Let::eval(Assoc &env) {
    vector<Value> vals;
    vals.reserve(bind.size());
    for (auto &kv : bind) vals.push_back(kv.second->eval(env));
    Assoc inner = env;
    for (size_t i = 0; i < bind.size(); ++i) inner = extend(bind[i].first, vals[i], inner);
    return body->eval(inner);
}

Value Letrec::eval(Assoc &env) {
    Assoc inner = env;
    for (auto &kv : bind) inner = extend(kv.first, VoidV(), inner);
    for (auto &kv : bind) {
        Value v = kv.second->eval(inner);
        modify(kv.first, v, inner);
    }
    return body->eval(inner);
}

Value Set::eval(Assoc &env) {
    Value cur = find(var, env);
    if (cur.get() == nullptr) throw RuntimeError("Undefined variable : " + var);
    Value nv = e->eval(env);
    modify(var, nv, env);
    return VoidV();
}

// Display primitive is implemented here via Unary; returns Void