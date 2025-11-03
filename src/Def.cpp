/**
 * @file Def.cpp
 * @brief Implementation of primitive functions and reserved words mappings
 * 
 * This file defines the mapping tables that associate Scheme function names
 * and special forms with their corresponding internal expression types.
 */

#include "Def.hpp"

/**
 * @brief Mapping of primitive function names to expression types
 */
std::map<std::string, ExprType> primitives = {
    // Arithmetic operations
    {"+",        E_PLUS},
    {"-",        E_MINUS},
    {"*",        E_MUL},
    {"/",        E_DIV},
    {"modulo",   E_MODULO},
    {"expt",     E_EXPT},
    
    // Comparison operations
    {"<",        E_LT},
    {"<=",       E_LE},
    {"=",        E_EQ},
    {">=",       E_GE},
    {">",        E_GT},

     // List operations
    {"cons",      E_CONS},
    {"car",       E_CAR},
    {"cdr",       E_CDR},
    {"list",      E_LIST},
    {"set-car!",  E_SETCAR},
    {"set-cdr!",  E_SETCDR},

    // Logic operations
    {"not",       E_NOT},
    {"and",       E_AND},
    {"or",        E_OR},
    
    // Type predicates
    {"eq?",        E_EQQ},
    {"boolean?",   E_BOOLQ},
    {"number?",    E_INTQ},      
    {"null?",      E_NULLQ},
    {"pair?",      E_PAIRQ},
    {"procedure?", E_PROCQ},
    {"symbol?",    E_SYMBOLQ},
    {"list?",      E_LISTQ},
    {"string?",    E_STRINGQ},
    
    // I/O operations
    {"display",   E_DISPLAY},
    
    // Special values and control
    {"void",      E_VOID},
    {"exit",      E_EXIT}
};

/**
 * @brief Mapping of reserved words (special forms) to expression types
 */
std::map<std::string, ExprType> reserved_words = {
    {"begin",   E_BEGIN},    
    {"quote",   E_QUOTE},    

    {"if",      E_IF},       
    {"cond",    E_COND},     

    {"lambda",  E_LAMBDA},   

    {"define",  E_DEFINE},   

    {"let",     E_LET},      
    {"letrec",  E_LETREC},   
    
    {"set!",    E_SET}      
};