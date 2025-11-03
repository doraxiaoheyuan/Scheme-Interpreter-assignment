#include "Def.hpp"
#include "syntax.hpp"
#include "expr.hpp"
#include "value.hpp"
#include "RE.hpp"
#include <iostream>
#include <map>
#include <vector>

extern std::map<std::string, ExprType> primitives;
extern std::map<std::string, ExprType> reserved_words;

static bool isExplicitVoidCall(Expr expr) {
    if (dynamic_cast<MakeVoid*>(expr.get())) {
        return true;
    }
    if (auto apply_expr = dynamic_cast<Apply*>(expr.get())) {
        if (auto var_expr = dynamic_cast<Var*>(apply_expr->rator.get())) {
            if (var_expr->x == "void") return true;
        }
    }
    if (auto begin_expr = dynamic_cast<Begin*>(expr.get())) {
        if (!begin_expr->es.empty()) return isExplicitVoidCall(begin_expr->es.back());
    }
    if (auto if_expr = dynamic_cast<If*>(expr.get())) {
        return isExplicitVoidCall(if_expr->conseq) || isExplicitVoidCall(if_expr->alter);
    }
    if (auto cond_expr = dynamic_cast<Cond*>(expr.get())) {
        for (const auto& clause : cond_expr->clauses) {
            if (!clause.empty() && isExplicitVoidCall(clause.back())) return true;
        }
    }
    return false;
}

void REPL(){
    Assoc global_env = empty();
    std::vector<std::pair<std::string,Expr>> pending_defines;

    while (true){
        #ifndef ONLINE_JUDGE
            std::cout << "scm> ";
        #endif
        Syntax stx = readSyntax(std::cin);
        try{
            Expr expr = stx->parse(global_env);

            // Batch top-level defines to support mutual recursion
            if (auto define_expr = dynamic_cast<Define*>(expr.get())) {
                pending_defines.push_back({define_expr->var, define_expr->e});
                continue;
            }
            if (!pending_defines.empty()) {
                // Create placeholders
                for (auto &def : pending_defines) {
                    if (find(def.first, global_env).get() == nullptr) {
                        global_env = extend(def.first, VoidV(), global_env);
                    }
                }
                // Evaluate each define body in the same env, then modify
                for (auto &def : pending_defines) {
                    Value val = def.second->eval(global_env);
                    modify(def.first, val, global_env);
                }
                pending_defines.clear();
            }

            Value val = expr->eval(global_env);
            if (val->v_type == V_TERMINATE) break;

            // Print unless #<void> from implicit void
            if (val->v_type != V_VOID || isExplicitVoidCall(expr)) {
                val->show(std::cout);
                std::cout << "\n";
            } else {
                std::cout << "\n";
            }
        }
        catch (const RuntimeError &){
            std::cout << "RuntimeError\n";
        }
    }
}

int main(int argc, char *argv[]) {
    REPL();
    return 0;
}