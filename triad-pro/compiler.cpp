// triad_lexer.hpp
// Triad Language Lexer — v1.0
// C++17, single-file, no deps.

#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <cctype>
#include <limits>
#include <sstream>

namespace triad {

struct SourcePos {
    int line = 1;
    int column = 1;
};

enum class TokenType {
    // Structure
    Eof,
    Eol,            // newline or ';'
    Identifier,
    Mode,           // Fastest/Softest/Hardest/Brightest/Deepest/... (recognized adverbs)
    Register,       // R0..R15 (or more)
    Number,
    String,

    // Punctuation
    LParen, RParen, LBrace, RBrace, LBracket, RBracket,
    Comma, Colon, Dot, Semicolon, Hash, // (Hash is only used if you want separate; we fold to immediate numbers)

    // Operators
    Plus, Minus, Star, Slash, Percent,
    Equal, EqualEqual, BangEqual,
    Less, LessEqual, Greater, GreaterEqual,

    // Logical keywords
    KwNot, KwAnd, KwOr,

    // Core keywords
    KwMacro, KwEnd, KwCapsule, KwLet, KwReturn, KwIf, KwElse, KwLoop, KwJump, KwFrom, KwTo,
    KwSay, KwEcho, KwTone, KwTrace, KwMutate, KwLoad, // Load|load → KwLoad

    // Types & OOP
    KwStruct, KwClass, KwEnum, KwFunc, KwInit, KwNew, KwThis,

    // Exceptions
    KwTry, KwCatch, KwFinally, KwThrow,

    // Modules
    KwImport, KwAs, KwUsing, KwWith,

    // Literals
    KwTrue, KwFalse, KwNull,
};

struct Token {
    TokenType type{};
    std::string lexeme;     // original slice (for identifiers/strings/raw)
    SourcePos pos{};
    // Optional payloads:
    bool hasNumber = false;
    double numberValue = 0.0;
    bool immediate = false; // true if number came from #<digits> form
    int regIndex = -1;      // for Register tokens (R7 -> 7)
};

struct LexError : std::runtime_error {
    SourcePos pos;
    explicit LexError(const std::string& msg, SourcePos p)
        : std::runtime_error(msg), pos(p) {}
};

class Lexer {
public:
    explicit Lexer(std::string src)
        : m_src(std::move(src)) { m_len = static_cast<int>(m_src.size()); }

    std::vector<Token> tokenize() {
        std::vector<Token> out;
        while (!isAtEnd()) {
            skipWhitespaceAndComments(out);
            if (isAtEnd()) break;

            const char c = peek();
            const SourcePos startPos = m_pos;

            // EOL or Semicolon → Eol
            if (c == ';') {
                advance();
                out.push_back(makeSimple(TokenType::Eol, ";", startPos));
                continue;
            }

            // Punctuation
            switch (c) {
                case '(': out.push_back(makeSimple(TokenType::LParen, consumeChar(), startPos)); continue;
                case ')': out.push_back(makeSimple(TokenType::RParen, consumeChar(), startPos)); continue;
                case '{': out.push_back(makeSimple(TokenType::LBrace, consumeChar(), startPos)); continue;
                case '}': out.push_back(makeSimple(TokenType::RBrace, consumeChar(), startPos)); continue;
                case '[': out.push_back(makeSimple(TokenType::LBracket, consumeChar(), startPos)); continue;
                case ']': out.push_back(makeSimple(TokenType::RBracket, consumeChar(), startPos)); continue;
                case ',': out.push_back(makeSimple(TokenType::Comma, consumeChar(), startPos)); continue;
                case ':': out.push_back(makeSimple(TokenType::Colon, consumeChar(), startPos)); continue;
                case '.': out.push_back(makeSimple(TokenType::Dot, consumeChar(), startPos)); continue;
            }

            // Operators (two-char first)
            if (c == '=') {
                advance();
                if (match('=')) out.push_back(makeSimple(TokenType::EqualEqual, "==", startPos));
                else            out.push_back(makeSimple(TokenType::Equal, "=", startPos));
                continue;
            }
            if (c == '!') {
                advance();
                if (match('=')) out.push_back(makeSimple(TokenType::BangEqual, "!=", startPos));
                else throw LexError("Unexpected '!'", startPos);
                continue;
            }
            if (c == '<') {
                advance();
                if (match('=')) out.push_back(makeSimple(TokenType::LessEqual, "<=", startPos));
                else            out.push_back(makeSimple(TokenType::Less, "<", startPos));
                continue;
            }
            if (c == '>') {
                advance();
                if (match('=')) out.push_back(makeSimple(TokenType::GreaterEqual, ">=", startPos));
                else            out.push_back(makeSimple(TokenType::Greater, ">", startPos));
                continue;
            }
            if (c == '+') { out.push_back(makeSimple(TokenType::Plus,  consumeChar(), startPos)); continue; }
            if (c == '-') { out.push_back(makeSimple(TokenType::Minus, consumeChar(), startPos)); continue; }
            if (c == '*') { out.push_back(makeSimple(TokenType::Star,  consumeChar(), startPos)); continue; }
            if (c == '%') { out.push_back(makeSimple(TokenType::Percent,consumeChar(), startPos)); continue; }

            // Slash may be division OR start of comment (comments already skipped above)
            if (c == '/') {
                // At this point it must be division, because comment starts were consumed earlier
                out.push_back(makeSimple(TokenType::Slash, consumeChar(), startPos));
                continue;
            }

            // String
            if (c == '"') {
                out.push_back(readString());
                continue;
            }

            // Immediate number: #<digits/hex/bin/oct/float?>
            if (c == '#') {
                // we treat '#123' as a number token with immediate=true
                advance(); // consume '#'
                if (!std::isdigit(peek()) && !(peek() == '.' && std::isdigit(peekNext())))
                    throw LexError("Expected number after '#'", startPos);
                Token num = readNumber(startPos);
                num.immediate = true;
                out.push_back(std::move(num));
                continue;
            }

            // Number (regular)
            if (std::isdigit(c) || (c == '.' && std::isdigit(peekNext()))) {
                out.push_back(readNumber(startPos));
                continue;
            }

            // Register: R + digits (R0..R15...)
            if ((c == 'R') && std::isdigit(peekNext())) {
                Token reg = readRegister(startPos);
                out.push_back(std::move(reg));
                continue;
            }

            // Identifier / keyword / mode
            if (isIdentStart(c)) {
                out.push_back(readIdentOrKeyword(startPos));
                continue;
            }

            // Newline becomes Eol (if not already consumed)
            if (isNewline(c)) {
                consumeNewline(out);
                continue;
            }

            // Unknown char
            std::ostringstream oss;
            oss << "Unexpected character: '" << c << "'";
            throw LexError(oss.str(), startPos);
        }

        // ensure trailing Eof
        out.push_back(makeSimple(TokenType::Eof, "", m_pos));
        return out;
    }

private:
    std::string m_src;
    int m_len = 0;
    int m_idx = 0;
    SourcePos m_pos{1,1};

    // ---------- utilities ----------
    bool isAtEnd() const { return m_idx >= m_len; }
    char peek() const { return isAtEnd() ? '\0' : m_src[m_idx]; }
    char peekNext() const { return (m_idx+1 < m_len) ? m_src[m_idx+1] : '\0'; }

    char advance() {
        if (isAtEnd()) return '\0';
        char c = m_src[m_idx++];
        if (c == '\r') {
            // normalize CRLF and CR into LF handling in consumeNewline()
            if (peek() == '\n') ; // consume in consumeNewline
            // do not bump line/col here
        } else if (c == '\n') {
            // handled in consumeNewline
        } else {
            m_pos.column++;
        }
        return c;
    }

    std::string consumeChar() { char c = advance(); return std::string(1, c); }

    static bool isNewline(char c) { return c == '\n' || c == '\r'; }

    static bool isIdentStart(char c) {
        return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
    }
    static bool isIdentCont(char c) {
        return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
    }

    bool match(char expected) {
        if (isAtEnd() || m_src[m_idx] != expected) return false;
        m_idx++;
        m_pos.column++;
        return true;
    }

    Token makeSimple(TokenType t, const std::string& lex, SourcePos at) {
        Token tok;
        tok.type = t;
        tok.lexeme = lex;
        tok.pos = at;
        return tok;
    }
    Token makeSimple(TokenType t, const char* lex, SourcePos at) {
        return makeSimple(t, std::string(lex), at);
    }

    // ---------- whitespace, comments, and line continuation ----------
    void skipWhitespaceAndComments(std::vector<Token>& out) {
        for (;;) {
            // skip spaces & tabs
            while (!isAtEnd()) {
                char c = peek();
                if (c == ' ' || c == '\t') { advance(); continue; }
                // line continuation: backslash then newline => skip both, no Eol
                if (c == '\\' && (peekNext() == '\n' || peekNext() == '\r')) {
                    advance(); // backslash
                    // consume newline sequence
                    consumeNewline(/*emitEol=*/false, out);
                    continue;
                }
                break;
            }
            if (isAtEnd()) return;

            // Comments?
            // 1) '#' to end of line
            if (peek() == '#') {
                while (!isAtEnd() && !isNewline(peek())) advance();
                // do not emit Eol yet; let outer loop handle newline
                continue;
            }
            // 2) '//' to end of line
            if (peek() == '/' && peekNext() == '/') {
                advance(); advance();
                while (!isAtEnd() && !isNewline(peek())) advance();
                continue;
            }
            // 3) '/* ... */' with nesting
            if (peek() == '/' && peekNext() == '*') {
                advance(); advance(); // consume '/*'
                skipBlockComment();
                continue;
            }

            // Newline → Eol (unless contiguous multiple newlines; emit one Eol per physical newline)
            if (isNewline(peek())) {
                consumeNewline(/*emitEol=*/true, out);
                continue;
            }

            // No more whitespace/comment/newline to skip
            break;
        }
    }

    void consumeNewline(bool emitEol, std::vector<Token>& out) {
        SourcePos at = m_pos;
        // Normalize CRLF / CR / LF: consume '\r' optionally then '\n' optionally, bump line once
        if (peek() == '\r') {
            advance(); // '\r'
            if (peek() == '\n') advance(); // '\n'
        } else if (peek() == '\n') {
            advance();
        }
        // move to next line
        m_pos.line++;
        m_pos.column = 1;

        if (emitEol)
            out.push_back(makeSimple(TokenType::Eol, "\\n", at));
    }

    void consumeNewline(std::vector<Token>& out) { consumeNewline(true, out); }

    void skipBlockComment() {
        int depth = 1;
        while (!isAtEnd() && depth > 0) {
            if (peek() == '/' && peekNext() == '*') {
                advance(); advance(); // '/*'
                depth++;
            } else if (peek() == '*' && peekNext() == '/') {
                advance(); advance(); // '*/'
                depth--;
            } else if (isNewline(peek())) {
                // bump line/col accurately
                // reuse dummy vector to advance line
                std::vector<Token> dummy;
                consumeNewline(false, dummy);
            } else {
                advance();
            }
        }
        if (depth != 0) throw LexError("Unterminated block comment", m_pos);
    }

    // ---------- strings ----------
    Token readString() {
        SourcePos startPos = m_pos;
        std::string value;
        advance(); // consume opening '"'

        while (!isAtEnd()) {
            char c = peek();
            if (c == '"') { advance(); break; }
            if (isNewline(c)) throw LexError("Unterminated string literal", startPos);
            if (c == '\\') {
                advance(); // '\'
                char esc = peek();
                if (isAtEnd()) throw LexError("Unterminated string escape", startPos);
                switch (esc) {
                    case '"': value.push_back('"');  advance(); break;
                    case '\\':value.push_back('\\'); advance(); break;
                    case 'n': value.push_back('\n'); advance(); break;
                    case 'r': value.push_back('\r'); advance(); break;
                    case 't': value.push_back('\t'); advance(); break;
                    case 'u': {
                        // \uXXXX
                        advance();
                        std::string hex;
                        for (int i = 0; i < 4; ++i) {
                            char h = peek();
                            if (!std::isxdigit(static_cast<unsigned char>(h)))
                                throw LexError("Invalid \\u escape (expected 4 hex digits)", m_pos);
                            hex.push_back(h);
                            advance();
                        }
                        // We keep raw \uXXXX in value (or you can decode to UTF-8 if desired)
                        value += "\\u" + hex;
                        break;
                    }
                    default:
                        throw LexError("Unknown string escape", m_pos);
                }
            } else {
                value.push_back(c);
                advance();
            }
        }

        Token t;
        t.type = TokenType::String;
        t.lexeme = "\"" + value + "\""; // original-like (not exact if we normalized)
        t.pos = startPos;
        t.hasNumber = false;
        t.immediate = false;
        t.numberValue = 0.0;
        return t;
    }

    // ---------- numbers ----------
    Token readNumber(SourcePos startPos) {
        // Supports:
        //  - Decimal integers and floats: 123, 3.14, .5, 1e9
        //  - Hex: 0xFF
        //  - Bin: 0b1010
        //  - Oct: 0o755
        //
        // We parse into double; downstream can cast as needed.

        std::string s;
        int startIdx = m_idx;

        // handle prefixes
        if (peek() == '0' && (peekNext() == 'x' || peekNext() == 'X')) {
            // hex
            s.push_back(advance()); // 0
            s.push_back(advance()); // x
            if (!std::isxdigit(static_cast<unsigned char>(peek())))
                throw LexError("Expected hex digits after 0x", startPos);
            while (std::isxdigit(static_cast<unsigned char>(peek()))) s.push_back(advance());
            return makeNumberTokenFromBase(startPos, s, 16);
        }
        if (peek() == '0' && (peekNext() == 'b' || peekNext() == 'B')) {
            s.push_back(advance()); s.push_back(advance()); // 0b
            if (!(peek()=='0' || peek()=='1'))
                throw LexError("Expected binary digits after 0b", startPos);
            while (peek()=='0' || peek()=='1') s.push_back(advance());
            return makeNumberTokenFromBase(startPos, s, 2);
        }
        if (peek() == '0' && (peekNext() == 'o' || peekNext() == 'O')) {
            s.push_back(advance()); s.push_back(advance()); // 0o
            if (!(peek()>='0' && peek()<='7'))
                throw LexError("Expected octal digits after 0o", startPos);
            while (peek()>='0' && peek()<='7') s.push_back(advance());
            return makeNumberTokenFromBase(startPos, s, 8);
        }

        // decimal/float
        bool sawDigit = false;
        auto appendDigits = [&](bool requireOne) {
            int count = 0;
            while (std::isdigit(static_cast<unsigned char>(peek()))) {
                s.push_back(advance());
                count++;
            }
            if (requireOne && count == 0) throw LexError("Expected digits", startPos);
            if (count > 0) sawDigit = true;
        };

        // integer part or leading dot
        if (peek() == '.') {
            s.push_back(advance()); // '.'
            appendDigits(true);     // must have digits after dot: .5
        } else {
            appendDigits(true);     // 123
            // fractional?
            if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peekNext()))) {
                s.push_back(advance()); // '.'
                appendDigits(true);
            }
        }

        // exponent?
        if (peek() == 'e' || peek() == 'E') {
            s.push_back(advance());
            if (peek() == '+' || peek() == '-') s.push_back(advance());
            appendDigits(true);
        }

        Token t;
        t.type = TokenType::Number;
        t.lexeme = m_src.substr(startIdx, m_idx - startIdx);
        t.pos = startPos;
        t.hasNumber = true;
        t.numberValue = strToDoubleStrict(s, startPos);
        t.immediate = false;
        return t;
    }

    static double strToDoubleStrict(const std::string& s, SourcePos at) {
        // simple robust parse
        char* end = nullptr;
        errno = 0;
        double v = std::strtod(s.c_str(), &end);
        if (end == s.c_str() || *end != '\0' || errno == ERANGE) {
            std::ostringstream oss;
            oss << "Invalid numeric literal: " << s;
            throw LexError(oss.str(), at);
        }
        return v;
    }

    Token makeNumberTokenFromBase(SourcePos startPos, const std::string& lexeme, int base) {
        // lexeme includes prefix (0x/0b/0o); strip it for conversion
        std::string body = lexeme.substr(2);
        unsigned long long acc = 0ULL;
        for (char c : body) {
            int val;
            if (std::isdigit(static_cast<unsigned char>(c))) val = c - '0';
            else if (std::isxdigit(static_cast<unsigned char>(c))) {
                val = (std::tolower(static_cast<unsigned char>(c)) - 'a') + 10;
            } else {
                throw LexError("Invalid digit in literal", startPos);
            }
            if (val >= base) throw LexError("Digit out of range for base", startPos);
            // overflow-safe-ish accumulate
            unsigned long long prev = acc;
            acc = acc * base + static_cast<unsigned long long>(val);
            if (acc < prev) throw LexError("Integer literal overflow", startPos);
        }
        Token t;
        t.type = TokenType::Number;
        t.lexeme = lexeme;
        t.pos = startPos;
        t.hasNumber = true;
        t.numberValue = static_cast<double>(acc);
        t.immediate = false;
        return t;
    }

    // ---------- register ----------
    Token readRegister(SourcePos startPos) {
        // We know current is 'R' and next is digit.
        int start = m_idx;
        std::string s;
        s.push_back(advance()); // 'R'
        while (std::isdigit(static_cast<unsigned char>(peek()))) {
            s.push_back(advance());
        }
        // parse index
        int idx = 0;
        try {
            idx = std::stoi(s.substr(1));
        } catch (...) {
            throw LexError("Invalid register index", startPos);
        }
        Token t;
        t.type = TokenType::Register;
        t.lexeme = s;
        t.pos = startPos;
        t.regIndex = idx;
        return t;
    }

    // ---------- identifiers / keywords / modes ----------
    Token readIdentOrKeyword(SourcePos startPos) {
        std::string s;
        while (!isAtEnd() && isIdentCont(peek())) s.push_back(advance());

        // Keyword map (lowercased for case-insensitive acceptance where desired)
        std::string lower = toLower(s);
        if (auto it = keywordMap().find(lower); it != keywordMap().end()) {
            // Special case: accept both 'load' and 'Load'
            Token t;
            t.type = it->second;
            t.lexeme = s;
            t.pos = startPos;
            return t;
        }

        // Modes set
        if (isModeWord(s)) {
            Token t;
            t.type = TokenType::Mode;
            t.lexeme = s;
            t.pos = startPos;
            return t;
        }

        // Register-like names are already handled in readRegister
        // Fall back to Identifier
        Token t;
        t.type = TokenType::Identifier;
        t.lexeme = s;
        t.pos = startPos;
        return t;
    }

    static std::string toLower(const std::string& s) {
        std::string r; r.reserve(s.size());
        for (char c : s) r.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        return r;
    }

    static const std::unordered_map<std::string, TokenType>& keywordMap() {
        static const std::unordered_map<std::string, TokenType> m = {
            // logical
            {"not", TokenType::KwNot}, {"and", TokenType::KwAnd}, {"or", TokenType::KwOr},
            // core
            {"macro", TokenType::KwMacro}, {"end", TokenType::KwEnd}, {"capsule", TokenType::KwCapsule},
            {"let", TokenType::KwLet}, {"return", TokenType::KwReturn}, {"if", TokenType::KwIf},
            {"else", TokenType::KwElse}, {"loop", TokenType::KwLoop}, {"jump", TokenType::KwJump},
            {"from", TokenType::KwFrom}, {"to", TokenType::KwTo},
            {"say", TokenType::KwSay}, {"echo", TokenType::KwEcho}, {"tone", TokenType::KwTone},
            {"trace", TokenType::KwTrace}, {"mutate", TokenType::KwMutate},
            {"load", TokenType::KwLoad}, // accepts both 'load' and 'Load' via toLower
            // types & oop
            {"struct", TokenType::KwStruct}, {"class", TokenType::KwClass}, {"enum", TokenType::KwEnum},
            {"func", TokenType::KwFunc}, {"init", TokenType::KwInit}, {"new", TokenType::KwNew}, {"this", TokenType::KwThis},
            // exceptions
            {"try", TokenType::KwTry}, {"catch", TokenType::KwCatch}, {"finally", TokenType::KwFinally}, {"throw", TokenType::KwThrow},
            // modules
            {"import", TokenType::KwImport}, {"as", TokenType::KwAs}, {"using", TokenType::KwUsing}, {"with", TokenType::KwWith},
            // literals
            {"true", TokenType::KwTrue}, {"false", TokenType::KwFalse}, {"null", TokenType::KwNull},
        };
        return m;
    }

    static bool isModeWord(const std::string& s) {
        // Recognized adverbs (extend freely)
        static const char* MODES[] = {
            "Fastest","Softest","Hardest","Brightest","Deepest",
            "Sharpest","Quietest",
            "Deterministic","Sandboxed","Introspective","Mutable"
        };
        for (auto* w : MODES) if (s == w) return true;
        return false;
    }
};

} // namespace triad

/*
Usage example:

#include "triad_lexer.hpp"
#include <iostream>

int main() {
    std::string src = R"(
        macro sparkle(level):
          let shine = level * 2
          echo "Shine: " + shine
          return shine
        end

        capsule AgentMain [introspective, mutable]:
          Load R1 Fastest #3
          loop Deepest Repeat:
            say "✨"
            mutate R1 Softest -1
            jump Repeat Hardest if R1 > 0
          end

          let glow = sparkle(R1)
          if glow > 4:
            tone Brightest "C#5"
          else:
            tone Softest "A3"
          end

          try:
            throw "boom"
          catch e:
            echo "caught: " + e
          finally:
            echo "done"
          end

          trace capsule
        end
    )";

    triad::Lexer lx(src);
    try {
        auto toks = lx.tokenize();
        for (auto& t : toks) {
            std::cout << (int)t.type << "  \"" << t.lexeme << "\"  (" << t.pos.line << ":" << t.pos.column << ")";
            if (t.hasNumber) std::cout << "  num=" << t.numberValue << (t.immediate ? " (imm)" : "");
            if (t.type == triad::TokenType::Register) std::cout << "  R=" << t.regIndex;
            std::cout << "\n";
        }
    } catch (const triad::LexError& e) {
        std::cerr << "Lex error at " << e.pos.line << ":" << e.pos.column << " -> " << e.what() << "\n";
    }
}
*/

// triad_min.cpp
// Minimal Triad Parser + AST + Interpreter VM (works with triad_lexer.hpp)

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <memory>
#include <optional>
#include <stdexcept>
#include <cmath>
#include "triad_lexer.hpp"   // from previous message

using triad::Lexer;
using triad::Token;
using triad::TokenType;
using triad::SourcePos;
using triad::LexError;

// ---------- Values ----------
struct Value {
    // null, bool, number (double), string
    using V = std::variant<std::monostate, bool, double, std::string>;
    V v;

    Value(): v(std::monostate{}) {}
    static Value Null(){ return Value(); }
    static Value Bool(bool b){ Value x; x.v=b; return x; }
    static Value Num(double d){ Value x; x.v=d; return x; }
    static Value Str(const std::string&s){ Value x; x.v=s; return x; }

    bool isNull() const { return std::holds_alternative<std::monostate>(v); }
    bool isBool() const { return std::holds_alternative<bool>(v); }
    bool isNum()  const { return std::holds_alternative<double>(v); }
    bool isStr()  const { return std::holds_alternative<std::string>(v); }

    bool asBool() const {
        if (isBool()) return std::get<bool>(v);
        if (isNum())  return std::get<double>(v)!=0.0;
        if (isStr())  return !std::get<std::string>(v).empty();
        return false;
    }
    double asNum() const {
        if (isNum()) return std::get<double>(v);
        if (isBool()) return std::get<bool>(v)?1.0:0.0;
        if (isStr())  return std::stod(std::get<std::string>(v));
        return 0.0;
    }
    std::string toString() const {
        if (isNull()) return "null";
        if (isBool()) return std::get<bool>(v) ? "true" : "false";
        if (isNum())  { char buf[64]; std::snprintf(buf,64,"%.15g", std::get<double>(v)); return buf; }
        return std::get<std::string>(v);
    }
};

// ---------- Exceptions ----------
struct TriadException {
    Value payload;
    explicit TriadException(Value v): payload(std::move(v)) {}
};

// ---------- AST ----------
struct Expr {
    virtual ~Expr() = default;
    virtual Value eval(struct Context&) = 0;
};

using ExprPtr = std::unique_ptr<Expr>;

struct E_Literal : Expr {
    Value val; explicit E_Literal(Value v): val(std::move(v)) {}
    Value eval(struct Context&) override { return val; }
};

struct E_Var : Expr {
    std::string name; explicit E_Var(std::string n): name(std::move(n)) {}
    Value eval(struct Context& cx) override;
};

struct E_Unary : Expr {
    std::string op; ExprPtr rhs;
    E_Unary(std::string o, ExprPtr r): op(std::move(o)), rhs(std::move(r)) {}
    Value eval(struct Context& cx) override;
};

struct E_Binary : Expr {
    std::string op; ExprPtr lhs, rhs;
    E_Binary(ExprPtr a, std::string o, ExprPtr b): op(std::move(o)), lhs(std::move(a)), rhs(std::move(b)) {}
    Value eval(struct Context& cx) override;
};

struct E_Call : Expr {
    std::string name; std::vector<ExprPtr> args;
    E_Call(std::string n, std::vector<ExprPtr> a): name(std::move(n)), args(std::move(a)) {}
    Value eval(struct Context& cx) override;
};

// ---------- Statements ----------
struct Stmt { virtual ~Stmt()=default; virtual void exec(struct Context&)=0; };
using StmtPtr = std::unique_ptr<Stmt>;

struct S_Let : Stmt {
    std::string name; ExprPtr expr;
    S_Let(std::string n, ExprPtr e): name(std::move(n)), expr(std::move(e)) {}
    void exec(struct Context& cx) override;
};

struct S_Say : Stmt {
    ExprPtr e; explicit S_Say(ExprPtr x): e(std::move(x)) {}
    void exec(struct Context& cx) override;
};

struct S_Echo : Stmt {
    ExprPtr e; explicit S_Echo(ExprPtr x): e(std::move(x)) {}
    void exec(struct Context& cx) override;
};

struct S_Tone : Stmt {
    // mode ignored for now; keep string param
    std::string modeOpt; ExprPtr note;
    S_Tone(std::string m, ExprPtr n): modeOpt(std::move(m)), note(std::move(n)) {}
    void exec(struct Context& cx) override;
};

struct S_Load : Stmt {
    int reg; double val;
    S_Load(int r, double v): reg(r), val(v) {}
    void exec(struct Context& cx) override;
};

struct S_Mutate : Stmt {
    int reg; char op; double amt; // '+', '-', '*', '/'
    S_Mutate(int r, char o, double a): reg(r), op(o), amt(a) {}
    void exec(struct Context& cx) override;
};

struct S_If : Stmt {
    ExprPtr cond; std::vector<StmtPtr> thenS, elseS;
    S_If(ExprPtr c, std::vector<StmtPtr> t, std::vector<StmtPtr> e)
    : cond(std::move(c)), thenS(std::move(t)), elseS(std::move(e)) {}
    void exec(struct Context& cx) override;
};

struct S_Return : Stmt {
    std::optional<ExprPtr> val;
    explicit S_Return(std::optional<ExprPtr> v): val(std::move(v)) {}
    void exec(struct Context& cx) override;
};

struct S_Throw : Stmt {
    ExprPtr e; explicit S_Throw(ExprPtr x): e(std::move(x)) {}
    void exec(struct Context& cx) override;
};

struct S_Try : Stmt {
    std::vector<StmtPtr> body;
    std::optional<std::string> catchName; // bind name
    std::vector<StmtPtr> catchBody;
    std::vector<StmtPtr> finallyBody;
    S_Try(std::vector<StmtPtr> b,
          std::optional<std::string> cn,
          std::vector<StmtPtr> cb,
          std::vector<StmtPtr> fb)
    : body(std::move(b)), catchName(std::move(cn)), catchBody(std::move(cb)), finallyBody(std::move(fb)) {}
    void exec(struct Context& cx) override;
};

struct S_Trace : Stmt {
    std::string what; explicit S_Trace(std::string w): what(std::move(w)) {}
    void exec(struct Context& cx) override;
};

struct S_ExprStmt : Stmt {
    ExprPtr e; explicit S_ExprStmt(ExprPtr x): e(std::move(x)) {}
    void exec(struct Context& cx) override { e->eval(cx); }
};

struct S_Loop : Stmt {
    std::string label;
    std::vector<StmtPtr> body;
    explicit S_Loop(std::string lab, std::vector<StmtPtr> b): label(std::move(lab)), body(std::move(b)) {}
    void exec(struct Context& cx) override;
};

struct S_Jump : Stmt {
    std::string label; std::optional<ExprPtr> cond;
    S_Jump(std::string l, std::optional<ExprPtr> c): label(std::move(l)), cond(std::move(c)) {}
    void exec(struct Context& cx) override;
};

// ---------- Functions / Capsules ----------
struct Function {
    std::vector<std::string> params;
    std::vector<StmtPtr> body;
};

struct Capsule {
    std::string name;
    std::vector<StmtPtr> body;
    bool introspective=false, mutableCap=false;
};

// ---------- Context / VM ----------
struct LoopFrame {
    std::string label;
    bool requestJump=false;
};

struct CallFrame {
    std::unordered_map<std::string, Value> locals;
};

struct Context {
    // registers
    double R[16]{0};

    // variables (capsule-level)
    std::unordered_map<std::string, Value> vars;

    // functions
    std::unordered_map<std::string, Function> functions;

    // capsules
    std::unordered_map<std::string, Capsule> capsules;

    // stacks
    std::vector<LoopFrame> loopStack;
    std::vector<CallFrame> callStack;

    // return handling in functions/capsules
    bool hasReturn=false;
    Value returnValue;

    // helpers
    Value getVar(const std::string& n) {
        for (auto it = callStack.rbegin(); it != callStack.rend(); ++it) {
            auto f = it->locals.find(n);
            if (f != it->locals.end()) return f->second;
        }
        auto it = vars.find(n);
        if (it!=vars.end()) return it->second;
        if (n.rfind("R",0)==0 && n.size()>=2) {
            // allow "R3" as identifier access in expressions
            int idx = std::stoi(n.substr(1));
            if (idx>=0 && idx<16) return Value::Num(R[idx]);
        }
        return Value::Null();
    }
    void setVar(const std::string& n, const Value& v) {
        if (!callStack.empty() && callStack.back().locals.count(n)) {
            callStack.back().locals[n]=v; return;
        }
        vars[n]=v;
    }
};

// ---------- Expr impl ----------
Value E_Var::eval(Context& cx) { return cx.getVar(name); }

Value E_Unary::eval(Context& cx) {
    Value r = rhs->eval(cx);
    if (op=="-") return Value::Num(-r.asNum());
    if (op=="+") return Value::Num(+r.asNum());
    if (op=="not") return Value::Bool(!r.asBool());
    return Value::Null();
}

static int cmp(double a, double b){ if (a<b) return -1; if (a>b) return 1; return 0; }

Value E_Binary::eval(Context& cx) {
    Value A = lhs->eval(cx);
    // short-circuit
    if (op=="and") { return Value::Bool(A.asBool() && rhs->eval(cx).asBool()); }
    if (op=="or")  { return Value::Bool(A.asBool() || rhs->eval(cx).asBool()); }
    Value B = rhs->eval(cx);

    if (op=="+") {
        if (A.isStr() || B.isStr()) return Value::Str(A.toString()+B.toString());
        return Value::Num(A.asNum()+B.asNum());
    }
    if (op=="-") return Value::Num(A.asNum()-B.asNum());
    if (op=="*") return Value::Num(A.asNum()*B.asNum());
    if (op=="/") return Value::Num(A.asNum()/B.asNum());
    if (op=="%") return Value::Num(std::fmod(A.asNum(), B.asNum()));
    if (op=="<") return Value::Bool(cmp(A.asNum(),B.asNum())<0);
    if (op=="<=") return Value::Bool(cmp(A.asNum(),B.asNum())<=0);
    if (op==">") return Value::Bool(cmp(A.asNum(),B.asNum())>0);
    if (op==">=") return Value::Bool(cmp(A.asNum(),B.asNum())>=0);
    if (op=="==") {
        if (A.isStr() || B.isStr()) return Value::Bool(A.toString()==B.toString());
        return Value::Bool(A.asNum()==B.asNum());
    }
    if (op=="!=") {
        if (A.isStr() || B.isStr()) return Value::Bool(A.toString()!=B.toString());
        return Value::Bool(A.asNum()!=B.asNum());
    }
    return Value::Null();
}

Value E_Call::eval(Context& cx) {
    auto it = cx.functions.find(name);
    if (it==cx.functions.end()) {
        // builtins callable as functions too
        if (name=="say" && args.size()==1) { Value v=args[0]->eval(cx); std::cout<<v.toString()<<std::endl; return Value::Null(); }
        if (name=="echo"&& args.size()==1) { Value v=args[0]->eval(cx); std::cerr<<v.toString()<<std::endl; return Value::Null(); }
        throw std::runtime_error("Unknown function: "+name);
    }
    const Function& fn = it->second;
    if (fn.params.size()!=args.size()) throw std::runtime_error("Arity mismatch in call to "+name);

    cx.callStack.push_back({});
    for (size_t i=0;i<args.size();++i){
        cx.callStack.back().locals[fn.params[i]] = args[i]->eval(cx);
    }
    cx.hasReturn=false; cx.returnValue = Value::Null();
    try {
        for (auto& s: fn.body) {
            s->exec(cx);
            if (cx.hasReturn) break;
        }
    } catch (TriadException&) {
        cx.callStack.pop_back();
        throw; // propagate
    }
    Value rv = cx.returnValue;
    cx.callStack.pop_back();
    return rv;
}

// ---------- Stmt impl ----------
void S_Let::exec(Context& cx) { cx.setVar(name, expr->eval(cx)); }

void S_Say::exec(Context& cx) { std::cout << e->eval(cx).toString() << std::endl; }
void S_Echo::exec(Context& cx){ std::cerr << e->eval(cx).toString() << std::endl; }

void S_Tone::exec(Context& cx){
    std::cout << "[tone" << (modeOpt.empty()?"":(":"+modeOpt)) << "] "
              << e->eval(cx).toString() << std::endl;
}

void S_Load::exec(Context& cx){
    if (reg<0||reg>=16) throw std::runtime_error("Bad register");
    cx.R[reg]=val;
}
void S_Mutate::exec(Context& cx){
    if (reg<0||reg>=16) throw std::runtime_error("Bad register");
    if (op=='+') cx.R[reg]+=amt;
    else if (op=='-') cx.R[reg]-=amt;
    else if (op=='*') cx.R[reg]*=amt;
    else if (op=='/') cx.R[reg]/=amt;
}

void S_If::exec(Context& cx){
    if (cond->eval(cx).asBool()) {
        for (auto& s: thenS) { s->exec(cx); if (cx.hasReturn) return; }
    } else {
        for (auto& s: elseS) { s->exec(cx); if (cx.hasReturn) return; }
    }
}

void S_Return::exec(Context& cx){
    cx.hasReturn=true;
    if (val) cx.returnValue = (*val)->eval(cx);
    else cx.returnValue = Value::Null();
}

void S_Throw::exec(Context& cx){
    Value v = e->eval(cx);
    throw TriadException(v);
}

void S_Try::exec(Context& cx){
    try{
        for (auto& s: body) { s->exec(cx); if (cx.hasReturn) break; }
    } catch (TriadException& ex) {
        if (catchName) cx.setVar(*catchName, ex.payload);
        // execute catch
        for (auto& s: catchBody) { s->exec(cx); if (cx.hasReturn) break; }
    }
    // finally
    for (auto& s: finallyBody) { s->exec(cx); if (cx.hasReturn) break; }
}

void S_Trace::exec(Context& cx){
    if (what=="capsule" || what=="all") {
        std::cerr << "[trace] vars:\n";
        for (auto& kv: cx.vars) std::cerr<<"  "<<kv.first<<" = "<<kv.second.toString()<<"\n";
    }
    if (what=="registers" || what=="all" || what=="capsule") {
        std::cerr << "[trace] registers:\n";
        for (int i=0;i<16;++i) std::cerr<<"  R"<<i<<" = "<<cx.R[i]<<"\n";
    }
    if (what=="vars") {
        std::cerr << "[trace] vars:\n";
        for (auto& kv: cx.vars) std::cerr<<"  "<<kv.first<<" = "<<kv.second.toString()<<"\n";
    }
}

void S_Loop::exec(Context& cx){
    cx.loopStack.push_back({label,false});
    for (;;) {
        // run body
        for (auto& s: body) {
            s->exec(cx);
            if (cx.hasReturn) { cx.loopStack.pop_back(); return; }
            if (!cx.loopStack.empty() && cx.loopStack.back().requestJump) break;
        }
        if (!cx.loopStack.empty() && cx.loopStack.back().requestJump) {
            // reset and continue loop from top
            cx.loopStack.back().requestJump = false;
            continue;
        }
        else break; // no jump requested; exit once-through loop
    }
    cx.loopStack.pop_back();
}

void S_Jump::exec(Context& cx){
    bool doJump = true;
    if (cond) doJump = (*cond)->eval(cx).asBool();
    if (!doJump) return;
    // find nearest loop with matching label
    for (auto it = cx.loopStack.rbegin(); it!=cx.loopStack.rend(); ++it) {
        if (it->label == label) { it->requestJump = true; return; }
    }
    throw std::runtime_error("No loop label found for jump: "+label);
}

// ---------- Parser ----------
class Parser {
public:
    explicit Parser(std::vector<Token> toks): t(std::move(toks)) {}

    void parseProgram(std::unordered_map<std::string, Function>& fns,
                      std::unordered_map<std::string, Capsule>& caps) {
        while (!match(TokenType::Eof)) {
            if (check(TokenType::Eol)) { advance(); continue; }
            if (match(TokenType::KwMacro) || match(TokenType::KwFunc)) {
                parseFunction(fns);
            } else if (match(TokenType::KwCapsule)) {
                parseCapsule(caps);
            } else {
                error("Top-level must be 'macro/func' or 'capsule'");
            }
        }
    }

private:
    std::vector<Token> t; int i=0;

    // --- helpers
    bool isAtEnd()  const { return i >= (int)t.size(); }
    const Token& peek() const { return t[i]; }
    const Token& prev() const { return t[i-1]; }
    bool check(TokenType k) const { return !isAtEnd() && t[i].type==k; }
    const Token& advance(){ if (!isAtEnd()) ++i; return prev(); }
    bool match(TokenType k){ if (check(k)){ advance(); return true; } return false; }
    void consumeEolOpt(){ while (match(TokenType::Eol)){} }

    [[noreturn]] void error(const std::string& m) {
        SourcePos p = isAtEnd()? prev().pos : peek().pos;
        throw std::runtime_error("Parse error at "+std::to_string(p.line)+":"+std::to_string(p.column)+" -> "+m);
    }

    bool isIdent(const Token& x) const {
        return x.type==TokenType::Identifier || x.type==TokenType::Mode; // allow Mode tokens as identifiers in some contexts
    }

    // --- parsing
    std::string parseIdent(const std::string& ctx){
        if (!(peek().type==TokenType::Identifier || peek().type==TokenType::Mode)) error("Expected identifier in "+ctx);
        std::string s = peek().lexeme; advance(); return s;
    }

    // func/macro name(params): block end
    void parseFunction(std::unordered_map<std::string, Function>& fns){
        std::string name = parseIdent("function name");
        expect(TokenType::LParen, "Expected '(' after function name");
        std::vector<std::string> params;
        if (!check(TokenType::RParen)) {
            do {
                params.push_back(parseIdent("param"));
            } while (match(TokenType::Comma));
        }
        expect(TokenType::RParen, "Expected ')' after params");
        expect(TokenType::Colon, "Expected ':' starting function body");
        consumeEolOpt();
        auto body = parseBlock();
        expect(TokenType::KwEnd, "Expected 'end' to close function");
        consumeEolOpt();
        fns[name] = Function{params, std::move(body)};
    }

    void parseCapsule(std::unordered_map<std::string, Capsule>& caps){
        std::string name = parseIdent("capsule name");
        bool introspective=false, mutableCap=false;
        if (match(TokenType::LBracket)) {
            // [attr, ...]
            do {
                std::string attr = parseIdent("attribute");
                if (attr=="introspective") introspective=true;
                if (attr=="mutable") mutableCap=true;
            } while (match(TokenType::Comma));
            expect(TokenType::RBracket, "Expected ']' after attributes");
        }
        expect(TokenType::Colon, "Expected ':' after capsule header");
        consumeEolOpt();
        auto body = parseBlock();
        expect(TokenType::KwEnd, "Expected 'end' to close capsule");
        consumeEolOpt();
        caps[name] = Capsule{name, std::move(body), introspective, mutableCap};
    }

    std::vector<StmtPtr> parseBlock(){
        std::vector<StmtPtr> out;
        while (!check(TokenType::KwEnd) && !check(TokenType::Eof)) {
            if (check(TokenType::Eol)) { advance(); continue; }
            out.push_back(parseStmt());
        }
        return out;
    }

    StmtPtr parseStmt(){
        // let / say / echo / tone / Load / mutate / if / loop / jump / trace / return / try / throw / expr
        if (match(TokenType::KwLet))   return parseLet();
        if (match(TokenType::KwSay))   return std::make_unique<S_Say>(parseExpr());
        if (match(TokenType::KwEcho))  return std::make_unique<S_Echo>(parseExpr());
        if (match(TokenType::KwTone))  return parseTone();
        if (match(TokenType::KwLoad))  return parseLoad();
        if (match(TokenType::KwMutate))return parseMutate();
        if (match(TokenType::KwIf))    return parseIf();
        if (match(TokenType::KwLoop))  return parseLoop();
        if (match(TokenType::KwJump))  return parseJump();
        if (match(TokenType::KwTrace)) return parseTrace();
        if (match(TokenType::KwReturn))return parseReturn();
        if (match(TokenType::KwTry))   return parseTry();
        if (match(TokenType::KwThrow)) return parseThrow();

        // expression statement
        ExprPtr e = parseExpr();
        consumeEolOpt();
        return std::make_unique<S_ExprStmt>(std::move(e));
    }

    StmtPtr parseLet(){
        std::string name = parseIdent("let");
        expect(TokenType::Equal, "Expected '=' in let");
        ExprPtr e = parseExpr();
        consumeEolOpt();
        return std::make_unique<S_Let>(name, std::move(e));
    }

    StmtPtr parseTone(){
        // tone [Mode] "Note"
        std::string mode;
        if (peek().type==TokenType::Mode) { mode = peek().lexeme; advance(); }
        ExprPtr e = parseExpr();
        consumeEolOpt();
        return std::make_unique<S_Tone>(mode, std::move(e));
    }

    StmtPtr parseLoad(){
        // Load Rn [Mode] literal/number [accept #imm via lexer]
        int reg = 0;
        if (peek().type!=TokenType::Register) error("Expected register in Load");
        reg = peek().regIndex; advance();
        if (peek().type==TokenType::Mode) advance(); // ignore mode
        // next token must be number or string literal (we only accept number here)
        if (peek().type!=TokenType::Number) error("Expected numeric literal in Load");
        double v = peek().numberValue; advance();
        consumeEolOpt();
        return std::make_unique<S_Load>(reg, v);
    }

    StmtPtr parseMutate(){
        // mutate Rn [Mode] (+n|-n|*n|/n) OR a bare number treated as +n
        if (peek().type!=TokenType::Register) error("Expected register in mutate");
        int reg = peek().regIndex; advance();
        if (peek().type==TokenType::Mode) advance(); // ignore
        char op = '+';
        if (peek().type==TokenType::Plus || peek().type==TokenType::Minus ||
            peek().type==TokenType::Star || peek().type==TokenType::Slash) {
            op = peek().lexeme[0]; advance();
        }
        if (peek().type!=TokenType::Number) error("Expected number in mutate");
        double amt = peek().numberValue; advance();
        consumeEolOpt();
        return std::make_unique<S_Mutate>(reg, op, amt);
    }

    StmtPtr parseIf(){
        ExprPtr cond = parseExpr();
        expect(TokenType::Colon, "Expected ':' after if condition");
        consumeEolOpt();
        auto thenB = parseBlock();
        std::vector<StmtPtr> elseB;
        if (match(TokenType::KwElse)) {
            if (match(TokenType::Colon)){} // optional
            consumeEolOpt();
            elseB = parseBlock();
        }
        expect(TokenType::KwEnd, "Expected 'end' after if");
        consumeEolOpt();
        return std::make_unique<S_If>(std::move(cond), std::move(thenB), std::move(elseB));
        }

    StmtPtr parseLoop(){
        // loop [Mode] Label:
        if (peek().type==TokenType::Mode) advance(); // ignore mode
        std::string label = parseIdent("loop label");
        expect(TokenType::Colon, "Expected ':' after loop label");
        consumeEolOpt();
        auto body = parseBlock();
        expect(TokenType::KwEnd, "Expected 'end' after loop");
        consumeEolOpt();
        return std::make_unique<S_Loop>(label, std::move(body));
    }

    StmtPtr parseJump(){
        std::string label = parseIdent("jump label");
        if (peek().type==TokenType::Mode) advance(); // ignore mode
        std::optional<ExprPtr> cond;
        if (match(TokenType::KwIf)) cond = parseExpr();
        consumeEolOpt();
        return std::make_unique<S_Jump>(label, std::move(cond));
    }

    StmtPtr parseTrace(){
        std::string w = parseIdent("trace");
        consumeEolOpt();
        return std::make_unique<S_Trace>(w);
    }

    StmtPtr parseReturn(){
        // optional expr
        if (check(TokenType::Eol) || check(TokenType::KwEnd)) {
            consumeEolOpt();
            return std::make_unique<S_Return>(std::nullopt);
        }
        ExprPtr e = parseExpr();
        consumeEolOpt();
        return std::make_unique<S_Return>(std::optional<ExprPtr>(std::move(e)));
    }

    StmtPtr parseThrow(){
        ExprPtr e = parseExpr();
        consumeEolOpt();
        return std::make_unique<S_Throw>(std::move(e));
    }

    StmtPtr parseTry(){
        expect(TokenType::Colon, "Expected ':' after try");
        consumeEolOpt();
        auto body = parseBlock();
        std::optional<std::string> catchName;
        std::vector<StmtPtr> cbody;
        if (match(TokenType::KwCatch)) {
            // 'catch name' [as Type]?  (Type ignored in this minimal impl)
            std::string nm = parseIdent("catch name");
            if (match(TokenType::KwAs)) { parseIdent("catch type"); } // ignored type
            if (match(TokenType::Colon)) {}
            consumeEolOpt();
            cbody = parseBlock();
            catchName = nm;
        }
        std::vector<StmtPtr> fbody;
        if (match(TokenType::KwFinally)) {
            if (match(TokenType::Colon)) {}
            consumeEolOpt();
            fbody = parseBlock();
        }
        expect(TokenType::KwEnd, "Expected 'end' after try/catch/finally");
        consumeEolOpt();
        return std::make_unique<S_Try>(std::move(body), std::move(catchName), std::move(cbody), std::move(fbody));
    }

    // ---------- Expressions (precedence climbing) ----------
    ExprPtr parseExpr(){ return parseOr(); }
    ExprPtr parseOr(){
        ExprPtr e = parseAnd();
        while (match(TokenType::KwOr)) e = std::make_unique<E_Binary>(std::move(e), "or", parseAnd());
        return e;
    }
    ExprPtr parseAnd(){
        ExprPtr e = parseEquality();
        while (match(TokenType::KwAnd)) e = std::make_unique<E_Binary>(std::move(e), "and", parseEquality());
        return e;
    }
    ExprPtr parseEquality(){
        ExprPtr e = parseRel();
        while (match(TokenType::EqualEqual) || match(TokenType::BangEqual)) {
            std::string op = prev().lexeme;
            e = std::make_unique<E_Binary>(std::move(e), op, parseRel());
        }
        return e;
    }
    ExprPtr parseRel(){
        ExprPtr e = parseAdd();
        while (match(TokenType::Less) || match(TokenType::LessEqual) ||
               match(TokenType::Greater) || match(TokenType::GreaterEqual)) {
            std::string op = prev().lexeme;
            e = std::make_unique<E_Binary>(std::move(e), op, parseAdd());
        }
        return e;
    }
    ExprPtr parseAdd(){
        ExprPtr e = parseMul();
        while (match(TokenType::Plus) || match(TokenType::Minus)) {
            std::string op = prev().lexeme;
            e = std::make_unique<E_Binary>(std::move(e), op, parseMul());
        }
        return e;
    }
    ExprPtr parseMul(){
        ExprPtr e = parseUnary();
        while (match(TokenType::Star) || match(TokenType::Slash) || match(TokenType::Percent)) {
            std::string op = prev().lexeme;
            e = std::make_unique<E_Binary>(std::move(e), op, parseUnary());
        }
        return e;
    }
    ExprPtr parseUnary(){
        if (match(TokenType::Minus) || match(TokenType::Plus) || match(TokenType::KwNot)) {
            std::string op = prev().type==TokenType::KwNot ? "not" : prev().lexeme;
            return std::make_unique<E_Unary>(op, parseUnary());
        }
        return parsePrimary();
    }
    ExprPtr parsePrimary(){
        if (match(TokenType::Number))  return std::make_unique<E_Literal>(Value::Num(prev().numberValue));
        if (match(TokenType::String))  return std::make_unique<E_Literal>(Value::Str(prev().lexeme.substr(1, prev().lexeme.size()-2)));
        if (match(TokenType::KwTrue))  return std::make_unique<E_Literal>(Value::Bool(true));
        if (match(TokenType::KwFalse)) return std::make_unique<E_Literal>(Value::Bool(false));
        if (match(TokenType::KwNull))  return std::make_unique<E_Literal>(Value::Null());
        if (match(TokenType::LParen)) {
            ExprPtr e = parseExpr();
            expect(TokenType::RParen, "Expected ')'");
            return e;
        }
        // identifier or call
        if (isIdent(peek())) {
            std::string name = parseIdent("expr");
            if (match(TokenType::LParen)) {
                std::vector<ExprPtr> args;
                if (!check(TokenType::RParen)) {
                    do { args.push_back(parseExpr()); } while (match(TokenType::Comma));
                }
                expect(TokenType::RParen, "Expected ')' after args");
                return std::make_unique<E_Call>(name, std::move(args));
            }
            return std::make_unique<E_Var>(name);
        }
        error("Expected expression");
        return nullptr;
    }

    void expect(TokenType k, const std::string& msg){ if (!match(k)) error(msg); }
};

// ---------- Driver helpers ----------
static void runCapsule(Context& cx, const std::string& name){
    auto it = cx.capsules.find(name);
    if (it==cx.capsules.end()) throw std::runtime_error("No capsule named "+name);
    cx.hasReturn=false;
    try{
        for (auto& s: it->second.body){
            s->exec(cx);
            if (cx.hasReturn) break;
        }
    }catch (TriadException& ex){
        std::cerr << "[uncaught] " << ex.payload.toString() << "\n";
    }
}

// ---------- Demo main ----------
static const char* DEMO = R"TRIAD(
macro sparkle(level):
  let shine = level * 2
  echo "Shine level: " + shine
  return shine
end

func sparkle(level):  # treated like macro for runtime
  let shine = level * 2
  echo "Shine level: " + shine
  return shine
end

capsule AgentMain [introspective, mutable]:
  Load R1 Fastest #3
  loop Deepest Repeat:
    say "✨"
    mutate R1 Softest -1
    jump Repeat Hardest if R1 > 0
  end

  let glow = sparkle(R1)
  if glow > 4:
    tone Brightest "C#5"
  else:
    tone Softest "A3"
  end

  try:
    echo "before throw?"
    # throw "boom"   # uncomment to test
  catch e:
    echo "caught: " + e
  finally:
    echo "cleanup"
  end

  trace capsule
end
)TRIAD";

int main() {
    try {
        // 1) Lex
        Lexer lx(DEMO);
        auto toks = lx.tokenize();

        // 2) Parse program
        Parser p(std::move(toks));
        Context cx;
        p.parseProgram(cx.functions, cx.capsules);

        // 3) Run a capsule
        runCapsule(cx, "AgentMain");
    } catch (const LexError& e) {
        std::cerr << "Lex error at " << e.pos.line << ":" << e.pos.column << " -> " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 2;
    }
    return 0;
}
