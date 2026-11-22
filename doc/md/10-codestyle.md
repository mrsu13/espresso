# Coding Style Guidelines

## Language Standards

The codebase uses the language mode ESP-IDF compiles every component with: **GNU C++26** (`-std=gnu++26`) for C++ and GNU C23 (`-std=gnu23`) for C.  The project does not override it: ESP-IDF's own headers rely on GNU extensions, and one standard for the toolchain and the project avoids surprises.  GNU extensions themselves are used only where the platform needs them — the ESP-IDF attribute macros, optimisation pragmas and direct register access of drivers (see *Embedded specifics*); everything else is written in standard C++.

Library features of C++23 and later that GCC provides are used where they fit; the main one is `std::expected<T, E>` for fallible operations (see *Error Handling*).

Language features in active use:

  * `static_assert`, `consteval`, `constexpr` functions, concepts, designated initializers, three-way comparison (`<=>`), structured bindings, and `std::span` / `std::string_view` are encouraged.
  * Variable-length arrays are forbidden.


## Line Ends

All source code files should use Unix line ends.


## Indentation

Use spaces, not tabs. Tabs should only appear in files that require them for semantic meaning, like `Makefiles`. The indent size is 4 spaces.

Right

```c
int main()
{
    return 0;
}
```

Wrong

```c
int main()
{
  return 0;
}
```


A case label should be indented from its switch statement. The case statement is indented, too.
Add one empty line between cases.

**Each case body sits on its own indented line(s) below the case label.** Never put statements on the same line as the label (`case 'i': do_thing(); break;`) -- even when the body is a single expression.  The one-statement-per-line shape makes every case look the same regardless of body length, keeps `break` aligned with sibling cases, and makes diffs touch a single line per change.

Right

```c
switch (condition)
{
    case foo_condition:
    case bar_condition:
        i++;
        break;

    case 'i':
        image_id = parse_int(val, val_len);
        break;

    default:
        i--;
}
```

Wrong

```c
switch (condition)
{
case foo_condition:
case bar_condition:
    i++;
    break;
default:
    i--;
}
```

```c
switch (c)
{
    case 'i': image_id     = parse_int(val, val_len); break;  // body on same line
    case 'p': placement_id = parse_int(val, val_len); break;  // also column-aligned
}
```

Multi-clause boolean expressions in control statements (`if`, `while`, `for`, ...) **break across lines with each operator at the start of the continuation line**, indented four spaces past the opening `(`.  Apply this whenever a condition has more than one clause -- even when the single-line form would still fit under the column budget -- so the structure of the test reads at a glance.

Right

```c
if (attr->name() == src_attr
        || attr->name() == lowsrc_attr
        || (attr->name() == usemap_attr && attr->value().dom_string()[0] != '#'))
    return;

while (i < len
        && std::isdigit(static_cast<unsigned char>(s[i])))
    ++i;
```

Wrong

```c
if (attr->name() == src_attr ||
    attr->name() == lowsrc_attr)               // operator at end of line
    return;

while (i < len && std::isdigit(static_cast<unsigned char>(s[i])))   // multi-clause on one line
    ++i;
```

Wrong

```c
if (attr->name() == src_attr ||
    attr->name() == lowsrc_attr ||
    (attr->name() == usemap_attr && attr->value().dom_string()[0] != '#'))
    return;
```


## Spacing

Do not place spaces around unary operators.

Right

```c
i++;
```

Wrong

```c
i ++;
```

Do place spaces around binary and ternary operators.

Right

```c
y = m * x + b;
f(a, b);
c = a | b;
return condition ? 1 : 0;
```

Wrong

```c
y=m*x+b;
f(a,b);
c = a|b;
return condition ? 1:0;
```

Do not place spaces before comma and semicolon.

Right

```c
for (int i = 0; i < 10; ++i)
    do_something();

f(a, b);
```

Wrong

```c
for (int i = 0 ; i < 10 ; ++i)
    do_something();

f(a , b) ;
```

Place spaces between control statements and their parentheses.

Right

```c
if (condition)
    do_it();
```

Wrong

```c
if(condition)
    do_it();
```

Do not place spaces between a function and its parentheses, or between a parenthesis and its content.

Right

```c
f(a, b);
```

Wrong

```c
f (a, b);
f( a, b );
```


## Line Breaking

Each statement should get its own line.

Right

```c
x++;
y++;
if (condition)
    do_it();
```

Wrong

```c
    x++; y++;
    if (condition) do_it();
```

An `else` statement should go on the next line after a preceding close brace if one is present, else it should line up with the `if` statement.

Right

```c
if (condition)
{
    ...
}
else
{
    ...
}

if (condition)
    do_something();
else
    do_something_else();

if (condition)
    do_something();
else
{
    ...
}
```

Wrong

```c
if (condition)
{
    ...
} else {
    ...
}

if (condition) do_something(); else do_something_else();

if (condition) do_something(); else {
    ...
}
```

An `else if` statement should be written as an if statement when the prior if concludes with a return statement.

Right

```c
if (condition)
{
    ...
    return someValue;
}
if (condition)
{
    ...
}
```

Wrong

```c
if (condition)
{
    ...
    return someValue;
}
else if (condition)
{
    ...
}
```

When a function call's arguments are split across multiple lines, the arguments after the first line should be aligned with the column immediately after the open parenthesis -- not indented one level from the call. The same applies to nested calls: each open parenthesis establishes its own alignment column for its trailing arguments.

Right

```c
fprintf(fp, "%s %s%s %s:%d: ",
            timestamp,
            level_color,
            event->file,
            event->line);

snprintf(buffer, sizeof(buffer),
         "score=%d, lives=%d",
         score, lives);
```

Wrong

```c
fprintf(fp, "%s %s%s %s:%d: ",
        timestamp,
        level_color,
        event->file,
        event->line);

snprintf(buffer, sizeof(buffer),
    "score=%d, lives=%d",
    score, lives);
```


## Braces


Place each brace on its own line.

Right

```c
int main()
{
    ...
}

struct my_struct
{
    ...
};

for (int i = 0; i < 10; ++i)
{
    ...
}
```

Wrong

```c
int main() {
    ...
}

struct my_class {
    ...
};
```

One-line control clauses should not use braces unless comments are included or a single statement spans multiple lines.

Right

```c
if (condition)
    do_it();

if (condition)
{
    // Some comment
    do_it();
}

if (condition)
{
    my_function(really_long_param1, really_long_param2, ...
                really_long_param5);
}
```

Wrong

```c
if (condition)
{
    do_it();
}

if (condition)
    // Some comment
    do_it();

if (condition)
    my_function(really_long_param1, really_long_param2, ...
                really_long_param5);
```

Control clauses without a body should use empty braces.

Right

```c
for ( ; current; current = current->next) {}
```

Wrong

```c
for ( ; current; current = current->next);
```


## NULL, nullptr, false and 0

The null pointer value is `nullptr` (never `NULL`, never `0`). Boolean values are `true` and `false` from the language.

Tests for true/false, null/non-null, and zero/non-zero are done without equality comparisons.

Right

```cpp
if (condition)
    do_it();

if (!ptr)                // ptr can be raw, unique_ptr, shared_ptr — all support !
    return;

if (!count)
    return;

if (!opt)                // std::optional supports !
    return;
```

Wrong

```c
if (condition == true)
    do_it();

if (ptr == NULL)
    return;

if (count == 0)
    return;
```

```cpp
if (ptr == NULL)         // use nullptr
    return;

if (ptr == nullptr)      // still wrong — use !ptr
    return;

if (count == 0)          // use !count
    return;
```


## File Names

**Header extension is `.h`.** Source extension is `.cpp`.

**File names include the namespace prefix.** A class `co::js::value` lives in `co_js_value.h` / `co_js_value.cpp`, not in `value.h` / `js_value.h` / `value.cpp`. A class `co::vt::parser` lives in `co_vt_parser.h`. This means **every project source / header file in the codebase starts with `co_`** since every toolkit entity is in the `co` namespace (or a nested namespace within it); a host app's own layer carries its own prefix and namespace the same way. The only exceptions are vendored 3rdparty headers under `3rdparty/`, the program entry point `src/main.cpp`, generated sources such as the font tables in `src/fonts`, and auto-generated build artefacts.

Right

```
src/core/script/js/co_js_value.h // class co::js::value
src/core/vt/parser/co_vt_parser.h // class co::vt::parser
```

Wrong

```
src/core/script/js/value.h // missing co_ prefix
src/core/vt/parser/parser.h // missing co_ prefix
```

**One class, enum, or struct per file.** If the type is `co::entity`, the public declaration goes in `co_entity.h`, the implementation in `co_entity.cpp`.  A module whose `.cpp` files share internal declarations (types, constants, lookup tables) also has a `co_entity_p.h` private header for them; it is included only by the module's own `.cpp` files, never from `co_entity.h`.  Even tiny helper types — a small `co::rect` value type, a single-purpose `co::orientation` enum — each live in their own paired `co_<name>.h` / `co_<name>.cpp` (or `.h`-only when header-only).  When two types are tightly coupled (for example, a rectangle type built on a point type), the consumer header `#include`s the dependency header.

The file name matches the primary class name. One class per file is the default; small tightly-coupled helper classes may share a file with their primary class.

**The `::p` namespace.** Every type and helper private to a module — internal state types, helper structs and functions, constants and lookup tables that only the module's own files use — lives in the owning namespace's `::p` sub-namespace: `co::p` for `co::entity`, `co::gui::p` for `co::gui::entity`, and so on.  The examples below use the plain-`co::` form; substitute the owning namespace for layered classes.  Public callers do not use `co::p::` names.

**Cross-namespace references are always fully qualified from the root.** Inside `namespace co { ... }`, write `co::p::entity_state` — not the shorter `p::entity_state` that namespace nesting would also accept. Inside `namespace co::p { ... }`, write `co::non_copyable` — not `non_copyable`. The goal is that any single line of code identifies the namespace of every type it touches without the reader having to find the enclosing `namespace` brace.

### Layout rules for namespaces, classes, and includes

These rules apply to every C++ header and source file in the codebase. The companion `.clang-format` at the repo root encodes them mechanically.

**No indentation inside a namespace.** A namespace block is purely a scope marker. Its contents sit flush-left, the same column as the namespace itself.

**One blank line after the opening `{` of a namespace; one blank line before the closing `}`.** The closing `}` stands alone — **no `// namespace <name>` trailing comment** (the brace is unambiguous and the comment drifts when files are edited). `.clang-format` enforces this with `FixNamespaceComments: false`.

**Nested namespaces use the C++17 syntax** `namespace co::terminal`.  Avoid stacking `namespace co { namespace terminal { ... } }` — one namespace open / close per file is the simplest reading of the "no indent" rule.

**Braces always sit on their own lines, even for tiny namespace blocks.**  The "Place each brace on its own line" rule from *Braces* applies uniformly: forward-declaration namespace blocks, `p`-namespace blocks wrapping a single data constant, and one-line `namespace ns { decl; }` shapes are all written in the expanded form:

```cpp
namespace co::p
{

struct vt_kitty;

}
```

never:

```cpp
namespace co::p { struct vt_kitty; }                 // braces inline -- forbidden
namespace co::p { struct vt_kitty; class vt_other; } // even worse: multiple decls
```

The blank-line-after-`{` / before-`}` rule applies here too -- the namespace body sits surrounded by blank lines regardless of how few lines it carries.  Gives the eye a consistent silhouette for every namespace in the project.

**Access modifiers** (`public:`, `protected:`, `private:`) are surrounded by **exactly one** blank line above and one blank line below. Not two; not zero. The first access modifier after a class's opening `{` also gets a single blank line above it (the blank line that already sits inside the namespace block per the "blank line after `{`" rule does not double up).

**No column-alignment spaces, anywhere.** Use a single space between a type and an identifier, and a single space on each side of `=`, `==`, member-initialiser `:`, designated-initialiser `=`, etc.  Never pad with extra spaces to visually align a column of declarations, assignments, designated initialisers, or `case`-label bodies.  Alignment drifts with the longest identifier each time the code changes; the diff churn is not worth the momentary readability win.

Right

```cpp
class value
{

public:

    inline value(JSContext *ctx, JSValue raw) noexcept;
    inline ~value() noexcept;
    inline JSValue get() const noexcept;
    inline void reset() noexcept;

private:

    JSContext *_ctx{nullptr};
    JSValue _val{JS_UNDEFINED};
};
```

Wrong

```cpp
class value
{

public:

    inline value(JSContext *ctx, JSValue raw) noexcept;
    inline ~value()                                noexcept;   // padded to align the `noexcept` column
    inline JSValue get() const                      noexcept;   // padded
    inline void     reset()                          noexcept;   // `void` padded to match `JSValue `

private:

    JSContext *_ctx{nullptr};
    JSValue    *_val{JS_UNDEFINED};                                   // `JSValue` padded to match `JSContext`
};
```

Column alignment is brittle — adding a longer-typed member or method forces a re-indent of every neighbour. The companion `.clang-format` disables `AlignConsecutive*` options to keep this enforced mechanically.

**Constructor initializer lists: one member per line, leading comma.** Each member-initializer occupies its own line. The first line starts with a colon (`:`); every subsequent line starts with a comma (leading-comma / "comma-first" style). Indent the colon and the commas by one level (4 spaces) past the function-definition column.

Right

```cpp
co::js::value::value(JSContext *ctx, JSValue raw) noexcept
    : _ctx{ctx}
    , _val{raw}
{
    if (_val)
        JS_DupValue(_ctx, _val);
}
```

Wrong

```cpp
co::js::value::value(JSContext *ctx, JSValue raw) noexcept
    : _ctx{ctx}, _val{raw}                    // multiple members on one line
{
}

co::js::value::value(JSContext *ctx, JSValue raw) noexcept :
    _ctx{ctx},                                // colon at end of signature
    _val{raw}                                       // trailing-comma style
{
}
```

Leading-comma + one-per-line makes diffs surgical (adding a member adds exactly one line; removing one removes exactly one line) and keeps the colon visible at the top of the list as a single anchor.

**Two blank lines after the last `#include` directive.** Single blank lines between include groups (own header → project headers → system headers → third-party). Two blank lines mark the transition from "includes block" to "actual code."

**Doxygen documentation lives in the `.cpp`, not the `.h`** when both files exist. Header declarations stay clean for at-a-glance overview; the implementation file carries the full prose. **Header-only types (no companion `.cpp`) keep their docs in the `.h`** since there is nowhere else to put them — `co::js::value` is an example.

**Definitions live OUTSIDE the `namespace co { ... }` block, using fully-qualified names.** Inside the namespace block: only declarations. After the namespace's closing `}`: definitions written as `co::class::method(...)` (or, for free functions, `co::free_function(...)`).

This rule applies to **all** definitions — inline methods in a header-only class, non-inline methods in a `.cpp`, free functions, static member-variable definitions, nested-class definitions. The class body in the header stays a pure interface; the definition surface is grouped together below it. Class declarations remain inside the namespace.

  * **Inline methods.** Declare inside the class body with the `inline` keyword. Define after the namespace's closing brace, fully qualified — **without repeating `inline`**. The keyword on the declaration is sufficient; repeating it on the definition adds noise without changing semantics.
  * **Non-inline methods.** Declare inside the class body with no modifier. Define in the companion `.cpp`, after that file's namespace-closing `}`, fully qualified.
  * **`= delete` / `= default`** stay in the class body — they are declaration-level annotations, not definitions.
  * **`constexpr` methods** carry an explicit `inline` on the declaration for consistency (constexpr is implicitly inline at function level, but the marker keeps the visual pattern uniform). `constexpr` itself **does** repeat on the definition — it's part of the function's semantics, not just inline status.
  * **Deduction guides** and other free declarations (not definitions) stay inside the namespace block.
  * **Template member functions** use the stacked `template<...>` syntax when defined outside the class.
  * **Doxygen comments live at the definition**, regardless of file. In a `.cpp`-paired class the docs sit with the `.cpp` definitions; in a header-only class the docs sit with the inline definitions in the `.h` (below the namespace block). The class body — the prototype-overview surface — stays free of Doxygen noise in both cases.
  * **The type's own `@brief` block goes to the `.cpp` too**, as a `@class co::foo` / `@struct co::p::bar` block above the first definition in the file. The class declaration in the `.h` stays Doxygen-free; the `@class`-prefixed block in the `.cpp` is the canonical type docstring. Header-only types still carry their `@brief` in the `.h` above the type declaration since there is no `.cpp` to host it. **File-level top-of-file comments** (`// co::js::value -- ...` describing the file / module) stay in the `.h` — they're not Doxygen and they describe the file, not the type.
  * **`override` method declarations** that don't add anything to the base's contract (no extra params, no special behaviour, no relaxed / strengthened invariants) carry **no Doxygen block at all** — Doxygen's `@copydoc` chain picks up the base's docs automatically. Add a fresh block (and a corresponding `.cpp`-side definition block) only when the override genuinely has something to say.
  * **Data-type Doxygen: enums, structs, classes, and their entries.** Every `enum class`, `struct`, and `class` declaration carries (in the `.cpp` as `@class`/`@struct`, or in the `.h` for header-only types) a `/** @brief ... */` block. **Every enum member, every struct field, and every class data member carries its own Doxygen block** above the entry **in the `.h`** — fields and enumerators only exist at the declaration so the block has nowhere else to live. Do not use trailing `// short note` comments on these — they get truncated by terminal width and don't show up in generated documentation.
  * **Doxygen comments are comprehensive, not telegraphic.** Each block explains what the entry is *for*, what invariant it carries, and how callers / readers interact with it — not just a restated name. "Cursor fill colour" is not enough; "Cursor fill colour painted over the cell underneath; alpha channel doubles as the overlay opacity (0xFF for an opaque block, lower for translucent overlays)" is the target. One sentence is usually too few; three is usually right; a paragraph is fine when the field is load-bearing.
  * **Doxygen commands use the `@`-prefix form, not `\`.** `@brief`, `@param`, `@return`, `@c`, `@p`, `@code` / `@endcode`, `@ref`, `@note`, `@file`, etc.  Both prefixes are accepted by Doxygen but the project picks one and uses it uniformly so a single grep finds every callsite.
  * **Emphasize identifiers and constants with `@c`.**  Any reference to a type, function, identifier, or bare constant in Doxygen prose gets `@c` so it renders in fixed-width font.  `@c true`, `@c false`, `@c nullptr`, `@c 0`, `@c on_print`, `@c std::size_t`, etc.  Function parameters specifically use `@p` (e.g. `@p obj`).  Plain English words stay un-`@c`-ed.
  * **Section markers between class body and definitions.**
    - **In a header-only `.h`**: a single `// Implementation` comment separates the namespace block (which ends with a bare `}`) from the definition surface below it. Without the marker the boundary between "declarations above" and "definitions below" reads as just an extra blank line.
    - **In a `.cpp`**: no `// Implementation` marker — the file itself is the implementation, the marker would be redundant. Definitions begin immediately after the includes.
    - In both file kinds, definitions are grouped by access control with a bare `// Public` / `// Protected` / `// Private` comment above each group. Only the access groups that actually have method definitions get a marker — a class with public-only methods uses one `// Public` marker, not three. No long `// ---------` rule lines; the bare access label is enough.

The reference layout, with all rules applied:

```cpp
// co_entity.h

#pragma once

#include <cstddef>


namespace co
{

class entity
{

public:

    entity();

    void set_count(std::size_t count);
    std::size_t count() const;

private:

    /** @brief Number of items the entity holds; set by set_count(), read by count(). */
    std::size_t _count;
};

}
```

```cpp
// co_entity.cpp

#include "co_entity.h"


// Public

/**
 * @brief Construct a default-state entity.
 */
co::entity::entity()
    : _count{0}
{
}

/**
 * @brief Replace the current count.
 *
 * @param count  New count value.  Negative values are not validated
 *               here; callers ensure their inputs are sensible.
 */
void co::entity::set_count(std::size_t count)
{
    _count = count;
}

/**
 * @brief The current count.
 */
std::size_t co::entity::count() const
{
    return _count;
}
```

Note in the `.h`:

  * `class entity { ... }` lives inside `namespace co { ... }`. No 4-space indent.
  * Blank line after `namespace co\n{` and before the bare closing `}`.
  * Blank line above `public:` and `private:` (including the first one).
  * Two blank lines between `#include <cstddef>` and `namespace co`.
  * No Doxygen comments on the methods — the header is a prototype-overview surface.  Data members are the exception: their blocks sit at the declaration, the only place they exist.

Note in the `.cpp`:

  * **No `namespace co { ... }` block.** Every definition uses the fully-qualified name `co::entity::method`.
  * Doxygen comments on every public-API definition.
  * Definitions separated by single blank lines; trivial defaulted special members are grouped without blank lines.

For a **header-only class** (no companion `.cpp`), the same rule applies — definitions go outside the namespace block, in the same `.h` file:

```cpp
// co_js_value.h (header-only)

#pragma once

#include <quickjs.h>


namespace co::js
{

class value
{

public:

    inline constexpr value() noexcept = default;
    inline value(JSContext *ctx, JSValue raw) noexcept;
    inline ~value() noexcept;

    inline value(value &&) noexcept;
    inline value & operator=(value &&) noexcept;

    value(const value &) = delete;
    value &operator=(const value &) = delete;

    inline JSValue get() const noexcept;
    inline explicit operator bool() const noexcept;

private:

    JSContext *_ctx{nullptr};
    JSValue _val{JS_UNDEFINED};
};

}


// Implementation

// Public

/**
 * @brief Adopt @p raw for the lifetime of this wrapper.
 *
 * Increments the refcount so the wrapper owns one reference; the
 * caller may drop their own reference without affecting the wrapped
 * object's lifetime.  Pass @c JS_UNDEFINED to construct empty.
 */
co::js::value::value(JSContext *ctx, JSValue raw) noexcept
    : _ctx{ctx}
    , _val{raw}
{
    if (_val)
        JS_DupValue(_ctx, _val);
}

/**
 * @brief Release the held reference (no-op when empty).
 */
co::js::value::~value() noexcept
{
    if (_val)
        JS_FreeValue(_ctx, _val);
}

// ...remaining definitions follow the same fully-qualified pattern.
```

Note the definitions in this header-only example:

  * **`// Implementation`** marks the boundary between the namespace block above and the definitions below.
  * **`// Public`** appears once, before the first public-method definition. If this class also had protected or private methods that were defined here, `// Protected` and `// Private` markers would appear above each respective group. Sections with no methods are omitted entirely.
  * **No `inline` keyword on the definitions** — the `inline` on the in-class declaration is sufficient.
  * **Doxygen comments on every definition**, even though we're in a `.h`. The class body stays clean; the documentation lives with the definitions below.

Free functions follow the same rule. The class body and any free-declaration block stay inside `namespace co { ... }`; their definitions go outside, fully qualified.


## Names

**Universal rule: snake_case for everything except macros and global constants.** This applies uniformly to classes, structs, enum types, enum members, functions, variables, and namespaces.

**Macros and global constants** use capital SNAKE_CASE — file-scope `const` / `constexpr` / `static const` tables, lookup arrays, and `#define`d values. This visually distinguishes them from mutable globals at the call site.

**Class / struct member variables** use a **leading underscore** prefix: `_member_name`. This avoids collisions with same-named getter functions and constructor parameters, and is unambiguous at every call site. The leading-underscore convention is safe in member context (the reserved-identifier rules apply only to file/global scope identifiers starting with underscore, and to all identifiers starting with `_<capital>` or `__`).

**Exception: public value-semantic data records (POD-like structs constructed via brace init at call sites) carry NO `_` prefix on their fields.** The underscore convention exists to disambiguate accessor methods from their backing storage and to mark "this lives on the impl, not the interface" — neither concern applies to a plain `co::color` or `co::script_error`. These types have no methods to collide with, and the absence of the prefix lets call sites read naturally: `co::script_error{co::script_status::error, "unexpected token"}` instead of the `_status=…, _message=…` look-alike. The rule of thumb: if the struct is data-only, lives in a public header, and is brace-initialised at call sites, fields stay bare. The `_` prefix returns the moment the type grows methods.

Right

```cpp
namespace co::terminal
{
    enum class color { red, green, blue };

    class scrollback
    {
    public:
        void push_line(std::string_view line);
        std::size_t line_count() const;

    private:
        std::vector<std::string> _lines;
        std::size_t _capacity{10000};
    };
}

static constexpr std::size_t CO_MAX_TABS = 16;
```

Wrong

```cpp
struct Data;                              // PascalCase struct
struct HTMLDocument;                      // PascalCase + multi-letter abbreviation
String MIMEType();                        // PascalCase function
namespace ArtTerminal { ... }             // PascalCase namespace
enum class Color { Red, Green, Blue };    // PascalCase enum + members

class scrollback
{
    std::vector<std::string> lines;       // bare member name — collides with potential getter
    size_t                   m_capacity;  // m_ prefix — wrong style for this codebase
    size_t                   capacity_;   // trailing-underscore style — wrong style for this codebase
};

static const char *co_log_level_names[] = { ... };  // mutable-name style for a constant
```

Use full words, except in the cases where an abbreviation would be more canonical and easier to understand.

Right

```c
size_t char_size;
size_t len;
short tab_index; // More canonical
```

Wrong

```c
size_t ch_size;
size_t l;
short tabulation_index; // bizarre
```

**The `_t` suffix is not used.** Type names are bare snake_case identifiers, with no suffix to distinguish typedefs from native types: the suffix adds typing without buying clarity at the call site.

Right

```cpp
class string { /* ... */ };

string s;
const string &str_ref;
```

Wrong

```cpp
using string_t = string;    // _t suffix

string_t s;
```

Precede boolean values with words like "is" and "did".

Right

```c
bool is_valid;
bool did_send_data;
```

Wrong

```c
bool valid;
bool sent_data;
```

Precede setters with the word "set". Use bare words for getters. Setter and getter names should match the names of the variables being set/gotten.

Right

```c
void set_count(size_t count);
size_t count();
```

Wrong

```c
void set_count(size_t count);
size_t get_count();
```

Precede getters that return values through out arguments with the word "get".

Right

```c
void get_inline_box_and_offset(inline_box *box, int *caret_offset);
```

Wrong

```c
void inline_box_and_offset(inline_box *box, int *caret_offset);
```

Do not use descriptive verbs in function names.

Right

```c
bool to_ascii(short *a, size_t b);
```

Wrong

```c
bool convert_to_ascii(short *a, size_t b);
```

Never omit variable names in function declarations.

Right

```c
void set_count(size_t count);

void do_something(script_execution_context_t *context);
```

Wrong

```c
void set_count(size_t);

void do_something(script_execution_context_t *);
```

Prefer enums to bools on function parameters if callers are likely to be passing constants, since named constants are easier to read at the call site. An exception to this rule is a setter function, where the name of the function already makes clear what the boolean is.

Right

```c
do_something(something, ALLOW_FOO_BAR);
paint_text_with_shadows(context, ..., text_stroke_width > 0, is_horizontal());
set_resizable(false);
```

Wrong

```c
do_something(something, false);
set_resizable(NOT_RESIZABLE);
```

`#defined` constants should use all uppercase names with words separated by underscores.

Macros that expand to function calls or other non-constant computation: these should be named like functions, and should have parentheses at the end, even if they take no arguments (with the exception of some special macros). Note that usually it is preferable to use an inline function in such cases instead of a macro.

Right

```c
#define WB_STOP_BUTTON_TITLE() \
        localized_string("Stop", "Stop button title")
```

Wrong

```c
#define wb_stop_button_title \
        localized_string("Stop", "Stop button title")

#define WBStopButtontitle \
        localized_string("Stop", "Stop button title")
```

`#pragma once` should be used instead of `#define` -- `#ifdef` "header guards".

**Directives nested in a conditional block are indented** by three spaces between the `#` and the keyword per nesting level, so the structure of a platform or feature branch reads at a glance; the conditional that opens a top-level block and its own `#else` / `#endif` stay in column 0, and the C++ code inside the block keeps its normal indentation -- the rule is about directives only:

```cpp
#ifdef CO_AI_ENABLED
#   include <ggml-backend.h>
#   ifdef CO_OS_WINDOWS
#       include <windows.h>
#   else
#       include <dlfcn.h>
#   endif
#endif
```

Right

```c
#pragma once
```

Wrong

```c
#ifndef __HTML_DOCUMENT_H__
#define __HTML_DOCUMENT_H__
...
#endif
```


## Pointers and References

**Pointer alignment.** Pointer types are written with a space between the type and the `*`, so the `*` is adjacent to the following identifier. The same alignment rule applies to references (`&` adjacent to the identifier).

Right

```cpp
image *do_something(svg_styled_element &element, const paint_info &info)
{
    const auto &dashes = element.dash_array();
    image *img = nullptr;
    ...
}
```

Wrong

```cpp
image* do_something(svg_styled_element& element, const paint_info& info)
{
    const auto& dashes = element.dash_array();
    image* img = nullptr;
    ...
}
```

**Pointer vs reference vs smart pointer.** The choice expresses ownership and nullability at the call site:

| Want | Use |
|---|---|
| Sole ownership | `std::unique_ptr<T>` |
| Shared ownership (only when truly shared) | `std::shared_ptr<T>` |
| Non-owning observation, never null | `T &` (reference) |
| Non-owning observation, may be null | `T *` (raw pointer) |
| Break a cycle in shared ownership | `std::weak_ptr<T>` |

Raw `T *` is **never** used for ownership in C++ code. `new` and `delete` are never used directly — always `std::make_unique<T>(...)` / `std::make_shared<T>(...)`. The only exception is interop with C APIs (quickjs-ng, SDL3, Steamworks): wrap those in RAII handles at the boundary; the rest of the codebase sees the wrapper, not the raw resource.

Right

```cpp
class atlas_registry
{
public:
    atlas *find(std::string_view name);                       // observation, may miss
    atlas &get(std::string_view name);                         // observation, asserts presence
    std::unique_ptr<atlas> take(std::string_view name);        // transfer ownership out

private:
    std::unordered_map<std::string, std::unique_ptr<atlas>> _atlases;
};
```

Wrong

```cpp
class atlas_registry
{
public:
    atlas *find(std::string_view name);
    atlas *get(std::string_view name);                         // unclear if owning or not
    atlas *take(std::string_view name);                        // raw pointer transfer — leak-prone

private:
    std::unordered_map<std::string, atlas *> _atlases;          // raw owning pointers — forbidden
};
```


## #include Statements

All implementation files must #include the primary header first. So for example, `node.cpp` should include `node.h` first, before other files. This guarantees that each header's completeness is tested. This also assures that each header can be compiled without requiring any other header files be included first.

Other `#include` statements should be in sorted order (case sensitive). Don't bother to organize them in a logical order.

Right

```c
// html_div_element.cpp
#include "html_div_element.h"

#include "attribute.h"
#include "html_element.h"
#include "qualified_name.h"
```

Wrong

```c
// html_div_element.cpp
#include "html_element.h"
#include "html_div_element.h"
#include "qualified_name.h"
#include "attribute.h"
```

**Framework headers use angle brackets.**  The four framework layers (`core/`, `gfx/`, `gui/`, `ai/` — everything `co_`-prefixed, including generated `co_*.rc.h` bundles) are included with `<...>` from EVERY consumer, host and toolkit alike: they are libraries, and angle brackets both signal the boundary and avoid same-directory shadowing.  Two things keep quotes: a module's OWN family (`co_foo.cpp` includes `"co_foo.h"` and `"co_foo_p.h"` — preserving the primary-header-first completeness test) and everything carrying the host's own prefix (the host's layer is the application, not a detachable library).

```cpp
// my_gui_example_page.cpp
#include "my_gui_example_page.h"

#include "my_gui_example_page_p.h"

#include "my_app.h"
#include "my_config.h"

#include <co_gui_tools.h>
#include <co_trace.h>

#include <string>
```

Includes of system headers must come after includes of other headers.

Right

```c
// connection_qt.cpp
#include "argument_encoder.h"
#include "process_launcher.h"
#include "web_page_proxy_message_kinds.h"
#include "work_item.h"

#include <assert.h>
```

Wrong

```c
// connection_qt.cpp
#include "argument_encoder.h"
#include "process_launcher.h"
#include <assert.h>
#include "web_page_proxy_message_kinds.h"
#include "work_item.h"
```


## Types

You can omit `int` when using `unsigned` modifier. Do not use `signed` modifier. Use `int` by itself instead.

Right

```c
unsigned a;
int b;
```

Wrong

```c
unsigned int a; // Doesn't omit "int".
signed b; // Uses "signed" instead of "int".
signed int c; // Doesn't omit "signed".
```


## Strings

Prefer `std::string` for **set-rarely / read-often member fields** — names, paths, prompt text, pre-baked SGR fragments, theme strings.  The setter assigns once (or a handful of times in the object's lifetime); the cost of the heap allocation is invisible against the work the read paths do.

Keep fixed-size `char[N]` (sized through a named constant in the owning layer's config header — see the auto-memory note on buffer sizes) when *any* of these holds:

  * **Per-frame / per-byte hot paths.**  Immediate-mode drawing payloads, VT-parser OSC / APC buffers, syntax-tokenizer scratch.
  * **Ring buffers.**  The line editor's history — fixed-cap arrays of fixed-cap entries, indexed in place.
  * **Many in flight at once.**  Records kept by the dozen in a per-query snapshot, where a `std::string` per slot would 4× the allocation count.
  * **Protocol caps.**  Anything dictated by a wire format (VT escape buffer, OSC byte limit) where the cap is a real upper bound and not a hopeful guess.
  * **Interrupt handlers and code they call.**  They never allocate (see *Embedded specifics*).

In doubt, default to `std::string` outside those paths — the policy split is conservative.  Reach for `std::string_view` for non-owning function arguments (`set_*(std::string_view name)`), and for fixed buffers always declare the cap via a constant in the owning layer's config header, never a magic literal at the call site.


## Comments

Use only one space before end of line comments and in between sentences in comments.

Right

```c
f(a, b); // This explains why the function call was done. This is another sentence.
```

Wrong

```c
int i;    // This is a comment with several spaces before it, which is a non-conforming style.
double f; // This is another comment.  There are two spaces before this sentence which is a non-conforming style.
```

Make comments look like sentences by starting with a capital letter and ending with a period (punctation). One exception may be end of line comments like this `if (x == y) // false for NaN`.

Use `@bug` to denote items that need to be addressed in the future.

Right

```c
draw_jpg(); ///< @bug Make this code handle jpg in addition to the png support.
```

Wrong

```c
draw_jpg(); // FIXME: Make this code handle jpg in addition to the png support.

draw_jpg(); // TODO: Make this code handle jpg in addition to the png support.
```

## ++ and --

Use prefixed `++` and `--` operators in stand-alone statements.

Right

```c
++i;
--j;
```

Wrong

```c
i++;
j--;
```


# C++ specifics

Where the rules in this section extend or override the language-agnostic rules above, they take precedence.

## C++ language subset

A constrained subset of C++ is used. The constraints exist to keep the binary small, the compile times short, the sandbox tractable, and the codebase legible.

**Platform and compiler splits go through `src/core/tools/co_platform.h`.**  Never test `_WIN32` / `__GNUC__` / `_MSC_VER` directly at a use site, and never write `__attribute__` bare — the codebase compiles under clang, GCC, and MSVC:

  * OS branches use the four canonical macros `CO_OS_WINDOWS` / `CO_OS_LINUX` / `CO_OS_MACOS` / `CO_OS_WASM` (exactly one defined per build).
  * GCC/Clang function attributes use the wrappers `CO_ATTR_FORMAT_PRINTF(fmt, first)` / `CO_ATTR_FORMAT_SCANF(fmt, first)` / `CO_ATTR_NONNULL(...)`; they expand to nothing on MSVC.
  * The pretty enclosing-function name for diagnostics is `CO_FUNCTION` (`__PRETTY_FUNCTION__` on GCC/Clang, `__FUNCSIG__` on MSVC).
  * Every file that branches on an `CO_OS_*` macro includes `<co_platform.h>` itself — relying on a transitive include invites the silently-dead-`#ifdef` bug (the macro reads as undefined and the guard picks the wrong arm without any diagnostic).
  * A constant, field or helper that only one platform's code reads lives under that platform's conditional -- `WASM_FS_SYNC_INTERVAL_MS` sits inside `#ifdef CO_OS_WASM` in `co_config.h`, the browser-only members of a struct inside the same guard in its `_p.h` -- never unconditionally, so the other platforms' builds carry no dead names.
  * **ESP-IDF code** uses the ESP-IDF attribute macros (`IRAM_ATTR`, `DRAM_ATTR`, `FORCE_INLINE_ATTR`, ...) and per-file `#pragma GCC optimize` directly where the code needs them; `__attribute__` itself is still never written bare.  See *Embedded specifics*.
  * **Virtual paths stringify in generic form.**  `corc:` URIs, asset names, and anything else matched against or persisted alongside registry keys uses `.generic_string()`, never `.string()` — on Windows `path::operator/` produces backslashes while the registered keys carry `/`, and a native-string compare silently matches nothing.  Native `.string()` is correct only for real on-disk paths shown to the user or handed to the OS.  The *encoding* axis is handled separately: the host's Windows application manifest sets the process ANSI codepage to UTF-8 on Windows 10 1903+, so narrow path strings are UTF-8 on every platform — no `u8string()` gymnastics at call sites.

**Forbidden:**

  * **Exceptions.** Compiled with `-fno-exceptions`. Error returns use `std::expected<T, E>` — see *Error Handling*. Rationale: binary size, control-flow opacity, ABI cost of unwind tables, and parity with the no-exceptions posture of the embedded C runtimes the project links against (quickjs-ng, SDL3, Steamworks).
  * **RTTI** (`dynamic_cast`, `typeid`). Compiled with `-fno-rtti` (`/GR-` on MSVC).  Where type identity is genuinely needed, use the codebase's RTTI-free patterns: the per-`T` sentinel keys in `co_meta_type_key.h` / `co_js_type_tag.h`, or an explicit discriminator enum (`co::meta::invokable_type`).
  * **`std::regex`.** Notoriously slow and binary-heavy. Use a small focused matcher where regex matters.
  * **Heavy template metaprogramming.** Concepts and simple templates are encouraged; SFINAE forests, recursive template instantiation chains, and Boost.Hana-class techniques are not.
  * **Boost outside the network code.**  The network classes use Boost.Asio, Beast and MQTT5 (and URL where a URL is parsed) — headers only, inside `.cpp` files and `_p.h` records, never in a class header, every fallible call through its error-code overload ([40-design.md](40-design.md) §3).  Everything else of Boost stays out: the toolkit has its own signal, scope guard, UUID and JSON.
  * **Bare `new` / `delete`.** Objects are static, or created with `std::make_unique<T>(...)` / `std::make_shared<T>(...)`.  Memory with ESP-IDF capability flags comes from `heap_caps_malloc` and is owned by a `std::unique_ptr` whose deleter calls `heap_caps_free` — see *Memory* in *Embedded specifics*.
  * **`using namespace`** and **namespace aliases** (`namespace fs = std::filesystem;`).  Identifiers are spelled out fully at every use site so a reader can grep any name verbatim.  Type aliases (`using foo_fn = int (*)(int);`) are allowed and **preferred over `typedef`** — they read left-to-right and compose with templates uniformly.  `typedef` is permitted but should be avoided in new code.  Type-alias names follow the same naming rules as other types (no `_t` suffix).
  * **Variable-length arrays.**
  * **Coroutines** (for v1 / v2). Out of scope; revisit when a concrete subsystem benefits.  Asynchronous network code is written with completion handlers bound to the object that owns the operation.
  * **C++ modules** (`import ...`). Tooling support is uneven; revisit later.
  * **`std::shared_ptr` where `std::unique_ptr` would do.** Reach for `shared_ptr` only when ownership is genuinely shared at runtime.

**Encouraged:**

  * **RAII** for every owned resource: GPU buffers, file handles, JSValue refcounts, allocations.
  * `std::vector`, `std::array`, `std::string`, `std::string_view`, `std::span`, `std::unordered_map`, `std::optional`, `std::variant`.
  * `std::unique_ptr<T>` for sole ownership.
  * **`constexpr`** for compile-time tables and pure functions.
  * **`consteval`** for functions that must run at compile time.
  * **Concepts (C++20)** to constrain templates — replaces SFINAE.
  * **Three-way comparison (`<=>`)** for value types that need ordering.
  * **Designated initializers** (C++20 form).
  * **Range-based `for`** loops.
  * **Structured bindings** for tuple / struct destructuring.
  * **Move semantics** with `noexcept` move constructors and assignment operators.
  * `[[nodiscard]]` on functions where ignoring the return value is a bug.
  * `enum class` — plain `enum` is not used in new code.
  * **`std::`-qualified fixed-width and size types in C++.** Always write `std::size_t`, `std::uint8_t`, `std::uint32_t`, `std::int64_t`, `std::ptrdiff_t`, etc. — never the C-style globals `size_t` / `uint8_t` / `int64_t` / `ptrdiff_t` (which are technically only guaranteed to exist with that bare name from `<stddef.h>` / `<stdint.h>`, not from their C++ wrappers `<cstddef>` / `<cstdint>`). Headers therefore use `#include <cstddef>` + `<cstdint>` (not their C counterparts) and every reference goes through `std::`. Applies even when the type is unambiguous in context — the rule is universal so call sites read uniformly.

## Classes

### Class layout

Order inside a class declaration: public special members → public methods → public data (only in PODs) → protected → private methods → private data. Within a section, group related members.

```cpp
class scrollback
{
public:
    // Special members
    scrollback();
    ~scrollback();
    scrollback(const scrollback &) = delete;
    scrollback & operator=(const scrollback &) = delete;
    scrollback(scrollback &&) noexcept;
    scrollback & operator=(scrollback &&) noexcept;

    // Methods
    void                  push_line(std::string_view line);
    std::string_view      line(std::size_t index) const;
    std::size_t           line_count() const;

private:
    std::vector<std::string> _lines;
    std::size_t _capacity = 10000;
};
```

### Special members (rule of five)

Every class with non-trivial resources declares all five special members explicitly — either `= default`, `= delete`, or user-defined.

  * **Defaultable types** (small value types like `rect`, `vec2`, `color`) get `= default` for all five.
  * **Non-copyable / non-movable resources** inherit from `co::non_copyable` (defined in `src/core/tools/co_non_copyable.h`).  The base privately deletes both copy operations and both move operations; deriving from it deletes the 4 special members on the derived class with one short `: co::non_copyable` instead of four `= delete` lines spread across the class body.  Most owning classes (GPU-resource holders, every `co::vt::*` class, `co::gui::tab_bar`, `co::gui::title_bar` etc.) are non-copyable in v1.  When a class genuinely needs movable semantics (a future container element, say) it does *not* inherit `co::non_copyable` and instead declares `= default` for the move special members directly.
  * **Move operations are `noexcept`.** This is mandatory for the class to be usable inside `std::vector` without falling back to slower copies.

### Methods belong with their data

  * **Free functions that always take a `T *` first argument should be members of `T`.** A function like `static void extract_token(token_ctx *t, ...)` is a hidden constructor / mutator on `token_ctx`; declare it as `token_ctx::extract(...)` so the call site reads `ctx.extract(...)` instead of `extract_token(&ctx, ...)`.  This applies equally to TU-local helper structs (`token_ctx`, `visit_ctx`, parser state records, etc.) and to the module-private structs in `co::p::`: any function whose first parameter is a `T *` belongs as a method of `T`.
  * **Free functions with no `T *` argument stay as free functions** — pure value transformations (`std::string parse_color(...)`, `bool is_keyword(const char *)`) don't gain anything from being methods.

### Member access

  * Member variables are `private` unless the class is an explicit POD (small value types: `rect`, `color`, `vec2`).
  * Public access goes through methods (`set_count` / `count`, per the existing naming rules).
  * Friend declarations are rare and require a comment explaining why.

### `override` and `final`

  * `override` is required on every method that overrides a virtual.
  * `final` is used on classes or methods that should not be subclassed / further overridden.
  * `virtual` is required only on the declaration that introduces the virtual (the base-class declaration).

### `explicit`

Single-argument constructors are `explicit` unless implicit conversion is genuinely the design intent.

## Error handling

### `std::expected<T, E>`

Fallible operations that return a payload on success return `std::expected<T, E>` (C++23, available in ESP-IDF's GNU C++26 mode).  `std::unexpected<E>` constructs the error state when returning an error.  Where a function only passes on the result of ESP-IDF calls, `esp_err_t` is fine as the error type `E`.

Callers check via the explicit-bool conversion and read the payload through `operator*`.

Conventions:

  * Per-module error enum, defined in the same header as the function that returns it.  Don't share a global `co::error_code` — each module's failure modes are specific to its domain.
  * Provide a companion `<module>_error_str(E)` returning a stable `const char *` for log lines and user-facing toast messages.  Saves callers from sprinkling switch statements when they just want a label.
  * All `std::expected`-returning functions are `[[nodiscard]]`.
  * Internal logging (parse-error pointer locations, errno, etc.) still happens inside the function via `CO_TRACE`; the returned error enum is the programmatic discriminator, the log is the human-readable detail.

### Three readers: `CO_TRACE`, `CO_NOTIFY`, `CO_TROUBLE`

One facility (`src/core/trace/co_trace.h`), three macros, one per reader.  The severity argument names the gravity in every case; the macro names who reads the line.

  * **`CO_TRACE`** talks to us.  The console log and nothing else: our diagnostics, internal invariants, anything the caller already reports through its own channel, anything that repeats per frame or per request, engine failures with no user context.  English technical wording and `--` are fine.
  * **`CO_NOTIFY`** talks to the person at the keyboard, through the notification widgets.  Use it when all three hold: the reader is the person, not us; nothing else on screen says it (no dialog, no terminal line, no status on the page they are looking at); and it is about their world — what they asked for did not happen (a save, an open, a copy, a snapshot), something they set up will not run as set up (the renderer, the AI's device or model, a port), or the app decided for them (a fallback).  `info` is "done, in case you wondered" for things that finish out of sight, `warning` is "runs, but not as you set it up", `error` is "what you asked for failed"; never `debug`, `trace` or `fatal` through it.  Once per cause, never per frame.
  * **`CO_TROUBLE`** talks to whoever wrote the running script, about their own code or assets — "your callback threw, so the agent saw nothing", "this image does not load, nothing draws", "a network call in a build without the network".  The host prints these into its terminal beside the script's own output (its trouble log: queued on any thread, printed on the frame thread, each distinct text once per script run), never as a popup, since an agent can emit one every turn.  A failure the script already receives as an exception or a return value is not a trouble line.

A notice or a trouble line is user-visible text: `CO_TR` in both languages, the spaced em dash, a sentence that says what happened and what it means for the reader; a path or an error string only when the reader can act on it.

`CO_TR` resolves where it is written, against the language the app set at start-up.  A string born in a static initialiser runs before that -- an `CO_STRUCT_FIELD`'s title -- and takes `CO_TR_LAZY` instead: an `co::translated` (the texts and their source position) the reader resolves with `co::translator::translate` when the words are wanted, as `a_struct_field::title()` and `description()` do.

Mark a function `noexcept` when the function truly does not throw and the marking is observable (move constructors, move assignment, destructors, swap functions). Don't sprinkle `noexcept` on everything — the compiler already knows we have `-fno-exceptions` globally, but the annotation is meaningful for `std::vector` growth, conditional `noexcept` specifications, and reader clarity.

### Assertions

`assert()` (from `<cassert>`) for invariants that should hold by construction. In release builds these compile out; in debug builds they catch logic bugs early. For security-critical invariants that must hold even in release, use a custom `co_assert` macro that aborts the process.

## Initialization

Use uniform brace initialization for everything **except** fundamental types, which use the simpler `=` form for readability.  **Fundamental types** for this rule means `bool`, integer types (including `char`, `std::size_t`, `std::uint32_t`, etc.), floating-point types, and all pointer types — anything that is not a class / struct / array / union.

Right

```cpp
int x = 0;                                      // fundamental — use =
double dt = 0.0;
bool is_open = false;
const char *name = nullptr;
std::size_t count = 0;                          // size_t is fundamental — use =
std::uint32_t flags = 0;
co::vt_screen *screen = nullptr;               // pointers are fundamental — use =

std::vector<int> v{1, 2, 3};                    // non-fundamental — use {}
std::string path{"hello"};
std::unique_ptr<atlas> a{std::make_unique<atlas>("ship")};
co::rect r{.x = 0, .y = 0, .w = 100, .h = 100};
co::vt_callbacks cb{};                         // value-initialise an aggregate — use {}
```

Wrong

```cpp
int x;                              // uninitialized — never leave fundamentals undefined
int x{0};                           // brace init on a fundamental — use = for readability
std::vector<int> v = {1, 2, 3};     // = form on a non-fundamental — use {}
std::string path("hello");          // function-call form — use {}
```

The split is pragmatic: brace init protects against narrowing conversions and uninitialised state for compound types where it matters; `=` is shorter and reads more naturally for plain numbers and booleans.

## const-correctness

`const` is the default. Apply liberally:

  * **Local variables** that are not reassigned are `const`.
  * **Function parameters** that are non-trivially-copyable and not mutated are `const T &`.
  * **Member functions** that do not mutate observable state are `const` (`std::size_t count() const`).
  * **Pointers and references** to objects that are not mutated are `const`.

```cpp
class scrollback
{
public:
    void   push_line(std::string_view line);
    void   clear();

    std::string_view line(std::size_t index) const;
    std::size_t      line_count() const;
    bool             is_empty() const;
};

void render(const scrollback &sb, co::rect bounds)
{
    const auto count = sb.line_count();
    for (std::size_t i = 0; i < count; ++i)
        render_line(sb.line(i), bounds);
}
```

## auto policy

`auto` is used **only** when the explicit type is impossible to write or is verbose enough to harm readability. Everywhere else, the type is spelled out.

The justified exceptions are:

  * **Lambda types.** A lambda's closure type is anonymous and cannot be named, so the storage variable's type can only be deduced. `auto print_line = [](std::string_view s) { ... };`
  * **Long template-instantiated types** that read worse spelled out than deduced — typical examples are nested `std::chrono::time_point_cast<...>` results, `std::unordered_map<K, V, Hash>::iterator` lookups, the `std::pair<iterator, bool>` halves of a `map::emplace` structured binding, and SFINAE / trailing-`decltype` machinery in helper templates. "Long enough to harm readability" is a judgment call — when in doubt, spell the type.
  * **Local types** declared inside a function body (e.g. a `struct group_t` inside a lambda) that cannot be named at the outer call site.

Right

```cpp
auto print_line = [](std::string_view s) noexcept { /* ... */ };           // lambda — impossible to spell

auto [ins, _] = cache.emplace(key, value);                                 // std::pair<unordered_map<...>::iterator, bool> — long enough

std::vector<scrollback_line>::iterator it = lines.begin();                 // explicit type — easy to read

for (const std::string &line : scrollback.lines())                          // explicit element type
    print_line(line);
```

Wrong

```cpp
auto it = lines.begin();                                                   // type is writable — spell it out
auto count = sb.line_count();                                              // type is std::size_t — spell it out

for (const auto &line : scrollback.lines())                                // type is std::string — spell it out
    print_line(line);
```

This is stricter than the typical C++ codebase. The rationale is reader-clarity at the cost of typing: when a reader sees `std::vector<scrollback_line>::iterator` they know the type without having to navigate to the function definition. `auto` is reserved for the cases where the alternative is genuinely worse.

## Templates and concepts

Templates are useful for generic containers and small generic helpers. Constrain them with C++20 concepts rather than SFINAE.

**No space between `template` and the opening angle bracket.** Write `template<typename T>`, not `template <typename T>`. This matches the codebase's general rule of no space between a function (or function-like construct) and its parenthesis or angle-bracket parameter list (see *Spacing*: `f(a, b)`, not `f (a, b)`).

Right

```cpp
template<std::integral T>
T clamp_in_range(T value, T low, T high)
{
    if (value < low)  return low;
    if (value > high) return high;
    return value;
}

template<typename T, typename E>
class expected
{
    /* ... */
};
```

Wrong

```cpp
template <std::integral T>                                             // space before < — wrong
T clamp_in_range(T value, T low, T high) { /* ... */ }

template <typename T,
          typename = std::enable_if_t<std::is_integral_v<T>>>
T clamp_in_range(T value, T low, T high)                               // SFINAE — use concepts instead
{
    /* ... */
}
```

Concept definitions live in a `concepts.h` per subsystem (e.g., `src/core/script/concepts.h`).

Avoid:

  * Recursive template instantiation chains.
  * Variadic templates beyond simple forwarding.
  * `std::enable_if_t` — always replaceable by a concept.
  * `std::is_*_v` traits at call sites — express the requirement as a concept.

## Lambdas

  * **Capture explicitly.** `[x, &y]` only — `[=]` and `[&]` are forbidden.
  * **Spell out the parameter list** unless the lambda is a one-line `std::ranges`-style helper.
  * **Mark `noexcept`** when applicable.

Right

```cpp
auto by_priority = [&priorities](const tab &a, const tab &b) noexcept
{
    return priorities[a.id] < priorities[b.id];
};
std::sort(tabs.begin(), tabs.end(), by_priority);
```

Wrong

```cpp
auto by_priority = [=](auto a, auto b) { return /* ... */; };          // [=] + auto + no noexcept
```

## Move semantics

  * `std::move` to enable a move from an lvalue. Never move from a `const` value (silently copies).
  * Never use a moved-from object except to reassign or destroy it.
  * Return by value and let RVO / NRVO elide. Don't `std::move` a local on return — it can pessimize NRVO.

Right

```cpp
std::vector<std::string> build_lines()
{
    std::vector<std::string> lines;
    /* ... */
    return lines;                                                       // NRVO elides the copy
}

void push(std::vector<std::string> v)
{
    _lines = std::move(v);                                              // moves from v; v is empty after
}
```

Wrong

```cpp
std::vector<std::string> build_lines()
{
    std::vector<std::string> lines;
    /* ... */
    return std::move(lines);                                            // pessimizes NRVO
}
```

## Annotations

  * `[[nodiscard]]` on functions where the return value must be checked: anything returning `std::expected<T, E>`, factory functions returning `std::unique_ptr`, getters whose result is meaningful at the call site.
  * `noexcept` per the *Error Handling* rules above.
  * `constexpr` on functions and variables that can be evaluated at compile time.
  * `consteval` on functions that **must** be evaluated at compile time.
  * `explicit` on single-argument constructors.
  * `override` on every method that overrides a virtual.
  * `final` on sealed classes / methods.

## Namespaces

  * **Snake_case namespace names.** `namespace co::terminal::vt { ... }`.
  * **C++17 nested-namespace syntax** for multi-level declarations: `namespace co::terminal { ... }`, not the older triple-nested `namespace co { namespace terminal { ... }}`.
  * **No `using namespace`** at any scope — file, function, or block.
  * **No namespace aliases** (`namespace fs = std::filesystem;` is forbidden; spell out `std::filesystem` at every use site).
  * **No renaming aliases.** A type alias names a new type (`using foo_fn = int (*)(int);`, `using rect_i = co::rect<int>;` — allowed and preferred over `typedef`, see the language subset); an alias that only shortens an existing name (`using string_view = std::string_view;`) is forbidden — spell out `std::string_view` at every use site.
  * **No anonymous namespaces, period.**  Everything TU-local -- helper functions, mutable data, constants, lookup tables, local structs -- goes in the owning namespace's `::p` sub-namespace inside the `.cpp` (`co::p` for files whose primary namespace is plain `co::`, e.g. `main.cpp`; `co::gfx::p` / `co::gui::p` / `co::ai::p` / `co::js::p` / `co::vt::p` / ... for files rooted in that namespace), or in the paired `_p.h` when other internal TUs need it.  Call sites outside the `p` block use the full `co::gui::p::helper_name(...)` qualification; sibling blocks of the same `p` namespace may stay unqualified.  Rationale: a qualified name tells the reader AT THE USE SITE that the thing is TU-private and where it lives -- anonymous namespaces hide exactly that, and "private" gets a single greppable home.  One consequence to mind: unlike anonymous-namespace entities, `p`-namespace data has external linkage, so names must stay unique across the layer's TUs -- the `the_<thing>` naming style for singleton-ish TU data (`the_bot_registry`, `the_assets_base_path`) keeps collisions structurally unlikely.

`using` *declarations* (bringing a name into scope, `using std::swap;`) are allowed only in one tightly-scoped case: **inheriting constructors** in a derived class (`using base::base;`). Type-alias declarations follow the rule above.

## Tooling

  * **`clang-format`** config committed at repo root. Codifies brace placement, indentation, spacing, pointer alignment, line wrapping. CI lint blocks unformatted PRs.
  * **`clang-tidy`** config with selected checks: `bugprone-*`, `cppcoreguidelines-*` (selectively), `modernize-*`, `performance-*`, `readability-*` (selectively). Notable disables: `cppcoreguidelines-avoid-magic-numbers` (too noisy at this style of code), `readability-named-parameter` (we already require names in declarations), `readability-identifier-length` (conflicts unevenly with the full-words rule).
  * **`compile_commands.json`** generated by CMake for IDE and clang-tidy use.
  * **CI** runs clang-format check + clang-tidy check + the three-platform build matrix on every PR.


# Embedded specifics

The firmware runs on an ESP32 under ESP-IDF: two cores, about 300 KB of heap, small task stacks, and interrupt handlers that keep running while the flash cache is disabled.  Where the rules in this section conflict with the rules above, they take precedence.

## Memory

  * **Static first.**  Long-lived objects — drivers, the application, buffers that live as long as the program — are static, or are allocated once at start-up.  Allocating and freeing while the program runs fragments the heap, and the VGA frame buffer needs large contiguous blocks.
  * **Capability memory through `heap_caps_malloc`.**  Buffers that need ESP-IDF memory capabilities — DMA-capable, internal, 32-bit accessible, PSRAM — come from `heap_caps_malloc` with the matching `MALLOC_CAP_*` flags and are released with `heap_caps_free`.  The pointer is owned by a `std::unique_ptr` whose deleter calls `heap_caps_free`; it is never kept bare.
  * **No allocation in interrupt handlers, per-frame or per-key code.**  These paths use fixed-size buffers sized by named constants.  `std::string`, `std::vector` and other allocating containers are used only outside them, with capacity reserved up front where the size is known.
  * **`alloca` is forbidden**, like variable-length arrays, and large local arrays are avoided: task stacks are small (the FabGL drawing task has 1200 bytes) and an overflow corrupts memory silently.

## Interrupt handlers and timing-critical code

  * **ESP-IDF attribute macros and optimisation pragmas are allowed where the code needs them**: `IRAM_ATTR`, `DRAM_ATTR`, `FORCE_INLINE_ATTR`, `NOINLINE_ATTR`, `WORD_ALIGNED_ATTR` and the like, and a per-file `#pragma GCC optimize` for drivers whose speed matters even in a debug build.  `__attribute__` itself is never written bare: an attribute ESP-IDF has no macro for gets one in a project header.
  * **Everything an interrupt handler calls is in IRAM or force-inlined.**  A function in flash crashes the handler whenever the flash cache is disabled — during a flash write, for example.  Moving such a function out of a header into a `.cpp` file therefore needs `IRAM_ATTR` on it, and splitting a file must keep every function under the same optimisation pragma it had.
  * **Data an interrupt handler reads while the flash cache may be disabled is in RAM**: `DRAM_ATTR` on constants, which the compiler would otherwise place in flash.
  * **No logging, allocation or blocking calls in interrupt handlers.**  `ESP_LOGx`, `printf`, `malloc`, `new`, mutexes and FreeRTOS calls without the `FromISR` suffix are not used there; `ESP_DRAM_LOGx` serves rare diagnostics.
  * **Data shared with interrupt handlers is `volatile` or `std::atomic`**, and updates of several related fields happen inside `portENTER_CRITICAL` / `portEXIT_CRITICAL` (the `_ISR` variants in the handlers themselves).
