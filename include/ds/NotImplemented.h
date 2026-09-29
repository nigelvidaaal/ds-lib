#pragma once

#include <stdexcept>
#include <string>

namespace ds {

// Thrown by every stub you haven't written yet. The test runner catches it
// and reports the test as "TODO" instead of crashing, so the project always
// builds and you can implement one function at a time.
class NotImplemented : public std::logic_error {
public:
    explicit NotImplemented(const std::string& what)
        : std::logic_error("not implemented: " + what) {}
};

} // namespace ds
