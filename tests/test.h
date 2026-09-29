#pragma once

// A tiny test framework so the project needs nothing but a compiler.
//
//   TEST(my_test_name) {
//       CHECK(1 + 1 == 2);
//       CHECK_THROWS(std::out_of_range, v.at(99));
//   }

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace test {

struct Case {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<Case>& registry() {
    static std::vector<Case> cases;
    return cases;
}

struct Registrar {
    Registrar(const char* name, std::function<void()> fn) {
        registry().push_back({name, std::move(fn)});
    }
};

struct Failure {
    std::string message;
};

} // namespace test

#define TEST(name)                                                   \
    static void name();                                              \
    static test::Registrar name##_registrar(#name, name);            \
    static void name()

// Variadic so expressions containing commas (like Foo<int, int>) still work.
#define CHECK(...)                                                   \
    do {                                                             \
        if (!(__VA_ARGS__)) {                                        \
            throw test::Failure{std::string(__FILE__) + ":" +        \
                                std::to_string(__LINE__) +           \
                                "  CHECK(" #__VA_ARGS__ ") failed"}; \
        }                                                            \
    } while (0)

#define CHECK_THROWS(ExceptionType, expr)                            \
    do {                                                             \
        bool threw_ = false;                                         \
        try {                                                        \
            expr;                                                    \
        } catch (const ExceptionType&) {                             \
            threw_ = true;                                           \
        }                                                            \
        if (!threw_) {                                               \
            throw test::Failure{std::string(__FILE__) + ":" +        \
                                std::to_string(__LINE__) +           \
                                "  expected " #ExceptionType         \
                                " from: " #expr};                    \
        }                                                            \
    } while (0)
