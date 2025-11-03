/**
 * @file parser.cpp
 * @brief Parsing implementation for Scheme syntax tree to expression tree conversion
 * 
 * This file implements the parsing logic that converts syntax trees into
 * expression trees that can be evaluated. It handles special forms, primitives,
 * and function applications.
 */

#include "RE.hpp"
#include "Def.hpp"
#include "syntax.hpp"
#include "value.hpp"
#include "expr.hpp"
#include <map>
#include <string>
#include <vector>

using std::string;
using std::vector;

extern std::map<std::string, ExprType> primitives;
extern std::map<std::string, ExprType> reserved_words;

static Expr parseSyntax(const Syntax &stx, Assoc &env);

Expr Syntax::parse(Assoc &env) {
    throw RuntimeError("Unimplemented parse method");
}

Expr Number::parse(Assoc &env) {
    return Expr(new Fixnum(n));
}

Expr RationalSyntax::parse(Assoc &env) {
    return Expr(new RationalNum(numerator, denominator));
}

Expr SymbolSyntax::parse(Assoc &env) {
    return Expr(new Var(s));
}

Expr StringSyntax::parse(Assoc &env) {
    return Expr(new StringExpr(s));
}

Expr TrueSyntax::parse(Assoc &env) {
    return Expr(new True());
}

Expr FalseSyntax::parse(Assoc &env) {
    return Expr(new False());
}

static Expr parseSyntax(const Syntax &stx, Assoc &env) {
    if (auto n = dynamic_cast<Number*>(stx.get()))        return Expr(new Fixnum(n->n));
    if (auto r = dynamic_cast<RationalSyntax*>(stx.get()))return Expr(new RationalNum(r->numerator, r->denominator));
    if (auto t = dynamic_cast<TrueSyntax*>(stx.get()))     return Expr(new True());
    if (auto f = dynamic_cast<FalseSyntax*>(stx.get()))    return Expr(new False());
    if (auto s = dynamic_cast<StringSyntax*>(stx.get()))   return Expr(new StringExpr(s->s));
    if (auto v = dynamic_cast<SymbolSyntax*>(stx.get()))   return Expr(new Var(v->s));
    if (auto l = dynamic_cast<List*>(stx.get()))           return l->parse(env);
    throw RuntimeError("Unknown syntax node");
}

static vector<Expr> parseListItemsFrom(const vector<Syntax> &items, size_t start, Assoc &env) {
    vector<Expr> out;
    for (size_t i = start; i < items.size(); ++i) out.push_back(parseSyntax(items[i], env));
    return out;
}

Expr List::parse(Assoc &env) {
    if (stxs.empty()) {
        // '() parsed as (quote ())
        return Expr(new Quote(Syntax(new List())));
    }

    // If head is not a symbol, treat as (Apply <head-expr> <arg> ...)
    auto symHead = dynamic_cast<SymbolSyntax*>(stxs[0].get());
    if (!symHead) {
        vector<Expr> args = parseListItemsFrom(stxs, 1, env);
        return Expr(new Apply(parseSyntax(stxs[0], env), args));
    }

    // Head is symbol; could be a user var (call), a primitive, or a reserved word
    const string op = symHead->s;

    // If op is bound in env (can shadow primitive/reserved), treat as function var apply
    if (find(op, env).get() != nullptr) {
        vector<Expr> args = parseListItemsFrom(stxs, 1, env);
        return Expr(new Apply(Expr(new Var(op)), args));
    }

    // Primitive functions
    if (primitives.count(op)) {
        vector<Expr> params = parseListItemsFrom(stxs, 1, env);
        ExprType t = primitives[op];

        switch (t) {
            // arithmetic
            case E_PLUS:
                if (params.size() == 2) return Expr(new Plus(params[0], params[1]));
                return Expr(new PlusVar(params));
            case E_MINUS:
                if (params.size() == 2) return Expr(new Minus(params[0], params[1]));
                if (params.empty()) throw RuntimeError("Wrong number of arguments for -");
                return Expr(new MinusVar(params));
            case E_MUL:
                if (params.size() == 2) return Expr(new Mult(params[0], params[1]));
                return Expr(new MultVar(params));
            case E_DIV:
                if (params.size() == 2) return Expr(new Div(params[0], params[1]));
                if (params.empty()) throw RuntimeError("Wrong number of arguments for /");
                return Expr(new DivVar(params));
            case E_MODULO:
                if (params.size() != 2) throw RuntimeError("Wrong number of arguments for modulo");
                return Expr(new Modulo(params[0], params[1]));
            case E_EXPT:
                if (params.size() != 2) throw RuntimeError("Wrong number of arguments for expt");
                return Expr(new Expt(params[0], params[1]));

            // comparisons (support variadic)
            case E_LT:
                if (params.size() < 2) throw RuntimeError("Wrong number of arguments for <");
                if (params.size() == 2) return Expr(new Less(params[0], params[1]));
                return Expr(new LessVar(params));
            case E_LE:
                if (params.size() < 2) throw RuntimeError("Wrong number of arguments for <=");
                if (params.size() == 2) return Expr(new LessEq(params[0], params[1]));
                return Expr(new LessEqVar(params));
            case E_EQ:
                if (params.size() < 2) throw RuntimeError("Wrong number of arguments for =");
                if (params.size() == 2) return Expr(new Equal(params[0], params[1]));
                return Expr(new EqualVar(params));
            case E_GE:
                if (params.size() < 2) throw RuntimeError("Wrong number of arguments for >=");
                if (params.size() == 2) return Expr(new GreaterEq(params[0], params[1]));
                return Expr(new GreaterEqVar(params));
            case E_GT:
                if (params.size() < 2) throw RuntimeError("Wrong number of arguments for >");
                if (params.size() == 2) return Expr(new Greater(params[0], params[1]));
                return Expr(new GreaterVar(params));

            // lists
            case E_LIST:
                return Expr(new ListFunc(params));
            case E_CONS:
                if (params.size() != 2) throw RuntimeError("Wrong number of arguments for cons");
                return Expr(new Cons(params[0], params[1]));
            case E_CAR:
                if (params.size() != 1) throw RuntimeError("Wrong number of arguments for car");
                return Expr(new Car(params[0]));
            case E_CDR:
                if (params.size() != 1) throw RuntimeError("Wrong number of arguments for cdr");
                return Expr(new Cdr(params[0]));
            case E_SETCAR:
                if (params.size() != 2) throw RuntimeError("Wrong number of arguments for set-car!");
                return Expr(new SetCar(params[0], params[1]));
            case E_SETCDR:
                if (params.size() != 2) throw RuntimeError("Wrong number of arguments for set-cdr!");
                return Expr(new SetCdr(params[0], params[1]));

            // logic and predicates, i/o, control
            case E_AND:
                return Expr(new AndVar(params));
            case E_OR:
                return Expr(new OrVar(params));
            case E_NOT:
                if (params.size() != 1) throw RuntimeError("Wrong number of arguments for not");
                return Expr(new Not(params[0]));

            case E_EQQ:
                if (params.size() != 2) throw RuntimeError("Wrong number of arguments for eq?");
                return Expr(new IsEq(params[0], params[1]));
            case E_BOOLQ:
                if (params.size() != 1) throw RuntimeError("Wrong number of arguments for boolean?");
                return Expr(new IsBoolean(params[0]));
            case E_INTQ:
                if (params.size() != 1) throw RuntimeError("Wrong number of arguments for number?");
                return Expr(new IsFixnum(params[0]));
            case E_NULLQ:
                if (params.size() != 1) throw RuntimeError("Wrong number of arguments for null?");
                return Expr(new IsNull(params[0]));
            case E_PAIRQ:
                if (params.size() != 1) throw RuntimeError("Wrong number of arguments for pair?");
                return Expr(new IsPair(params[0]));
            case E_PROCQ:
                if (params.size() != 1) throw RuntimeError("Wrong number of arguments for procedure?");
                return Expr(new IsProcedure(params[0]));
            case E_SYMBOLQ:
                if (params.size() != 1) throw RuntimeError("Wrong number of arguments for symbol?");
                return Expr(new IsSymbol(params[0]));
            case E_LISTQ:
                if (params.size() != 1) throw RuntimeError("Wrong number of arguments for list?");
                return Expr(new IsList(params[0]));
            case E_STRINGQ:
                if (params.size() != 1) throw RuntimeError("Wrong number of arguments for string?");
                return Expr(new IsString(params[0]));

            case E_DISPLAY:
                if (params.size() != 1) throw RuntimeError("Wrong number of arguments for display");
                return Expr(new Display(params[0]));

            case E_VOID:
                if (!params.empty()) throw RuntimeError("Wrong number of arguments for void");
                return Expr(new MakeVoid());
            case E_EXIT:
                if (!params.empty()) throw RuntimeError("Wrong number of arguments for exit");
                return Expr(new Exit());

            default:
                throw RuntimeError("Unknown primitive: " + op);
        }
    }

    // Reserved words (special forms)
    if (reserved_words.count(op)) {
        switch (reserved_words[op]) {
            case E_BEGIN: {
                // (begin e1 e2 ...)
                vector<Expr> seq = parseListItemsFrom(stxs, 1, env);
                return Expr(new Begin(seq));
            }
            case E_QUOTE: {
                if (stxs.size() != 2) throw RuntimeError("Wrong number of arguments for quote");
                return Expr(new Quote(stxs[1]));
            }
            case E_IF: {
                if (stxs.size() != 4) throw RuntimeError("Wrong number of arguments for if");
                Expr c = parseSyntax(stxs[1], env);
                Expr t = parseSyntax(stxs[2], env);
                Expr f = parseSyntax(stxs[3], env);
                return Expr(new If(c, t, f));
            }
            case E_COND: {
                if (stxs.size() < 2) throw RuntimeError("No clauses for cond");
                vector<vector<Expr>> clauses;
                for (size_t i = 1; i < stxs.size(); ++i) {
                    auto sub = dynamic_cast<List*>(stxs[i].get());
                    if (!sub) throw RuntimeError("Wrong clause in cond");
                    vector<Expr> one;
                    for (auto &s : sub->stxs) one.push_back(parseSyntax(s, env));
                    clauses.push_back(one);
                }
                return Expr(new Cond(clauses));
            }
            case E_LAMBDA: {
                if (stxs.size() < 3) throw RuntimeError("Wrong number of arguments for lambda");
                auto paramsList = dynamic_cast<List*>(stxs[1].get());
                if (!paramsList) throw RuntimeError("Invalid parameter list in lambda");
                vector<string> params;
                for (auto &p : paramsList->stxs) {
                    auto s = dynamic_cast<SymbolSyntax*>(p.get());
                    if (!s) throw RuntimeError("Invalid parameter");
                    params.push_back(s->s);
                }
                vector<Expr> bodies = parseListItemsFrom(stxs, 2, env);
                Expr body = bodies.size() == 1 ? bodies[0] : Expr(new Begin(bodies));
                return Expr(new Lambda(params, body));
            }
            case E_DEFINE: {
                if (stxs.size() < 3) throw RuntimeError("Wrong number of arguments for define");
                // function sugar: (define (fname a b ...) body...)
                if (auto sig = dynamic_cast<List*>(stxs[1].get())) {
                    if (sig->stxs.empty()) throw RuntimeError("Invalid function signature in define");
                    auto nameSym = dynamic_cast<SymbolSyntax*>(sig->stxs[0].get());
                    if (!nameSym) throw RuntimeError("Invalid function name in define");
                    string fname = nameSym->s;
                    vector<string> params;
                    for (size_t i = 1; i < sig->stxs.size(); ++i) {
                        auto s = dynamic_cast<SymbolSyntax*>(sig->stxs[i].get());
                        if (!s) throw RuntimeError("Invalid parameter in define");
                        params.push_back(s->s);
                    }
                    vector<Expr> bodies = parseListItemsFrom(stxs, 2, env);
                    Expr body = bodies.size() == 1 ? bodies[0] : Expr(new Begin(bodies));
                    Expr lam = Expr(new Lambda(params, body));
                    return Expr(new Define(fname, lam));
                }
                // variable define: (define name expr-or-seq)
                auto nameSym = dynamic_cast<SymbolSyntax*>(stxs[1].get());
                if (!nameSym) throw RuntimeError("Invalid variable name in define");
                vector<Expr> rhsParts = parseListItemsFrom(stxs, 2, env);
                Expr rhs = rhsParts.size() == 1 ? rhsParts[0] : Expr(new Begin(rhsParts));
                return Expr(new Define(nameSym->s, rhs));
            }
            case E_LET: {
                if (stxs.size() < 3) throw RuntimeError("Wrong number of arguments for let");
                auto binds = dynamic_cast<List*>(stxs[1].get());
                if (!binds) throw RuntimeError("Invalid binding list in let");
                vector<std::pair<string, Expr>> pairs;
                for (auto &b : binds->stxs) {
                    auto kv = dynamic_cast<List*>(b.get());
                    if (!kv || kv->stxs.size() != 2) throw RuntimeError("Wrong binding in let");
                    auto keySym = dynamic_cast<SymbolSyntax*>(kv->stxs[0].get());
                    if (!keySym) throw RuntimeError("Invalid let variable");
                    pairs.push_back({keySym->s, parseSyntax(kv->stxs[1], env)});
                }
                vector<Expr> bodies = parseListItemsFrom(stxs, 2, env);
                Expr body = bodies.size() == 1 ? bodies[0] : Expr(new Begin(bodies));
                return Expr(new Let(pairs, body));
            }
            case E_LETREC: {
                if (stxs.size() < 3) throw RuntimeError("Wrong number of arguments for letrec");
                auto binds = dynamic_cast<List*>(stxs[1].get());
                if (!binds) throw RuntimeError("Invalid binding list in letrec");
                vector<std::pair<string, Expr>> pairs;
                for (auto &b : binds->stxs) {
                    auto kv = dynamic_cast<List*>(b.get());
                    if (!kv || kv->stxs.size() != 2) throw RuntimeError("Wrong binding in letrec");
                    auto keySym = dynamic_cast<SymbolSyntax*>(kv->stxs[0].get());
                    if (!keySym) throw RuntimeError("Invalid letrec variable");
                    pairs.push_back({keySym->s, parseSyntax(kv->stxs[1], env)});
                }
                vector<Expr> bodies = parseListItemsFrom(stxs, 2, env);
                Expr body = bodies.size() == 1 ? bodies[0] : Expr(new Begin(bodies));
                return Expr(new Letrec(pairs, body));
            }
            case E_SET: {
                if (stxs.size() != 3) throw RuntimeError("Wrong number of arguments for set!");
                auto nameSym = dynamic_cast<SymbolSyntax*>(stxs[1].get());
                if (!nameSym) throw RuntimeError("Invalid variable name in set!");
                Expr rhs = parseSyntax(stxs[2], env);
                return Expr(new Set(nameSym->s, rhs));
            }
            default:
                throw RuntimeError("Unknown reserved word: " + op);
        }
    }

    // Default: treat as application of a variable (even if it looks like a keyword name but unbound here)
    vector<Expr> args = parseListItemsFrom(stxs, 1, env);
    return Expr(new Apply(Expr(new Var(op)), args));
}