#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <cctype>
#include <stdexcept>
#include <utility>
#include <ostream>
#include <iostream>

namespace triad {

  enum class TokKind : uint8_t {
    Eof, Id, Num, Str,
    LParen, RParen, LBrace, RBrace, LBracket, RBracket,
    Comma, Dot, Colon, Semicolon, Range,
    Plus, Minus, Star, Slash, Percent,
    Eq, EqEq, Ne, Lt, Le, Gt, Ge, Bang,
    KwMacro, KwEnd, KwStruct, KwClass, KwEnum, KwPure, KwDef,
    KwTry, KwCatch, KwFinally, KwThrow,
    KwIf, KwElse, KwFor, KwIn, KwLoop, KwNew,
    KwAnd, KwOr, KwSay, KwEcho, KwReturn
  };

  struct Token {
    TokKind kind;
    std::string text;
    double number = 0.0;
    int line = 1;
    int col = 1;

    constexpr Token(TokKind k, std::string t = "", double n = 0.0, int l = 1, int c = 1)
      : kind(k), text(std::move(t)), number(n), line(l), col(c) {}
  };

  class Lexer {
    std::string_view src;
    size_t index = 0;
    int line = 1;
    int col = 1;

  public:
    explicit Lexer(std::string_view s) noexcept : src(s) {}

    [[nodiscard]] char peek(int offset = 0) const noexcept {
      return (index + offset < src.size()) ? src[index + offset] : '\0';
    }

    char get() noexcept {
      char c = peek();
      if (c == '\n') { ++line; col = 1; }
      else { ++col; }
      if (index < src.size()) ++index;
      return c;
    }

    static constexpr bool isIdStart(char c) noexcept {
      return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
    }

    static constexpr bool isIdChar(char c) noexcept {
      return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
    }

    void skipWhitespace() {
      while (true) {
        char c = peek();
        if (std::isspace(static_cast<unsigned char>(c))) {
          get();
        } else if (c == '/' && peek(1) == '/') {
          while (peek() && peek() != '\n') get();
        } else if (c == '/' && peek(1) == '*') {
          get(); get();
          while (peek() && !(peek() == '*' && peek(1) == '/')) get();
          if (peek()) { get(); get(); }
        } else {
          break;
        }
      }
    }

    [[nodiscard]] Token makeToken(TokKind kind, std::string text = "", double num = 0.0) const {
      return Token{kind, std::move(text), num, line, col};
    }

    Token lexNumber() {
      size_t start = index;
      while (std::isdigit(peek())) get();
      if (peek() == '.' && std::isdigit(peek(1))) { get(); get(); }
      while (std::isdigit(peek())) get();
      double val = std::stod(std::string(src.substr(start, index - start)));
      return makeToken(TokKind::Num, "", val);
    }

    Token lexString() {
      get(); // skip opening quote
      std::string result;
      while (peek() && peek() != '"') result.push_back(get());
      if (peek() == '"') get();
      return makeToken(TokKind::Str, result);
    }

    TokKind keyword(std::string_view id) const noexcept {
      std::string lowered;
      lowered.reserve(id.size());
      for (char c : id) lowered.push_back(std::tolower(static_cast<unsigned char>(c)));

      if (lowered == "macro") return TokKind::KwMacro;
      if (lowered == "end") return TokKind::KwEnd;
      if (lowered == "struct") return TokKind::KwStruct;
      if (lowered == "class") return TokKind::KwClass;
      if (lowered == "enum") return TokKind::KwEnum;
      if (lowered == "pure") return TokKind::KwPure;
      if (lowered == "def") return TokKind::KwDef;
      if (lowered == "try") return TokKind::KwTry;
      if (lowered == "catch") return TokKind::KwCatch;
      if (lowered == "finally") return TokKind::KwFinally;
      if (lowered == "throw") return TokKind::KwThrow;
      if (lowered == "if") return TokKind::KwIf;
      if (lowered == "else") return TokKind::KwElse;
      if (lowered == "for") return TokKind::KwFor;
      if (lowered == "in") return TokKind::KwIn;
      if (lowered == "loop") return TokKind::KwLoop;
      if (lowered == "new") return TokKind::KwNew;
      if (lowered == "and") return TokKind::KwAnd;
      if (lowered == "or") return TokKind::KwOr;
      if (lowered == "say") return TokKind::KwSay;
      if (lowered == "echo") return TokKind::KwEcho;
      if (lowered == "return") return TokKind::KwReturn;

      return TokKind::Id;
    }

    std::vector<Token> run(); // To be implemented in triad_lexer.cpp
  };

  inline const char* tok_kind_name(TokKind kind) {
    switch (kind) {
      case TokKind::Eof: return "Eof";
      case TokKind::Id: return "Id";
      case TokKind::Num: return "Num";
      case TokKind::Str: return "Str";
      case TokKind::LParen: return "LParen";
      case TokKind::RParen: return "RParen";
      case TokKind::LBrace: return "LBrace";
      case TokKind::RBrace: return "RBrace";
      case TokKind::LBracket: return "LBracket";
      case TokKind::RBracket: return "RBracket";
      case TokKind::Comma: return "Comma";
      case TokKind::Dot: return "Dot";
      case TokKind::Colon: return "Colon";
      case TokKind::Semicolon: return "Semicolon";
      case TokKind::Range: return "Range";
      case TokKind::Plus: return "Plus";
      case TokKind::Minus: return "Minus";
      case TokKind::Star: return "Star";
      case TokKind::Slash: return "Slash";
      case TokKind::Percent: return "Percent";
      case TokKind::Eq: return "Eq";
      case TokKind::EqEq: return "EqEq";
      case TokKind::Ne: return "Ne";
      case TokKind::Lt: return "Lt";
      case TokKind::Le: return "Le";
      case TokKind::Gt: return "Gt";
      case TokKind::Ge: return "Ge";
      case TokKind::Bang: return "Bang";
      case TokKind::KwMacro: return "KwMacro";
      case TokKind::KwEnd: return "KwEnd";
      case TokKind::KwStruct: return "KwStruct";
      case TokKind::KwClass: return "KwClass";
      case TokKind::KwEnum: return "KwEnum";
      case TokKind::KwPure: return "KwPure";
      case TokKind::KwDef: return "KwDef";
      case TokKind::KwTry: return "KwTry";
      case TokKind::KwCatch: return "KwCatch";
      case TokKind::KwFinally: return "KwFinally";
      case TokKind::KwThrow: return "KwThrow";
      case TokKind::KwIf: return "KwIf";
      case TokKind::KwElse: return "KwElse";
      case TokKind::KwFor: return "KwFor";
      case TokKind::KwIn: return "KwIn";
      case TokKind::KwLoop: return "KwLoop";
      case TokKind::KwNew: return "KwNew";
      case TokKind::KwAnd: return "KwAnd";
      case TokKind::KwOr: return "KwOr";
      case TokKind::KwSay: return "KwSay";
      case TokKind::KwEcho: return "KwEcho";
      case TokKind::KwReturn: return "KwReturn";
      default: return "Unknown";
    }
  }

  inline void dump_tokens(const std::vector<Token>& tokens, std::ostream& os = std::cout) {
    for (const auto& tok : tokens) {
      os << "Token(" << tok_kind_name(tok.kind)
         << ", text=\"" << tok.text << "\""
         << ", number=" << tok.number
         << ", line=" << tok.line
         << ", col=" << tok.col
         << ")\n";
    }
  }

} // namespace triad

#ifdef TRIAD_LEXER_DUMP_MAIN
#include <iostream>
#include <string>

int main() {
  using namespace triad;
  std::string src = "let x = 42; say x;";
  Lexer lexer(src);
  auto tokens = lexer.run();
  dump_tokens(tokens);
  return 0;
}
#endif
