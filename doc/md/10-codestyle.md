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
        ++i;
        break;

    case 'i':
        image_id = parse_int(val, val_len);
        break;

    default:
        --i;
}
```

Wrong

```c
switch (condition)
{
case foo_condition:
case bar_condition:
    ++i;
    break;
default:
    --i;
}
```

```c
switch (c)
{
    case 'i': image_id     = parse_int(val, val_len); break;  // body on same line
    case 'p': placement_id = parse_int(val, val_len); break;  // also column-aligned
}
```

Long boolean expressions in control statements (`if`, `while`, `for`, ...) **break before an operator**, so every continuation line starts with `||` or `&&`, and the continuation is indented four spaces past the start of the statement.  A condition that fits within the 100-column limit stays on one line.  `.clang-format` (`BreakBeforeBinaryOperators: NonAssignment`) joins and breaks conditions by exactly these rules, so the shape follows from the formatter, not from hand-made line breaks.

Right

```c
if (attr->name() == src_attr || attr->name() == lowsrc_attr
    || (attr->name() == usemap_attr && attr->value().dom_string()[0] != '#'))
    return;

while (i < len && std::isdigit(static_cast<unsigned char>(s[i])))
    ++i;
```

Wrong

```c
if (attr->name() == src_attr ||
    attr->name() == lowsrc_attr || // operator at end of line
    (attr->name() == usemap_attr && attr->value().dom_string()[0] != '#'))
    return;
```


## Spacing

Do not place spaces around unary operators.

Right

```c
++i;
```

Wrong

```c
++ i;
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
++x;
++y;
if (condition)
    do_it();
```

Wrong

```c
    ++x; ++y;
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
    return some_value;
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
    return some_value;
}
else if (condition)
{
    ...
}
```

When a function call's arguments are split across lines, the arguments after the first line are aligned with the column immediately after the open parenthesis -- not indented one level from the call.  Each line holds as many arguments as fit (`.clang-format` packs them); a nested call that is split aligns its own arguments after its own parenthesis.

Right

```cpp
std::snprintf(text, sizeof(text), "Sound:    %s; F1-F8 play notes, F9 changes the waveform",
              esp::p::WAVEFORMS[_waveform]);
```

Wrong

```cpp
std::snprintf(text, sizeof(text), "Sound:    %s; F1-F8 play notes, F9 changes the waveform",
    esp::p::WAVEFORMS[_waveform]); // indented one level instead of aligned

std::snprintf(text,
              sizeof(text), // one argument per line although more fit
              "Sound:    %s; F1-F8 play notes, F9 changes the waveform",
              esp::p::WAVEFORMS[_waveform]);
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

Control clauses without a body use an empty pair of braces, each brace on its own line like any other.

Right

```c
for (; current; current = current->next)
{
}
```

Wrong

```c
for (; current; current = current->next);
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

**File names include the namespace prefix.** A class `esp::gfx::rect` lives in `esp_gfx_rect.h` / `esp_gfx_rect.cpp`, not in `rect.h` / `gfx_rect.h` / `rect.cpp`. A class `esp::ps2::keyboard` lives in `esp_ps2_keyboard.h`. This means **every project source / header file starts with `esp_`**, since every entity of the firmware is in the `esp` namespace or a namespace nested in it. The only exceptions are the vendored third-party libraries under `src/3rdparty/`, the program entry point `src/main.cpp`, generated sources such as the font tables in `src/fonts`, and build artefacts.

Right

```
src/gfx/esp_gfx_rect.h // class esp::gfx::rect
src/ps2/esp_ps2_keyboard.h // class esp::ps2::keyboard
```

Wrong

```
src/gfx/rect.h // missing esp_gfx_ prefix
src/ps2/esp_keyboard.h // missing the ps2_ part of the prefix
```

**One class, enum, or struct per file.** If the type is `esp::sound::sound_generator`, the declaration goes in `esp_sound_sound_generator.h`, the implementation in `esp_sound_sound_generator.cpp`.  A module whose `.cpp` files share internal declarations (types, constants, lookup tables) also has a private `_p.h` header for them -- `esp_vga_utils_p.h`, `esp_gfx_bitmapped_display_controller_p.h`; it is included only by the module's own `.cpp` files, never from a public header.  Even tiny helper types — the value type `esp::gfx::point`, the enum `esp::vga::vga_scan_start` — each live in their own header (and `.cpp`, when they have one).  When two types are tightly coupled (for example, a rectangle type built on a point type), the consumer header `#include`s the dependency header.

The file name matches the primary class name. One class per file is the default; small tightly-coupled helper classes may share a file with their primary class.

Free functions and constants are grouped by purpose rather than one per file: a module's settings in `esp_<module>_config.h` (`esp_vga_config.h`), its helpers in `esp_<module>_utils.h` (`esp_gfx_utils.h`), the helpers shared by all modules in `src/tools/esp_tools.h`.

**The `::p` namespace.** Every type and helper private to a module — internal state types, helper structs and functions, constants and lookup tables that only the module's own files use — lives in the owning namespace's `::p` sub-namespace: `esp::p` for code in plain `esp`, `esp::gfx::p` for `esp::gfx`, and so on.  Public callers do not use `p::` names.

**Cross-namespace references are always fully qualified from the root.** Inside `namespace esp::vga { ... }`, write `esp::vga::p::apll_params` — not the shorter `p::apll_params` that namespace nesting would also accept. Inside `namespace esp::sound { ... }`, write `esp::non_copyable` — not `non_copyable`. The goal is that any single line of code identifies the namespace of every type it touches without the reader having to find the enclosing `namespace` brace.

**Licence notice.** Files derived from FabGL start with a three-line notice, kept through every later change:

```cpp
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.
```

Code taken from ESP-IDF keeps Espressif's own Apache-2.0 header and its own coding style (`esp_ps2_ulp_macro_ex.cpp`); only notes and documentation change there. The project's own files carry no notice; the repository's `LICENSE` covers them.

### Layout rules for namespaces, classes, and includes

These rules apply to every C++ header and source file in the codebase. The companion `.clang-format` at the repo root encodes them mechanically.

**No indentation inside a namespace.** A namespace block is purely a scope marker. Its contents sit flush-left, the same column as the namespace itself.

**One blank line after the opening `{` of a namespace; one blank line before the closing `}`.** The closing `}` stands alone — **no `// namespace <name>` trailing comment** (the brace is unambiguous and the comment drifts when files are edited). `.clang-format` enforces this with `FixNamespaceComments: false`.

**Nested namespaces use the C++17 syntax** `namespace esp::gfx`.  Avoid stacking `namespace esp { namespace gfx { ... } }` — one namespace open / close per file is the simplest reading of the "no indent" rule.

**Braces always sit on their own lines, even for tiny namespace blocks.**  The "Place each brace on its own line" rule from *Braces* applies uniformly: forward-declaration namespace blocks, `p`-namespace blocks wrapping a single data constant, and one-line `namespace ns { decl; }` shapes are all written in the expanded form:

```cpp
namespace esp::ps2
{

class keyboard;

}
```

never:

```cpp
namespace esp::ps2 { class keyboard; } // braces inline -- forbidden
namespace esp::ps2 { class keyboard; class mouse; } // even worse: multiple decls
```

The blank-line-after-`{` / before-`}` rule applies here too -- the namespace body sits surrounded by blank lines regardless of how few lines it carries.  Gives the eye a consistent silhouette for every namespace in the project.

**Access modifiers** (`public:`, `protected:`, `private:`) are surrounded by **exactly one** blank line above and one blank line below. Not two; not zero. The first access modifier after a class's opening `{` also gets a single blank line above it (the blank line that already sits inside the namespace block per the "blank line after `{`" rule does not double up).

**No column-alignment spaces, anywhere.** Use a single space between a type and an identifier, and a single space on each side of `=`, `==`, member-initialiser `:`, designated-initialiser `=`, etc.  Never pad with extra spaces to visually align a column of declarations, assignments, designated initialisers, or `case`-label bodies.  Alignment drifts with the longest identifier each time the code changes; the diff churn is not worth the momentary readability win.

Right

```cpp
class ps2_controller
{

public:

    ps2_controller() = delete;

    static bool data_available(int ps2_port);
    static int get_data(int ps2_port, int time_out_ms);
    static void send_data(std::uint8_t data, int ps2_port);

private:

    static esp::ps2::keyboard *_keyboard;
    static bool _port_enabled[2];
};
```

Wrong

```cpp
class ps2_controller
{

public:

    ps2_controller() = delete;

    static bool data_available(int ps2_port);
    static int  get_data(int ps2_port, int time_out_ms);  // `int` padded to match `bool`
    static void send_data(std::uint8_t data, int ps2_port);

private:

    static esp::ps2::keyboard *_keyboard;
    static bool                _port_enabled[2];          // padded to match the line above
};
```

Column alignment is brittle — adding a longer-typed member or method forces a re-indent of every neighbour. The companion `.clang-format` disables `AlignConsecutive*` options to keep this enforced mechanically.

The one exception is the formatter's own: a braced list that does not fit on one line is laid out in columns by `.clang-format` — the sine table of `esp::sound::sine_waveform_generator`, the keyboard layouts, a long constructor argument list in braces.  That layout is kept as the formatter writes it.

**Constructor initializer lists: one member per line, leading comma.** Each member-initializer occupies its own line. The first line starts with a colon (`:`); every subsequent line starts with a comma (leading-comma / "comma-first" style). Indent the colon and the commas by one level (4 spaces) past the function-definition column.

Each member is initialised with braces, fundamental types included (`_volume{100}`): `=` is not available in the list, and braces reject narrowing conversions.  The examples are shortened from the constructor of `esp::sound::sound_generator`.

Right

```cpp
esp::sound::sound_generator::sound_generator(int sample_rate, gpio_num_t gpio,
                                             sound_gen_method gen_method)
    : _channels{nullptr}
    , _volume{100}
    , _gpio{gpio}
{
    portMUX_INITIALIZE(&_lock);
}
```

Wrong

```cpp
esp::sound::sound_generator::sound_generator(int sample_rate, gpio_num_t gpio,
                                             sound_gen_method gen_method)
    : _channels{nullptr}, _volume{100}, _gpio{gpio} // several members on one line
{
}

esp::sound::sound_generator::sound_generator(int sample_rate, gpio_num_t gpio,
                                             sound_gen_method gen_method) :
    _channels{nullptr}, // colon at end of signature, trailing-comma style
    _volume(100) // parentheses instead of braces
{
}
```

Leading-comma + one-per-line makes diffs surgical (adding a member adds exactly one line; removing one removes exactly one line) and keeps the colon visible at the top of the list as a single anchor.

**Two blank lines after the last `#include` directive.** Single blank lines between include groups (own header → project headers → C++ standard headers → ESP-IDF headers; see *`#include` statements*). Two blank lines mark the transition from "includes block" to "actual code."

**Doxygen documentation lives in the `.cpp`, not the `.h`** when both files exist. Header declarations stay clean for at-a-glance overview; the implementation file carries the full prose. **Header-only types (no companion `.cpp`) keep their docs in the `.h`** since there is nowhere else to put them — `esp::ps2::ps2_device_lock` is an example.

**Definitions live OUTSIDE the namespace block, using fully-qualified names.** Inside `namespace esp::gfx { ... }`: only declarations. After the namespace's closing `}`: definitions written as `esp::gfx::canvas::clear(...)` (or, for free functions, `esp::gfx::clip_line(...)`).

This rule applies to **all** definitions — inline methods in a header-only class, non-inline methods in a `.cpp`, free functions, static member-variable definitions, nested-class definitions. The class body in the header stays a pure interface; the definition surface is grouped together below it. Class declarations remain inside the namespace.

  * **Inline methods.** Declare inside the class body with the `inline` keyword. Define after the namespace's closing brace, fully qualified — **without repeating `inline`**. The keyword on the declaration is sufficient; repeating it on the definition adds noise without changing semantics.
  * **Non-inline methods.** Declare inside the class body with no modifier. Define in the companion `.cpp`, after that file's namespace-closing `}`, fully qualified.
  * **`= delete` / `= default`** stay in the class body — they are declaration-level annotations, not definitions.
  * **`constexpr` methods** carry an explicit `inline` on the declaration for consistency (constexpr is implicitly inline at function level, but the marker keeps the visual pattern uniform). `constexpr` itself **does** repeat on the definition — it's part of the function's semantics, not just inline status.
  * **Deduction guides** and other free declarations (not definitions) stay inside the namespace block.
  * **Template member functions** use the stacked `template<...>` syntax when defined outside the class.
  * **Doxygen comments live at the definition**, regardless of file. In a `.cpp`-paired class the docs sit with the `.cpp` definitions; in a header-only class the docs sit with the inline definitions in the `.h` (below the namespace block). The class body — the prototype-overview surface — stays free of Doxygen noise in both cases.
  * **The type's own `@brief` block goes to the `.cpp` too**, as a `@class esp::foo` / `@struct esp::p::bar` block placed after the includes and the `p` declarations, above the first definition in the file. The class declaration in the `.h` stays Doxygen-free; the `@class`-prefixed block in the `.cpp` is the canonical type docstring. Header-only types still carry their `@brief` in the `.h` above the type declaration since there is no `.cpp` to host it.
  * **File documentation.** A `.cpp` with a companion header starts (after the licence notice) with two `@file` blocks: `@file esp_foo.h` with a one-line `@brief` saying what the header declares, then `@file esp_foo.cpp` saying what the file implements. A header-only file carries its own `@file` block right after `#pragma once`.
  * **`override` method declarations** that don't add anything to the base's contract (no extra params, no special behaviour, no relaxed / strengthened invariants) carry **no Doxygen block at all** — Doxygen's `@copydoc` chain picks up the base's docs automatically. Add a fresh block (and a corresponding `.cpp`-side definition block) only when the override genuinely has something to say.
  * **Data-type Doxygen: enums, structs, classes, and their entries.** Every `enum class`, `struct`, and `class` declaration carries (in the `.cpp` as `@class`/`@struct`, or in the `.h` for header-only types) a `/** @brief ... */` block. **Every enum member, every struct field, and every class data member carries its own Doxygen block** above the entry **in the `.h`** — fields and enumerators only exist at the declaration so the block has nowhere else to live. Do not use trailing `// short note` comments on these — they get truncated by terminal width and don't show up in generated documentation.  Pure virtual methods of a header-only class have no definition to carry their docs; they are documented with `@fn` blocks in the header's `// Реализация` section.
  * **Doxygen comments are comprehensive, not telegraphic.** Each block explains what the entry is *for*, what invariant it carries, and how callers / readers interact with it — not just a restated name. "Cursor fill colour" is not enough; "Cursor fill colour painted over the cell underneath; alpha channel doubles as the overlay opacity (0xFF for an opaque block, lower for translucent overlays)" is the target. One sentence is usually too few; three is usually right; a paragraph is fine when the field is load-bearing.
  * **Doxygen commands use the `@`-prefix form, not `\`.** `@brief`, `@param`, `@return`, `@c`, `@p`, `@code` / `@endcode`, `@ref`, `@note`, `@file`, etc.  Both prefixes are accepted by Doxygen but the project picks one and uses it uniformly so a single grep finds every callsite.
  * **Emphasize identifiers and constants with `@c`.**  Any reference to a type, function, identifier, or bare constant in Doxygen prose gets `@c` so it renders in fixed-width font.  `@c true`, `@c false`, `@c nullptr`, `@c 0`, `@c on_print`, `@c std::size_t`, etc.  Function parameters specifically use `@p` (e.g. `@p obj`).  Plain words stay un-`@c`-ed.
  * **Section markers between class body and definitions.**
    - **In a header-only `.h`**: a single `// Реализация` comment separates the namespace block (which ends with a bare `}`) from the definition surface below it. Without the marker the boundary between "declarations above" and "definitions below" reads as just an extra blank line.
    - **In a `.cpp`**: no `// Реализация` marker — the file itself is the implementation, the marker would be redundant. Definitions begin after the includes, the `p` declarations and the `@class` block.
    - In both file kinds, definitions are grouped by access control with a bare `// Открытые` / `// Защищённые` / `// Закрытые` comment (public / protected / private; like all comments, the markers are in Russian) above each group. Definitions of a `.cpp`'s `p` helpers go under `// Закрытые` too. Only the access groups that actually have definitions get a marker — a class with public-only methods uses one `// Открытые` marker, not three. No long `// ---------` rule lines; the bare access label is enough.

The reference layout, with all rules applied:

```cpp
// esp_entity.h

#pragma once

#include <cstddef>


namespace esp
{

class entity
{

public:

    entity();

    void set_count(std::size_t count);
    std::size_t count() const;

private:

    /** @brief Число элементов; задаёт set_count(), читает count(). */
    std::size_t _count;
};

}
```

```cpp
// esp_entity.cpp

/**
 * @file esp_entity.h
 * @brief Объявление счётчика esp::entity.
 */

/**
 * @file esp_entity.cpp
 * @brief Счётчик элементов esp::entity.
 */

#include "esp_entity.h"


/**
 * @class esp::entity
 * @brief Хранит число элементов.
 */


// Открытые

/**
 * @brief Создаёт счётчик без элементов.
 */
esp::entity::entity()
    : _count{0}
{
}

/**
 * @brief Заменяет число элементов.
 * @param count Новое число элементов; проверять его — забота вызывающего.
 */
void esp::entity::set_count(std::size_t count)
{
    _count = count;
}

/**
 * @brief Возвращает число элементов.
 * @return Число элементов.
 */
std::size_t esp::entity::count() const
{
    return _count;
}
```

Note in the `.h`:

  * `class entity { ... }` lives inside `namespace esp { ... }`. No 4-space indent.
  * Blank line after `namespace esp\n{` and before the bare closing `}`.
  * Blank line above `public:` and `private:` (including the first one).
  * Two blank lines between `#include <cstddef>` and `namespace esp`.
  * No Doxygen comments on the methods — the header is a prototype-overview surface.  Data members are the exception: their blocks sit at the declaration, the only place they exist.

Note in the `.cpp`:

  * **No `namespace esp { ... }` block.** Every definition uses the fully-qualified name `esp::entity::method`.
  * The two `@file` blocks open the file, the `@class` block follows the includes.
  * Doxygen comments on every definition, in Russian.
  * Definitions separated by single blank lines; trivial defaulted special members are grouped without blank lines.

For a **header-only class** (no companion `.cpp`), the same rule applies — definitions go outside the namespace block, in the same `.h` file.  `esp::ps2::ps2_device_lock` in full:

```cpp
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019-2022 Fabrizio Di Vittorio.
// Изменено для Espresso в 2026 г.


#pragma once


/**
 * @file
 * @brief Захват устройства PS/2 на время блока esp::ps2::ps2_device_lock.
 */


#include "esp_non_copyable.h"
#include "esp_ps2_ps2_device.h"


namespace esp::ps2
{

/**
 * @brief Захватывает устройство PS/2 на время жизни объекта.
 *
 * Конструктор ждёт захвата без ограничения времени, деструктор освобождает
 * устройство — так последовательность команд не перемешается с командами
 * других задач.
 */
class ps2_device_lock : esp::non_copyable
{

public:

    inline explicit ps2_device_lock(esp::ps2::ps2_device *device);
    inline ~ps2_device_lock();

private:

    /** @brief Захваченное устройство. */
    esp::ps2::ps2_device *_device;
};

}


// Реализация

// Открытые

/**
 * @brief Захватывает устройство, ожидая сколько потребуется.
 * @param device Устройство; должно существовать дольше объекта захвата.
 */
esp::ps2::ps2_device_lock::ps2_device_lock(esp::ps2::ps2_device *device)
    : _device{device}
{
    _device->lock(-1);
}

/**
 * @brief Освобождает устройство.
 */
esp::ps2::ps2_device_lock::~ps2_device_lock()
{
    _device->unlock();
}
```

Note the definitions in this header-only example:

  * **`// Реализация`** marks the boundary between the namespace block above and the definitions below.
  * **`// Открытые`** appears once, before the first public definition. If this class also had protected or private methods defined here, `// Защищённые` and `// Закрытые` markers would appear above each respective group. Sections with no methods are omitted entirely.
  * **No `inline` keyword on the definitions** — the `inline` on the in-class declaration is sufficient.
  * **Doxygen comments on every definition**, even though we're in a `.h`. The class body stays clean apart from the data members; the type's own `@brief` sits above the class because there is no `.cpp`.

Free functions follow the same rule. The class body and any free-declaration block stay inside the namespace block; their definitions go outside, fully qualified.

## Names

**Universal rule: snake_case for everything except macros and global constants.** This applies uniformly to classes, structs, enum types, enum members, functions, variables, and namespaces.

**Macros and global constants** use capital SNAKE_CASE — file-scope `const` / `constexpr` / `static const` tables, lookup arrays, and `#define`d values. This visually distinguishes them from mutable globals at the call site.

**Private and protected data members** use a **leading underscore** prefix: `_member_name`. This avoids collisions with same-named getter functions and constructor parameters, and is unambiguous at every call site. The leading-underscore convention is safe in member context (the reserved-identifier rules apply only to file/global scope identifiers starting with underscore, and to all identifiers starting with `_<capital>` or `__`).

**Public data members carry no prefix**, whether the type is a plain data record or has methods: `rect.x1`, `rgb888.r`, `item.vk`. A public field is part of the interface rather than storage behind an accessor, so there is nothing for the underscore to disambiguate, and call sites read naturally. Protected members count as non-public: a protected `_screen_width` and its public getter `screen_width()` must not clash.

Right

```cpp
namespace esp::sound
{

enum class sound_gen_method
{
    dac,
    sigma_delta,
    automatic,
};

class sound_generator : esp::non_copyable
{

public:

    inline void set_volume(int value);
    inline int volume() const;

private:

    std::int8_t _volume;
};

constexpr int VOICE_COUNT = 8;

}
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

static const char *log_level_names[] = { ... }; // mutable-name style for a constant
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

A boolean reads as a yes/no statement: precede it with a word like "is", "did" or "has", or name the state with a past participle or an adjective.

Right

```c
bool is_valid;
bool did_send_data;
bool _keyboard_found;
bool _init_done;
```

Wrong

```c
bool data; // a noun: what about the data?
bool sent_data; // reads as "the data that was sent"
bool check; // a verb
```

**Enum values** are snake_case like every other name.  A value keeps a prefix only where the bare name would not be a valid identifier or would be a keyword: `esp::ps2::virtual_key::vk_0`, `vk_delete`.  Where letter case is the only difference between two names, it stays (`vk_a` / `vk_A`).

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

**Constants are `constexpr` variables, not macros.**  Settings and other constants live as `constexpr` variables in their module's namespace -- `esp::vga::PRIMITIVE_TASK_STACK_SIZE` in `esp_vga_config.h`, `esp::GPIO_UNUSED` in `esp_tools.h`.  A macro is used only where the preprocessor has to see the value: a switch tested by `#if` (`ESP_VGA_USE_APLL_AB_COEF`), an attribute wrapper (`ESP_ALWAYS_INLINE_ATTR`), the ULP instruction macros (which follow the naming of Espressif's `ulp.h`).  Project macros carry the `ESP_` prefix, followed by the module name when they belong to a module (`ESP_VGA_`, `ESP_PS2_`).

Macros that expand to function calls or other non-constant computation: these should be named like functions, and should have parentheses at the end, even if they take no arguments (with the exception of some special macros). Note that usually it is preferable to use an inline function in such cases instead of a macro.

Right

```c
#define ESP_VGA_PSRAM_HACK() asm(" nop")
```

Wrong

```c
#define esp_vga_psram_hack() asm(" nop")

#define ESP_VGA_PSRAM_HACK asm(" nop")
```

`#pragma once` should be used instead of `#define` -- `#ifdef` "header guards".

**Directives nested in a conditional block are indented** by three spaces between the `#` and the keyword per nesting level, so the structure of a platform or feature branch reads at a glance; the conditional that opens a top-level block and its own `#else` / `#endif` stay in column 0, and the C++ code inside the block keeps its normal indentation -- the rule is about directives only:

```cpp
#if __has_include(<hal/rtc_io_periph.h>)
#   include <hal/rtc_io_periph.h>
#elif __has_include(<soc/rtc_io_periph.h>)
#   include <soc/rtc_io_periph.h>
#endif
```

A directive nested one level deeper gets three more spaces (`#      define`).

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

Raw `T *` is **never** used for ownership in C++ code. `new` and `delete` are never used directly — always `std::make_unique<T>(...)` / `std::make_shared<T>(...)`. The only exception is interop with C APIs (ESP-IDF, FreeRTOS): a handle they return is owned by one object, which releases it in its destructor; the rest of the code sees that object, not the raw handle.

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


## `#include` statements

Every implementation file includes its primary header first: `esp_gfx_rect.cpp` starts with `#include "esp_gfx_rect.h"`. This guarantees that each header's completeness is tested, so each header compiles without requiring any other header to be included first. A module's private header comes next (`esp_vga_utils_p.h`).

Then come these groups, separated by one blank line and each sorted (case-sensitive; `.clang-format` sorts within a group but does not move lines between groups):

  1. Other project headers, in quotes: `"esp_tools.h"`, `"esp_gfx_canvas.h"`, and the vendored libraries under `src/3rdparty` (`"utf8.h"`, `"yyjson.h"`).
  2. C++ standard library headers, in angle brackets: `<cstdint>`, `<memory>`.
  3. ESP-IDF and FreeRTOS headers, in angle brackets: `<driver/gpio.h>`, `<esp_log.h>`, `<freertos/queue.h>`, `<sdkconfig.h>`.

Right

```cpp
// esp_ps2_ps2_controller.cpp
#include "esp_ps2_ps2_controller.h"

#include "esp_ps2_keyboard.h"
#include "esp_ps2_mouse.h"
#include "esp_tools.h"

#include <cassert>
#include <cstdint>
#include <memory>

#include <driver/rtc_io.h>
#include <esp_err.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
```

Wrong

```cpp
// esp_ps2_ps2_controller.cpp
#include "esp_ps2_keyboard.h" // primary header not first
#include "esp_ps2_ps2_controller.h"
#include "esp_log.h" // ESP-IDF header in quotes
#include <memory>
#include "esp_tools.h" // groups mixed
```

A file that tests a configuration macro (`CONFIG_*`) includes `<sdkconfig.h>` itself.  Through a transitive include that later disappears, the `#if` would silently read the macro as undefined and take the wrong branch without any diagnostic.


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

Prefer `std::string` for **set-rarely / read-often data** outside the paths listed below — the board test's input line, names, paths.  The cost of the heap allocation is invisible against the work the read paths do.

Keep a fixed-size `char[N]` (sized through a named constant in the module's config header, never a magic literal at the call site) when *any* of these holds:

  * **Interrupt handlers and code they call.**  They never allocate (see *Embedded specifics*).
  * **Per-frame / per-key hot paths.**  Drawing primitives, glyph rendering, keyboard event handling.
  * **Many in flight at once.**  Records kept by the dozen in a queue, where a `std::string` per slot would multiply the allocations.
  * **Format caps.**  A size dictated by a format, such as the label of a VGA modeline, where the cap is a real upper bound and not a hopeful guess.
  * **Short-lived formatting.**  A line built with `std::snprintf` for the screen or the log fits a local `char` array.

In doubt, default to `std::string` outside those paths.  Reach for `std::string_view` for non-owning function arguments (`set_*(std::string_view name)`).


## Comments

**Comments and Doxygen documentation are written in Russian.**  Identifiers, log messages, text shown on screen, the README and this guide are in English.  Russian text follows Russian typography: «ёлочки» for quotes, a spaced em dash — like this, the decimal comma (0,07 мкс).

Use only one space before end of line comments and in between sentences in comments.

Right

```cpp
f(a, b); // Вызов сбрасывает буфер. Второе предложение.
```

Wrong

```cpp
int i;    // Перед комментарием несколько пробелов.
double f; // Ещё комментарий.  Между предложениями два пробела.
```

Make comments look like sentences by starting with a capital letter and ending with a period. One exception may be end of line comments like this `if (x == y) // ложно для NaN`.

Use `@bug` to denote known problems that stay in the code for now, in the Doxygen block of the function or class that has them.

Right

```cpp
/**
 * ...
 * @bug fill_path() держит точки пересечения строки с многоугольником в
 *      массиве переменной длины на стеке обработчика прерывания.
 */
```

Wrong

```cpp
fill_path(path); // FIXME: массив переменной длины на стеке.

fill_path(path); // TODO: убрать массив переменной длины.
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

A constrained subset of C++ is used. The constraints exist to keep the binary small, the compile times short, and the codebase legible.

**One target, one compiler.**  The firmware is built only by ESP-IDF's GCC for the ESP32 (Xtensa), so there are no compiler or operating-system branches.  What varies is the ESP-IDF configuration: code branches on the `CONFIG_*` macros of `<sdkconfig.h>` (see *`#include` statements*), or on `__has_include` where a header moves between ESP-IDF versions.  GNU attributes go through the ESP-IDF attribute macros (`IRAM_ATTR`, `DRAM_ATTR`, `FORCE_INLINE_ATTR`, ...) or a project wrapper such as `ESP_ALWAYS_INLINE_ATTR` in `src/tools/esp_attributes.h`; `__attribute__` itself is never written bare.  See *Embedded specifics*.

**Forbidden:**

  * **Exceptions.** ESP-IDF compiles C++ without exceptions (`CONFIG_COMPILER_CXX_EXCEPTIONS` is off). Errors are returned — see *Error handling*. Rationale: binary size, control-flow opacity, and the cost of unwind tables.
  * **RTTI** (`dynamic_cast`, `typeid`). Compiled without it (`CONFIG_COMPILER_CXX_RTTI` is off).  Where type identity is genuinely needed, use an explicit discriminator enum, as `esp::gfx::primitive_cmd` does for drawing primitives.
  * **`std::regex`.** Notoriously slow and binary-heavy. Use a small focused matcher where matching matters.
  * **Heavy template metaprogramming.** Concepts and simple templates are encouraged; SFINAE forests and recursive template instantiation chains are not.
  * **Bare `new` / `delete`.** Objects are static, or created with `std::make_unique<T>(...)` / `std::make_shared<T>(...)`.  Memory with ESP-IDF capability flags comes from `heap_caps_malloc` and is owned by a `std::unique_ptr` whose deleter calls `heap_caps_free` — see *Memory* in *Embedded specifics*.
  * **`using namespace`** and **namespace aliases** (`namespace fs = std::filesystem;`).  Identifiers are spelled out fully at every use site so a reader can grep any name verbatim.  Type aliases (`using foo_fn = int (*)(int);`) are allowed and **preferred over `typedef`** — they read left-to-right and compose with templates uniformly.  `typedef` is permitted but should be avoided in new code.  Type-alias names follow the same naming rules as other types (no `_t` suffix).
  * **C-style casts** (`(int)gpio`).  `static_cast`, `reinterpret_cast` and `const_cast` say which kind of conversion is meant, refuse the others, and can be found with grep.
  * **Variable-length arrays** and **`alloca`** (see *Memory*).
  * **Coroutines** and **C++ modules** (`import ...`).  Toolchain support is uneven; revisit when a concrete subsystem benefits.
  * **`std::shared_ptr` where `std::unique_ptr` would do.** Reach for `shared_ptr` only when ownership is genuinely shared at runtime.

**Encouraged:**

  * **RAII** for every owned resource: DMA buffers, interrupt handles, FreeRTOS queues and tasks, allocations.
  * `std::array`, `std::span`, `std::string_view`, `std::optional`, `std::variant`; outside interrupt handlers and hot paths also `std::vector`, `std::string` and `std::unordered_map` (see *Memory*).
  * `std::unique_ptr<T>` for sole ownership.
  * **Standard algorithms instead of home-made helpers**: `std::min`, `std::max`, `std::clamp`, `std::swap`, `std::fill`, `std::copy`.  `std::clamp(value, low, high)` requires `low <= high` and is undefined otherwise.  Where the range can be empty — an empty clipping rectangle, for example — test for that first, or intersect with `std::max` / `std::min` after an explicit "outside" check, as the drawing functions of `esp::gfx` do.
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
  * **`std::`-qualified fixed-width and size types in C++.** Always write `std::size_t`, `std::uint8_t`, `std::uint32_t`, `std::int64_t`, `std::ptrdiff_t`, etc. — never the C-style globals `size_t` / `uint8_t` / `int64_t` / `ptrdiff_t` (which are technically only guaranteed to exist with that bare name from `<stddef.h>` / `<stdint.h>`, not from their C++ wrappers `<cstddef>` / `<cstdint>`). Headers therefore use `#include <cstddef>` + `<cstdint>` (not their C counterparts) and every reference goes through `std::`. Applies even when the type is unambiguous in context — the rule is universal so call sites read uniformly.  Types that ESP-IDF and FreeRTOS define for their APIs (`gpio_num_t`, `esp_err_t`, `TickType_t`, `QueueHandle_t`) are used as they are.

## Classes

### Class layout

Order inside a class declaration: public special members → public methods → public data (only in plain data types) → protected → private methods → private data. Within a section, group related members.

```cpp
class scrollback : esp::non_copyable
{

public:

    scrollback();
    ~scrollback();

    void push_line(std::string_view line);
    std::string_view line(std::size_t index) const;
    std::size_t line_count() const;

private:

    std::vector<std::string> _lines;
    std::size_t _capacity = 10000;
};
```

### Special members (rule of five)

Every class with non-trivial resources declares all five special members explicitly — either `= default`, `= delete`, or user-defined.

  * **Value types** (`esp::gfx::point`, `esp::gfx::rgb888`) follow the rule of zero: they declare no destructor, copy or move members and get the implicit ones.  A hand-written one needs a reason in its Doxygen block: `esp::gfx::rect` keeps a user-defined copy constructor because a trivially copyable packed `rect` is returned in registers and unpacked byte by byte at every call.
  * **Non-copyable / non-movable resources** inherit from `esp::non_copyable` (defined in `src/tools/esp_non_copyable.h`).  The base privately deletes both copy operations and both move operations; deriving from it deletes the 4 special members on the derived class with one short `: esp::non_copyable` instead of four `= delete` lines spread across the class body.  The owning classes — the display controllers, `esp::gfx::bitmap`, `esp::gfx::sprite`, `esp::sound::sound_generator` — are non-copyable.  When a class genuinely needs movable semantics it does *not* inherit `esp::non_copyable` and instead declares `= default` for the move special members directly.
  * **Classes with only static members** (`esp::ps2::ps2_controller`, `esp::core_usage`) declare `name() = delete;` instead: no object of them exists, so there is nothing to copy.
  * **Move operations are `noexcept`.** This is mandatory for the class to be usable inside `std::vector` without falling back to slower copies.

### Methods belong with their data

  * **Free functions that always take a `T *` first argument should be members of `T`.** A function like `static void extract_token(token_ctx *t, ...)` is a hidden constructor / mutator on `token_ctx`; declare it as `token_ctx::extract(...)` so the call site reads `ctx.extract(...)` instead of `extract_token(&ctx, ...)`.  This applies equally to file-local helper structs and to the module-private structs in `p` namespaces: any function whose first parameter is a `T *` belongs as a method of `T`.
  * **Free functions with no `T *` argument stay as free functions** — pure value transformations (`std::string parse_color(...)`, `bool is_keyword(const char *)`) don't gain anything from being methods.

### Member access

  * Member variables are `private` unless the class is an explicit plain data type (small value types: `esp::gfx::rect`, `esp::gfx::rgb888`).
  * Public access goes through methods (`set_count` / `count`, per the existing naming rules).
  * Friend declarations are rare and require a comment explaining why.

### `override` and `final`

  * `override` is required on every method that overrides a virtual.
  * `final` is used on classes or methods that should not be subclassed / further overridden.
  * `virtual` is required only on the declaration that introduces the virtual (the base-class declaration).

### `explicit`

Single-argument constructors are `explicit` unless implicit conversion is genuinely the design intent: `esp::gfx::rgb222` converts implicitly from `esp::gfx::rgb888` because the VGA drivers rely on it.

## Error handling

### Return values, not exceptions

  * **Functions that wrap ESP-IDF calls return `esp_err_t`** — `ESP_OK` or the ESP-IDF error code — and are `[[nodiscard]]`.  `esp::esp_intr_alloc_pinned_to_core()` passes on the results of `esp_ipc_call_blocking()` and `esp_intr_alloc()` that way.
  * **Fallible operations that return a payload return `std::expected<T, E>`** (C++23, available in ESP-IDF's GNU C++26 mode); `std::unexpected<E>` constructs the error state, callers check via the explicit-bool conversion and read the payload through `operator*`.  `E` is `esp_err_t` where the errors come from ESP-IDF, otherwise a per-module `enum class` defined next to the function.  These functions are `[[nodiscard]]` too.
  * **A `bool` result** is enough where the caller only needs to know whether it worked: `esp::ps2::keyboard::reset()` answers whether the keyboard passed its self-test.

### Checking at start-up

Initialisation that the program cannot run without is checked with `ESP_ERROR_CHECK`: on failure it prints the error, the file, the line and the expression, and stops the program.

```cpp
ESP_ERROR_CHECK(esp::esp_intr_alloc_pinned_to_core(
    ETS_I2S1_INTR_SOURCE, ESP_INTR_FLAG_LEVEL1 | ESP_INTR_FLAG_IRAM, isr_handler, this,
    &_isr_handle, esp::VIDEO_TASKS_CORE));
```

With assertions disabled (`NDEBUG`, which `CONFIG_COMPILER_OPTIMIZATION_ASSERTIONS_DISABLE` sets), `ESP_ERROR_CHECK` only evaluates its argument; an error that must be handled in every build is checked explicitly.  Where the program can go on — a missing SD card, an absent keyboard — the error is logged and handled, not checked.

### Logging

Diagnostics go to the serial console through `ESP_LOGE` / `ESP_LOGW` / `ESP_LOGI` / `ESP_LOGD` / `ESP_LOGV`, with a `TAG` constant in the `p` namespace of each module or file (`esp::ps2::p::TAG` is `"ps2"`).  Messages are English sentences ending with a period; an ESP-IDF error code is shown with `esp_err_to_name()`:

```cpp
ESP_LOGE(esp::p::TAG, "Failed to mount filesystem: %s.", esp_err_to_name(ret));
```

`LOG_LOCAL_LEVEL` is `ESP_LOG_DEBUG` for the whole program (`src/CMakeLists.txt`), so `ESP_LOGD` lines are compiled in; `esp::app` turns them on at run time for its list of tags in builds with assertions.  Interrupt handlers do not log (see *Embedded specifics*).

### `noexcept`

Mark a function `noexcept` when the function truly does not throw and the marking is observable (move constructors, move assignment, destructors, swap functions). Don't sprinkle `noexcept` on everything — the compiler already knows we have `-fno-exceptions` globally, but the annotation is meaningful for `std::vector` growth, conditional `noexcept` specifications, and reader clarity.

### Assertions

`assert()` (from `<cassert>`) for invariants that should hold by construction.  The project's configuration keeps assertions in (`CONFIG_COMPILER_OPTIMIZATION_ASSERTIONS_ENABLE`); with `NDEBUG` they compile out.  An invariant that must hold in every build is checked explicitly and stops the program with `abort()`, which prints a backtrace.

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
esp::gfx::canvas *canvas = nullptr;            // pointers are fundamental — use =

std::vector<int> v{1, 2, 3};                    // non-fundamental — use {}
std::string path{"hello"};
std::unique_ptr<atlas> a{std::make_unique<atlas>("ship")};
esp::gfx::rect clip{0, 0, 639, 479};
esp::p::intr_alloc_args args{.source = source, .flags = flags}; // designated initializers
esp::vga::p::apll_params params{};             // value-initialise an aggregate — use {}
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

    void push_line(std::string_view line);
    void clear();

    std::string_view line(std::size_t index) const;
    std::size_t line_count() const;
    bool is_empty() const;
};

void render(const scrollback &sb, esp::gfx::rect bounds)
{
    const std::size_t count = sb.line_count();
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

Concept definitions live in the module that uses them; when several files of a module share them, in an `esp_<module>_concepts.h` header.

Avoid:

  * Recursive template instantiation chains.
  * Variadic templates beyond simple forwarding.
  * `std::enable_if_t` — always replaceable by a concept.
  * `std::is_*_v` traits at call sites — express the requirement as a concept.

## Lambdas

  * **Capture explicitly when the lambda outlives the call** — when it is stored, handed to a task, or kept as a callback: `[this]`, `[value]`, and a reference capture only of an object that outlives the lambda.  `[=]` is not used: it captures `this` implicitly, which C++20 deprecates.
  * **`[&]` is allowed for a lambda that is used before the enclosing function returns** — passed straight to an algorithm or to a template that calls it, as the VGA drivers pass their pixel operations to the drawing templates of `esp::gfx::generic_bitmapped_display_controller` (C++ Core Guidelines F.52).
  * **Spell out the parameter list** unless the lambda is a one-line `std::ranges`-style helper.

Right

```cpp
const auto corner = [this](int x, int y, esp::gfx::color color)
{
    _canvas.set_brush_color(color);
    _canvas.fill_rectangle(x - 24, y - 12, x + 23, y + 11);
};

generic_invert_rect(rect, update_rect,
                    [&](int y, int x1, int x2) { raw_invert_row(y, x1, x2); });
```

Wrong

```cpp
auto by_priority = [=](auto a, auto b) { return /* ... */; }; // [=] and auto parameters
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

  * `[[nodiscard]]` on functions where the return value must be checked: anything returning `esp_err_t` or `std::expected<T, E>`, and factory functions returning `std::unique_ptr`.
  * `noexcept` per the *Error Handling* rules above.
  * `constexpr` on functions and variables that can be evaluated at compile time.
  * `consteval` on functions that **must** be evaluated at compile time.
  * `explicit` on single-argument constructors.
  * `override` on every method that overrides a virtual.
  * `final` on sealed classes / methods.

## Namespaces

  * **Snake_case namespace names.** `namespace esp::gfx { ... }`.
  * **C++17 nested-namespace syntax** for multi-level declarations: `namespace esp::gfx { ... }`, not the older nested `namespace esp { namespace gfx { ... } }`.
  * **No `using namespace`** at any scope — file, function, or block.
  * **No namespace aliases** (`namespace fs = std::filesystem;` is forbidden; spell out `std::filesystem` at every use site).
  * **No renaming aliases.** A type alias names a new type (`using foo_fn = int (*)(int);` — allowed and preferred over `typedef`, see the language subset); an alias that only shortens an existing name (`using string_view = std::string_view;`) is forbidden — spell out `std::string_view` at every use site.
  * **No anonymous namespaces, period.**  Everything private to a file — helper functions, mutable data, constants, lookup tables, local structs — goes in the owning namespace's `::p` sub-namespace inside the `.cpp` (`esp::p` for files whose primary namespace is plain `esp`, such as `esp_app.cpp`; `esp::gfx::p`, `esp::vga::p`, `esp::ps2::p`, `esp::sound::p` in the modules), or in the module's `_p.h` when other files of the module need it.  Call sites outside the `p` block use the full `esp::vga::p::helper_name(...)` qualification; sibling blocks of the same `p` namespace may stay unqualified.  Rationale: a qualified name tells the reader AT THE USE SITE that the thing is private and where it lives -- anonymous namespaces hide exactly that, and "private" gets a single greppable home.  One consequence to mind: unlike anonymous-namespace entities, functions, types and non-const data in a `p` namespace have external linkage, so their names must stay unique across the files of that namespace.  `const` and `constexpr` variables keep internal linkage, so every file may have its own `esp::p::TAG`.

`using` *declarations* (bringing a name into scope, `using std::swap;`) are allowed only in one tightly-scoped case: **inheriting constructors** in a derived class (`using base::base;`). Type-alias declarations follow the rule above.

## Tooling

  * **`clang-format`.**  `.clang-format` at the repository root codifies brace placement, indentation, spacing, pointer alignment, line wrapping and include sorting; `.clang-format-ignore` leaves out the vendored libraries (`src/3rdparty`), the generated font tables and the build directory.  Every changed file is formatted with it.
  * **Build.**  `./build.sh` builds with ESP-IDF's warning flags, which treat warnings as errors; the build stays free of warnings.  `idf.py reconfigure` is needed after adding or removing source files, because the source lists are collected when CMake configures the project.  `build/compile_commands.json` serves IDEs.
  * **Doxygen.**  `doc/Doxyfile` builds the documentation (`make -C doc`); a change adds no Doxygen warnings.  Attribute macros that stand between a declaration and its name are listed in `PREDEFINED` (`IRAM_ATTR=`, `ESP_ALWAYS_INLINE_ATTR=`, ...), otherwise Doxygen cannot match declarations with their definitions.
  * **`clang-tidy`** is not part of the build: the host's clang cannot compile for the Xtensa toolchain directly.


# Embedded specifics

The firmware runs on an ESP32 under ESP-IDF: two cores, about 300 KB of heap, small task stacks, and interrupt handlers that keep running while the flash cache is disabled.  Where the rules in this section conflict with the rules above, they take precedence.

## Memory

  * **Static first.**  Long-lived objects — drivers, the application, buffers that live as long as the program — are static, or are allocated once at start-up.  Allocating and freeing while the program runs fragments the heap, and the VGA frame buffer needs large contiguous blocks.
  * **Capability memory through `heap_caps_malloc`.**  Buffers that need ESP-IDF memory capabilities — DMA-capable, internal, 32-bit accessible, PSRAM — come from `heap_caps_malloc` with the matching `MALLOC_CAP_*` flags and are released with `heap_caps_free`.  The pointer is owned by a `std::unique_ptr` whose deleter calls `heap_caps_free`; it is never kept bare.
  * **No allocation in interrupt handlers, per-frame or per-key code.**  These paths use fixed-size buffers sized by named constants.  `std::string`, `std::vector` and other allocating containers are used only outside them, with capacity reserved up front where the size is known.
  * **`alloca` is forbidden**, like variable-length arrays, and large local arrays are avoided: task stacks are small (the VGA primitive task has 1200 bytes, `esp::vga::PRIMITIVE_TASK_STACK_SIZE`) and an overflow corrupts memory silently.

## Interrupt handlers and timing-critical code

  * **ESP-IDF attribute macros and optimisation pragmas are allowed where the code needs them**: `IRAM_ATTR`, `DRAM_ATTR`, `FORCE_INLINE_ATTR`, `NOINLINE_ATTR`, `WORD_ALIGNED_ATTR` and the like, and a per-file `#pragma GCC optimize` for drivers whose speed matters even in a debug build.  `__attribute__` itself is never written bare: an attribute ESP-IDF has no macro for gets one in a project header.  `FORCE_INLINE_ATTR` contains `static` and suits free functions only; methods use `ESP_ALWAYS_INLINE_ATTR` from `src/tools/esp_attributes.h`.
  * **Everything an interrupt handler calls is in IRAM or force-inlined.**  A function in flash crashes the handler whenever the flash cache is disabled — during a flash write, for example.  Moving such a function out of a header into a `.cpp` file therefore needs `IRAM_ATTR` on it, and splitting a file must keep every function under the same optimisation pragma it had.
  * **`#pragma GCC optimize` applies only to functions defined after it.**  Inline functions from headers included above the pragma keep the file's command-line level, and GCC does not inline a function with different optimisation options into the faster code.  Helpers that the hot code must inline are therefore `FORCE_INLINE_ATTR` or `ESP_ALWAYS_INLINE_ATTR`.
  * **The drivers derived from FabGL build with `-Os` even in a debug build**, so that the interrupt handlers keep up; their files are listed in `esp_size_optimized_sources` in `src/CMakeLists.txt`, and a new file of these modules (or of their helpers in `src/tools`) goes on the list.
  * **Data an interrupt handler reads while the flash cache may be disabled is in RAM**: `DRAM_ATTR` on constants, which the compiler would otherwise place in flash.
  * **No logging, allocation or blocking calls in interrupt handlers.**  `ESP_LOGx`, `printf`, `malloc`, `new`, mutexes and FreeRTOS calls without the `FromISR` suffix are not used there; `ESP_DRAM_LOGx` serves rare diagnostics.
  * **Data shared with interrupt handlers is `volatile` or `std::atomic`**, and updates of several related fields happen inside `portENTER_CRITICAL` / `portEXIT_CRITICAL` (the `_ISR` variants in the handlers themselves).
