#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <cctype>
#include <stdexcept>
#include <utility>
#include <ostream>
#include <iostream>
#include <map>

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

// --- Additional: Utility to convert a token to a string (single token pretty-printer) ---

namespace triad {

inline std::string token_to_string(const Token& tok) {
  std::ostringstream oss;
  oss << "Token(" << tok_kind_name(tok.kind)
      << ", text=\"" << tok.text << "\""
      << ", number=" << tok.number
      << ", line=" << tok.line
      << ", col=" << tok.col
      << ")";
  return oss.str();
}

// Utility: Find all tokens of a given kind in a token vector
inline std::vector<Token> find_tokens(const std::vector<Token>& tokens, TokKind kind) {
  std::vector<Token> result;
  for (const auto& tok : tokens) {
    if (tok.kind == kind) result.push_back(tok);
  }
  return result;
}

} // namespace triad

// --- Additional: Utility to count tokens by kind and print a summary ---

namespace triad {

inline std::map<TokKind, int> count_tokens_by_kind(const std::vector<Token>& tokens) {
  std::map<TokKind, int> counts;
  for (const auto& tok : tokens) {
    ++counts[tok.kind];
  }
  return counts;
}

inline void print_token_summary(const std::vector<Token>& tokens, std::ostream& os = std::cout) {
  auto counts = count_tokens_by_kind(tokens);
  os << "Token summary:\n";
  for (const auto& pair : counts) {
    os << "  " << tok_kind_name(pair.first) << ": " << pair.second << "\n";
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

#ifdef TRIAD_LEXER_UTILS_MAIN
#include <sstream>
#include <string>

int main() {
  using namespace triad;
  std::string src = "let x = 42; say x;";
  Lexer lexer(src);
  auto tokens = lexer.run();

  // Print all tokens using token_to_string
  for (const auto& tok : tokens) {
    std::cout << token_to_string(tok) << "\n";
  }

  // Find and print all identifier tokens
  auto ids = find_tokens(tokens, TokKind::Id);
  std::cout << "Identifiers found: " << ids.size() << "\n";
  for (const auto& id : ids) {
    std::cout << "  " << token_to_string(id) << "\n";
  }

  return 0;
}
#endif

#ifdef TRIAD_LEXER_SUMMARY_MAIN
#include <string>

int main() {
  using namespace triad;
  std::string src = "let x = 42; say x; if (x > 0) { echo x; }";
  Lexer lexer(src);
  auto tokens = lexer.run();

  print_token_summary(tokens);

  return 0;
}
#endif
// --- Additional: Utility to serialize and deserialize Chunk to/from a simple text format ---
#include <sstream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <iostream>

#pragma once

struct Chunk {
    // Define the members of Chunk as needed
    // Example:
    // int id;
    // std::vector<char> data;
};

#pragma once

// Define the Chunk struct or class as needed
struct Chunk {
    // Example fields - replace with actual members as required
    int id;
    std::vector<char> data;
};
#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <sstream>
#include "triad_bytecode.hpp"
namespace triad {
    // Serialize a Chunk to a text stream (for debugging or simple persistence)
    inline void serialize_chunk(const Chunk& ch, std::ostream& os) {
        os << "CHUNK\n";
        os << "CODE " << ch.code.size() << "\n";
        for (const auto& instr : ch.code) {
            os << static_cast<int>(instr.op) << " " << instr.a << " " << instr.b << " " << instr.c << "\n";
        }
        os << "CONSTS " << ch.consts.size() << "\n";
        for (const auto& v : ch.consts) {
            if (v.tag == Value::Num) {
                os << "N " << v.num << "\n";
            }
            else if (v.tag == Value::Str) {
                os << "S " << v.str << "\n";
            }
        }
        os << "NAMES " << ch.names.size() << "\n";
        for (const auto& name : ch.names) {
            os << name << "\n";
        }
    }
    // Deserialize a Chunk from a text stream
    inline Chunk deserialize_chunk(std::istream& is) {
        Chunk ch;
        std::string header;
        is >> header;
        if (header != "CHUNK") throw std::runtime_error("Invalid chunk header");
        std::string section;
        size_t count = 0;
        is >> section >> count;
        if (section != "CODE") throw std::runtime_error("Expected CODE section");
        for (size_t i = 0; i < count; ++i) {
            int op, a, b, c;
            is >> op >> a >> b >> c;
            ch.code.push_back({ static_cast<Op>(op), a, b, c });
        }
        is >> section >> count;
        if (section != "CONSTS") throw std::runtime_error("Expected CONSTS section");
        for (size_t i = 0; i < count; ++i) {
            char type;
            is >> type;
            if (type == 'N') {
                double num;
                is >> num;
                ch.consts.push_back(Value::number(num));
            }
            else if (type == 'S') {
                std::string str;
                is >> std::ws;
                std::getline(is, str);
                ch.consts.push_back(Value::string(str));
            }
        }
        is >> section >> count;
        if (section != "NAMES") throw std::runtime_error("Expected NAMES section");
        is >> std::ws;
        for (size_t i = 0; i < count; ++i) {
            std::string name;
            std::getline(is, name);
            ch.names.push_back(name);
        }
        return ch;
    }
} // namespace triad
#ifdef TRIAD_SERIALIZE_MAIN
int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: triad_serialize <input.triad> [output.chunk]\n";
        return 1;
    }
    std::ifstream infile(argv[1]);
    if (!infile) {
        std::cerr << "Failed to open input file: " << argv[1] << "\n";
        return 1;
    }
    std::string src((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>());
    Lexer lexer(src);
    auto tokens = lexer.run();
    Parser parser(std::move(tokens));
    Chunk chunk = parser.parse();
    if (argc >= 3) {
        std::ofstream outfile(argv[2]);
        if (!outfile) {
            std::cerr << "Failed to open output file: " << argv[2] << "\n";
            return 1;
        }
        serialize_chunk(chunk, outfile);
    }
    else {
        serialize_chunk(chunk, std::cout);
    }
    return 0;
}
#endif
    else {
    std::cerr << "Unknown argument: " << arg << "\n";
    return 1;
    }
   }
   if (mode == "run-vm") {
       if (target.empty()) {
           std::cerr << "No input file specified for run-vm mode.\n";
           return 1;
       }
       std::string src = slurp(target);
       Chunk ch = parse_to_chunk(src);
       if (showBytecode) dump_chunk(ch);
       run_vm(ch, traceVm);
   }
   else if (mode == "run-ast") {
       if (target.empty()) {
           std::cerr << "No input file specified for run-ast mode.\n";
           return 1;
       }
       std::string src = slurp(target);
       if (showAst) {
           std::cout << "[AST display not yet implemented]\n";
       }
       run_ast(src);
   }
   else if (mode == "emit-nasm") {
       if (target.empty()) {
           std::cerr << "No input file specified for emit-nasm mode.\n";
           return 1;
       }
       std::string src = slurp(target);
       Chunk ch = parse_to_chunk(src);
       std::string nasmCode = emit_nasm(ch);
       if (!outFile.empty()) emit_to_file(nasmCode, outFile);
       else std::cout << nasmCode;
   }
   else if (mode == "emit-llvm") {
       if (target.empty()) {
           std::cerr << "No input file specified for emit-llvm mode.\n";
           return 1;
       }
       std::string src = slurp(target);
       Chunk ch = parse_to_chunk(src);
       std::string llvmIr = emit_llvm(ch);
       if (!outFile.empty()) emit_to_file(llvmIr, outFile);
       else std::cout << llvmIr;
   }
   else if (mode == "run-tests") {
       run_tests(verbose);
   }
   else {
       std::cerr << "Unknown mode: " << mode << "\n";
       return 1;
   }
   return 0;
}
// ASTNode* ast = parse_to_ast(src); ast->dump();
       }
       if (mode == "run-vm") {
           run_vm(ch, traceVm);
       }
       else if (mode == "run-ast") {
           run_ast(src);
       }
       else if (mode == "emit-nasm") {
           std::string asm_code = emit_nasm(ch); // stub
           if (!outFile.empty()) emit_to_file(asm_code, outFile);
           else std::cout << asm_code; // Print to stdout if no output file specified
       }
       else if (mode == "emit-llvm") {
           std::string llvm_code = emit_llvm(ch); // stub
           if (!outFile.empty()) emit_to_file(llvm_code, outFile);
           else std::cout << llvm_code; // Print to stdout if no output file specified
       }
       else {
           std::cerr << "Unknown mode: " << mode << "\n";
           return 1;
       }
     }
     catch (const std::exception& e) {
         std::cerr << "Error: " << e.what() << "\n";
         return 1;
     }
     return 0;
     }
     std::cout << "Triad Compiler v0.9.1\n";
     std::string mode = argv[1];
     std::string target = argc > 2 ? argv[2] : "";
     std::string outFile;
     bool verbose = false, showAst = false, showBytecode = false, traceVm
         = false;
     for (int i = 3; i < argc; ++i) {
         std::string arg = argv[i];
         if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
         else if (arg == "--verbose") verbose = true;
         else if (arg == "--show-ast") showAst = true;
         else if (arg == "--show-bytecode") showBytecode = true;
         else if (arg == "--trace-vm") trace
             Vm = true;
         else if (arg == "--version") {
             std::cout << "Triad Compiler v0.9.1\n";
             return 0;
         }
         else {
             std::cerr << "Unknown argument: " << arg << "\n";
             return 1;
         }
     }
     try {
         if (mode == "run-tests") {
             run_tests(verbose);
             return 0;
         }
         if (target.empty()) {
             std::cerr << "Error: No input file specified.\n";
             return 1;
         }
         std::string src = slurp(target);
         Chunk ch = parse_to_chunk(src);
         if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
         if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
         if (showAst) {
             std::cout << "[AST dump not yet implemented]\n";
             // ASTNode* ast = parse_to_ast(src); ast->dump();
         }
         if (mode == "run-vm") {
             run_vm(ch, traceVm);
         }
         else if (mode == "run-ast") {
             run_ast(src);
         }
         else if (mode == "emit-nasm") {
             std::string asm_code = emit_nasm(ch); // stub
             if (!outFile.empty()) emit_to_file(asm_code, outFile);
             else std::cout << asm_code; // Print to stdout if no output file specified
         }
         else if (mode == "emit-llvm") {
             std::string llvm_code = emit_llvm(ch); // stub
             if (!outFile.empty()) emit_to_file(llvm_code, outFile);
             else std::cout << llvm_code; // Print to stdout if no output file specified
         }
         else {
             std::cerr << "Unknown mode: " << mode << "\n";
             return 1;
         }
     }
     catch (const std::exception& e) {
         std::cerr << "Error: " << e.what() << "\n";
         return 1;
     }
     return 0;
     }
     std::cout << "Triad Compiler v0.9.1\n";
     std::string mode = argv[1];
     std::string target = argc > 2 ? argv[2] : "";
     std::string outFile;
     bool verbose = false, showAst = false, showBytecode = false, traceVm
         = false;
     for (int i = 3; i < argc; ++i) {
         std::string arg = argv[i];
         if (arg == "-o" && i +
             1 < argc) outFile = argv[++i];
         else if (arg == "--verbose") verbose = true;
         else if (arg == "--show-ast") showAst = true;
         else if (arg == "--show-bytecode") showBytecode = true;
         else if (arg == "--trace-vm") trace
             Vm = true;
         else if (arg == "--version") {
             std::cout << "Triad Compiler v0.9.1\n";
             return 0;
         }
         else {
             std::cerr << "Unknown argument: " << arg << "\n";
             return 1;
         }
     }
     try {
         if (mode == "run-tests") {
             run_tests(verbose);
             return 0;
         }
         if (target.empty()) {
             std::cerr << "Error: No input file specified.\n";
             return 1;
         }
         std::string src = slurp(target);
         Chunk ch = parse_to_chunk(src);
         if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
         if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
         if (showAst) {
             std::cout << "[AST dump not yet implemented]\n";
             // ASTNode* ast = parse_to_ast(src); ast->dump();
         }
         if (mode == "run-vm") {
             run_vm(ch, traceVm);
         }
         else if (mode == "run-ast") {
             run_ast(src);
         }
         else if (mode == "emit-nasm") {
             std::string asm_code = emit_nasm(ch); // stub
             if (!outFile.empty()) emit_to_file(asm_code, outFile);
             else std::cout << asm_code; // Print to stdout if no output file specified
         }
         else if (mode == "emit-llvm") {
             std::string llvm_code = emit_llvm(ch); // stub
             if (!outFile.empty()) emit_to_file(llvm_code, outFile);
             else std::cout << llvm_code; // Print to stdout if no output file specified
         }
         else {
             std::cerr << "Unknown mode: " << mode << "\n";
             return 1;
         }
     }
     catch (const std::exception& e) {
         std::cerr << "Error: " << e.what() << "\n";
         return 1;
     }
     return 0;
     }
     std::cout << "Triad Compiler v0.9.1\n";
     std::string mode = argv[1];
     std::string target = argc > 2 ? argv[2] : "";
     std::string outFile;
     bool verbose = false, showAst = false, showBytecode = false, traceVm
         = false;
     for (int i = 3; i < argc; ++i) {
         std::string arg = argv[i];
         if (arg == "-o" &&
             i + 1 < argc) outFile = argv[++i];
         else if (arg == "--verbose") verbose = true;
         else if (arg == "--show-ast") showAst = true;
         else if (arg == "--show-bytecode") showBytecode = true;
         else if (arg == "--trace-vm") trace
             Vm = true;
         else if (arg == "--version") {
             std::cout << "Triad Compiler v0.9.1\n";
             return 0;
         }
         else {
             std::cerr << "Unknown argument: " << arg << "\n";
             return 1;
         }
     }
     try {
         if (mode == "run-tests") {
             run_tests(verbose);
             return 0;
         }
         if (target.empty()) {
             std::cerr << "Error: No input file specified.\n";
             return 1;
         }
         std::string src = slurp(target);
         Chunk ch = parse_to_chunk(src);
         if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
         if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
         if (showAst) {
             std::cout << "[AST dump not yet implemented]\n";
             // ASTNode* ast = parse_to_ast(src); ast->dump();
         }
         if (mode == "run-vm") {
             run_vm(ch, traceVm);
         }
         else if (mode == "run-ast") {
             run_ast(src);
         }
         else if (mode == "emit-nasm") {
             std::string asm_code = emit_nasm(ch); // stub
             if (!outFile.empty()) emit_to_file(asm_code, outFile);
             else std::cout << asm_code; // Print to stdout if no output file specified
         }
         else if (mode == "emit-llvm") {
             std::string llvm_code = emit_llvm(ch); // stub
             if (!outFile.empty()) emit_to_file(llvm_code, outFile);
             else std::cout << llvm_code; // Print to stdout if no output file specified
         }
         else {
             std::cerr << "Unknown mode: " << mode << "\n";
             return 1;
         }
     }
     catch (const std::exception& e) {
         std::cerr << "Error: " << e.what() << "\n";
         return 1;
     }
     return 0;
     }
     std::cout << "Triad Compiler v0.9.1\n";
     std::string mode = argv[1];
     std::string target = argc > 2 ? argv[2] : "";
     std::string outFile;
     bool verbose = false, showAst = false, showBytecode = false, traceVm
         = false;
     for (int i = 3; i < argc; ++i) {
         std::string arg = argv[i];
         if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
         else if (arg == "--verbose") verbose = true;
         else if (arg == "--show-ast") showAst = true;
         else if (arg == "--show-bytecode") showBytecode = true;
         else if (arg == "--trace-vm") trace
             Vm = true;
         else if (arg == "--version") {
             std::cout << "Triad Compiler v0.9.1\n";
             return 0;
         }
         else {
             std::cerr << "Unknown argument: " << arg << "\n";
             return 1;
         }
     }
     try {
         if (mode == "run-tests") {
             run_tests(verbose);
             return 0;
         }
         if (target.empty()) {
             std::cerr << "Error: No input file specified.\n";
             return 1;
         }
         std::string src = slurp(target);
         Chunk ch = parse_to_chunk(src);
         if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
         if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
         if (showAst) {
             std::cout << "[AST dump not yet implemented]\n";
             // ASTNode* ast = parse_to_ast(src); ast->dump();
         }
         if (mode == "run-vm") {
             run_vm(ch, traceVm);
         }
         else if (mode == "run-ast") {
             run_ast(src);
         }
         else if (mode == "emit-nasm") {
             std::string asm_code = emit_nasm(ch); // stub
             if (!outFile.empty()) emit_to_file(asm_code, outFile);
             else std::cout << asm_code; // Print to stdout if
         }
         else if (mode == "emit-llvm") {
             std::string llvm_code = emit_llvm(ch); // stub
             if (!outFile.empty()) emit_to_file(llvm_code, outFile);
             else std::cout << llvm_code; // Print to stdout if no output file specified
         }
         else {
             std::cerr << "Unknown mode: " << mode << "\n";
             return 1;
         }
     }
     catch (const std::exception& e) {
         std::cerr << "Error: " << e.what() << "\n";
         return 1;
     }
     return 0;
     }
     std::cout << "Triad Compiler v0.9.1\n";
     std::string mode = argv[1];
     std::string target = argc > 2 ? argv[2] : "";
     std::string outFile;
     bool verbose = false, showAst = false, showBytecode = false, traceVm
         = false;
     for (int i = 3; i < argc; ++i) {
         std::string arg = argv[i];
         if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
         else if (arg == "--verbose") verbose = true;
         else if (arg == "--show-ast") showAst = true;
         else if (arg == "--show-bytecode") showBytecode = true;
         else if (arg == "--trace-vm") trace
             Vm = true;
         else if (arg == "--version") {
             std::cout << "Triad Compiler v0.9.1\n";
             return 0;
         }
         else {
             std::cerr << "Unknown argument: " << arg << "\n";
             return 1;
         }
     }
     try {
         if (mode == "run-tests") {
             run_tests(verbose);
             return 0;
         }
         if (target.empty()) {
             std::cerr << "Error: No input file specified.\n";
             return 1;
         }
         std::string src = slurp(target);
         Chunk ch = parse_to_chunk(src);
         if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
         if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
         if (showAst) {
             std::cout << "[AST dump not yet implemented]\n";
             // ASTNode* ast = parse_to_ast(src); ast->dump();
         }
         if (mode == "run-vm") {
             run_vm(ch, traceVm);
         }
         else if (mode == "run-ast") {
             run_ast(src);
         }
         else if (mode == "emit-nasm") {
             std::string asm_code = emit_nasm(ch); // stub
             if (!outFile.empty()) emit_to_file(asm_code, outFile);
             else std::cout << asm_code; // Print to stdout if
         }
         else if (mode == "emit-llvm") {
             std::string llvm_code = emit_llvm(ch); // stub
             if (!outFile.empty()) emit_to_file(llvm_code, outFile);
             else std::cout << llvm_code; // Print to stdout if no output file specified
         }
         else {
             std::cerr << "Unknown mode: " << mode << "\n";
             return 1;
         }
     }
     catch (const std::exception& e) {
         std::cerr << "Error: " << e.what() << "\n";
         return 1;
     }
     return 0;
     }
     std::cout << "Triad Compiler v0.9.1\n";
     std::string mode = argv[1];
     std::string target = argc > 2 ? argv[2] : "";
     std::string outFile;
     bool verbose = false, showAst = false, showBytecode = false, traceVm
         = false;
     for (int i = 3; i < argc; ++i) {
         std::string arg = argv[i];
         if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
         else if (arg == "--verbose") verbose = true;
         else if (arg == "--show-ast") showAst = true;
         else if (arg == "--show-bytecode") showBytecode = true;
         else if (arg == "--trace-vm") trace
             Vm = true;
         else if (arg == "--version") {
             std::cout << "Triad Compiler v0.9.1\n";
             return 0;
         }
         else {
             std::cerr << "Unknown argument: " << arg << "\n";
             return 1;
         }
     }
     try {
         if (mode == "run-tests") {
             run_tests(verbose);
             return 0;
         }
         if (target.empty()) {
             std::cerr << "Error: No input file specified.\n";
             return 1;
         }
         std::string src = slurp(target);
         Chunk ch = parse_to_chunk(src);
         if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
         if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
         if (showAst) {
             std::cout << "[AST dump not yet implemented]\n";
             // ASTNode* ast = parse_to_ast(src); ast->dump();
         }
         if (mode == "run-vm") {
             run_vm(ch, traceVm);
         }
         else if (mode == "run-ast") {
             run_ast(src);
         }
         else if (mode == "emit-nasm") {
             std::string asm_code = emit_nasm(ch); // stub
             if (!outFile.empty()) emit_to_file(asm_code, outFile);
             else std::cout << asm_code; // Print to stdout if
         }
         else if (mode == "emit-llvm") {
             std::string llvm_code = emit_llvm(ch); // stub
             if (!outFile.empty()) emit_to_file(llvm_code, outFile);
             else std::cout << llvm_code; // Print to stdout if no output file specified
         }
         else {
             std::cerr << "Unknown mode: " << mode << "\n";
             return 1;
         }
     }
     catch (const std::exception& e) {
         std::cerr << "Error: " << e.what() << "\n";
         return 1;
     }
     return 0;
     }
     std::cout << "Triad Compiler v0.9.1\n";
     std::string mode = argv[1];
     std::string target = argc > 2 ? argv[2] : "";
     std::string outFile;
     bool verbose = false, showAst = false, showBytecode = false, traceVm
         = false;
     for (int i = 3; i < argc; ++i) {
         std::string arg = argv[i];
         if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
         else if (arg == "--verbose") verbose = true;
         else if (arg == "--show-ast") showAst = true;
         else if (arg == "--show-bytecode") showBytecode = true;
         else if (arg == "--trace-vm") trace
             Vm = true;
         else if (arg == "--version") {
             std::cout << "Triad Compiler v0.9.1\n";
             return 0;
         }
         else {
             std::cerr << "Unknown argument: " << arg << "\n";
             return 1;
         }
     }
     try {
         if (mode == "run-tests") {
             run_tests(verbose);
             return 0;
         }
         if (target.empty()) {
             std::cerr << "Error: No input file specified.\n";
             return 1;
         }
         std::string src = slurp(target);
         Chunk ch = parse_to_chunk(src);
         if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
         if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
         if (showAst) {
             std::cout << "[AST dump not yet implemented]\n";
             // ASTNode* ast = parse_to_ast(src); ast->dump();
         }
         if (mode == "run-vm") {
             run_vm(ch, traceVm);
         }
         else if (mode == "run-ast") {
             run_ast(src);
         }
         else if (mode == "emit-nasm") {
             std::string asm_code = emit_nasm(ch); // stub
             if (!outFile.empty()) emit_to_file(asm_code, outFile);
             else std::cout << asm_code; // Print to stdout if
         }
         else if (mode == "emit-llvm") {
             std::string llvm_code = emit_llvm(ch); // stub
             if (!outFile.empty()) emit_to_file(llvm_code, outFile);
             else std::cout << llvm_code; // Print to stdout if no output file specified
         }
         else {
             std::cerr << "Unknown mode: " << mode << "\n";
             return 1;
         }
     }
     catch (const std::exception& e) {
         std::cerr << "Error: " << e.what() << "\n";
         return 1;
     }
     return 0;
     }
     std::cout << "Triad Compiler v0.9.1\n";
     std::string mode = argv[1];
     std::string target = argc > 2 ? argv[2] : "";
     std::string outFile;
     bool verbose = false, showAst = false, showBytecode = false, traceVm
         = false;
     for (int i = 3; i < argc; ++i) {
         std::string arg = argv[i];
         if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
         else if (arg == "--verbose") verbose = true;
         else if (arg == "--show-ast") showAst = true;
         else if (arg == "--show-bytecode") showBytecode = true;
         else if (arg == "--trace-vm") trace
             Vm = true;
         else if (arg == "--version") {
             std::cout << "Triad Compiler v0.9.1\n";
             return 0;
         }
         else {
             std::cerr << "Unknown argument: " << arg << "\n";
             return 1;
         }
     }
     try {
         if (mode == "run-tests") {
             run_tests(verbose);
             return 0;
         }
         if (target.empty()) {
             std::cerr << "Error: No input file specified.\n";
             return 1;
         }
         std::string src = slurp(target);
         Chunk ch = parse_to_chunk(src);
         if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
         if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
         if (showAst) {
             std::cout << "[AST dump not yet implemented]\n";
             // ASTNode* ast = parse_to_ast(src); ast->dump();
         }
         if (mode == "run-vm") {
             run_vm(ch, traceVm);
         }
         else if (mode == "run-ast") {
             run_ast(src);
         }
         else if (mode == "emit-nasm") {
             std::string asm_code = emit_nasm(ch); // stub
             if (!outFile.empty()) emit_to_file(asm_code, outFile);
             else std::cout << asm_code; // Print to stdout if
         }
         else if (mode == "emit-llvm") {
             std::string llvm_code = emit_llvm(ch); // stub
             if (!outFile.empty()) emit_to_file(llvm_code, outFile);
             else std::cout << llvm_code; // Print to stdout if no output file specified
         }
         else {
             std::cerr << "Unknown mode: " << mode << "\n";
             return 1;
         }
     }
     catch (const std::exception& e) {
         std::cerr << "Error: " << e.what() << "\n";
         return 1;
     }
     return 0;
     }
     std::cout << "Triad Compiler v0.9.1\n";
     std::string mode = argv[1];
     std::string target = argc > 2 ? argv[2] : "";
     std::string outFile;
     bool verbose = false, showAst = false, showBytecode = false, traceVm
         = false;
     for (int i = 3; i < argc; ++i) {
         std::string arg = argv[i];
         if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
         else if (arg == "--verbose") verbose = true;
         else if (arg == "--show-ast") showAst = true;
         else if (arg == "--show-bytecode") showBytecode = true;
         else if (arg == "--trace-vm") trace
             Vm = true;
         else if (arg == "--version") {
             std::cout << "Triad Compiler v0.9.1\n";
             return 0;
         }
         else {
             std::cerr << "Unknown argument: " << arg << "\n";
             return 1;
         }
     }
     try {
         if (mode == "run-tests") {
             run_tests(verbose);
             return 0;
         }
         if (target.empty()) {
             std::cerr << "Error: No input file specified.\n";
             return 1;
         }
         std::string src = slurp(target);
         Chunk ch = parse_to_chunk(src);
         if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
         if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
         if (showAst) {
             std::cout << "[AST dump not yet implemented]\n";
             // ASTNode* ast = parse_to_ast(src); ast->dump();
         }
         if (mode == "run-vm") {
             run_vm(ch, traceVm);
         }
         else if (mode == "run-ast") {
             run_ast(src);
         }
         else if (mode == "emit-nasm") {
             std::string asm_code = emit_nasm(ch); // stub
             if (!outFile.empty()) emit_to_file(asm_code, outFile);
             else std::cout << asm_code; // Print to stdout if
         }
         else if (mode == "emit-llvm") {
             std::string llvm_code = emit_llvm(ch); // stub
             if (!outFile.empty()) emit_to_file(llvm_code, outFile);
             else std::cout << llvm_code; // Print to stdout if no output file specified
         }
         else {
             std::cerr << "Unknown mode: " << mode << "\n";
             return 1;
         }
     }
     catch (const std::exception& e) {
         std::cerr << "Error: " << e.what() << "\n";
         return 1;
     }
     return 0;
     }
#include <vector>
#include <string>
#include <iostream>
#include "triad_bytecode.hpp"
#include "triad_chunk.hpp"
#include "triad_lexer.hpp"
#include "triad_parser.hpp"
#include "triad_vm.hpp"
#include "triad_nasm.hpp"
#include "triad_llvm.hpp"
#include "triad_tests.hpp"
     int main(int argc, char** argv) {
         if (argc < 2) {
             std::cerr << "Usage: triad <mode> [options] <input.triad> [output.chunk]\n";
             return 1;
         }
         std::cout << "Triad Compiler v0.9.1\n";
         std::string mode = argv[1];
         std::string target = argc > 2 ? argv[2] : "";
         std::string outFile;
         bool verbose = false, showAst = false, showBytecode = false, traceVm = false;
         for (int i = 3; i < argc; ++i) {
             std::string arg = argv[i];
             if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
             else if (arg == "--verbose") verbose = true;
             else if (arg == "--show-ast") showAst = true;
             else if (arg == "--show-bytecode") showBytecode = true;
             else if (arg == "--trace-vm") trace
                 Vm = true;
             else if (arg == "--version") {
                 std::cout << "Triad Compiler v0.9.1\n";
                 return 0;
             }
             else {
                 std::cerr << "Unknown argument: " << arg << "\n";
                 return 1;
             }
         }
         try {
             if (mode == "run-tests") {
                 run_tests(verbose);
                 return 0;
             }
             if (target.empty()) {
                 std::cerr << "Error: No input file specified.\n";
                 return 1;
             }
             std::string src = slurp(target);
             Chunk ch = parse_to_chunk(src);
             if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
             if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
             if (showAst) {
                 std::cout << "[AST dump not yet implemented]\n";
                 // ASTNode* ast = parse_to_ast(src); ast->dump();
             }
             if (mode == "run-vm") {
                 run_vm(ch, traceVm);
             }
             else if (mode == "run-ast") {
                 run_ast(src);
             }
             else if (mode == "emit-nasm") {
                 std::string asm_code = emit_nasm(ch); // stub
                 if (!outFile.empty()) emit_to_file(asm_code, outFile);
                 else std::cout << asm_code; // Print to stdout if no output file specified
             }
             else if (mode == "emit-llvm") {
                 std::string llvm_code = emit_llvm(ch); // stub
                 if (!outFile.empty()) emit_to_file(llvm_code, outFile);
                 else std::cout << llvm_code; // Print to stdout if no output file specified
             }
             else {
                 std::cerr << "Unknown mode: " << mode << "\n";
                 return 1;
             }
         }
         catch (const std::exception& e) {
             std::cerr << "Error: " << e.what() << "\n";
             return 1;
         }
         return 0;
     }
#include <iostream>
#include <fstream>
#include "triad_chunk.hpp"
#include "triad_value.hpp"
     namespace triad {
         Chunk parse_chunk(std::istream& is) {
             Chunk ch;
             std::string section;
             size_t count;
             is >> section >> count;
             if (section != "CODE") throw std::runtime_error("Expected CODE section");
             for (size_t i = 0; i < count; ++i) {
                 int op, a, b, c;
                 is >> op >> a >> b >> c;
                 if (op < 0 || op >= static_cast<int>(Op::COUNT)) {
                     throw std::runtime_error("Invalid opcode: " + std::to_string(op));
                 }
                 ch.code.push_back({ static_cast<Op>(op), a, b, c });
             }
             is >> section >> count;
             if (section != "CONSTS") throw std::runtime_error("Expected CONSTS section");
             is >> std::ws; // Skip whitespace
             for (size_t i = 0; i < count; ++i) {
                 std::string str;
                 std::getline(is, str);
                 if (!str.empty()) {
                     Value val;
                     if (str[0] == '"') {
                         val = Value::from_string(str.substr(1, str.size() - 2));
                     }
                     else {
                         try {
                             val = Value::from_number(std::stod(str));
                         }
                         catch (...) {
                             throw std::runtime_error("Invalid constant value: " + str);
                         }
                     }
                     ch.consts.push_back(val);
                 }
             }
             return ch;
         }
         void serialize_chunk(const Chunk& ch, std::ostream& os) {
             os << "CODE " << ch.code.size() << "\n";
             for (const auto& inst : ch.code) {
                 os << static_cast<int>(inst.op) << " " << inst.a << " " << inst.b << " " << inst.c << "\n";
             }
             os << "CONSTS " << ch.consts.size() << "\n";
             for (const auto& val : ch.consts) {
                 os << val.to_string() << "\n";
             }
         }
         std::string slurp(const std::string& filename) {
             std
                 ::ifstream file(filename);
             if (!file) {
                 throw std::runtime_error("Could not open file: " + filename);
             }
             std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
             return content;
         }
         int main(int argc, char** argv) {
             if (argc < 2) {
                 std::cerr << "Usage: triad <mode> [options] <input.triad> [output.chunk]\n";
                 return 1;
             }
             std::cout << "Triad Compiler v0.9.1\n";
             std::string mode = argv[1];
             std::string target = argc > 2 ? argv[2] : "";
             std::string outFile;
             bool verbose = false, showAst = false, showBytecode = false, traceVm = false;
             for (int i = 3; i < argc; ++i) {
                 std::string arg = argv[i];
                 if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
                 else if (arg == "--verbose") verbose = true;
                 else if (arg == "--show-ast") showAst = true;
                 else if (arg == "--show-bytecode") showBytecode = true;
                 else if (arg == "--trace-vm") trace
                     Vm = true;
                 else if (arg == "--version") {
                     std::cout << "Triad Compiler v0.9.1\n";
                     return 0;
                 }
                 else {
                     std::cerr << "Unknown argument: " << arg << "\n";
                     return 1;
                 }
             }
             if (mode == "run-vm") {
                 if (target.empty()) {
                     std::cerr << "No input file specified for run-vm mode.\n";
                     return 1;
                 }
                 try {
                     std::string src = slurp(target);
                     Chunk ch = parse_to_chunk(src);
                     if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
                     if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
                     if (showAst) {
                         std::cout << "[AST dump not yet implemented]\n";
                         // ASTNode* ast = parse_to_ast(src); ast->dump();
                     }
                     run_vm(ch, traceVm);
                 }
                 catch (const std::exception& e) {
                     std::cerr << "Error: " << e.what() << "\n";
                     return 1;
                 }
             }
             else if (mode == "run-ast") {
                 if (target.empty()) {
                     std::cerr << "No input file specified for run-ast mode.\n";
                     return 1;
                 }
                 try {
                     std::string src = slurp(target);
                     run_ast(src);
                 }
                 catch (const std::exception& e) {
                     std::cerr << "Error: " << e.what() << "\n";
                     return 1;
                 }
             }
             else if (mode == "emit-nasm" || mode == "emit-llvm") {
                 if (target.empty()) {
                     std::cerr << "No input file specified for emit mode.\n";
                     return 1;
                 }
                 try {
                     std::string src = slurp(target);
                     Chunk ch = parse_to_chunk(src);
                     if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
                     if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
                     if (showAst) {
                         std::cout << "[AST dump not yet implemented]\n";
                         // ASTNode* ast = parse_to_ast(src); ast->dump();
                     }
                     if (mode == "emit-nasm") {
                         std::string asm_code = emit_nasm(ch); // stub
                         if (!outFile.empty()) emit_to_file(asm_code, outFile);
                         else std::cout << asm_code; // Print to stdout if no output file specified
                     }
                     else if (mode == "emit-llvm") {
                         std::string llvm_code = emit_llvm(ch); // stub
                         if (!outFile.empty()) emit_to_file(llvm_code, outFile);
                         else std::cout << llvm_code; // Print to stdout if no output file specified
                     }
                 }
                 catch (const std::exception& e) {
                     std::cerr << "Error: " << e.what() << "\n";
                     return 1;
                 }
             }
             else {
                 std::cerr << "Unknown mode: " << mode << "\n";
                 return 1;
             }
             try {
                 if (mode == "run-tests") {
                     run_tests(verbose);
                     return 0;
                 }
                 if (target.empty()) {
                     std::cerr << "Error: No input file specified.\n";
                     return 1;
                 }
                 std::string src = slurp(target);
                 Chunk ch = parse_to_chunk(src);
                 if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
                 if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
                 if (showAst) {
                     std::cout << "[AST dump not yet implemented]\n";
                     // ASTNode* ast = parse_to_ast(src); ast->dump();
                 }
                 if (mode == "run-vm") {
                     run_vm(ch, traceVm);
                 }
                 else if (mode == "run-ast") {
                     run_ast(src);
                 }
                 else if (mode == "emit-nasm") {
                     std::string asm_code = emit_nasm(ch); // stub
                     if (!outFile.empty()) emit_to_file(asm_code, outFile);
                     else std::cout << asm_code; // Print to stdout if no output file specified
                 }
                 else if (mode == "emit-llvm") {
                     std::string llvm_code = emit_llvm(ch); // stub
                     if (!outFile.empty()) emit_to_file(llvm_code, outFile);
                     else std::cout << llvm_code; // Print to stdout if no output file specified
                 }
                 else {
                     std::cerr << "Unknown mode: " << mode << "\n";
                     return 1;
                 }
             }
             catch (const std::exception& e) {
                 std::cerr << "Error: " << e.what() << "\n";
                 return 1;
             }
             return 0;
         }
         std::cout << "Triad Compiler v0.9.1\n";
         std::string mode = argv[1];
         std::string target = argc > 2 ? argv[2] : "";
         std::string outFile;
         bool verbose = false, showAst = false, showBytecode = false, traceVm
             = false;
         for (int i = 3; i < argc; ++i) {
             std::string arg = argv[i];
             if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
             else if (arg == "--verbose") verbose = true;
             else if (arg == "--show-ast") showAst = true;
             else if (arg == "--show-bytecode") showBytecode = true;
             else if (arg == "--trace-vm") trace
                 Vm = true;
             else if (arg == "--version") {
                 std::cout << "Triad Compiler v0.9.1\n";
                 return 0;
             }
             else {
                 std::cerr << "Unknown argument: " << arg << "\n";
                 return 1;
             }
         }
         try {
             if (mode == "run-tests") {
                 run_tests(verbose);
                 return 0;
             }
             if (target.empty()) {
                 std::cerr << "Error: No input file specified.\n";
                 return 1;
             }
             std::string src = slurp(target);
             Chunk ch = parse_to_chunk(src);
             if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
             if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
             if (showAst) {
                 std::cout << "[AST dump not yet implemented]\n";
                 // ASTNode* ast = parse_to_ast(src); ast->dump();
             }
             if (mode == "run-vm") {
                 run_vm(ch, traceVm);
             }
             else if (mode == "run-ast") {
                 run_ast(src);
             }
             else if (mode == "emit-nasm") {
                 std::string asm_code = emit_nasm(ch); // stub
                 if (!outFile.empty()) emit_to_file(asm_code, outFile);
                 else std::cout << asm_code; // Print to stdout if no output file specified
             }
             else if (mode == "emit-llvm") {
                 std::string llvm_code = emit_llvm(ch); // stub
                 if (!outFile.empty()) emit_to_file(llvm_code, outFile);
                 else std::cout << llvm_code; // Print to stdout if no output file specified
             }
             else {
                 std::cerr << "Unknown mode: " << mode << "\n";
                 return 1;
             }
         }
         catch (const std::exception& e) {
             std::cerr << "Error: " << e.what() << "\n";
             return 1;
         }
         return 0;
     }
     std::cout << "Triad Compiler v0.9.1\n";
     std::string mode = argv[1];
     std::string target = argc > 2 ? argv[2] : "";
     std::string outFile;
     bool verbose = false, showAst = false, showBytecode = false, traceVm
         = false;
     for (int i = 3; i < argc; ++i) {
         std::string arg = argv[i];
         if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
         else if (arg == "--verbose") verbose = true;
         else if (arg == "--show-ast") showAst = true;
         else if (arg == "--show-bytecode") showBytecode = true;
         else if (arg == "--trace-vm") trace
             Vm = true;
         else if (arg == "--version") {
             std::cout << "Triad Compiler v0.9.1\n";
             return 0;
         }
         else {
             std::cerr << "Unknown argument: " << arg << "\n";
             return 1;
         }
     }
     try {
         if (mode == "run-tests") {
             run_tests(verbose);
             return 0;
         }
         if (target.empty()) {
             std::cerr << "Error: No input file specified.\n";
             return 1;
         }
         std::string src = slurp(target);
         Chunk ch = parse_to_chunk(src);
         if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
         if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
         if (showAst) {
             std::cout << "[AST dump not yet implemented]\n";
             // ASTNode* ast = parse_to_ast(src); ast->dump();
         }
         if (mode == "run-vm") {
             run_vm(ch, traceVm);
         }
         else if (mode == "run-ast") {
             run_ast(src);
         }
         else if (mode == "emit-nasm") {
             std::string asm_code = emit_nasm(ch); // stub
             if (!outFile.empty()) emit_to_file(asm_code, outFile);
             else std::cout << asm_code; // Print to stdout if no output file specified
         }
         else if (mode == "emit-llvm") {
             std::string llvm_code = emit_llvm(ch); // stub
             if (!outFile.empty()) emit_to_file(llvm_code, outFile);
             else std::cout << llvm_code; // Print to stdout if no output file specified
         }
         else {
             std
                 ::cerr << "Unknown mode: " << mode << "\n";
             return 1;
         }
     }
     catch (const std::exception& e) {
         std::cerr << "Error: " << e.what() << "\n";
         return 1;
     }
     return 0;
     }
     std::cout << "Triad Compiler v0.9.1\n";
     std::string mode = argv[1];
     std::string target = argc > 2 ? argv[2] : "";
     std::string outFile;
     bool verbose = false, showAst = false, showBytecode = false, traceVm
         = false;
     for (int i = 3; i < argc; ++i) {
         std::string arg = argv[i];
         if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
         else if (arg == "--verbose") verbose = true;
         else if (arg == "--show-ast") showAst = true;
         else if (arg == "--show-bytecode") showBytecode = true;
         else if (arg == "--trace-vm") trace
             Vm = true;
         else if (arg == "--version") {
             std::cout << "Triad Compiler v0.9.1\n";
             return 0;
         }
         else {
             std::cerr << "Unknown argument: " << arg << "\n";
             return 1;
         }
     }
     try {
         if (mode == "run-tests") {
             run_tests(verbose);
             return 0;
         }
         if (target.empty()) {
             std::cerr << "Error: No input file specified.\n";
             return 1;
         }
         std::string src = slurp(target);
         Chunk ch = parse_to_chunk(src);
         if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
         if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
         if (showAst) {
             std::cout << "[AST dump not yet implemented]\n";
             // ASTNode* ast = parse_to_ast(src); ast->dump();
         }
         if (mode == "run-vm") {
             run_vm(ch, traceVm);
         }
         else if (mode == "run-ast") {
             run_ast(src);
         }
         else if (mode == "emit-nasm") {
             std::string asm_code = emit_nasm(ch); // stub
             if (!outFile.empty()) emit_to_file(asm_code, outFile);
             else std::cout << asm_code; // Print to stdout if no output file specified
         }
         else if (mode == "emit-llvm") {
             std::string llvm_code = emit_llvm(ch); // stub
             if (!outFile.empty()) emit_to_file(llvm_code, outFile);
             else std::cout << llvm_code; // Print to stdout if no output file specified
         }
         else {
             std::cerr << "Unknown mode: " << mode << "\n";
             return 1;
         }
     }
     catch (const std::exception& e) {
         std::cerr << "Error: " << e.what() << "\n";
         return 1;
     }
     return 0;
     }
     std::cout << "Triad Compiler v0.9.1\n";
     std::string mode = argv[1];
     std::string target = argc > 2 ? argv[2] : "";
     std::string outFile;
     bool verbose = false, showAst = false, showBytecode = false, traceVm
         = false;
     for (int i = 3; i < argc; ++i) {
         std::string arg = argv[i];
         if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
         else if (arg == "--verbose") verbose = true;
         else if (arg == "--show-ast") showAst = true;
         else if (arg == "--show-bytecode") showBytecode = true;
         else if (arg == "--trace-vm") trace
             Vm = true;
         else if (arg == "--version") {
             std::cout << "Triad Compiler v0.9.1\n";
             return 0;
         }
         else {
             std::cerr << "Unknown argument: " << arg << "\n";
             return 1;
         }
     }
     try {
         if (mode == "run-tests") {
             run_tests(verbose);
             return 0;
         }
         if (target.empty()) {
             std::cerr << "Error: No input file specified.\n";
             return 1;
         }
         std::string src = slurp(target);
         Chunk ch = parse_to_chunk(src);
         if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
         if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
         if (showAst) {
             std::cout << "[AST dump not yet implemented]\n";
             // ASTNode* ast = parse_to_ast(src); ast->dump();
         }
         if (mode == "run-vm") {
             run_vm(ch, traceVm);
         }
         else if (mode == "run-ast") {
             run_ast(src);
         }
         else if (mode == "emit-nasm") {
             std::string asm_code = emit_nasm(ch); // stub
             if (!outFile.empty()) emit_to_file(asm_code, outFile);
             else std::cout << asm_code; // Print to stdout if no output file specified
         }
         else if (mode == "emit-llvm") {
             std::string llvm_code = emit_llvm(ch); // stub
             if (!outFile.empty()) emit_to_file(llvm_code, outFile);
             else std::cout << llvm_code; // Print to stdout if no output file specified
         }
         else {
             std::cerr << "Unknown mode: " << mode << "\n";
             return 1;
         }
     }
     catch (const std::exception& e) {
         std::cerr << "Error: " << e.what() << "\n";
         return 1;
     }
     return 0;
     }
     std::cout << "Triad Compiler v0.9.1\n";
     std::string mode = argv[1];
     std::string target = argc > 2 ? argv[2] : "";
     std::string outFile;
     bool verbose = false, showAst = false, showBytecode = false, traceVm
         = false;
     for (int i = 3; i < argc; ++i) {
         std::string arg = argv[i];
         if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
         else if (arg == "--verbose") verbose = true;
         else if (arg == "--show-ast") showAst = true;
         else if (arg == "--show-bytecode") showBytecode = true;
         else if (arg == "--trace-vm") trace
             Vm = true;
         else if (arg == "--version") {
             std::cout << "Triad Compiler v0.9.1\n";
             return 0;
         }
         else {
             std::cerr << "Unknown argument: " << arg << "\n";
             return 1;
         }
     }
     try {
         if (mode == "run-tests") {
             run_tests(verbose);
             return 0;
         }
         if (target.empty()) {
             std::cerr << "Error: No input file specified.\n";
             return 1;
         }
         std::string src = slurp(target);
         Chunk ch = parse_to_chunk(src);
         if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
         if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
         if (showAst) {
             std::cout << "[AST dump not yet implemented]\n";
             // ASTNode* ast = parse_to_ast(src); ast->dump();
         }
         if (mode == "run-vm") {
             run_vm(ch, traceVm);
         }
         else if (mode == "run-ast") {
             run_ast(src);
         }
         else if (mode == "emit-nasm") {
             std::string asm_code = emit_nasm(ch); // stub
             if (!outFile.empty()) emit_to_file(asm_code, outFile);
             else std::cout << asm_code; // Print to stdout if no output file specified
         }
         else if (mode == "emit-llvm") {
             std::string llvm_code = emit_llvm(ch); // stub
             if (!outFile.empty()) emit_to_file(llvm_code, outFile);
             else std::cout << llvm_code; // Print to stdout if no output file specified
         }
         else {
             std::cerr << "Unknown mode: " << mode << "\n";
             return 1;
         }
     }
     catch (const std::exception& e) {
         std::cerr << "Error: " << e.what() << "\n";
         return 1;
     }
     return 0;
     }
     std::cout << "Triad Compiler v0.9.1\n";
     std::string mode = argv[1];
     std::string target = argc > 2 ? argv[2] : "";
     std::string outFile;
     bool verbose = false, showAst = false, showBytecode = false, traceVm
         = false;
     for (int i = 3; i < argc; ++i) {
         std::string arg = argv[i];
         if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
         else if (arg == "--verbose") verbose = true;
         else if (arg == "--show-ast") showAst = true;
         else if (arg == "--show-bytecode") showBytecode = true;
         else if (arg == "--trace-vm") trace
             Vm = true;
         else if (arg == "--version") {
             std::cout << "Triad Compiler v0.9.1\n";
             return 0;
         }
         else {
             std::cerr << "Unknown argument: " << arg << "\n";
             return 1;
         }
     }
     try {
         if (mode == "run-tests") {
             run_tests(verbose);
             return 0;
         }
         if (target.empty()) {
             std::cerr << "Error: No input file specified.\n";
             return 1;
         }
         std::string src = slurp(target);
         Chunk ch = parse_to_chunk(src);
         if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
         if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
         if (showAst) {
             std::cout << "[AST dump not yet implemented]\n";
             // ASTNode* ast = parse_to_ast(src); ast->dump();
         }
         if (mode == "run-vm") {
             run_vm(ch, traceVm);
         }
         else if (mode == "run-ast") {
             run_ast(src);
         }
         else if (mode == "emit-nasm") {
             std::string asm_code = emit_nasm(ch); // stub
             if (!outFile.empty()) emit_to_file(asm_code, outFile);
             else std::cout << asm_code; // Print to stdout if no output file specified
         }
         else if (mode == "emit-llvm") {
             std::string llvm_code = emit_llvm(ch); // stub
             if (!outFile.empty()) emit_to_file(llvm_code, outFile);
             else std::cout << llvm_code; // Print to stdout if no output file specified
         }
         else {
             std::cerr << "Unknown mode: " << mode << "\n";
             return 1;
         }
     }
     catch (const std::exception& e) {
         std::cerr << "Error: " << e.what() << "\n";
         return 1;
     }
     return 0;
     }
     std::cout << "Triad Compiler v0.9.1\n";
     std::string mode = argv[1];
     std::string target = argc > 2 ? argv[2] : "";
     std::string outFile;
     bool verbose = false, showAst = false, showBytecode = false, traceVm
         = false;
     for (int i = 3; i < argc; ++i) {
         std::string arg = argv[i];
         if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
         else if (arg == "--verbose") verbose = true;
         else if (arg == "--show-ast") showAst = true;
         else if (arg == "--show-bytecode") showBytecode = true;
         else if (arg == "--trace-vm") trace
             Vm = true;
         else if (arg == "--version") {
             std::cout << "Triad Compiler v0.9.1\n";
             return 0;
         }
         else {
             std::cerr << "Unknown argument: " << arg << "\n";
             return 1;
         }
     }
     try {
         if (mode == "run-tests") {
             run_tests(verbose);
             return 0;
         }
         if (target.empty()) {
             std::cerr << "Error: No input file specified.\n";
             return 1;
         }
         std::string src = slurp(target);
         Chunk ch = parse_to_chunk(src);
         if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
         if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
         if (showAst) {
             std::cout << "[AST dump not yet implemented]\n";
             // ASTNode* ast = parse_to_ast(src); ast->dump();
         }
         if (mode == "run-vm") {
             run_vm(ch, traceVm);
         }
         else if (mode == "run-ast") {
             run_ast(src);
         }
         else if (mode == "emit-nasm") {
             std::string asm_code = emit_nasm(ch); // stub
             if (!outFile.empty()) emit_to_file(asm_code, outFile);
             else std::cout << asm_code; // Print to stdout if no output file specified
         }
         else if (mode == "emit-llvm") {
             std::string llvm_code = emit_llvm(ch); // stub
             if (!outFile.empty()) emit_to_file(llvm_code, outFile);
             else std::cout << llvm_code; // Print to stdout if no output file specified
         }
         else {
             std::cerr << "Unknown mode: " << mode << "\n";
             return 1;
         }
     }
     catch (const std::exception& e) {
         std::cerr << "Error: " << e.what() << "\n";
         return 1;
     }
     return 0;
     }
     std::cout << "Triad Compiler v0.9.1\n";
     std::string mode = argv[1];
     std::string target = argc > 2 ? argv[2] : "";
     std::string outFile;
     bool verbose = false, showAst = false, showBytecode = false, traceVm
         = false;
     for (int i = 3; i < argc; ++i) {
         std::string arg = argv[i];
         if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
         else if (arg == "--verbose") verbose = true;
         else if (arg == "--show-ast") showAst = true;
         else if (arg == "--show-bytecode") showBytecode = true;
         else if (arg == "--trace-vm") trace
             Vm = true;
         else if (arg == "--version") {
             std::cout << "Triad Compiler v0.9.1\n";
             return 0;
         }
         else {
             std::cerr << "Unknown argument: " << arg << "\n";
             return 1;
         }
     }
     try {
         if (mode == "run-tests") {
             run_tests(verbose);
             return 0;
         }
         if (target.empty()) {
             std::cerr << "Error: No input file specified.\n";
             return 1;
         }
         std::string src = slurp(target);
         Chunk ch = parse_to_chunk(src);
         if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
         if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
         if (showAst) {
             std::cout << "[AST dump not yet implemented]\n";
             // ASTNode* ast = parse_to_ast(src); ast->dump();
         }
         if (mode == "run-vm") {
             run_vm(ch, traceVm);
         }
         else if (mode == "run-ast") {
             run_ast(src);
         }
         else if (mode == "emit-nasm") {
             std::string asm_code = emit_nasm(ch); // stub
             if (!outFile.empty()) emit_to_file(asm_code, outFile);
             else std::cout << asm_code; // Print to stdout if no output file specified
         }
         else if (mode == "emit-llvm") {
             std::string llvm_code = emit_llvm(ch); // stub
             if (!outFile.empty()) emit_to_file(llvm_code, outFile);
             else std::cout << llvm_code; // Print to stdout if no output file specified
         }
         else {
             std::cerr << "Unknown mode: " << mode << "\n";
             return 1;
         }
     }
     catch (const std::exception& e) {
         std::cerr << "Error: " << e.what() << "\n";
         return 1;
     }
     return 0;
     }
#include <iostream>
#include <fstream>
#include "triad_chunk.hpp"
#include "triad_value.hpp"
     namespace triad {
         Chunk parse_chunk(std::istream& is) {
             Chunk ch;
             std::string section;
             size_t count;
             is >> section >> count;
             if (section != "CODE") throw std::runtime_error("Expected CODE section");
             for (size_t i = 0; i < count; ++i) {
                 int op, a, b, c;
                 is >> op >> a >> b >> c;
                 if (op < 0 || op >= static_cast<int>(Op::COUNT)) {
                     throw std::runtime_error("Invalid opcode: " + std::to_string(op));
                 }
                 ch.code.push_back({ static_cast<Op>(op), a, b, c });
             }
             is >> section >> count;
             if (section != "CONSTS") throw std::runtime_error("Expected CONSTS section");
             is >> std::ws; // Skip whitespace
             for (size_t i = 0; i < count; ++i) {
                 std::string str;
                 std::getline(is, str);
                 if (!str.empty()) {
                     Value val;
                     if (str[0] == '"') {
                         val = Value::from_string(str.substr(1, str.size() - 2));
                     }
                     else {
                         try {
                             val = Value::from_number(std::stod(str));
                         }
                         catch (...) {
                             throw std::runtime_error("Invalid constant value: " + str);
                         }
                     }
                     ch.consts.push_back(val);
                 }
             }
             return ch;
         }
         void serialize_chunk(const Chunk& ch, std::ostream& os) {
             os << "CODE " << ch.code.size() << "\n";
             for (const auto& inst : ch.code) {
                 os << static_cast<int>(inst.op) << " " << inst.a << " " << inst.b << " " << inst.c << "\n";
             }
             os << "CONSTS " << ch.consts.size() << "\n";
             for (const auto& val : ch.consts) {
                 os << val.to_string() << "\n";
             }
         }
         std::string slurp(const std::string& filename) {
             std
                 ::ifstream file(filename);
             if (!file) {
                 throw std::runtime_error("Could not open file: " + filename);
             }
             std
                 std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
             return content;
         }
         int main(int argc, char** argv) {
             if (argc < 2) {
                 std::cerr << "Usage: triad <mode> [options] <input.triad> [output.chunk]\n";
                 return 1;
             }
             std::cout << "Triad Compiler v0.9.1\n";
             std::string mode = argv[1];
             std::string target = argc > 2 ? argv[2] : "";
             std::string outFile;
             bool verbose = false, showAst = false, showBytecode = false, traceVm = false;
             for (int i = 3; i < argc; ++i) {
                 std::string arg = argv[i];
                 if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
                 else if (arg == "--verbose") verbose = true;
                 else if (arg == "--show-ast") showAst = true;
                 else if (arg == "--show-bytecode") showBytecode = true;
                 else if (arg == "--trace-vm") traceVm = true;
                 else if (arg == "--version") {
                     std::cout << "Triad Compiler v0.9.1\n";
                     return 0;
                 }
                 else {
                     std::cerr << "Unknown argument: " << arg << "\n";
                     return 1;
                 }
             }
             try {
                 if (mode == "run-tests") {
                     run_tests(verbose);
                     return 0;
                 }
                 if (target.empty()) {
                     std::cerr << "Error: No input file specified.\n";
                     return 1;
                 }
                 std::string src = slurp(target);
                 Chunk ch = parse_to_chunk(src);
                 if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
                 if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
                 if (showAst) {
                     std::cout << "[AST dump not yet implemented]\n";
                     // ASTNode* ast = parse_to_ast(src); ast->dump();
                 }
                 if (mode == "run-vm") {
                     run_vm(ch, traceVm
                     );
                 }
                 else if (mode == "run-ast") {
                     run_ast(src);
                 }
                 else if (mode == "emit-nasm") {
                     std::string asm_code = emit_nasm(ch); // stub
                     if (!outFile.empty()) emit_to_file(asm_code, outFile);
                     else std::cout << asm_code; // Print to stdout if no output file specified
                 }
                 else if (mode == "emit-llvm") {
                     std::string llvm_code = emit_llvm(ch); // stub
                     if (!outFile.empty()) emit_to_file(llvm_code, outFile);
                     else std::cout << llvm_code; // Print to stdout if no output file specified
                 }
                 else {
                     std::cerr << "Unknown mode: " << mode << "\n";
                     return 1;
                 }
             }
             catch (const std::exception& e) {
                 std::cerr << "Error: " << e.what() << "\n";
                 return 1;
             }
             return 0;
         }
         void run_tests(bool verbose) {
             // Placeholder for test runner implementation
             if (verbose) std::cout << "Running tests...\n";
             // Actual test logic would go here
             if (verbose) std::cout << "All tests passed!\n";
         }
         void run_ast(const std::string& src) {
             // Placeholder for AST execution logic
             std::cout << "Running AST: " << src << "\n";
         }
         void run_vm(const Chunk& ch, bool traceVm) {
             // Placeholder for VM execution logic
             if (traceVm) std::cout << "Tracing VM execution...\n";
             std::cout << "Executing chunk with " << ch.code.size() << " instructions.\n";
         }
         std::string emit_nasm(const Chunk& ch) {
             // Placeholder for NASM code generation
             std::string asm_code = "section .text\n";
             for (const auto& inst : ch.code) {
                 asm_code += "  ; " + std::to_string(static_cast<int>(inst.op)) + "\n";
                 asm_code += "  nop\n"; // Replace with actual instruction translation
             }
             return asm_code;
         }
         std::string emit_llvm(const Chunk& ch) {
             // Placeholder for LLVM code generation
             std::string llvm_code = "define i32 @main() {\n";
             for (const auto& inst : ch.code) {
                 llvm_code += "  ; " + std::to_string(static_cast<int>(inst.op)) + "\n";
                 llvm_code += "  ret i32 0\n"; // Replace with actual instruction translation
             }
             llvm_code += "}\n";
             return llvm_code;
         }
         void emit_to_file(const std::string& code, const std::string& filename) {
             std::ofstream ofs(filename);
             if (!ofs) {
                 throw std::runtime_error("Could not open output file: " + filename);
             }
             ofs << code;
         }
         std::string slurp(const std::string& filename) {
             std
                 std

                 ::ifstream file(filename);
             if (!file) {
                 throw std::runtime_error("Could not open file: " + filename);
             }
             return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
         }
         std::string slurp(const std::string& filename) {
             std::ifstream file(filename);
             if (!file) {
                 throw std::runtime_error("Could not open file: " + filename);
             }
             return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
         }
#include <iostream>
#include <fstream>
#include "triad_chunk.hpp"
#include "triad_value.hpp"
         int main(int argc, char** argv) {
             if (argc < 2) {
                 std::cerr << "Usage: triad <mode> [options] <input.triad> [output.chunk]\n";
                 return 1;
             }
             std::cout << "Triad Compiler v0.9.1\n";
             std::string mode = argv[1];
             std::string target = argc > 2 ? argv[2] : "";
             std::string outFile;
             bool verbose = false, showAst = false, showBytecode = false, traceVm
                 = false;
             for (int i = 3; i < argc; ++i) {
                 std::string arg = argv[i];
                 if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
                 else if (arg == "--verbose") verbose = true;
                 else if (arg == "--show-ast") showAst = true;
                 else if (arg == "--show-bytecode") showBytecode = true;
                 else if (arg == "--trace-vm") traceVm = true;
                 else if (arg == "--version") {
                     std::cout << "Triad Compiler v0.9.1\n";
                     return 0;
                 }
                 else {
                     std::cerr << "Unknown argument: " << arg << "\n";
                     return 1;
                 }
             }
             if (mode == "run-tests") {
                 run_tests(verbose);
                 return 0;
             }
             if (target.empty()) {
                 std::cerr << "Error: No input file specified.\n";
                 return 1;
             }
             if (mode != "run-vm" && mode != "run-ast" && mode != "emit-nasm" &&
                 mode != "emit-llvm") {
                 std::cerr << "Error: Invalid mode. Use 'run-vm', 'run-ast', 'emit-nasm', or 'emit-llvm'.\n";
                 return 1;
             }
             try {
                 std::string src = slurp(target);
                 Chunk ch = parse_to_chunk(src);
                 if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
                 if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
                 if (showAst) {
                     std::cout << "[AST dump not yet implemented]\n";
                     // ASTNode* ast = parse_to_ast(src); ast->dump();
                 }
                 if (mode == "run-vm") {
                     run_vm(ch, traceVm);
                 }
                 else if (mode == "run-ast") {
                     run_ast(src);
                 }
                 else if (mode == "emit-nasm") {
                     std::string asm_code = emit_nasm(ch); // stub
                     if (!outFile.empty()) emit_to_file(asm_code, outFile);
                     else std::cout << asm_code; // Print to stdout if no output file specified
                 }
                 else if (mode == "emit-llvm") {
                     std::string llvm_code = emit_llvm(ch); // stub
                     if (!outFile.empty()) emit_to_file(llvm_code, outFile);
                     else std::cout << llvm_code; // Print to stdout if no output file specified
                 }
                 else {
                     std::cerr << "Unknown mode: " << mode << "\n";
                     return 1;
                 }
             }
             catch (const std::exception& e) {
                 std::cerr << "Error: " << e.what() << "\n";
                 return 1;
             }
             return 0;
         }
         std::cout << "Triad Compiler v0.9.1\n";
         std::string mode = argv[1];
         std::string target = argc > 2 ? argv[2] : "";
         std::string outFile;
         bool verbose = false, showAst = false, showBytecode = false, traceVm
             = false;
         for (int i = 3; i < argc; ++i) {
             std::string arg = argv[i];
             if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
             else if (arg == "--verbose") verbose = true;
             else if (arg == "--show-ast") showAst = true;
             else if (arg == "--show-bytecode") showBytecode = true;
             else if (arg == "--trace-vm") trace
                 Vm = true;
             else if (arg == "--version") {
                 std::cout << "Triad Compiler v0.9.1\n";
                 return 0;
             }
             else {
                 std::cerr << "Unknown argument: " << arg << "\n";
                 return 1;
             }
         }
         try {
             if (mode == "run-tests") {
                 run_tests(verbose);
                 return 0;
             }
             if (target.empty()) {
                 std::cerr << "Error: No input file specified.\n";
                 return 1;
             }
             std::string src = slurp(target);
             Chunk ch = parse_to_chunk(src);
             if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
             if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
             if (showAst) {
                 std::cout << "[AST dump not yet implemented]\n";
                 // ASTNode* ast = parse_to_ast(src); ast->dump();
             }
             if (mode == "run-vm") {
                 run_vm(ch, traceVm);
             }
             else if (mode == "run-ast") {
                 run_ast(src);
             }
             else if (mode == "emit-nasm") {
                 std::string asm_code = emit_nasm(ch); // stub
                 if (!outFile.empty()) emit_to_file(asm_code, outFile);
                 else std::cout << asm_code; // Print to stdout if no output file specified
             }
             else if (mode == "emit-llvm") {
                 std::string llvm_code = emit_llvm(ch); // stub
                 if (!outFile.empty()) emit_to_file(llvm_code, outFile);
                 else std::cout << llvm_code; // Print to stdout if no output file specified
             }
             else {
                 std::cerr << "Unknown mode: " << mode << "\n";
                 return 1;
             }
         }
         catch (const std::exception& e) {
             std::cerr << "Error: " << e.what() << "\n";
             return 1;
         }
         return 0;
     }
     std::cout << "Triad Compiler v0.9.1\n";
     std::string mode = argv[1];
     std::string target = argc > 2 ? argv[2] : "";
     std::string outFile;
     bool verbose = false, showAst = false, showBytecode = false, traceVm
         = false;
     for (int i = 3; i < argc; ++i) {
         std::string arg = argv[i];
         if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
         else if (arg == "--verbose") verbose = true;
         else if (arg == "--show-ast") showAst = true;
         else if (arg == "--show-bytecode") showBytecode = true;
         else if (arg == "--trace-vm") trace
             Vm = true;
         else if (arg == "--version") {
             std::cout << "Triad Compiler v0.9.1\n";
             return 0;
         }
         else {
             std::cerr << "Unknown argument: " << arg << "\n";
             return 1;
         }
     }
     try {
         if (mode == "run-tests") {
             run_tests(verbose);
             return 0;
         }
         if (target.empty()) {
             std::cerr << "Error: No input file specified.\n";
             return 1;
         }
         std::string src = slurp(target);
         Chunk ch = parse_to_chunk(src);
         if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
         if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
         if (showAst) {
             std::cout << "[AST dump not yet implemented]\n";
             // ASTNode* ast = parse_to_ast(src); ast->dump();
         }
         if (mode == "run-vm") {
             run_vm(ch, traceVm);
         }
         else if (mode == "run-ast") {
             run_ast(src);
         }
         else if (mode == "emit-nasm") {
             std::string asm_code = emit_nasm(ch); // stub
             if (!outFile.empty()) emit_to_file(asm_code, outFile);
             else std::cout << asm_code; // Print to stdout if no output file specified
         }
         else if (mode == "emit-llvm") {
             std::string llvm_code = emit_llvm(ch); // stub
             if (!outFile.empty()) emit_to_file(llvm_code, outFile);
             else std::cout << llvm_code; // Print to stdout if no output file specified
         }
         else {
             std::cerr << "Unknown mode: " << mode << "\n";
             return 1;
         }
     }
     catch (const std::exception& e) {
         std::cerr << "Error: " << e.what() << "\n";
         return 1;
     }
     return 0;
     }
     std::cout << "Triad Compiler v0.9.1\n";
     std::string mode = argv[1];
     std::string target = argc > 2 ? argv[2] : "";
     std::string outFile;
     bool verbose = false, showAst = false, showBytecode = false, traceVm
         = false;
     for (int i = 3; i < argc; ++i) {
         std::string arg = argv[i];
         if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
         else if (arg == "--verbose") verbose = true;
         else if (arg == "--show-ast") showAst = true;
         else if (arg == "--show-bytecode") showBytecode = true;
         else if (arg == "--trace-vm") trace
             Vm = true;
         else if (arg == "--version") {
             std::cout << "Triad Compiler v0.9.1\n";
             return 0;
         }
         else {
             std::cerr << "Unknown argument: " << arg << "\n";
             return 1;
         }
     }
     try {
         if (mode == "run-tests") {
             run_tests(verbose);
             return 0;
         }
         if (target.empty()) {
             std::cerr << "Error: No input file specified.\n";
             return 1;
         }
         std::string src = slurp(target);
         Chunk ch = parse_to_chunk(src);
         if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
         if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
         if (showAst) {
             std::cout << "[AST dump not yet implemented]\n";
             // ASTNode* ast = parse_to_ast(src); ast->dump();
         }
         if (mode == "run-vm") {
             run_vm(ch, traceVm);
         }
         else if (mode == "run-ast") {
             run_ast(src);
         }
         else if (mode == "emit-nasm") {
             std::string asm_code = emit_nasm(ch); // stub
             if (!outFile.empty()) emit_to_file(asm_code, outFile);
             else std::cout << asm_code; // Print to stdout if no output file specified
         }
         else if (mode == "emit-llvm") {
             std::string llvm_code = emit_llvm(ch); // stub
             if (!outFile.empty()) emit_to_file(llvm_code, outFile);
             else std::cout << llvm_code; // Print to stdout if no output file specified
         }
         else {
             std::cerr << "Unknown mode: " << mode << "\n";
             return 1;
         }
     }
     catch (const std::exception& e) {
         std::cerr << "Error: " << e.what() << "\n";
         return 1;
     }
     return 0;
     }
     std::cout << "Triad Compiler v0.9.1\n";
     std::string mode = argv[1];
     std::string target = argc > 2 ? argv[2] : "";
     std::string outFile;
     bool verbose = false, showAst = false, showBytecode = false, traceVm
         = false;
     for (int i = 3; i < argc; ++i) {
         std::string arg = argv[i];
         if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
         else if (arg == "--verbose") verbose = true;
         else if (arg == "--show-ast") showAst = true;
         else if (arg == "--show-bytecode") showBytecode = true;
         else if (arg == "--trace-vm") trace
             Vm = true;
         else if (arg == "--version") {
             std::cout << "Triad Compiler v0.9.1\n";
             return 0;
         }
         else {
             std::cerr << "Unknown argument: " << arg << "\n";
             return 1;
         }
     }
     try {
         if (mode == "run-tests") {
             run_tests(verbose);
             return 0;
         }
         if (target.empty()) {
             std::cerr << "Error: No input file specified.\n";
             return 1;
         }
         std::string src = slurp(target);
         Chunk ch = parse_to_chunk(src);
         if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
         if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
         if (showAst) {
             std::cout << "[AST dump not yet implemented]\n";
             // ASTNode* ast = parse_to_ast(src); ast->dump();
         }
         if (mode == "run-vm") {
             run_vm(ch, traceVm);
         }
         else if (mode == "run-ast") {
             run_ast(src);
         }
         else if (mode == "emit-nasm") {
             std::string asm_code = emit_nasm(ch); // stub
             if (!outFile.empty()) emit_to_file(asm_code, outFile);
             else std::cout << asm_code; // Print to stdout if no output file specified
         }
         else if (mode == "emit-llvm") {
             std::string llvm_code = emit_llvm(ch); // stub
             if (!outFile.empty()) emit_to_file(llvm_code, outFile);
             else std::cout << llvm_code; // Print to stdout if no output file specified
         }
         else {
             std::cerr << "Unknown mode: " << mode << "\n";
             return 1;
         }
     }
     catch (const std::exception& e) {
         std::cerr << "Error: " << e.what() << "\n";
         return 1;
     }
     }
     return 0;
     }
     std::cout << "Triad Compiler v0.9.1\n";
     std::string mode = argv[1];
     std::string target = argc > 2 ? argv[2] : "";
     std::string outFile;
     bool verbose = false, showAst = false, showBytecode = false, traceVm
         = false;
     for (int i = 3; i < argc; ++i) {
         std::string arg = argv[i];
         if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
         else if (arg == "--verbose") verbose = true;
         else if (arg == "--show-ast") showAst = true;
         else if (arg == "--show-bytecode") showBytecode = true;
         else if (arg == "--trace-vm") trace
             Vm = true;
         else if (arg == "--version") {
             std::cout << "Triad Compiler v0.9.1\n";
             return 0;
         }
         else {
             std::cerr << "Unknown argument: " << arg << "\n";
             return 1;
         }
     }
     try {
         if (mode == "run-tests") {
             run_tests(verbose);
             return 0;
         }
         if (target.empty()) {
             std::cerr << "Error: No input file specified.\n";
             return 1;
         }
         std::string src = slurp(target);
         Chunk ch = parse_to_chunk(src);
         if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
         if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
         if (showAst) {
             std::cout << "[AST dump not yet implemented]\n";
             // ASTNode* ast = parse_to_ast(src); ast->dump();
         }
         if (mode == "run-vm") {
             run_vm(ch, traceVm);
         }
         else if (mode == "run-ast") {
             run_ast(src);
         }
         else if (mode == "emit-nasm") {
             std::string asm_code = emit_nasm(ch); // stub
             if (!outFile.empty()) emit_to_file(asm_code, outFile);
             else std::cout << asm_code; // Print to stdout if no output file specified
         }
         else if (mode == "emit-llvm") {
             std::string llvm_code = emit_llvm(ch); // stub
             if (!outFile.empty()) emit_to_file(llvm_code, outFile);
             else std::cout << llvm_code; // Print to stdout if no output file specified
         }
         else {
             std::cerr << "Unknown mode: " << mode << "\n";
             return 1;
         }
     }
     catch (const std::exception& e) {
         std::cerr << "Error: " << e.what() << "\n";
         return 1;
     }
     return 0;
     }
     std::cout << "Triad Compiler v0.9.1\n";
     std::string mode = argv[1];
     std::string target = argc > 2 ? argv[2] : "";
     std::string outFile;
     bool verbose = false, showAst = false, showBytecode = false, traceVm
         = false;
     for (int i = 3; i < argc; ++i) {
         std::string arg = argv[i];
         if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
         else if (arg == "--verbose") verbose = true;
         else if (arg == "--show-ast") showAst = true;
         else if (arg == "--show-bytecode") showBytecode = true;
         else if (arg == "--trace-vm") trace
             Vm = true;
         else if (arg == "--version") {
             std::cout << "Triad Compiler v0.9.1\n";
             return 0;
         }
         else {
             std::cerr << "Unknown argument: " << arg << "\n";
             return 1;
         }
     }
     try {
         if (mode == "run-tests") {
             run_tests(verbose);
             return 0;
         }
         if (target.empty()) {
             std::cerr << "Error: No input file specified.\n";
             return 1;
         }
         std::string src = slurp(target);
         Chunk ch = parse_to_chunk(src);
         if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
         if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
         if (showAst) {
             std::cout << "[AST dump not yet implemented]\n";
             // ASTNode* ast = parse_to_ast(src); ast->dump();
         }
         if (mode == "run-vm") {
             run_vm(ch, traceVm);
         }
         else if (mode == "run-ast") {
             run_ast(src);
         }
         else if (mode == "emit-nasm") {
             std::string asm_code = emit_nasm(ch); // stub
             if (!outFile.empty()) emit_to_file(asm_code, outFile);
             else std::cout << asm_code; // Print to stdout if no output file specified
         }
         else if (mode == "emit-llvm") {
             std::string llvm_code = emit_llvm(ch); // stub
             if (!outFile.empty()) emit_to_file(llvm_code, outFile);
             else std::cout << llvm_code; // Print to stdout if no output file specified
         }
         else {
             std::cerr << "Unknown mode: " << mode << "\n";
             return 1;
         }
     }
     catch (const std::exception& e) {
         std::cerr << "Error: " << e.what() << "\n";
         return 1;
     }
     return 0;
     }
     std::cout << "Triad Compiler v0.9.1\n";
     std::string mode = argv[1];
     std::string target = argc > 2 ? argv[2] : "";
     std::string outFile;
     bool verbose = false, showAst = false, showBytecode = false, traceVm
         = false;
     for (int i = 3; i < argc; ++i) {
         std::string arg = argv[i];
         if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
         else if (arg == "--verbose") verbose = true;
         else if (arg == "--show-ast") showAst = true;
         else if (arg == "--show-bytecode") showBytecode = true;
         else if (arg == "--trace-vm") trace
             Vm = true;
         else if (arg == "--version") {
             std::cout << "Triad Compiler v0.9.1\n";
             return 0;
         }
         else {
             std::cerr << "Unknown argument: " << arg << "\n";
             return 1;
         }
     }
     try {
         if (mode == "run-tests") {
             run_tests(verbose);
             return 0;
         }
         if (target.empty()) {
             std::cerr << "Error: No input file specified.\n";
             return 1;
         }
         std::string src = slurp(target);
         Chunk ch = parse_to_chunk(src);
         if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
         if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
         if (showAst) {
             std::cout << "[AST dump not yet implemented]\n";
             // ASTNode* ast = parse_to_ast(src); ast->dump();
         }
         if (mode == "run-vm") {
             run_vm(ch, traceVm);
         }
         else if (mode == "run-ast") {
             run_ast(src);
         }
         else if (mode == "emit-nasm") {
             std::string asm_code = emit_nasm(ch); // stub
             if (!outFile.empty()) emit_to_file(asm_code, outFile);
             else std::cout << asm_code; // Print to stdout if no output file specified
         }
         else if (mode == "emit-llvm") {
             std::string llvm_code = emit_llvm(ch); // stub
             if (!outFile.empty()) emit_to_file(llvm_code, outFile);
             else std::cout << llvm_code; // Print to stdout if no output file specified
         }
         else {
             std::cerr << "Unknown mode: " << mode << "\n";
             return 1;
         }
     }
     catch (const std::exception& e) {
         std::cerr << "Error: " << e.what() << "\n";
         return 1;
     }
     return 0;
     }
#include <iostream>
#include <fstream>
#include "triad_chunk.hpp"
#include "triad_value.hpp"
     int main(int argc, char** argv) {
         if (argc < 2) {
             std::cerr << "Usage: triad <mode> [options] <input.triad> [output.chunk]\n";
             return 1;
         }
         std::cout << "Triad Compiler v0.9.1\n";
         std::string mode = argv[1];
         std::string target = argc > 2 ? argv[2] : "";
         std::string outFile;
         bool verbose = false, showAst = false, showBytecode = false, traceVm
             = false;
         for (int i = 3; i < argc; ++i) {
             std::string arg = argv[i];
             if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
             else if (arg == "--verbose") verbose = true;
             else if (arg == "--show-ast") showAst = true;
             else if (arg == "--show-bytecode") showBytecode = true;
             else if (arg == "--trace-vm") traceVm = true;
             else if (arg == "--version") {
                 std::cout << "Triad Compiler v0.9.1\n";
                 return 0;
             }
             else {
                 std::cerr << "Unknown argument: " << arg << "\n";
                 return 1;
             }
         }
         try {
             if (mode == "run-tests") {
                 run_tests(verbose);
                 return 0;
             }
             if (target.empty()) {
                 std::cerr << "Error: No input file specified.\n";
                 return 1;
             }
             std::string src = slurp(target);
             Chunk ch = parse_to_chunk(src);
             if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
             if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
             if (showAst) {
                 std::cout << "[AST dump not yet implemented]\n";
                 // ASTNode* ast = parse_to_ast(src); ast->dump();
             }
             if (mode == "run-vm") {
                 run_vm(ch, traceVm);
             }
             else if (mode == "run-ast") {
                 run_ast(src);
             }
             else if (mode == "emit-nasm") {
                 std::string asm_code = emit_nasm(ch); // stub
                 if (!outFile.empty()) emit_to_file(asm_code, outFile);
                 else std::cout << asm_code; // Print to stdout if no output file specified
             }
             else if (mode == "emit-llvm") {
                 std::string llvm_code = emit_llvm(ch); // stub
                 if (!outFile.empty()) emit_to_file(llvm_code, outFile);
                 else std::cout << llvm_code; // Print to stdout if no output file specified
             }
             else {
                 std::cerr << "Unknown mode: " << mode << "\n";
                 return 1;
             }
         }
         catch (const std::exception& e) {
             std::cerr << "Error: " << e.what() << "\n";
             return 1;
         }
         return 0;
     }
     Chunk parse_to_chunk(const std::string& src) {
         Chunk ch;
         std::istringstream is(src);
         std::string section;
         size_t count;
         is >> section >> count;
         if (section != "CODE") throw std::runtime_error("Expected CODE section");
         for (size_t i = 0; i < count; ++i) {
             int op, a, b, c;
             is >> op >> a >> b >> c;
             ch.code.push_back(Instruction{ static_cast<OpCode>(op), a, b, c });
         }
         is >> section >> count;
         if (section != "CONSTS") throw std::runtime_error("Expected CONSTS section");
         for (size_t i = 0; i < count; ++i) {
             std::string str;
             is >> str;
             if (str == "null") {
                 ch.consts.push_back(Value::null());
             }
             else if (str == "true") {
                 ch.consts.push_back(Value::from_bool(true));
             }
             else if (str == "false") {
                 ch.consts.push_back(Value::from_bool(false));
             }
             else {
                 Value val;
                 if (str.size() >= 2 && str.front() == '"' && str.back() == '"') {
                     val = Value::from_string(str.substr(1, str.size() - 2));
                 }
                 else if (str.find('.') != std::string::npos) {
                     val = Value::from_float(std::stof(str));
                 }
                 else {
                     try {
                         val = Value::from_int(std::stoi(str));
                     }
                     catch (const std::invalid_argument&) {
                         throw std::runtime_error("Invalid constant value: " + str);
                     }
                 }
                 ch.consts.push_back(val);
             }
         }
         return ch;
     }
     std::string slurp(const std::string& filename) {
         std
             std
             ::ifstream file(filename);
         if (!file) {
             throw std::runtime_error("Could not open file: " + filename);
         }
         return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
     }
     int main(int argc, char** argv) {
         if (argc < 2) {
             std::cerr << "Usage: triad <mode> [options] <input.triad> [output.chunk]\n";
             return 1;
         }
         std::cout << "Triad Compiler v0.9.1\n";
         std::string mode = argv[1];
         std::string target = argc > 2 ? argv[2] : "";
         std::string outFile;
         bool verbose = false, showAst = false, showBytecode = false, traceVm
             = false;
         for (int i = 3; i < argc; ++i) {
             std::string arg = argv[i];
             if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
             // Handle other arguments
             else if (arg == "--help") {
                 std::cout << "Usage: triad <mode> [options] <input.triad> [output.chunk]\n";
                 std::cout << "Modes:\n";
                 std::cout << "  run-vm       Run the VM with the given chunk\n";
                 std::cout << "  run-ast      Run the AST directly\n";
                 std::cout << "  emit-nasm    Emit NASM assembly code\n";
                 std::cout << "  emit-llvm    Emit LLVM IR code\n";
                 std::cout << "  run-tests    Run the test suite\n";
                 return 0;
             }
             else if (arg == "--verbose") verbose = true;
             else if (arg == "--show-ast") showAst = true;
             else if (arg == "--show-bytecode") showBytecode = true;
             else if (arg == "--trace-vm") traceVm = true;
             else if (arg == "--version") {
                 std::cout << "Triad Compiler v0.9.1\n";
                 return 0;
             }
             else {
                 std::cerr << "Unknown argument: " << arg << "\n";
                 return 1;
             }
         }
         try {
             if (mode == "run-tests") {
                 run_tests(verbose);
                 return 0;
             }
             if (target.empty()) {
                 std::cerr << "Error: No input file specified.\n";
                 return 1;
             }
             std::string src = slurp(target);
             Chunk ch = parse_to_chunk(src);
             if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
             if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
             if (showAst) {
                 std::cout << "[AST dump not yet implemented]\n";
                 // ASTNode* ast = parse_to_ast(src); ast->dump();
             }
             if (mode == "run-vm") {
                 run_vm(ch, traceVm);
             }
             else if (mode == "run-ast") {
                 run_ast(src);
             }
             else if (mode == "emit-nasm") {
                 std::string asm_code = emit_nasm(ch); // stub
                 if (!outFile.empty()) emit_to_file(asm_code, outFile);
                 else std::cout << asm_code; // Print to stdout if no output file specified
             }
             else if (mode == "emit-llvm") {
                 std::string llvm_code = emit_llvm(ch); // stub
                 if (!outFile.empty()) emit_to_file(llvm_code, outFile);
                 else std::cout << llvm_code; // Print to stdout if no output file specified
             }
             else {
                 std::cerr << "Unknown mode: " << mode << "\n";
                 return 1;
             }
         }
         catch (const std::exception& e) {
             std::cerr << "Error: " << e.what() << "\n";
             return 1;
         }
         return 0;
     }
     void run_vm(const Chunk& ch, bool trace) {
         // Placeholder for VM execution logic
         if (trace) {
             std::cout << "[VM execution trace enabled]\n";
         }
         for (const auto& inst : ch.code) {
             // Simulate instruction execution
             std::cout << "Executing: " << static_cast<int>(inst.op) << " "
                 << inst.a << " " << inst.b << " " << inst.c << "\n";
             // Actual VM logic would go here
         }
     }
     void run_ast(const std::string& src) {
         // Placeholder for AST execution logic
         std::cout << "[Running AST directly]\n";
         // Actual AST execution logic would go here
     }
     std::string emit_nasm(const Chunk& ch) {
         // Placeholder for NASM code generation
         std::string asm_code = "section .text\n";
         asm_code += "global _start\n";
         asm_code += "_start:\n";
         for (const auto& inst : ch.code) {
             asm_code += "    ; Instruction: " + std::to_string(static_cast<int>(inst.op)) + "\n";
             asm_code += "    ; Operands: " + std::to_string(inst.a) + ", "
                 + std::to_string(inst.b) + ", " + std::to_string(inst.c) + "\n";
             // Actual NASM code generation logic would go here
         }
         return asm_code;
     }
     std::string emit_llvm(const Chunk& ch) {
         // Placeholder for LLVM IR code generation
         std::string llvm_code = "define i32 @main() {\n";
         llvm_code += "    ret i32 0\n";
         llvm_code += "}\n";
         for (const auto& inst : ch.code) {
             llvm_code += "    ; Instruction: " + std::to_string(static_cast<int>(inst.op)) + "\n";
             llvm_code += "    ; Operands: " + std::to_string(inst.a) + ", "
                 + std::to_string(inst.b) + ", " + std::to_string(inst.c) + "\n";
             // Actual LLVM IR code generation logic would go here
         }
         return llvm_code;
     }
     void emit_to_file(const std::string& code, const std::string& filename) {
         std::ofstream out(filename);
         if (!out) {
             throw std::runtime_error("Could not open output file: " + filename);
         }
         out << code;
         out.close();
     }
     void run_tests(bool verbose) {
         // Placeholder for test suite execution
         if (verbose) {
             std::cout << "[Running test suite]\n";
         }
         // Actual test logic would go here
     }
     int main(int argc, char** argv) {
         if (argc < 2) {
             std::cerr << "Usage: triad <mode> [options] <input.triad> [output.chunk]\n";
             return 1;
         }
         std::cout << "Triad Compiler v0.9.1\n";
         std::string mode = argv[1];
         std::string target = argc > 2 ? argv[2] : "";
         std::string outFile;
         bool verbose = false, showAst = false, showBytecode = false, traceVm
             = false;
         for (int i = 3; i < argc; ++i) {
             std::string arg = argv[i];
             if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
             else if (arg == "--verbose") verbose = true;
             else if (arg == "--show-ast") showAst = true;
             else if (arg == "--show-bytecode") showBytecode = true;
             else if (arg == "--trace-vm") traceVm = true;
             else if (arg == "--version") {
                 std::cout << "Triad Compiler v0.9.1\n";
                 return 0;
             }
             else {
                 std::cerr << "Unknown argument: " << arg << "\n";
                 return 1;
             }
         }
         if (mode == "run-tests") {
             run_tests(verbose);
             return 0;
         }
         if (target.empty()) {
             std::cerr << "Error: No input file specified.\n";
             return 1;
         }
         std::string src = slurp(target);
         Chunk ch = parse_to_chunk(src);
         if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
         if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
         if (showAst) {
             std::cout << "[AST dump not yet implemented]\n";
             // ASTNode* ast = parse_to_ast(src); ast->dump();
         }
         try {
             if (mode == "run-vm") {
                 run_vm(ch, traceVm);
             }
             else if (mode == "run-ast") {
                 run_ast(src);
             }
             else if (mode == "emit-nasm") {
                 std::string asm_code = emit_nasm(ch); // stub
                 if (!outFile.empty()) emit_to_file(asm_code, outFile);
                 else std::cout << asm_code; // Print to stdout if no output file specified
             }
             else if (mode == "emit-llvm") {
                 std::string llvm_code = emit_llvm(ch); // stub
                 if (!outFile.empty()) emit_to_file(llvm_code, outFile);
                 else std::cout << llvm_code; // Print to stdout if no output file specified
             }
             else {
                 std::cerr << "Unknown mode: " << mode << "\n";
                 return 1;
             }
         }
         catch (const std::exception& e) {
             std::cerr << "Error: " << e.what() << "\n";
             return 1;
         }
         return 0;
     }
     std::cout << "Triad Compiler v0.9.1\n";
     std::string mode = argv[1];
     std::string target = argc > 2 ? argv[2] : "";
     std::string outFile;
     bool verbose = false, showAst = false, showBytecode = false, traceVm
         = false;
     for (int i = 3; i < argc; ++i) {
         std::string arg = argv[i];
         if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
         else if (arg == "--verbose") verbose = true;
         else if (arg == "--show-ast") showAst = true;
         else if (arg == "--show-bytecode") showBytecode = true;
         else if (arg == "--trace-vm") trace
             Vm = true;
         else if (arg == "--version") {
             std::cout << "Triad Compiler v0.9.1\n";
             return 0;
         }
         else {
             std::cerr << "Unknown argument: " << arg << "\n";
             return 1;
         }
     }
     try {
         if (mode == "run-tests") {
             run_tests(verbose);
             return 0;
         }
         if (target.empty()) {
             std::cerr << "Error: No input file specified.\n";
             return 1;
         }
         std::string src = slurp(target);
         Chunk ch = parse_to_chunk(src);
         if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
         if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
         if (showAst) {
             std::cout << "[AST dump not yet implemented]\n";
             // ASTNode* ast = parse_to_ast(src); ast->dump();
         }
         if (mode == "run-vm") {
             run_vm(ch, traceVm);
         }
         else if (mode == "run-ast") {
             run_ast(src);
         }
         else if (mode == "emit-nasm") {
             std::string asm_code = emit_nasm(ch); // stub
             if (!outFile.empty()) emit_to_file(asm_code, outFile);
             else std::cout << asm_code; // Print to stdout if no output file specified
         }
         else if (mode == "emit-llvm") {
             std::string llvm_code = emit_llvm(ch); // stub
             if (!outFile.empty()) emit_to_file(llvm_code, outFile);
             else std::cout << llvm_code; // Print to stdout if no output file specified
         }
         else {
             std::cerr << "Unknown mode: " << mode << "\n";
             return 1;
         }
     }
     catch (const std::exception& e) {
         std::cerr << "Error: " << e.what() << "\n";
         return 1;
     }
     return 0;
     }
     std::cout << "Triad Compiler v0.9.1\n";
     std::string mode = argv[1];
     std::string target = argc > 2 ? argv[2] : "";
     std::string outFile;
     bool verbose = false, showAst = false, showBytecode = false, traceVm
         = false;
     for (int i = 3; i < argc; ++i) {
         std::string arg = argv[i];
         if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
         else if (arg == "--verbose") verbose = true;
         else if (arg == "--show-ast") showAst = true;
         else if (arg == "--show-bytecode") showBytecode = true;
         else if (arg == "--trace-vm") trace
             Vm = true;
         else if (arg == "--version") {
             std::cout << "Triad Compiler v0.9.1\n";
             return 0;
         }
         else {
             std::cerr << "Unknown argument: " << arg << "\n";
             return 1;
         }
     }
     try {
         if (mode == "run-tests") {
             run_tests(verbose);
             return 0;
         }
         if (target.empty()) {
             std::cerr << "Error: No input file specified.\n";
             return 1;
         }
         std::string src = slurp(target);
         Chunk ch = parse_to_chunk(src);
         if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
         if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
         if (showAst) {
             std::cout << "[AST dump not yet implemented]\n";
             // ASTNode* ast = parse_to_ast(src); ast->dump();
         }
         if (mode == "run-vm") {
             run_vm(ch, traceVm);
         }
         else if (mode == "run-ast") {
             run_ast(src);
         }
         else if (mode == "emit-nasm") {
             std::string asm_code = emit_nasm(ch); // stub
             if (!outFile.empty()) emit_to_file(asm_code, outFile);
             else std::cout << asm_code; // Print to stdout if no output file specified
         }
         else if (mode == "emit-llvm") {
             std::string llvm_code = emit_llvm(ch); // stub
             if (!outFile.empty()) emit_to_file(llvm_code, outFile);
             else std::cout << llvm_code; // Print to stdout if no output file specified
         }
         else {
             std::cerr << "Unknown mode: " << mode << "\n";
             return 1;
         }
     }
     catch (const std::exception& e) {
         std::cerr << "Error: " << e.what() << "\n";
         return 1;
     }
     return 0;
     }
     std::cout << "Triad Compiler v0.9.1\n";
     std::string mode = argv[1];
     std::string target = argc > 2 ? argv[2] : "";
     std::string outFile;
     bool verbose = false, showAst = false, showBytecode = false, traceVm
         = false;
     for (int i = 3; i < argc; ++i) {
         std::string arg = argv[i];
         if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
         else if (arg == "--verbose") verbose = true;
         else if (arg == "--show-ast") showAst = true;
         else if (arg == "--show-bytecode") showBytecode = true;
         else if (arg == "--trace-vm") traceVm = true;
         else if (arg == "--version") {
             std::cout << "Triad Compiler v0.9.1\n";
             return 0;
         }
         else {
             std::cerr << "Unknown argument: " << arg << "\n";
             return 1;
         }
     }
     try {
         if (mode == "run-tests") {
             run_tests(verbose);
             return 0;
         }
         if (target.empty()) {
             std::cerr << "Error: No input file specified.\n";
             return 1;
         }
         std::string src = slurp(target);
         Chunk ch = parse_to_chunk(src);
         if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
         if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
         if (showAst) {
             std::cout << "[AST dump not yet implemented]\n";
             // ASTNode* ast = parse_to_ast(src); ast->dump();
         }
         if (mode == "run-vm") {
             run_vm(ch, traceVm);
         }
         else if (mode == "run-ast") {
             run_ast(src);
         }
         else if (mode == "emit-nasm") {
             std::string asm_code = emit_nasm(ch); // stub
             if (!outFile.empty()) emit_to_file(asm_code, outFile);
             else std::cout << asm_code; // Print to stdout if no output file specified
         }
         else if (mode == "emit-llvm") {
             std::string llvm_code = emit_llvm(ch); // stub
             if (!outFile.empty()) emit_to_file(llvm_code, outFile);
             else std::cout << llvm_code; // Print to stdout if no output file specified
         }
         else {
             std::cerr << "Unknown mode: " << mode << "\n";
             return 1;
         }
     }
     catch (const std::exception& e) {
         std::cerr << "Error: " << e.what() << "\n";
         return 1;
     }
     return 0;
     }
#include <iostream>
#include <fstream>
#include "triad_chunk.hpp"
#include "triad_value.hpp"
     int main(int argc, char** argv) {
         if (argc < 2) {
             std::cerr << "Usage: triad <mode> [options] <input.triad> [output.chunk]\n";
             return 1;
         }
         std::cout << "Triad Compiler v0.9.1\n";
         std::string mode = argv[1];
         std::string target = argc > 2 ? argv[2] : "";
         std::string outFile;
         bool verbose = false, showAst = false, showBytecode = false, traceVm
             = false;
         for (int i = 3; i < argc; ++i) {
             std::string arg = argv[i];
             if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
             else if (arg == "--verbose") verbose = true;
             else if (arg == "--show-ast") showAst = true;
             else if (arg == "--show-bytecode") showBytecode = true;
             else if (arg == "--trace-vm") trace
                 Vm = true;
             else if (arg == "--version") {
                 std::cout << "Triad Compiler v0.9.1\n";
                 return 0;
             }
             else {
                 std::cerr << "Unknown argument: " << arg << "\n";
                 return 1;
             }
         }
         try {
             if (mode == "run-tests") {
                 run_tests(verbose);
                 return 0;
             }
             if (target.empty()) {
                 std::cerr << "Error: No input file specified.\n";
                 return 1;
             }
             std::string src = slurp(target);
             Chunk ch = parse_to_chunk(src);
             if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
             if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
             if (showAst) {
                 std::cout << "[AST dump not yet implemented]\n";
                 // ASTNode* ast = parse_to_ast(src); ast->dump();
             }
             if (mode == "run-vm") {
                 run_vm(ch, traceVm);
             }
             else if (mode == "run-ast") {
                 run_ast(src);
             }
             else if (mode == "emit-nasm") {
                 std::string asm_code = emit_nasm(ch); // stub
                 if (!outFile.empty()) emit_to_file(asm_code, outFile);
                 else std::cout << asm_code; // Print to stdout if no output file specified
             }
             else if (mode == "emit-llvm") {
                 std::string llvm_code = emit_llvm(ch); // stub
                 if (!outFile.empty()) emit_to_file(llvm_code, outFile);
                 else std::cout << llvm_code; // Print to stdout if no output file specified
             }
             else {
                 std::cerr << "Unknown mode: " << mode << "\n";
                 return 1;
             }
         }
         catch (const std::exception& e) {
             std::cerr << "Error: " << e.what() << "\n";
             return 1;
         }
         return 0;
     }



namespace triad {
    // Serialize a Chunk to a text stream (for debugging or simple persistence)
    inline void serialize_chunk(const Chunk& ch, std::ostream& os) {
        os << "CHUNK\n";
        os << "CODE " << ch.code.size() << "\n";
        for (const auto& instr : ch.code) {
             os << static_cast<int>(instr.op) << " " << instr.a << " " << instr.b << " " << instr.c << "\n";
        }
        os << "CONSTS " << ch.consts.size() << "\n";
         for (const auto& v : ch.consts) {
             if (v.tag == Value::Num) {
                 os << "N " << v.num << "\n";
             } else if (v.tag == Value::Str) {
                 os << "S " << v.str << "\n";
             } else {
                 os << "U\n"; // Unknown type
             }
        }
        os << "NAMES " << ch.names.size() << "\n";
         for (const auto& name : ch.names) {
             os << name << "\n";
        }
    }
    // Deserialize a Chunk from a text stream
    inline Chunk deserialize_chunk(std::istream& is) {
        Chunk ch;
        std::string section;
        size_t count;
        is >> section;
        if (section != "CHUNK") throw std::runtime_error("Expected CHUNK header");
        is >> section >> count;
        if (section != "CODE") throw std::runtime_error("Expected CODE section");
        for (size_t i = 0; i < count; ++i) {
            int op, a, b, c;
            is >> op >> a >> b >> c;
            ch.code.push_back(Instr{static_cast<Op>(op), a, b, c});
        }
        is >> section >> count;
        if (section != "CONSTS") throw std::runtime_error("Expected CONSTS section");
        is >> std::ws;
        for (size_t i = 0; i < count; ++i) {
            char type;
            is >> type;
            if (type == 'N') {
                double num;
                is >> num;
                ch.consts.push_back(Value::number(num));
            } else if (type == 'S') {
				std::string str
					is >> std::ws;
                std::getline(is, str);
                ch.consts.push_back(Value::string(str));
            } else {
                throw std::runtime_error("Unknown constant type: " + std::string(1, type));
            }
        }
        is >> section >> count;
        if (section != "NAMES") throw std::runtime_error("Expected NAMES section");
        ch.names.reserve(count);
        is >> std::ws; // Skip whitespace before names
        for (size_t i = 0; i < count; ++i) {
            std::string name;
            std::getline(is, name);
            ch.names.push_back(name);
        }
		return ch;
	}
#ifdef TRIAD_SERIALIZE_MAIN
    int main(int argc, char** argv) {
        if (argc < 2) {
            std::cerr << "Usage: triad_serialize <input.triad> [output.chunk]\n";
            return 1;
        }
        std::ifstream infile(argv[1]);
        if (!infile) {
            std::cerr << "Failed to open input file: " << argv[1] << "\n";
            return 1;
        }
        std::string src((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>());
        Lexer lexer(src);
        auto tokens = lexer.run();
        Parser parser(std::move(tokens));
        Chunk chunk = parser.parse();
        if (argc >= 3) {
            std::ofstream outfile(argv[2]);
            if (!outfile) {
                std::cerr << "Failed to open output file: " << argv[2] << "\n";
                return 1;
            }
            serialize_chunk(chunk, outfile);
        } else {
            serialize_chunk(chunk, std::cout);
        }
        return 0;
	}
#endif
    #ifdef TRIAD_DESERIALIZE_MAIN
    int main(int argc, char** argv) {
        if (argc < 2) {
            std::cerr << "Usage: triad_deserialize <input.chunk>\n";
            return 1;
        }
        std::ifstream infile(argv[1]);
        if (!infile) {
            std::cerr << "Failed to open input file: " << argv[1] << "\n";
            return 1;
        }
        Chunk chunk = deserialize_chunk(infile);
        chunk.dump();
        return 0;
	}
#endif
    struct Chunk {
        // Define the members of Chunk as needed
        // Example:
        // int id;
        // std::vector<char> data;
    };
	} // namespace triad
