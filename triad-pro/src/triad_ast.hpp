#include "triad_vm.cpp"
#include "triad_ast.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <string_view>
#include <variant>
#include <optional>

using namespace triad;
namespace fs = std::filesystem;
using std::string_view;

[[nodiscard]] static std::string slurp(const std::string& path) noexcept {
  std::ifstream f(path);
  if (!f) throw std::runtime_error("Cannot open file: " + path);
  return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

static void emit_to_file(const std::string& code, const std::string& outPath) {
  std::ofstream out(outPath);
  if (!out) throw std::runtime_error("Cannot write to: " + outPath);
  out << code;
}

static void run_vm(const Chunk& ch, bool trace = false) {
  VM vm;
  if (trace) vm.enable_trace(); // hypothetical hook
  vm.exec(ch);
}

static void run_ast(const std::string& src) {
  std::cout << "[AST interpreter not yet implemented]\n";
}

static void run_tests(bool verbose = false) {
  for (const auto& entry : fs::directory_iterator("tests")) {
    if (entry.path().extension() == ".triad") {
      std::cout << "Running: " << entry.path().filename() << "\n";
      std::string src = slurp(entry.path().string());
      Chunk ch = parse_to_chunk(src);
      if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
      run_vm(ch);
    }
  }
}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cout << "Triad Compiler CLI\n"
              << "Usage: triadc <mode> <file.triad | dir> [options]\n"
              << "Modes:\n"
              << "  run-vm       Execute via VM\n"
              << "  run-ast      Execute via AST interpreter\n"
              << "  emit-nasm    Emit NASM assembly\n"
              << "  emit-llvm    Emit LLVM IR\n"
              << "  run-tests    Execute all .triad files in /tests\n"
              << "Options:\n"
              << "  -o <file>    Output to file\n"
              << "  --verbose    Show debug info\n"
              << "  --show-ast   Print AST\n"
              << "  --show-bytecode Print bytecode\n"
              << "  --trace-vm   Trace VM execution\n"
              << "  --version    Show version\n";
    return 0;
  }

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
  }

  try {
    if (mode == "run-tests") {
      run_tests(verbose);
      return 0;
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
    } else if (mode == "run-ast") {
      run_ast(src);
    } else if (mode == "emit-nasm") {
      std::string asm_code = emit_nasm(ch); // stub
      if (!outFile.empty()) emit_to_file(asm_code, outFile);
      else std::cout << asm_code;
    } else if (mode == "emit-llvm") {
      std::string llvm_code = emit_llvm(ch); // stub
      if (!outFile.empty()) emit_to_file(llvm_code, outFile);
      else std::cout << llvm_code;
    } else {
      std::cerr << "Unknown mode: " << mode << "\n";
      return 1;
    }
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
  return 0;
}


// --- Additional: Minimal AST node structure and pretty-print utility ---

namespace triad {

// Basic AST node kinds
enum class ASTKind {
  Program,
  Number,
  String,
  Identifier,
  BinaryOp,
  UnaryOp,
  Call,
  Assignment,
  Block,
  If,
  For,
  Say,
  Echo,
  Unknown
};

// Minimal AST node structure
struct ASTNode {
  ASTKind kind = ASTKind::Unknown;
  std::string value; // For identifiers, literals, or operator
  std::vector<std::unique_ptr<ASTNode>> children;

  ASTNode(ASTKind k, std::string v = {}) : kind(k), value(std::move(v)) {}

  // Add a child node
  void add_child(std::unique_ptr<ASTNode> child) {
    children.push_back(std::move(child));
  }

  // Pretty-print the AST recursively
  void dump(std::ostream& os = std::cout, int indent = 0) const {
    for (int i = 0; i < indent; ++i) os << "  ";
    os << ast_kind_name(kind);
    if (!value.empty()) os << " (" << value << ")";
    os << "\n";
    for (const auto& child : children) {
      if (child) child->dump(os, indent + 1);
    }
  }

  static const char* ast_kind_name(ASTKind k) {
    switch (k) {
      case ASTKind::Program: return "Program";
      case ASTKind::Number: return "Number";
      case ASTKind::String: return "String";
      case ASTKind::Identifier: return "Identifier";
      case ASTKind::BinaryOp: return "BinaryOp";
      case ASTKind::UnaryOp: return "UnaryOp";
      case ASTKind::Call: return "Call";
      case ASTKind::Assignment: return "Assignment";
      case ASTKind::Block: return "Block";
      case ASTKind::If: return "If";
      case ASTKind::For: return "For";
      case ASTKind::Say: return "Say";
      case ASTKind::Echo: return "Echo";
      default: return "Unknown";
    }
  }
};

// --- Additional: AST traversal and search utilities ---

namespace triad {

// Visitor pattern for AST traversal
template <typename Visitor>
void traverse_ast(const ASTNode& node, Visitor&& visitor) {
  visitor(node);
  for (const auto& child : node.children) {
    if (child) traverse_ast(*child, visitor);
  }
}

// Find all nodes of a given kind in the AST
inline void find_nodes_by_kind(const ASTNode& node, ASTKind kind, std::vector<const ASTNode*>& out) {
  if (node.kind == kind) out.push_back(&node);
  for (const auto& child : node.children) {
    if (child) find_nodes_by_kind(*child, kind, out);
  }
}

// Utility: Count total nodes in the AST
inline size_t count_ast_nodes(const ASTNode& node) {
  size_t count = 1;
  for (const auto& child : node.children) {
    if (child) count += count_ast_nodes(*child);
  }
  return count;
}

} // namespace triad

#ifdef TRIAD_AST_DUMP_MAIN
#include <memory>

int main() {
  using namespace triad;

  // Example: Build and print a simple AST for: say 1 + 2
  auto root = std::make_unique<ASTNode>(ASTKind::Program);
  auto say = std::make_unique<ASTNode>(ASTKind::Say);
  auto add = std::make_unique<ASTNode>(ASTKind::BinaryOp, "+");
  add->add_child(std::make_unique<ASTNode>(ASTKind::Number, "1"));
  add->add_child(std::make_unique<ASTNode>(ASTKind::Number, "2"));
  say->add_child(std::move(add));
  root->add_child(std::move(say));

  root->dump();

  return 0;
}
#endif

#ifdef TRIAD_AST_UTILS_MAIN
#include <memory>
#include <vector>

int main() {
  using namespace triad;

  // Build a simple AST: say (1 + 2)
  auto root = std::make_unique<ASTNode>(ASTKind::Program);
  auto say = std::make_unique<ASTNode>(ASTKind::Say);
  auto add = std::make_unique<ASTNode>(ASTKind::BinaryOp, "+");
  add->add_child(std::make_unique<ASTNode>(ASTKind::Number, "1"));
  add->add_child(std::make_unique<ASTNode>(ASTKind::Number, "2"));
  say->add_child(std::move(add));
  root->add_child(std::move(say));

  // Traverse and print all node kinds
  std::cout << "AST node kinds:\n";
  traverse_ast(*root, [](const ASTNode& n) {
    std::cout << "  " << ASTNode::ast_kind_name(n.kind) << "\n";
  });

  // Find all Number nodes
  std::vector<const ASTNode*> numbers;
  find_nodes_by_kind(*root, ASTKind::Number, numbers);
  std::cout << "Number nodes found: " << numbers.size() << "\n";

  // Count total nodes
  std::cout << "Total AST nodes: " << count_ast_nodes(*root) << "\n";

  return 0;
}
#endif



// --- Capsule-aware AST mutation, Symbolic tracing, Optimization passes, Serialization/deserialization ---

namespace triad {

// Capsule-aware AST mutation: apply a mutator to all nodes, optionally skipping subtrees ("capsules")
template <typename Mutator>
void mutate_ast(ASTNode& node, Mutator&& mutator, bool capsule = false) {
  if (capsule && (node.kind == ASTKind::Call || node.kind == ASTKind::Block)) {
    // Treat Call/Block as a capsule: do not mutate children
    mutator(node);
    return;
  }
  mutator(node);
  for (auto& child : node.children) {
    if (child) mutate_ast(*child, mutator, capsule);
  }
}

// Symbolic tracing: collect a trace of node kinds and values as a string
inline void symbolic_trace(const ASTNode& node, std::ostream& os, int indent = 0) {
  for (int i = 0; i < indent; ++i) os << "  ";
  os << ASTNode::ast_kind_name(node.kind);
  if (!node.value.empty()) os << " (" << node.value << ")";
  os << "\n";
  for (const auto& child : node.children) {
    if (child) symbolic_trace(*child, os, indent + 1);
  }
}

// Optimization pass: constant folding for simple binary ops (e.g., Number + Number)
inline bool fold_constants(ASTNode& node) {
  bool changed = false;
  for (auto& child : node.children) {
    if (child) changed |= fold_constants(*child);
  }
  if (node.kind == ASTKind::BinaryOp && node.children.size() == 2) {
    const ASTNode* lhs = node.children[0].get();
    const ASTNode* rhs = node.children[1].get();
    if (lhs && rhs && lhs->kind == ASTKind::Number && rhs->kind == ASTKind::Number) {
      double a = std::stod(lhs->value);
      double b = std::stod(rhs->value);
      double result = 0.0;
      if (node.value == "+") result = a + b;
      else if (node.value == "-") result = a - b;
      else if (node.value == "*") result = a * b;
      else if (node.value == "/") result = b != 0.0 ? a / b : 0.0;
      else return changed;
      node.kind = ASTKind::Number;
      node.value = std::to_string(result);
      node.children.clear();
      changed = true;
    }
  }
  return changed;
}

// Serialization: write AST to a stream in a simple text format
inline void serialize_ast(const ASTNode& node, std::ostream& os, int indent = 0) {
  for (int i = 0; i < indent; ++i) os << "  ";
  os << ASTNode::ast_kind_name(node.kind);
  if (!node.value.empty()) os << " " << std::quoted(node.value);
  os << " " << node.children.size() << "\n";
  for (const auto& child : node.children) {
    if (child) serialize_ast(*child, os, indent + 1);
  }
}

// Deserialization: read AST from a stream (must match the format above)
inline std::unique_ptr<ASTNode> deserialize_ast(std::istream& is, int indent = 0) {
  std::string kind_str;
  if (!(is >> std::ws)) return nullptr;
  for (int i = 0; i < indent; ++i) is.ignore(2); // skip two spaces per indent
  if (!(is >> kind_str)) return nullptr;
  ASTKind kind = ASTKind::Unknown;
  for (int k = 0; k <= static_cast<int>(ASTKind::Unknown); ++k) {
    if (kind_str == ASTNode::ast_kind_name(static_cast<ASTKind>(k))) {
      kind = static_cast<ASTKind>(k);
      break;
    }
  }
  std::string value;
  if (is.peek() == ' ') is.get();
  if (is.peek() == '"') is >> std::quoted(value);
  size_t nchildren = 0;
  is >> nchildren;
  auto node = std::make_unique<ASTNode>(kind, value);
  for (size_t i = 0; i < nchildren; ++i) {
    node->add_child(deserialize_ast(is, indent + 1));
  }
  return node;
}

} // namespace triad

#ifdef TRIAD_AST_ADVANCED_MAIN
#include <sstream>
#include <iomanip>

int main() {
  using namespace triad;

  // Build a simple AST: say (1 + 2)
  auto root = std::make_unique<ASTNode>(ASTKind::Program);
  auto say = std::make_unique<ASTNode>(ASTKind::Say);
  auto add = std::make_unique<ASTNode>(ASTKind::BinaryOp, "+");
  add->add_child(std::make_unique<ASTNode>(ASTKind::Number, "1"));
  add->add_child(std::make_unique<ASTNode>(ASTKind::Number, "2"));
  say->add_child(std::move(add));
  root->add_child(std::move(say));

  // Capsule-aware mutation: uppercase all Number values, but skip inside Call/Block
  mutate_ast(*root, [](ASTNode& n) {
    if (n.kind == ASTKind::Number) {
      for (auto& c : n.value) c = std::toupper(c);
    }
  }, true);

  // Symbolic trace
  std::cout << "Symbolic trace:\n";
  symbolic_trace(*root, std::cout);

  // Optimization: constant folding
  if (fold_constants(*root)) {
    std::cout << "After constant folding:\n";
    root->dump();
  }

  // Serialize AST
  std::ostringstream oss;
  serialize_ast(*root, oss);
  std::cout << "Serialized AST:\n" << oss.str();

  // Deserialize AST
  std::istringstream iss(oss.str());
  auto loaded = deserialize_ast(iss);
  std::cout << "Deserialized AST:\n";
  if (loaded) loaded->dump();

  return 0;
}
#endif

