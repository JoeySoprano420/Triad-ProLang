# Triad Language — Complete Syntax Sheet (v0.9 draft)

Triad is a compact, execution-oriented language built around **capsules** (self-contained runnable units), **macros** (compile-time expansion), symbolic **registers**, and **introspective tracing**. It favors readable, imperative flow with optional energy/priority **modes** (e.g., `Fastest`, `Softest`, `Hardest`, `Brightest`, `Deepest`) that annotate intent, cost, or quality.

---

## 1) Lexical Structure

### 1.1 Whitespace & Newlines

* Significant for separating tokens; otherwise ignored.
* Newlines may terminate simple statements unless a trailing `\` continues the line.

### 1.2 Comments

* Line comments: `# …` or `// …`
* Block comments: `/* … */` (nesting allowed).

### 1.3 Identifiers

* Regex: `[A-Za-z_][A-Za-z0-9_]*`
* Case-sensitive.
* Examples: `AgentMain`, `sparkle`, `R1`, `Repeat`, `glow`.

### 1.4 Keywords (reserved)

```
macro end capsule let return if else loop from to jump say echo tone trace
mutate load Load struct class enum const import as using with try catch finally throw
true false null
```

> Notes
> • `struct/class/enum/try/catch/finally/throw` are reserved for future expansion.
> • Both `load` and `Load` are accepted; canonical form is `Load`.

### 1.5 Literals

* Integers: `0`, `42`, `-7`, `0xFF`, `0b1010`, `0o755`
* Floats: `3.14`, `-0.5`, `1e9`, `6.02e-23`
* Strings: `"..."` with escapes `\n \t \" \\ \uXXXX`
* Symbols/Emoji: inside strings (UTF-8).
* Booleans: `true`, `false`
* Null: `null`

### 1.6 Modes (adverbs)

Words like `Fastest`, `Softest`, `Hardest`, `Brightest`, `Deepest`, `Sharpest`, `Quietest`, etc. They’re symbolic **modifiers** that annotate operations. They are identifiers but **treated specially** in instruction positions.

---

## 2) Program Units

### 2.1 File

A file contains **imports**, **macro definitions**, and **capsules** (optionally types in later versions). The entry point is a capsule you run explicitly (Triad does not auto-select `main`).

### 2.2 Attributes

Square-bracket attribute lists annotate capsules/macros:

```triad
capsule AgentMain [introspective, mutable, sandboxed]:
    ...
end
```

Common attributes:

* `introspective` — enables `trace` and runtime state querying
* `mutable` — allows register/memory mutation
* `sandboxed` — restricts I/O to permitted ops
* `deterministic` — forbids non-deterministic sources (time, RNG)

Attributes are optional.

---

## 3) Macros

### 3.1 Definition

```triad
macro name(param1, param2, ...):
    # statements (compile-time expanded)
    return expr       # optional final value expansion
end
```

* Macros expand at compile time.
* `return` in a macro returns the expansion value (an expression or spliceable AST node).

### 3.2 Call

Like a function call within expressions/statements:

```triad
let x = sparkle(3)
```

---

## 4) Capsules

### 4.1 Definition

```triad
capsule Name [attr, ...]:
    # statements
end
```

A capsule is a runnable unit with its own local symbol table, registers, and label namespace.

### 4.2 Registers

* Canonical general registers: `R0..R15` (implementation-defined max; commonly 16).
* Registers hold integers or floats; strings must be in variables.
* Access: by name only (no pointers).

---

## 5) Statements

### 5.1 Variable Binding

```triad
let name = expression
let a, b, c = 1, 2, 3     # parallel binding
```

Variables are immutable by default; re-assign by `let` again in a new scope or use `mutate` for registers.

### 5.2 Load (Register Set)

```triad
Load R1 Fastest #3
Load R2 Softest  10
Load R3          0xFF
```

Forms:

* `Load <Reg> [Mode] <Literal>`
* `#n` form is a literal synonym (e.g., `#3` = `3`).

### 5.3 Mutate (Register Arithmetic)

```triad
mutate R1 Softest -1
mutate R3 Hardest +8
mutate R2 * 2          # mode optional
mutate R4 Quietest / 3
```

Forms:

* `mutate <Reg> [Mode] <delta>` where delta may be `+n`, `-n`, `*n`, `/n`.
* Division truncates toward zero for integers.

### 5.4 Output

```triad
say "✨"
echo "Shine level: " + shine
tone Brightest "C#5"
```

* `say <string-expr>` — console/UI output.
* `echo <string-expr>` — diagnostic/log output (respects introspection level).
* `tone [Mode] "NoteOctave"` — symbolic audio emit (implementation-defined).

### 5.5 Control Flow

#### 5.5.1 If

```triad
if condition:
    # then
else:
    # else
end
```

#### 5.5.2 Loop + Labels

```triad
loop Deepest Repeat:
    # body
    jump Repeat Hardest if R1 > 0
end
```

* Syntax sugar:

  * `loop Label:` == `loop Default Label:`
  * Use `jump Label` (unconditional) or `jump Label if <expr>` (conditional).
  * Modes adjust semantic hints (e.g., branch priority).

#### 5.5.3 Jump

```triad
jump Label
jump Label if R1 > 0
jump Label Hardest if (x < 10) and not done
```

Forms:

* `jump <Label>`
* `jump <Label> [Mode] if <boolean-expr>`

### 5.6 Return

Inside **macros** returns an expansion value:

```triad
return expr
```

Inside **capsules**, `return` is permitted for early termination:

```triad
return
return expr   # optional status code (implementation-defined)
```

### 5.7 Trace / Introspection

```triad
trace capsule        # dump capsule state
trace registers      # dump R0..Rn
trace vars           # dump variable table
trace all            # everything
```

Requires `introspective` attribute or debug build.

---

## 6) Expressions

### 6.1 Precedence (high → low)

1. Parentheses: `( … )`
2. Unary: `+ - not`
3. Multiplicative: `* / %`
4. Additive: `+ -`
5. Comparisons: `< <= > >=`
6. Equality: `== !=`
7. Logical AND: `and`
8. Logical OR: `or`

All binary operators are left-associative.

### 6.2 Operators

* Arithmetic: `+ - * / %`
* Unary: `-x`, `+x`, `not x`
* Comparison: `< <= > >= == !=`
* Logical: `and`, `or`, `not`
* String concatenation: `+`
* Grouping: `(expr)`

### 6.3 Types (lightweight)

* **Int**, **Float**, **Bool**, **String**, **Null**.
* Registers: numeric only (int or float).
* Implicit conversions:

  * int ⇄ float for arithmetic
  * string concat uses `toString` on RHS/LHS.

---

## 7) Labels

### 7.1 Definition

Declared inline with loop:

```triad
loop Deepest Start:
    ...
end
```

A label exists within the capsule’s label namespace.

### 7.2 Use

```triad
jump Start
jump Start if R1 > 0
```

---

## 8) Modes (Semantics)

Modes are **adverbs** that qualify operations. They don’t change correctness; they declare **intent** that engines may use for scheduling, energy shaping, or quality:

* `Fastest` — prioritize speed
* `Softest` — minimize energy/impact
* `Hardest` — force decisive/priority execution
* `Brightest` — prefer high-quality/clarity output
* `Deepest` — thoroughness/exhaustiveness

They can appear after the opcode or before its operand, e.g.:

```triad
Load R1 Fastest 3
jump Repeat Hardest if cond
tone Brightest "A4"
```

---

## 9) Imports (optional, forward-compatible)

```triad
import audio
import ui as display
```

* `import name [as alias]`
* The baseline Triad core has `say`, `echo`, `tone`, `trace` without imports; modules enrich capabilities.

---

## 10) Errors & Diagnostics

* **Lexical**: invalid character, unterminated string, bad escape.
* **Syntax**: unexpected token, missing `end`, misplaced `if/else`, duplicate label.
* **Semantic**: unknown identifier/register/label, type mismatch, forbidden op in non-`mutable` capsule, `trace` used without `introspective`.
* **Runtime**: division by zero, out-of-range tone, user throw (future).

`echo` and `trace` aid debugging; compilers should report line/column.

---

## 11) Standard Ops (Core)

| Op       | Signature / Form                          | Effect                      |      |       |                    |
| -------- | ----------------------------------------- | --------------------------- | ---- | ----- | ------------------ |
| `Load`   | `Load Rn [Mode] <literal>`                | Set register to value       |      |       |                    |
| `mutate` | `mutate Rn [Mode] (+/-/*//) <n>`          | Arithmetic mutate           |      |       |                    |
| `say`    | `say <string-expr>`                       | User output                 |      |       |                    |
| `echo`   | `echo <string-expr>`                      | Diagnostic output           |      |       |                    |
| `tone`   | `tone [Mode] "NoteOctave"`                | Emit tone                   |      |       |                    |
| `jump`   | `jump <Label> [Mode] [if <cond>]`         | Branch                      |      |       |                    |
| `trace`  | \`trace capsule                           | registers                   | vars | all\` | Introspection dump |
| `let`    | `let name (= expr)?` (with parallel form) | Bind variable               |      |       |                    |
| `if`     | `if cond: … else: … end`                  | Conditional                 |      |       |                    |
| `loop`   | `loop [Mode] Label: … end`                | Loop block                  |      |       |                    |
| `return` | `return [expr]`                           | Macro value or capsule exit |      |       |                    |

---

## 12) Grammar (EBNF)

```ebnf
program         := { import_stmt | macro_def | capsule_def } ;

import_stmt     := "import" IDENT [ "as" IDENT ] EOL ;

macro_def       := "macro" IDENT "(" [ param_list ] "):" EOL
                   block
                   "end" EOL ;

param_list      := IDENT { "," IDENT } ;

capsule_def     := "capsule" IDENT [ attr_list ] ":" EOL
                   block
                   "end" EOL ;

attr_list       := "[" IDENT { "," IDENT } "]" ;

block           := { statement } ;

statement       := var_decl
                 | load_stmt
                 | mutate_stmt
                 | say_stmt
                 | echo_stmt
                 | tone_stmt
                 | jump_stmt
                 | loop_stmt
                 | if_stmt
                 | trace_stmt
                 | return_stmt
                 | EOL ;

var_decl        := "let" ident_list "=" expr_list EOL
                 | "let" IDENT EOL ;

ident_list      := IDENT { "," IDENT } ;
expr_list       := expr { "," expr } ;

load_stmt       := ("Load"|"load") REG [ mode ] literal EOL ;
mutate_stmt     := "mutate" REG [ mode ] mutate_arg EOL ;
mutate_arg      := (("+"|"-"|"*"|"/") NUMBER) | NUMBER ;

say_stmt        := "say" expr EOL ;
echo_stmt       := "echo" expr EOL ;
tone_stmt       := "tone" [ mode ] STRING EOL ;

jump_stmt       := "jump" IDENT [ mode ] [ "if" expr ] EOL ;

loop_stmt       := "loop" [ mode ] IDENT ":" EOL
                   block
                   "end" EOL ;

if_stmt         := "if" expr ":" EOL
                   block
                   [ "else" ":"? EOL block ]
                   "end" EOL ;

trace_stmt      := "trace" ("capsule"|"registers"|"vars"|"all") EOL ;

return_stmt     := "return" [ expr ] EOL ;

mode            := IDENT ;            (* e.g., Fastest, Softest, Hardest ... *)
REG             := "R" DIGIT { DIGIT };
literal         := NUMBER | STRING | BOOL | NULL ;

expr            := or_expr ;
or_expr         := and_expr { "or" and_expr } ;
and_expr        := equality { "and" equality } ;
equality        := relational { ("=="|"!=") relational } ;
relational      := additive { ("<"|"<="|">"|">=") additive } ;
additive        := multiplicative { ("+"|"-") multiplicative } ;
multiplicative  := unary { ("*"|"/"|"%" ) unary } ;
unary           := [("+"|"-"|"not")] primary ;
primary         := NUMBER | STRING | BOOL | NULL | IDENT func_call_opt | "(" expr ")" ;

func_call_opt   := [ "(" [ expr_list ] ")" ] ;

BOOL            := "true" | "false" ;
NULL            := "null" ;
EOL             := newline | ";" ;
```

---

## 13) Worked Example (from your snippet)

```triad
macro sparkle(level):
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

  trace capsule
end
```

---

## 14) Conventions & Style

* Prefer `Load` over `load`.
* Use attributes sparingly; `introspective` when you need `trace`.
* Reserve modes for “why/quality” hints, not correctness.
* Label loops with meaningful names when using `jump`.

---

## 15) Minimal Capsule Template

```triad
capsule Hello [introspective]:
  say "Hello, Triad."
  trace registers
end
```

---

## 16) Common Errors (and fixes)

* **“unknown label”** → verify the label name after `loop … Label:`
* **“trace requires introspective”** → add `[introspective]` to the capsule
* **“string used in register op”** → keep strings in variables; registers are numeric
* **“missing end”** → each `macro`, `capsule`, `if`, and `loop` must close with `end`

---

## 17) Quick Cheat-Sheet

* **Vars:** `let x = 1`
* **Load:** `Load R1 Fastest 3`
* **Mutate:** `mutate R1 -1`
* **Loop:**

  ```
  loop Repeat:
    ...
    jump Repeat if cond
  end
  ```
* **If:**

  ```
  if a > b:
    ...
  else:
    ...
  end
  ```
* **I/O:** `say "text"`, `echo "dbg"`, `tone Brightest "A4"`
* **Trace:** `trace capsule|registers|vars|all`
* **Macro:**

  ```
  macro m(a): let b=a+1; return b; end
  ```
# Triad Language — Complete Syntax Sheet (v1.0, expanded)

Triad is an execution-oriented language built around **capsules** (runnable units), **macros** (compile-time expansion), symbolic **registers**, and **introspective tracing**. This edition fully specifies **struct/class/enum** user types and **exceptions** via **try/catch/finally/throw**.

---

## 0) Quick legend

* **Modes (adverbs):** optional intent hints like `Fastest`, `Softest`, `Hardest`, `Brightest`, `Deepest`. They never change correctness; engines may use them for scheduling/quality.
* **Capsule:** the unit you actually run.
* **Registers:** `R0..R15` numeric cells; strings/objects live in variables.

---

## 1) Lexical & Basics (unchanged highlights)

* **Comments:** `# …`, `// …`, or `/* … */` (nestable).
* **Identifiers:** `[A-Za-z_][A-Za-z0-9_]*` (case-sensitive).
* **Literals:** ints (`42`, `0xFF`), floats, strings (`"..."`), `true/false`, `null`.
* **Reserved words (now active):**

  ```
  macro end capsule let return if else loop jump from to say echo tone trace mutate load Load
  struct class enum new init this try catch finally throw
  true false null import as using with not and or
  ```

---

## 2) Types

### 2.1 Built-ins

`Int`, `Float`, `Bool`, `String`, `Null`.

### 2.2 Structs (value types)

* Immutable by default (fields set at construction).
* Copy by value; equality is **structural**.
* May include **methods** (which receive an implicit `this`).
* No inheritance; can implement behavior via methods and macros.

```triad
struct Point:
  let x: Int
  let y: Int

  # Method (value semantics; returns a new Point)
  func offset(dx: Int, dy: Int) -> Point:
    return Point { x: this.x + dx, y: this.y + dy }
  end
end

let p = Point { x: 3, y: 5 }
let q = p.offset(1, -2)  # => Point {4,3}
```

**Construction forms**

* **Literal/init-list:** `Point { x: 1, y: 2 }`
* **With `new`:** `new Point { x: 1, y: 2 }` (identical; `new` is stylistic)

### 2.3 Classes (reference types)

* Reference semantics; equality is **by identity** unless a `equals()` method is defined.
* Optional **constructor** named `init`.
* Methods receive `this`.
* No inheritance in v1.0 (simple, composable design); composition preferred.

```triad
class Counter:
  let value: Int  # internally mutable via methods (class allows interior mutation)

  func init(start: Int):
    this.value = start
  end

  func inc() -> Int:
    this.value = this.value + 1
    return this.value
  end

  func reset():
    this.value = 0
  end
end

let c = new Counter( start: 10 )  # constructor call using named args
echo "v=" + c.inc()               # 11
```

**Construction forms**

* **Constructor:** `new ClassName(arg: value, ...)`
* **Init-list (no `init` defined):** `new ClassName { field: value, ... }`

> **Mutability note:**
> Struct fields are set once at creation. Class fields may be reassigned **inside methods** of the same class. Outside callers mutate via methods (encapsulation). Triad does not expose `public/private` modifiers in v1.0; convention: prefix private fields/methods with `_`.

### 2.4 Enums (sum types)

* Algebraic variants; each case may carry payloads.
* Useful for **Result/Error** and domain states.

```triad
enum Result:
  Ok(value: Int)
  Err(kind: ErrorKind, message: String)
end

enum ErrorKind:
  NotFound
  Invalid
  IO(code: Int)
end

# Construction
let r1 = Result.Ok(42)
let r2 = Result.Err(ErrorKind.IO(5), "Disk")
```

**Matching (minimal)**

* Triad v1.0 uses `if/else` + predicate helpers; a `match` form may land later.
* Engines should provide `isCase(value, "Ok")` and field accessors:

  * `asOk(value).value`, `asErr(value).kind`, etc. (implementation-supplied helpers)

---

## 3) Macros (compile-time)

```triad
macro sparkle(level):
  let shine = level * 2
  echo "Shine level: " + shine
  return shine
end
```

* Expand inline at compile time.
* `return` yields an expression/AST splice.

---

## 4) Capsules (runtime)

```triad
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

  trace capsule
end
```

**Attributes**

* `introspective` enables `trace`.
* `mutable` allows register/memory mutation within the capsule.
* `sandboxed`, `deterministic` are available intent flags.

---

## 5) Statements (core recap)

* **Variables:** `let x = expr` (parallel: `let a,b = 1,2`).
* **Registers:** `Load R1 Fastest 3`, `mutate R1 -1`.
* **I/O:** `say`, `echo`, `tone`.
* **Flow:** `if/else`, `loop Label: … end`, `jump Label [if cond]`.
* **Trace:** `trace capsule|registers|vars|all`.
* **Return:** `return [expr]` (capsule = exit; macro = expansion value; function/method = value).

> New in this spec: **func** inside `struct/class` bodies for methods.
> (Standalone top-level `func` is reserved for v1.1; for now, methods live inside user types.)

---

## 6) Exceptions

Triad introduces **structured exceptions** with `try/catch/finally/throw`. Exceptions are objects (any value can be thrown; best practice is using **struct/enum** types).

### 6.1 Throwing

```triad
throw "boom"
throw Error.IO(code: 5, message: "Disk fail")
throw Result.Err(ErrorKind.Invalid, "Bad input")
```

* `throw expr` immediately unwinds to the nearest active `catch`; `finally` blocks still run.

### 6.2 Try/Catch/Finally

```triad
try:
  maybeRisky()
catch e:
  echo "Caught: " + e
finally:
  echo "cleanup"
end
```

* `catch` binds the thrown value to an identifier (`e` above).
* Multiple `catch` blocks are allowed; the first one whose **type guard** matches will handle it.
* `finally` always runs (whether an exception occurred or not).

#### 6.2.1 Typed catches

Triad supports **type guards** using `as` patterns:

```triad
try:
  openFile("data.bin")
catch e as ErrorKind.IO:
  echo "I/O code = " + e.code
catch e as ErrorKind.Invalid:
  echo "Invalid!"
catch e:  # fallback
  echo "Unknown error: " + e
end
```

* When using `as <Type>`, `e` is treated as that type (field access allowed).
* If no guard matches, the exception propagates upward.

#### 6.2.2 Rethrow

```triad
catch e:
  logError(e)
  throw e     # rethrow the same object
```

### 6.3 Exceptions & capsules

* Uncaught exceptions **terminate the capsule** with a non-zero status (implementation-defined code).
* `trace capsule` should include the **exception stack** when available and the capsule is `introspective`.

---

## 7) Functions & Methods

### 7.1 Methods inside types

```triad
struct Box:
  let w: Int
  let h: Int

  func area() -> Int:
    return this.w * this.h
  end
end
```

* Methods may be used in expressions: `let a = Box{w:3,h:4}.area()`.

### 7.2 Constructors

* `class`: define `func init(...)` with **named** parameters.
* `struct`: no `init`; construct with literal/init-list.

### 7.3 Throws annotation (advisory)

You may annotate a method/capsule with a `throws` attribute in its bracket list:

```triad
capsule ImportData [throws(ErrorKind.IO), introspective]:
  try:
    ingest()
  catch e as ErrorKind.IO:
    echo "Handled"
  end
end
```

This is **documentation/intent**; engines may enforce or simply warn.

---

## 8) Examples tying it together

### 8.1 Error model with enums

```triad
enum FileError:
  NotFound(path: String)
  Permission(path: String)
  IO(code: Int, message: String)
end

class FileReader:
  let _path: String

  func init(path: String):
    this._path = path
  end

  func readText() -> String:
    # pretend to do I/O; demonstrate throwing
    if this._path == "":
      throw FileError.NotFound("(empty)")
    end
    return "contents"
  end
end

capsule Demo [introspective]:
  let r = new FileReader(path: "")
  try:
    let text = r.readText()
    say text
  catch e as FileError.NotFound:
    echo "Missing: " + e.path
  catch e as FileError.IO:
    echo "I/O(" + e.code + "): " + e.message
  finally:
    echo "Done."
  end
  trace capsule
end
```

### 8.2 Value vs reference

```triad
struct Vec2:
  let x: Int
  let y: Int
  func add(o: Vec2) -> Vec2:
    return Vec2 { x: this.x + o.x, y: this.y + o.y }
  end
end

class Bag:
  let item: Int
  func bump():
    this.item = this.item + 1
  end
end

let v1 = Vec2 { x:1, y:2 }
let v2 = v1.add( Vec2 { x:2, y:3 } )   # v1 unchanged, v2 is new

let b = new Bag( item: 7 )
b.bump()                              # modifies same object
```

---

## 9) Formal grammar (EBNF, updated)

```ebnf
program         := { import_stmt | macro_def | type_def | capsule_def } ;

import_stmt     := "import" IDENT [ "as" IDENT ] EOL ;

type_def        := struct_def | class_def | enum_def ;

struct_def      := "struct" IDENT ":" EOL
                   { member | method } 
                   "end" EOL ;

class_def       := "class" IDENT ":" EOL
                   { member | ctor | method }
                   "end" EOL ;

enum_def        := "enum" IDENT ":" EOL
                   enum_case { enum_case }
                   "end" EOL ;

enum_case       := IDENT [ "(" enum_fields ")" ] EOL ;
enum_fields     := enum_field { "," enum_field } ;
enum_field      := IDENT [ ":" type_ref ] ;

member          := "let" IDENT ":" type_ref EOL ;

ctor            := "func" "init" "(" [ named_params ] ")" ":"? EOL
                   block
                   "end" EOL ;

method          := "func" IDENT "(" [ param_list ] ")" [ "->" type_ref ] ":"? EOL
                   block
                   "end" EOL ;

named_params    := named_param { "," named_param } ;
named_param     := IDENT ":" type_ref ;

param_list      := param { "," param } ;
param           := IDENT ":" type_ref ;

type_ref        := IDENT ;  (* simple names in v1.0 *)

capsule_def     := "capsule" IDENT [ attr_list ] ":" EOL
                   block
                   "end" EOL ;

attr_list       := "[" IDENT { "(" arg_list ")" } { "," IDENT [ "(" arg_list ")" ] } "]" ;
arg_list        := expr { "," expr } ;

block           := { statement } ;

statement       := var_decl
                 | load_stmt
                 | mutate_stmt
                 | say_stmt
                 | echo_stmt
                 | tone_stmt
                 | jump_stmt
                 | loop_stmt
                 | if_stmt
                 | trace_stmt
                 | return_stmt
                 | try_stmt
                 | throw_stmt
                 | expr_stmt
                 | EOL ;

expr_stmt       := expr EOL ;

var_decl        := "let" ident_list "=" expr_list EOL
                 | "let" IDENT [ ":" type_ref ] EOL ;

ident_list      := IDENT { "," IDENT } ;
expr_list       := expr { "," expr } ;

load_stmt       := ("Load"|"load") REG [ mode ] literal EOL ;
mutate_stmt     := "mutate" REG [ mode ] mutate_arg EOL ;
mutate_arg      := (("+"|"-"|"*"|"/") NUMBER) | NUMBER ;

say_stmt        := "say" expr EOL ;
echo_stmt       := "echo" expr EOL ;
tone_stmt       := "tone" [ mode ] STRING EOL ;

jump_stmt       := "jump" IDENT [ mode ] [ "if" expr ] EOL ;

loop_stmt       := "loop" [ mode ] IDENT ":" EOL
                   block
                   "end" EOL ;

if_stmt         := "if" expr ":" EOL
                   block
                   [ "else" ":"? EOL block ]
                   "end" EOL ;

trace_stmt      := "trace" ("capsule"|"registers"|"vars"|"all") EOL ;

try_stmt        := "try" ":" EOL
                   block
                   { catch_clause }
                   [ "finally" ":" EOL block ]
                   "end" EOL ;

catch_clause    := "catch" IDENT [ "as" type_ref ] ":"? EOL block ;

throw_stmt      := "throw" expr EOL ;

mode            := IDENT ;             (* e.g., Fastest, Softest, Hardest, ... *)
REG             := "R" DIGIT { DIGIT } ;
literal         := NUMBER | STRING | BOOL | NULL ;

expr            := or_expr ;
or_expr         := and_expr { "or" and_expr } ;
and_expr        := equality { "and" equality } ;
equality        := relational { ("=="|"!=") relational } ;
relational      := additive { ("<"|"<="|">"|">=") additive } ;
additive        := multiplicative { ("+"|"-") multiplicative } ;
multiplicative  := unary { ("*"|"/"|"%" ) unary } ;
unary           := [("+"|"-"|"not")] primary ;
primary         := literal
                 | IDENT func_or_access
                 | "new" IDENT ctor_args
                 | "(" expr ")" ;

func_or_access  := ( "(" [ call_args ] ")" | object_init | member_tail )* ;

member_tail     := "." IDENT ( "(" [ call_args ] ")" )? ;

ctor_args       := "(" [ named_call_args ] ")"
                 | object_init ;

object_init     := "{" init_field { "," init_field } "}" ;
init_field      := IDENT ":" expr ;

call_args       := expr { "," expr } ;
named_call_args := named_call_arg { "," named_call_arg } ;
named_call_arg  := IDENT ":" expr ;

BOOL            := "true" | "false" ;
NULL            := "null" ;
EOL             := newline | ";" ;
```

---

## 10) Diagnostics & rules of thumb

* **Unknown field/variant** → check struct/enum spelling.
* **Illegal assignment to struct field** → struct fields are immutable post-construction; return a new struct or use a class.
* **Uncaught exception** → add `try/catch` or let the capsule fail (non-zero status).
* **Trace without introspection** → add `[introspective]` to the capsule.

---

## 11) Micro-cheat sheet (new bits)

* **Struct:**
  `struct S: let a:Int; func f()->Int: ... end end`
* **Class:**
  `class C: let a:Int; func init(a:Int): ... end end; let c=new C(a:1)`
* **Enum:**
  `enum E: CaseA(x:Int) CaseB end; let e=E.CaseA(5)`
* **Try/Catch/Finally:**

  ```
  try: risky()
  catch e as E.CaseA: ...
  catch e: ...
  finally: cleanup()
  end
  ```
* **Throw:** `throw E.CaseB`

---

