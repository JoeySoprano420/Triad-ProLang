#include "triad_bytecode.hpp"
#include <iostream>
#include <unordered_map>
#include <variant>
#include <vector>
#include <string>
#include <stdexcept>

namespace triad {

  using VMValue = std::variant<double, std::string>;
  using Frame = std::unordered_map<std::string, VMValue>;

  class VM {
    std::vector<VMValue> stack_;
    std::vector<Frame> frames_;
    const Chunk* chunk_ = nullptr;
    bool trace_ = false;

  public:
    VM() noexcept = default;

    void enable_trace() noexcept { trace_ = true; }

    void exec(const Chunk& ch) {
      chunk_ = &ch;
      frames_.emplace_back(); // global frame

      size_t ip = 0;
      while (ip < ch.code().size()) {
        const Instr& instr = ch.code()[ip];
        if (trace_) dump_instr(ip, instr);

        switch (instr.op) {
          case Op::PUSH_CONST:
            stack_.push_back(ch.consts()[instr.a]);
            break;

          case Op::SAY:
            print_top();
            break;

          case Op::ADD:
            binary_op(std::plus<>{});
            break;

          case Op::SUB:
            binary_op(std::minus<>{});
            break;

          case Op::MUL:
            binary_op(std::multiplies<>{});
            break;

          case Op::DIV:
            binary_op(std::divides<>{});
            break;

          case Op::MOD:
            binary_op([](double a, double b) { return std::fmod(a, b); });
            break;

          case Op::EQ:
            compare_op(std::equal_to<>{});
            break;

          case Op::NE:
            compare_op(std::not_equal_to<>{});
            break;

          case Op::LT:
            compare_op(std::less<>{});
            break;

          case Op::LE:
            compare_op(std::less_equal<>{});
            break;

          case Op::GT:
            compare_op(std::greater<>{});
            break;

          case Op::GE:
            compare_op(std::greater_equal<>{});
            break;

          case Op::JMP:
            ip = instr.a;
            continue;

          case Op::IF_FALSE_JMP:
            if (!truthy(pop())) {
              ip = instr.a;
              continue;
            }
            break;

          case Op::RET:
            return;

          default:
            throw std::runtime_error("Unhandled opcode");
        }

        ++ip;
      }
    }

  private:
    [[nodiscard]] VMValue pop() {
      if (stack_.empty()) throw std::runtime_error("Stack underflow");
      VMValue val = std::move(stack_.back());
      stack_.pop_back();
      return val;
    }

    void print_top() {
      const VMValue& val = stack_.back();
      std::visit([](auto&& v) { std::cout << v << "\n"; }, val);
    }

    template<typename Op>
    void binary_op(Op&& op) {
      auto b = std::get<double>(pop());
      auto a = std::get<double>(pop());
      stack_.push_back(op(a, b));
    }

    template<typename Op>
    void compare_op(Op&& op) {
      auto b = std::get<double>(pop());
      auto a = std::get<double>(pop());
      stack_.push_back(op(a, b) ? 1.0 : 0.0);
    }

    [[nodiscard]] bool truthy(const VMValue& val) const {
      return std::visit([](auto&& v) -> bool {
        if constexpr (std::is_same_v<decltype(v), double>) return v != 0.0;
        if constexpr (std::is_same_v<decltype(v), std::string>) return !v.empty();
        return false;
      }, val);
    }

    void dump_instr(size_t ip, const Instr& instr) const {
      std::cout << "IP " << ip << ": "
                << static_cast<int>(instr.op) << " "
                << instr.a << " " << instr.b << " " << instr.c << "\n";
    }
  };

} // namespace triad
