#include <cstring>
#include <exception>
#include <iostream>

#include "test.h"

// Run all tests:              ds_tests
// Run tests matching a name:  ds_tests vector
int main(int argc, char** argv) {
    const char* filter = argc > 1 ? argv[1] : nullptr;

    int passed = 0, failed = 0;

    for (const auto& tc : test::registry()) {
        if (filter && std::strstr(tc.name.c_str(), filter) == nullptr) {
            continue;
        }

        try {
            tc.fn();
            std::cout << "[PASS] " << tc.name << '\n';
            ++passed;
        } catch (const test::Failure& f) {
            std::cout << "[FAIL] " << tc.name << "\n       " << f.message << '\n';
            ++failed;
        } catch (const std::exception& e) {
            std::cout << "[FAIL] " << tc.name << "\n       unexpected exception: " << e.what() << '\n';
            ++failed;
        }
    }

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
