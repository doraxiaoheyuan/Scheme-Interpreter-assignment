/**
 * @file parser.cpp
 * @brief Turn Syntax (reader output) into Expr (evaluator input).
 *
 * Parsing decides which constructs are primitives/special forms vs. ordinary
 * applications. Variable shadowing is honored: if a name is already bound in
 * the given environment, we parse it as a normal variable reference/call,
 * even if it looks like a primitive or a special form keyword.
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
    (void)env;
    return Expr(new Fixnum(n));
}

Expr RationalSyntax::parse(Assoc &env) {
    (void)env;
    return Expr(new RationalNum(numerator, denominator));
}

Expr SymbolSyntax::parse(Assoc &env) {
    (void)env;
    return Expr(new Var(s));
}

Expr StringSyntax::parse(Assoc &env) {
    (void)env;
    return Expr(new StringExpr(s));
}

Expr TrueSyntax::parse(Assoc &env) {
    (void)env;
    return Expr(new True());
}

Expr FalseSyntax::parse(Assoc &env) {
    (void)env;
    return Expr(new False());
}

static Expr parseSyntax(const Syntax &stx, Assoc &env) {
    if (auto n = dynamic_cast<Number*>(stx.get()))         return Expr(new Fixnum(n->n));
    if (auto r = dynamic_cast<RationalSyntax*>(stx.get())) return Expr(new RationalNum(r->numerator, r->denominator));
    if (auto t = dynamic_cast<TrueSyntax*>(stx.get()))      return Expr(new True());
    if (auto f = dynamic_cast<FalseSyntax*>(stx.get()))     return Expr(new False());
    if (auto s = dynamic_cast<StringSyntax*>(stx.get()))    return Expr(new StringExpr(s->s));
    if (auto v = dynamic_cast<SymbolSyntax*>(stx.get()))    return Expr(new Var(v->s));
    if (auto l = dynamic_cast<List*>(stx.get()))            return l->parse(env);
    throw RuntimeError("Unknown syntax node");
}

static vector<Expr> parseFromIndex(const vector<Syntax> &items, size_t start, Assoc &env) {
    vector<Expr> out;
    out.reserve(items.size() - start);
    for (size_t i = start; i < items.size(); ++i) out.push_back(parseSyntax(items[i], env));
    return out;
}

static Assoc extendWithPlaceholders(const vector<string> &names, Assoc env) { // 扩展环境(占位符)
    Assoc tmp = env;
    for (const auto &nm : names) {
        tmp = extend(nm, VoidV(), tmp);
    }
    return tmp;
}

Expr List::parse(Assoc &env) {
    if (stxs.empty()) {
        return Expr(new Quote(Syntax(new List())));
    }

    auto symHead = dynamic_cast<SymbolSyntax*>(stxs[0].get());
    if (!symHead) {
        vector<Expr> args = parseFromIndex(stxs, 1, env);
        return Expr(new Apply(parseSyntax(stxs[0], env), args)); // 操作符=第一个元素的parsing
    }

    const string op = symHead->s;

    if (find(op, env).get() != nullptr) {
        vector<Expr> args = parseFromIndex(stxs, 1, env);
        return Expr(new Apply(Expr(new Var(op)), args));
    }

    if (primitives.count(op)) {
        vector<Expr> ps = parseFromIndex(stxs, 1, env);
        ExprType t = primitives[op];

        switch (t) {
            case E_PLUS:
                if (ps.size() == 2) return Expr(new Plus(ps[0], ps[1]));
                return Expr(new PlusVar(ps));
            case E_MINUS:
                if (ps.size() == 2) return Expr(new Minus(ps[0], ps[1]));
                if (ps.empty()) throw RuntimeError("Wrong number of arguments for -");
                return Expr(new MinusVar(ps));
            case E_MUL:
                if (ps.size() == 2) return Expr(new Mult(ps[0], ps[1]));
                return Expr(new MultVar(ps));
            case E_DIV:
                if (ps.size() == 2) return Expr(new Div(ps[0], ps[1]));
                if (ps.empty()) throw RuntimeError("Wrong number of arguments for /");
                return Expr(new DivVar(ps));
            case E_MODULO:
                if (ps.size() != 2) throw RuntimeError("Wrong number of arguments for modulo");
                return Expr(new Modulo(ps[0], ps[1]));
            case E_EXPT:
                if (ps.size() != 2) throw RuntimeError("Wrong number of arguments for expt");
                return Expr(new Expt(ps[0], ps[1]));

            case E_LT:
                if (ps.size() < 2) throw RuntimeError("Wrong number of arguments for <");
                if (ps.size() == 2) return Expr(new Less(ps[0], ps[1]));
                return Expr(new LessVar(ps));
            case E_LE:
                if (ps.size() < 2) throw RuntimeError("Wrong number of arguments for <=");
                if (ps.size() == 2) return Expr(new LessEq(ps[0], ps[1]));
                return Expr(new LessEqVar(ps));
            case E_EQ:
                if (ps.size() < 2) throw RuntimeError("Wrong number of arguments for =");
                if (ps.size() == 2) return Expr(new Equal(ps[0], ps[1]));
                return Expr(new EqualVar(ps));
            case E_GE:
                if (ps.size() < 2) throw RuntimeError("Wrong number of arguments for >=");
                if (ps.size() == 2) return Expr(new GreaterEq(ps[0], ps[1]));
                return Expr(new GreaterEqVar(ps));
            case E_GT:
                if (ps.size() < 2) throw RuntimeError("Wrong number of arguments for >");
                if (ps.size() == 2) return Expr(new Greater(ps[0], ps[1]));
                return Expr(new GreaterVar(ps));

            case E_LIST:
                return Expr(new ListFunc(ps));
            case E_CONS:
                if (ps.size() != 2) throw RuntimeError("Wrong number of arguments for cons");
                return Expr(new Cons(ps[0], ps[1]));
            case E_CAR:
                if (ps.size() != 1) throw RuntimeError("Wrong number of arguments for car");
                return Expr(new Car(ps[0]));
            case E_CDR:
                if (ps.size() != 1) throw RuntimeError("Wrong number of arguments for cdr");
                return Expr(new Cdr(ps[0]));
            case E_SETCAR:
                if (ps.size() != 2) throw RuntimeError("Wrong number of arguments for set-car!");
                return Expr(new SetCar(ps[0], ps[1]));
            case E_SETCDR:
                if (ps.size() != 2) throw RuntimeError("Wrong number of arguments for set-cdr!");
                return Expr(new SetCdr(ps[0], ps[1]));

            case E_AND:
                return Expr(new AndVar(ps));
            case E_OR:
                return Expr(new OrVar(ps));
            case E_NOT:
                if (ps.size() != 1) throw RuntimeError("Wrong number of arguments for not");
                return Expr(new Not(ps[0]));

            case E_EQQ:
                if (ps.size() != 2) throw RuntimeError("Wrong number of arguments for eq?");
                return Expr(new IsEq(ps[0], ps[1]));
            case E_BOOLQ:
                if (ps.size() != 1) throw RuntimeError("Wrong number of arguments for boolean?");
                return Expr(new IsBoolean(ps[0]));
            case E_INTQ:
                if (ps.size() != 1) throw RuntimeError("Wrong number of arguments for number?");
                return Expr(new IsFixnum(ps[0]));
            case E_NULLQ:
                if (ps.size() != 1) throw RuntimeError("Wrong number of arguments for null?");
                return Expr(new IsNull(ps[0]));
            case E_PAIRQ:
                if (ps.size() != 1) throw RuntimeError("Wrong number of arguments for pair?");
                return Expr(new IsPair(ps[0]));
            case E_PROCQ:
                if (ps.size() != 1) throw RuntimeError("Wrong number of arguments for procedure?");
                return Expr(new IsProcedure(ps[0]));
            case E_SYMBOLQ:
                if (ps.size() != 1) throw RuntimeError("Wrong number of arguments for symbol?");
                return Expr(new IsSymbol(ps[0]));
            case E_LISTQ:
                if (ps.size() != 1) throw RuntimeError("Wrong number of arguments for list?");
                return Expr(new IsList(ps[0]));
            case E_STRINGQ:
                if (ps.size() != 1) throw RuntimeError("Wrong number of arguments for string?");
                return Expr(new IsString(ps[0]));

            case E_DISPLAY:
                if (ps.size() != 1) throw RuntimeError("Wrong number of arguments for display");
                return Expr(new Display(ps[0]));

            case E_VOID:
                if (!ps.empty()) throw RuntimeError("Wrong number of arguments for void");
                return Expr(new MakeVoid());
            case E_EXIT:
                if (!ps.empty()) throw RuntimeError("Wrong number of arguments for exit");
                return Expr(new Exit());
        }
        throw RuntimeError("Unknown primitive: " + op);
    }

    if (reserved_words.count(op)) {
        switch (reserved_words[op]) {
            case E_BEGIN: {
                vector<Expr> seq = parseFromIndex(stxs, 1, env);
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
                    one.reserve(sub->stxs.size());
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
                params.reserve(paramsList->stxs.size());
                for (auto &p : paramsList->stxs) {
                    auto s = dynamic_cast<SymbolSyntax*>(p.get());
                    if (!s) throw RuntimeError("Invalid parameter");
                    params.push_back(s->s);
                }

                // 先绑定
                Assoc bodyEnv = extendWithPlaceholders(params, env);
                vector<Expr> bodies = parseFromIndex(stxs, 2, bodyEnv);
                Expr body = bodies.size() == 1 ? bodies[0] : Expr(new Begin(bodies));
                return Expr(new Lambda(params, body));
            }
            case E_DEFINE: {
                if (stxs.size() < 3) throw RuntimeError("Wrong number of arguments for define");

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

                    // 先绑定
                    Assoc bodyEnv = extendWithPlaceholders(params, env);
                    bodyEnv = extend(fname, VoidV(), bodyEnv);

                    vector<Expr> bodies = parseFromIndex(stxs, 2, bodyEnv);
                    Expr body = bodies.size() == 1 ? bodies[0] : Expr(new Begin(bodies));
                    Expr lam = Expr(new Lambda(params, body));
                    return Expr(new Define(fname, lam));
                }

                // 定义变量
                auto nameSym = dynamic_cast<SymbolSyntax*>(stxs[1].get());
                if (!nameSym) throw RuntimeError("Invalid variable name in define");

                vector<Expr> rhsParts = parseFromIndex(stxs, 2, env);
                Expr rhs = rhsParts.size() == 1 ? rhsParts[0] : Expr(new Begin(rhsParts));
                return Expr(new Define(nameSym->s, rhs));
            }
            case E_LET: {
                if (stxs.size() < 3) throw RuntimeError("Wrong number of arguments for let");
                auto binds = dynamic_cast<List*>(stxs[1].get());
                if (!binds) throw RuntimeError("Invalid binding list in let");

                vector<std::pair<string, Expr>> pairs;
                vector<string> names;
                pairs.reserve(binds->stxs.size());
                names.reserve(binds->stxs.size());

                // 在外部环境里parse rhs
                for (auto &b : binds->stxs) {
                    auto kv = dynamic_cast<List*>(b.get());
                    if (!kv || kv->stxs.size() != 2) throw RuntimeError("Wrong binding in let");
                    auto keySym = dynamic_cast<SymbolSyntax*>(kv->stxs[0].get());
                    if (!keySym) throw RuntimeError("Invalid let variable");
                    names.push_back(keySym->s);
                    pairs.push_back({keySym->s, parseSyntax(kv->stxs[1], env)});
                }

                // 占位符绑定
                Assoc bodyEnv = extendWithPlaceholders(names, env);
                vector<Expr> bodies = parseFromIndex(stxs, 2, bodyEnv);
                Expr body = bodies.size() == 1 ? bodies[0] : Expr(new Begin(bodies));
                return Expr(new Let(pairs, body));
            }
            case E_LETREC: {
                if (stxs.size() < 3) throw RuntimeError("Wrong number of arguments for letrec");
                auto binds = dynamic_cast<List*>(stxs[1].get());
                if (!binds) throw RuntimeError("Invalid binding list in letrec");

                vector<std::pair<string, Expr>> pairs;
                vector<string> names;
                pairs.reserve(binds->stxs.size());
                names.reserve(binds->stxs.size());

                // 环境(占位符)
                Assoc preEnv = env;
                for (auto &b : binds->stxs) {
                    auto kv = dynamic_cast<List*>(b.get());
                    if (!kv || kv->stxs.size() != 2) throw RuntimeError("Wrong binding in letrec");
                    auto keySym = dynamic_cast<SymbolSyntax*>(kv->stxs[0].get());
                    if (!keySym) throw RuntimeError("Invalid letrec variable");
                    names.push_back(keySym->s);
                    preEnv = extend(keySym->s, VoidV(), preEnv);
                }

                // 在占位符环境中parse rhs
                for (size_t i = 0; i < binds->stxs.size(); ++i) {
                    auto kv = dynamic_cast<List*>(binds->stxs[i].get());
                    auto keySym = dynamic_cast<SymbolSyntax*>(kv->stxs[0].get());
                    pairs.push_back({keySym->s, parseSyntax(kv->stxs[1], preEnv)});
                }

                // 在占位符符环境中parse body
                vector<Expr> bodies = parseFromIndex(stxs, 2, preEnv);
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
        }
        throw RuntimeError("Unknown reserved word: " + op);
    }

    vector<Expr> args = parseFromIndex(stxs, 1, env);
    return Expr(new Apply(Expr(new Var(op)), args));
}