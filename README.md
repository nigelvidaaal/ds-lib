# ds-lib

A header-only C++17 library of hand-written containers, `Vector<T>`, `LinkedList<T>`, and `HashMap<K, V>`, built from scratch to understand manual memory management, templates, and value semantics. It includes a 67-test suite that checks for leaks and a demo program that uses all three.

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue) ![header-only](https://img.shields.io/badge/header--only-yes-green)

## Highlights

- **`Vector<T>`** uses raw storage plus placement `new`, like `std::vector`. Elements are constructed only when added, so `T` doesn't need a default constructor and reserved capacity costs nothing. It has full Rule of Five support, `emplace_back` with perfect forwarding, amortized O(1) growth by doubling, the strong exception guarantee on copy assignment (copy-and-swap), and `std::move_if_noexcept` during reallocation.
- **`LinkedList<T>`** is a singly linked list with a tail pointer, which makes both `push_front` and `push_back` O(1). It supports full Rule of Five, forward iterators (range-based `for`, with `iterator` → `const_iterator` conversion), and in-place O(n) `reverse`.
- **`HashMap<K, V, Hash>`** uses separate chaining with automatic rehashing past a 0.75 load factor. Rehashing splices list nodes instead of copying them, so references returned by `operator[]` stay valid when the table grows. It follows the Rule of Zero: no hand-written destructor, copy, or move.
- **Testing:** 67 tests on a minimal test framework written for the project. A `Tracker` type counts live objects, copies, and moves, which lets the tests prove there are no leaks or double-frees and that moves really avoid copies. Verified with AddressSanitizer, UBSan, and Valgrind, and it compiles warning-free under `-Wall -Wextra -Wpedantic -Wshadow -Wconversion` on GCC and Clang.

## Quick look

```cpp
#include "ds/HashMap.h"
#include "ds/LinkedList.h"
#include "ds/Vector.h"

ds::Vector<std::string> names{"ada", "linus"};
names.emplace_back("grace");            // constructed in place, no temporary

ds::LinkedList<int> list;
list.push_back(1);
list.push_front(0);
list.reverse();
for (int x : list) { /* 1, 0 */ }

ds::HashMap<std::string, int> counts;
++counts["hello"];                       // inserts 0, then increments
counts.at("hello");                      // 1; throws std::out_of_range if missing
```

## Project layout

```
ds-lib/
├── CMakeLists.txt
├── include/ds/
│   ├── Vector.h         growable array (Rule of Five, placement new)
│   ├── LinkedList.h     singly linked list with iterators (Rule of Five)
│   └── HashMap.h        separate-chaining hash map (Rule of Zero)
├── examples/
│   └── demo.cpp         word-frequency counter using all three containers
└── tests/
    ├── test.h           minimal TEST / CHECK framework
    ├── Tracker.h        counts live objects, copies, and moves
    ├── test_main.cpp    test runner with name filtering
    ├── test_vector.cpp
    ├── test_linkedlist.cpp
    └── test_hashmap.cpp
```

## Building

You need CMake 3.16+ and a C++17 compiler (Visual Studio 2019+, MinGW-w64, GCC, or Clang).

```powershell
cmake -S . -B build
cmake --build build
```

Run the tests and the demo. Visual Studio puts executables in `build\Debug\`, while MinGW, GCC, and Clang put them directly in `build\`:

```powershell
.\build\Debug\ds_tests.exe              # all tests
.\build\Debug\ds_tests.exe hashmap      # only tests whose name contains "hashmap"
.\build\Debug\ds_demo.exe               # word counts for a built-in sample
.\build\Debug\ds_demo.exe README.md     # word counts for any text file
```

On Linux or macOS, add `-DDS_SANITIZE=ON` to the first command to build the tests with AddressSanitizer and UBSan.

Sample demo output:

```
60 words, 20 unique
hash map: 32 buckets, load factor 0.625

Top words:
  it            10
  of            10
  the           10
  was           10
  age           2
  ...
```

## Complexity

| Container | Insert | Access / lookup | Remove |
| --- | --- | --- | --- |
| `Vector` | `push_back`: O(1) amortized | by index: O(1) | `pop_back`: O(1) |
| `LinkedList` | `push_front` / `push_back`: O(1) | `front` / `back`: O(1); `contains`: O(n) | `pop_front`: O(1); `remove(value)`: O(n) |
| `HashMap` | `insert`: O(1) average | `find` / `at`: O(1) average | `erase`: O(1) average |

## Design notes

### Why header-only?

`Vector<T>` isn't real code until the compiler sees it used with a specific type, like `Vector<int>`. At that moment it needs the full definition, not just the declaration. If the definitions lived in a `.cpp`, other files couldn't see them and you'd get linker errors. That's why template libraries usually live entirely in headers, and why CMake declares `ds` as an `INTERFACE` library with nothing to compile.

### Rule of Five vs. Rule of Zero

When a class owns a raw resource, the compiler's default copy just copies the pointer, so two objects end up freeing the same memory. `Vector` and `LinkedList` own raw memory, so each defines all five special members: destructor, copy constructor, copy assignment, move constructor, and move assignment.

`HashMap` stores its data in `std::vector` and `std::list`, which already manage themselves. So the compiler-generated versions are correct, and it defines none of them. Whenever possible, that's the better design.

### Raw storage and placement `new`

The simplest `Vector` allocates with `new T[capacity]`. That default-constructs every slot, including the ones not in use yet, and it doesn't compile for types without a default constructor. This `Vector` instead allocates raw bytes with `::operator new`. It constructs each element in place with placement `new` when it's added, and calls `~T()` explicitly when it's removed. A test (`vector_reserve_constructs_nothing`) confirms that `reserve(100)` creates zero objects.

### Exception safety

- Copy assignment uses **copy-and-swap**. The copy is built first, so if it throws, the original is untouched.
- Reallocation uses `std::move_if_noexcept`. If `T`'s move constructor might throw, it copies instead, so a failure partway through leaves the old buffer intact.
- Constructors clean up after themselves if an element copy throws, because a destructor never runs for an object whose constructor didn't finish.
- Moves are `noexcept`. This matters: `std::vector<ds::Vector<int>>` will only move elements during a resize if the move can't throw. Otherwise it falls back to copying.

### A subtle aliasing bug, handled

`v.push_back(v[0])` on a full vector passes a reference into the very buffer that's about to be freed by the resize. `emplace_back` builds the new element before reallocating, so the reference is still valid when it's read. The test `vector_push_back_own_element_while_full` covers this.

### Why doubling?

Growing by doubling means a single `push_back` sometimes copies everything, but across n pushes the total copying is under 2n, which averages to O(1) per push. Growing by a fixed amount instead would make filling a vector O(n²). The test `vector_growth_is_geometric` checks that 1,000 pushes cause at most 11 reallocations.

## Possible extensions

- A bidirectional iterator for `HashMap`
- Using `ds::Vector` and `ds::LinkedList` internally in `HashMap` in place of the standard containers
- Allocator support
- A benchmark comparing these containers against `std::vector`, `std::list`, and `std::unordered_map`
