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

---

