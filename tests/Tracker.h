#pragma once

// A test helper type that counts how many instances are alive and how many
// copies and moves happened. Lets the tests catch memory leaks, double
// deletes, and accidental copies without any external tools.

struct Tracker {
    static inline int live = 0;
    static inline int copies = 0;
    static inline int moves = 0;

    int value = 0;

    Tracker() { ++live; }
    explicit Tracker(int v) : value(v) { ++live; }
    Tracker(const Tracker& o) : value(o.value) { ++live; ++copies; }
    Tracker(Tracker&& o) noexcept : value(o.value) { ++live; ++moves; }
    Tracker& operator=(const Tracker& o) { value = o.value; ++copies; return *this; }
    Tracker& operator=(Tracker&& o) noexcept { value = o.value; ++moves; return *this; }
    ~Tracker() { --live; }

    bool operator==(const Tracker& o) const { return value == o.value; }

    static void reset() { live = copies = moves = 0; }
};
