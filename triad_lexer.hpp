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
