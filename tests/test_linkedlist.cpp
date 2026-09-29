#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Tracker.h"
#include "ds/LinkedList.h"
#include "test.h"

using ds::LinkedList;

// Collects the list's values into a std::vector so tests can compare easily.
template <typename T>
static std::vector<T> toStd(const LinkedList<T>& list) {
    std::vector<T> out;
    list.forEach([&](const T& x) { out.push_back(x); });
    return out;
}

// ---------- basics ----------

TEST(list_default_is_empty) {
    LinkedList<int> l;
    CHECK(l.size() == 0);
    CHECK(l.empty());
}

TEST(list_push_back_keeps_order) {
    LinkedList<int> l;
    l.push_back(1);
    l.push_back(2);
    l.push_back(3);
    CHECK((toStd(l) == std::vector<int>{1, 2, 3}));
    CHECK(l.front() == 1);
    CHECK(l.back() == 3);
}

TEST(list_push_front_reverses_order) {
    LinkedList<int> l;
    l.push_front(1);
    l.push_front(2);
    l.push_front(3);
    CHECK((toStd(l) == std::vector<int>{3, 2, 1}));
    CHECK(l.back() == 1);
}

TEST(list_push_front_then_back_on_empty) {
    LinkedList<int> l;
    l.push_front(1);   // tail_ must be set here too
    l.push_back(2);
    CHECK((toStd(l) == std::vector<int>{1, 2}));
}

TEST(list_pop_front) {
    LinkedList<int> l;
    l.push_back(1);
    l.push_back(2);
    l.pop_front();
    CHECK(l.front() == 2);
    CHECK(l.size() == 1);
    l.pop_front();
    CHECK(l.empty());
    l.push_back(3);    // tail_ must have been reset, or this breaks
    CHECK(l.front() == 3 && l.back() == 3);
}

TEST(list_empty_access_throws) {
    LinkedList<int> l;
    CHECK_THROWS(std::out_of_range, l.front());
    CHECK_THROWS(std::out_of_range, l.back());
    CHECK_THROWS(std::out_of_range, l.pop_front());
}

TEST(list_contains) {
    LinkedList<int> l;
    l.push_back(4);
    l.push_back(8);
    CHECK(l.contains(8));
    CHECK(!l.contains(5));
}

TEST(list_remove_middle_head_tail) {
    LinkedList<int> l;
    for (int i = 1; i <= 5; ++i) {
        l.push_back(i);
    }
    CHECK(l.remove(3));                                    // middle
    CHECK((toStd(l) == std::vector<int>{1, 2, 4, 5}));
    CHECK(l.remove(1));                                    // head
    CHECK(l.front() == 2);
    CHECK(l.remove(5));                                    // tail
    CHECK(l.back() == 4);
    l.push_back(6);                                        // tail_ still correct?
    CHECK((toStd(l) == std::vector<int>{2, 4, 6}));
    CHECK(!l.remove(42));
    CHECK(l.size() == 3);
}

TEST(list_remove_only_node) {
    LinkedList<int> l;
    l.push_back(7);
    CHECK(l.remove(7));
    CHECK(l.empty());
    l.push_back(8);
    CHECK(l.front() == 8 && l.back() == 8);
}

TEST(list_clear) {
    LinkedList<std::string> l;
    l.push_back("a");
    l.push_back("b");
    l.clear();
    CHECK(l.empty());
    l.push_back("c");
    CHECK(l.front() == "c");
}

// ---------- Rule of Five ----------

TEST(list_copy_constructor_is_deep) {
    LinkedList<int> a;
    a.push_back(1);
    a.push_back(2);
    LinkedList<int> b(a);
    b.front() = 99;
    CHECK(a.front() == 1);
    CHECK((toStd(b) == std::vector<int>{99, 2}));
}

TEST(list_copy_assignment_is_deep) {
    LinkedList<int> a;
    a.push_back(1);
    LinkedList<int> b;
    b.push_back(5);
    b.push_back(6);
    b = a;
    b.front() = 99;
    CHECK(a.front() == 1);
    CHECK(b.size() == 1);
}

TEST(list_self_assignment_is_safe) {
    LinkedList<int> a;
    a.push_back(1);
    LinkedList<int>& alias = a;
    a = alias;
    CHECK(a.size() == 1 && a.front() == 1);
}

TEST(list_move_constructor) {
    LinkedList<int> a;
    a.push_back(1);
    a.push_back(2);
    LinkedList<int> b(std::move(a));
    CHECK((toStd(b) == std::vector<int>{1, 2}));
    CHECK(a.empty());
    a.push_back(3);
    CHECK(a.front() == 3);
}

TEST(list_move_assignment) {
    LinkedList<int> a;
    a.push_back(1);
    LinkedList<int> b;
    b.push_back(9);
    b = std::move(a);
    CHECK(b.size() == 1 && b.front() == 1);
    CHECK(a.empty());
}

TEST(list_no_leaks) {
    Tracker::reset();
    {
        LinkedList<Tracker> l;
        for (int i = 0; i < 10; ++i) {
            l.push_back(Tracker(i));
        }
        l.remove(Tracker(3));
        l.pop_front();
        LinkedList<Tracker> copy(l);
        LinkedList<Tracker> moved(std::move(copy));
    }
    CHECK(Tracker::live == 0);
}

// ---------- iterators and reverse ----------

TEST(list_range_for) {
    LinkedList<int> l;
    for (int i = 1; i <= 4; ++i) {
        l.push_back(i);
    }
    int sum = 0;
    for (int x : l) {
        sum += x;
    }
    CHECK(sum == 10);
}

TEST(list_iterator_can_modify) {
    LinkedList<int> l;
    l.push_back(1);
    l.push_back(2);
    for (int& x : l) {
        x *= 10;
    }
    CHECK((toStd(l) == std::vector<int>{10, 20}));
}

TEST(list_const_iteration) {
    LinkedList<std::string> l;
    l.push_back("a");
    l.push_back("b");
    const LinkedList<std::string>& cl = l;
    std::string joined;
    for (auto it = cl.begin(); it != cl.end(); ++it) {
        joined += *it;
    }
    CHECK(joined == "ab");
    LinkedList<std::string>::const_iterator converted = l.begin();  // iterator -> const_iterator
    CHECK(*converted == "a");
}

TEST(list_reverse) {
    LinkedList<int> l;
    for (int i = 1; i <= 5; ++i) {
        l.push_back(i);
    }
    l.reverse();
    CHECK((toStd(l) == std::vector<int>{5, 4, 3, 2, 1}));
    CHECK(l.front() == 5 && l.back() == 1);
    l.push_back(0);                                   // tail_ must be correct after reversing
    CHECK((toStd(l) == std::vector<int>{5, 4, 3, 2, 1, 0}));
}

TEST(list_reverse_empty_and_single) {
    LinkedList<int> empty;
    empty.reverse();
    CHECK(empty.empty());
    LinkedList<int> one;
    one.push_back(7);
    one.reverse();
    CHECK(one.front() == 7 && one.back() == 7);
}

TEST(list_push_back_rvalue_moves) {
    Tracker::reset();
    {
        LinkedList<Tracker> l;
        l.push_back(Tracker(1));
        CHECK(Tracker::copies == 0);
    }
    CHECK(Tracker::live == 0);
}
