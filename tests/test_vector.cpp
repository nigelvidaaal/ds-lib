#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

#include "Tracker.h"
#include "ds/Vector.h"
#include "test.h"

using ds::Vector;

// ---------- basics ----------

TEST(vector_default_is_empty) {
    Vector<int> v;
    CHECK(v.size() == 0);
    CHECK(v.capacity() == 0);
    CHECK(v.empty());
}

TEST(vector_count_value_constructor) {
    Vector<int> v(3, 7);
    CHECK(v.size() == 3);
    CHECK(v[0] == 7 && v[1] == 7 && v[2] == 7);
}

TEST(vector_initializer_list_constructor) {
    Vector<int> v{1, 2, 3};
    CHECK(v.size() == 3);
    CHECK(v[0] == 1 && v[1] == 2 && v[2] == 3);
}

TEST(vector_push_back_grows) {
    Vector<int> v;
    for (int i = 0; i < 100; ++i) {
        v.push_back(i);
    }
    CHECK(v.size() == 100);
    CHECK(v.capacity() >= 100);
    for (std::size_t i = 0; i < 100; ++i) {
        CHECK(v[i] == static_cast<int>(i));
    }
}

TEST(vector_index_operator_can_write) {
    Vector<int> v{1, 2, 3};
    v[1] = 42;
    CHECK(v[1] == 42);
}

TEST(vector_at_throws_out_of_range) {
    Vector<int> v{1, 2, 3};
    CHECK(v.at(2) == 3);
    CHECK_THROWS(std::out_of_range, v.at(3));
}

TEST(vector_pop_back_removes_last) {
    Vector<int> v{1, 2, 3};
    v.pop_back();
    CHECK(v.size() == 2);
    CHECK(v[1] == 2);
}

TEST(vector_clear_empties) {
    Vector<int> v{1, 2, 3};
    v.clear();
    CHECK(v.empty());
}

TEST(vector_reserve_grows_capacity_not_size) {
    Vector<int> v{1, 2};
    v.reserve(50);
    CHECK(v.capacity() >= 50);
    CHECK(v.size() == 2);
    CHECK(v[0] == 1 && v[1] == 2);
}

TEST(vector_reserve_never_shrinks) {
    Vector<int> v;
    v.reserve(50);
    v.reserve(10);
    CHECK(v.capacity() >= 50);
}

TEST(vector_range_for) {
    Vector<int> v{1, 2, 3, 4};
    int sum = 0;
    for (int x : v) {
        sum += x;
    }
    CHECK(sum == 10);
}

TEST(vector_works_with_strings) {
    Vector<std::string> v;
    v.push_back("hello");
    v.push_back("world");
    CHECK(v[0] + " " + v[1] == "hello world");
}

// ---------- Rule of Five ----------

TEST(vector_copy_constructor_is_deep) {
    Vector<int> a{1, 2, 3};
    Vector<int> b(a);
    b[0] = 99;
    CHECK(a[0] == 1);     // changing the copy must not change the original
    CHECK(b.size() == 3);
    CHECK(a.begin() != b.begin());
}

TEST(vector_copy_assignment_is_deep) {
    Vector<int> a{1, 2, 3};
    Vector<int> b{9};
    b = a;
    b[0] = 99;
    CHECK(a[0] == 1);
    CHECK(b.size() == 3);
}

TEST(vector_self_assignment_is_safe) {
    Vector<int> a{1, 2, 3};
    Vector<int>& alias = a;
    a = alias;
    CHECK(a.size() == 3);
    CHECK(a[2] == 3);
}

TEST(vector_move_constructor_steals_buffer) {
    Vector<int> a{1, 2, 3};
    const int* oldBuffer = a.begin();
    Vector<int> b(std::move(a));
    CHECK(b.begin() == oldBuffer); // same memory, nothing was copied
    CHECK(b.size() == 3);
    CHECK(a.size() == 0);          // moved-from object is empty but usable
    a.push_back(5);
    CHECK(a[0] == 5);
}

TEST(vector_move_assignment_steals_buffer) {
    Vector<int> a{1, 2, 3};
    Vector<int> b{9, 9};
    const int* oldBuffer = a.begin();
    b = std::move(a);
    CHECK(b.begin() == oldBuffer);
    CHECK(b.size() == 3);
    CHECK(a.size() == 0);
}

TEST(vector_push_back_rvalue_moves_not_copies) {
    Tracker::reset();
    {
        Vector<Tracker> v;
        v.reserve(4);
        Tracker::reset();
        v.push_back(Tracker(1));
        CHECK(Tracker::copies == 0);
        CHECK(Tracker::moves >= 1);
    }
}

TEST(vector_no_leaks) {
    Tracker::reset();
    {
        Vector<Tracker> v;
        for (int i = 0; i < 20; ++i) {
            v.push_back(Tracker(i));
        }
        Vector<Tracker> copy(v);
        Vector<Tracker> moved(std::move(copy));
    }
    CHECK(Tracker::live == 0); // every Tracker created was destroyed exactly once
}

// ---------- beyond the basics ----------

// A type with no default constructor. new T[n] would fail to compile for this;
// Vector's raw-storage + placement-new design handles it.
struct NoDefault {
    explicit NoDefault(int v) : value(v) {}
    int value;
};

TEST(vector_works_without_default_constructor) {
    Vector<NoDefault> v;
    for (int i = 0; i < 10; ++i) {
        v.push_back(NoDefault(i));
    }
    CHECK(v.size() == 10);
    CHECK(v[9].value == 9);
}

TEST(vector_reserve_constructs_nothing) {
    Tracker::reset();
    {
        Vector<Tracker> v;
        v.reserve(100);
        CHECK(Tracker::live == 0);   // capacity is raw memory, not live objects
        v.push_back(Tracker(1));
        CHECK(Tracker::live == 1);
    }
    CHECK(Tracker::live == 0);
}

TEST(vector_emplace_back_constructs_in_place) {
    Tracker::reset();
    {
        Vector<Tracker> v;
        v.reserve(2);
        Tracker& t = v.emplace_back(42);
        CHECK(t.value == 42);
        CHECK(Tracker::copies == 0 && Tracker::moves == 0);
    }
}

TEST(vector_push_back_own_element_while_full) {
    Vector<std::string> v{"alpha"};
    CHECK(v.size() == v.capacity());   // full, so the next push must reallocate
    v.push_back(v[0]);                 // reference into the buffer being replaced
    CHECK(v[1] == "alpha");
}

TEST(vector_pop_back_destroys_element) {
    Tracker::reset();
    {
        Vector<Tracker> v;
        v.push_back(Tracker(1));
        v.push_back(Tracker(2));
        v.pop_back();
        CHECK(Tracker::live == 1);
    }
    CHECK(Tracker::live == 0);
}

TEST(vector_pop_back_on_empty_throws) {
    Vector<int> v;
    CHECK_THROWS(std::out_of_range, v.pop_back());
}

TEST(vector_back) {
    Vector<int> v{1, 2, 3};
    CHECK(v.back() == 3);
    Vector<int> empty;
    CHECK_THROWS(std::out_of_range, empty.back());
}

TEST(vector_clear_keeps_capacity) {
    Vector<int> v{1, 2, 3, 4};
    std::size_t cap = v.capacity();
    v.clear();
    CHECK(v.empty());
    CHECK(v.capacity() == cap);
}

TEST(vector_growth_is_geometric) {
    Vector<int> v;
    int reallocations = 0;
    std::size_t lastCap = v.capacity();
    for (int i = 0; i < 1000; ++i) {
        v.push_back(i);
        if (v.capacity() != lastCap) {
            ++reallocations;
            lastCap = v.capacity();
        }
    }
    CHECK(reallocations <= 11);        // doubling: 1, 2, 4, ..., 1024
}

TEST(vector_of_vectors) {
    Vector<Vector<int>> grid;
    for (int r = 0; r < 5; ++r) {
        grid.push_back(Vector<int>(3, r));
    }
    CHECK(grid.size() == 5);
    CHECK(grid[4][2] == 4);
    Vector<Vector<int>> copy = grid;
    copy[0][0] = 99;
    CHECK(grid[0][0] == 0);
}
