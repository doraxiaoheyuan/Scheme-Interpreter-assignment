#include "syntax.hpp"
#include <cstring>
#include <vector>

Syntax::Syntax(SyntaxBase *stx) : ptr(stx) {}
SyntaxBase* Syntax::operator->() const { return ptr.get(); }
SyntaxBase& Syntax::operator*() { return *ptr; }
SyntaxBase* Syntax::get() const { return ptr.get(); }

Number::Number(int n) : n(n) {}
void Number::show(std::ostream &os) {
  os << "the-number-" << n;
}

RationalSyntax::RationalSyntax(int num, int den) : numerator(num), denominator(den) {}
void RationalSyntax::show(std::ostream &os) {
  os << numerator << "/" << denominator;
}

void TrueSyntax::show(std::ostream &os) {
  os << "#t";
}

void FalseSyntax::show(std::ostream &os) {
  os << "#f";
}

SymbolSyntax::SymbolSyntax(const std::string &s1) : s(s1) {}
void SymbolSyntax::show(std::ostream &os) {
    os << s;
}

StringSyntax::StringSyntax(const std::string &s1) : s(s1) {}
void StringSyntax::show(std::ostream &os) {
    os << "\"" << s << "\"";
}

List::List() {}
void List::show(std::ostream &os) {
    os << '(';
    for (auto stx : stxs) {
        stx->show(os);
        os << ' ';
    }
    os << ')';
}

std::istream &readSpace(std::istream &is) {
  while (true) {
    while (isspace(is.peek()))
      is.get();
    if (is.peek() == ';') {
      while (is.peek() != '\n' && is.peek() != EOF)
        is.get();
    } else {
      break;
    }
  }
  return is;
}

Syntax readList(std::istream &is);

static bool tryParseNumber(const std::string &s, int &result) {
  bool neg = false;
  int n = 0;
  int i = 0;
  if (s.size() == 1 && (s[0] == '+' || s[0] == '-'))
    return false;
  if (s[0] == '-') { i += 1; neg = true; }
  else if (s[0] == '+') { i += 1; }
  for (; i < (int)s.size(); i++) {
    if ('0' <= s[i] && s[i] <= '9') n = n * 10 + s[i] - '0';
    else return false;
  }
  result = neg ? -n : n;
  return true;
}

static bool tryParseRational(const std::string &s, int &numerator, int &denominator) {
  size_t slash_pos = s.find('/');
  if (slash_pos == std::string::npos || slash_pos == 0 || slash_pos == s.size() - 1)
    return false;
  std::string num_str = s.substr(0, slash_pos);
  std::string den_str = s.substr(slash_pos + 1);
  if (!tryParseNumber(num_str, numerator)) return false;
  if (!tryParseNumber(den_str, denominator) || denominator <= 0) return false;
  return true;
}

static Syntax createIdentifierSyntax(const std::string &s) {
  if (s == "#t") return Syntax(new TrueSyntax());
  if (s == "#f") return Syntax(new FalseSyntax());
  return Syntax(new SymbolSyntax(s));
}

// no leading space
Syntax readItem(std::istream &is) {
  if (is.peek() == '(' || is.peek() == '[') {
    is.get();
    return readList(is);
  }
  if (is.peek() == '\'') {
    is.get();
    Syntax quoted_syntax = readItem(is);
    List *quote_list = new List();
    quote_list->stxs.push_back(Syntax(new SymbolSyntax("quote")));
    quote_list->stxs.push_back(quoted_syntax);
    return Syntax(quote_list);
  }
  if (is.peek() == '"') {
    is.get();
    std::string str;
    while (is.peek() != '"' && is.peek() != EOF) {
      char c = is.get();
      if (c == '\\') {
        char next = is.get();
        switch (next) {
          case 'n': str.push_back('\n'); break;
          case 't': str.push_back('\t'); break;
          case 'r': str.push_back('\r'); break;
          case '\\': str.push_back('\\'); break;
          case '"': str.push_back('"'); break;
          default: str.push_back(next); break;
        }
      } else {
        str.push_back(c);
      }
    }
    if (is.peek() == '"') is.get();
    return Syntax(new StringSyntax(str));
  }
  
  std::string s;
  do {
    int c = is.peek();
    if (c == '(' || c == ')' ||
        c == '[' || c == ']' || 
        c == ';' ||
        isspace(c) ||
        c == EOF) break;
    is.get();
    s.push_back(c);
  } while (true);
  
  int numerator, denominator;
  if (tryParseRational(s, numerator, denominator)) {
    return Syntax(new RationalSyntax(numerator, denominator));
  }
  int number_value;
  if (tryParseNumber(s, number_value)) {
    return Syntax(new Number(number_value));
  }
  return createIdentifierSyntax(s);
}

Syntax readList(std::istream &is) {
    List *stx = new List();
    while (readSpace(is).peek() != ')' && readSpace(is).peek() != ')')
        stx->stxs.push_back(readItem(is));
    is.get(); // ')'
    return Syntax(stx);
}

Syntax readSyntax(std::istream &is) {
  return readItem(readSpace(is));
}

std::istream &operator>>(std::istream &is, Syntax &stx) {
  stx = readSyntax(is);
  return is;
}