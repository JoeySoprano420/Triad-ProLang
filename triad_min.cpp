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
