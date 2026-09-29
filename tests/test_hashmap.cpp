#include <cstddef>
#include <stdexcept>
#include <string>

#include "ds/HashMap.h"
#include "test.h"

using ds::HashMap;

// A deliberately terrible hash: every key lands in the same bucket.
// If your map still works with this, your collision handling is right.
struct AlwaysZeroHash {
    std::size_t operator()(int) const { return 0; }
};

TEST(map_default_is_empty) {
    HashMap<std::string, int> m;
    CHECK(m.empty());
    CHECK(m.bucketCount() > 0);
}

TEST(map_insert_and_find) {
    HashMap<std::string, int> m;
    m.insert("apple", 3);
    m.insert("pear", 5);
    CHECK(m.size() == 2);
    CHECK(m.find("apple") != nullptr && *m.find("apple") == 3);
    CHECK(*m.find("pear") == 5);
    CHECK(m.find("plum") == nullptr);
}

TEST(map_insert_overwrites_existing_key) {
    HashMap<std::string, int> m;
    m.insert("apple", 3);
    m.insert("apple", 10);
    CHECK(m.size() == 1);
    CHECK(*m.find("apple") == 10);
}

TEST(map_contains) {
    HashMap<int, int> m;
    m.insert(1, 100);
    CHECK(m.contains(1));
    CHECK(!m.contains(2));
}

TEST(map_erase) {
    HashMap<int, int> m;
    m.insert(1, 100);
    m.insert(2, 200);
    CHECK(m.erase(1));
    CHECK(!m.contains(1));
    CHECK(m.contains(2));
    CHECK(m.size() == 1);
    CHECK(!m.erase(1));   // already gone
}

TEST(map_bracket_inserts_default) {
    HashMap<std::string, int> m;
    m["hits"] += 1;
    m["hits"] += 1;
    CHECK(m["hits"] == 2);
    CHECK(m["misses"] == 0);
    CHECK(m.size() == 2);
}

TEST(map_const_find) {
    HashMap<int, int> m;
    m.insert(5, 50);
    const HashMap<int, int>& cm = m;
    CHECK(cm.find(5) != nullptr && *cm.find(5) == 50);
    CHECK(cm.find(6) == nullptr);
}

TEST(map_rehash_keeps_everything) {
    HashMap<int, int> m(4);
    for (int i = 0; i < 1000; ++i) {
        m.insert(i, i * i);
    }
    CHECK(m.size() == 1000);
    CHECK(m.bucketCount() > 4);                         // it grew
    CHECK(m.loadFactor() <= HashMap<int, int>::kMaxLoadFactor);
    for (int i = 0; i < 1000; ++i) {
        CHECK(m.find(i) != nullptr && *m.find(i) == i * i);
    }
}

TEST(map_bracket_survives_rehash) {
    HashMap<int, int> m(2);
    for (int i = 0; i < 50; ++i) {
        m[i] = i;                                       // many of these trigger a rehash
    }
    for (int i = 0; i < 50; ++i) {
        CHECK(*m.find(i) == i);
    }
}

TEST(map_handles_collisions) {
    HashMap<int, std::string, AlwaysZeroHash> m;
    m.insert(1, "one");
    m.insert(2, "two");
    m.insert(3, "three");
    CHECK(*m.find(2) == "two");
    CHECK(m.erase(2));
    CHECK(*m.find(1) == "one" && *m.find(3) == "three");
    CHECK(m.find(2) == nullptr);
}

TEST(map_copy_is_deep) {
    HashMap<int, int> a;
    a.insert(1, 1);
    HashMap<int, int> b = a;   // compiler-generated copy (Rule of Zero)
    b.insert(1, 99);
    CHECK(*a.find(1) == 1);
}

// ---------- beyond the basics ----------

TEST(map_at_throws_when_missing) {
    HashMap<std::string, int> m;
    m.insert("a", 1);
    CHECK(m.at("a") == 1);
    CHECK_THROWS(std::out_of_range, m.at("b"));
}

TEST(map_clear) {
    HashMap<int, int> m;
    for (int i = 0; i < 20; ++i) {
        m.insert(i, i);
    }
    m.clear();
    CHECK(m.empty());
    CHECK(!m.contains(5));
    m.insert(5, 50);
    CHECK(m.at(5) == 50);
}

TEST(map_for_each_visits_everything) {
    HashMap<int, int> m;
    for (int i = 1; i <= 100; ++i) {
        m.insert(i, i);
    }
    int count = 0;
    long sum = 0;
    m.forEach([&](const int&, const int& v) {
        ++count;
        sum += v;
    });
    CHECK(count == 100);
    CHECK(sum == 5050);
}

TEST(map_references_survive_rehash) {
    HashMap<int, std::string> m(2);
    std::string& first = m[0];
    first = "zero";
    for (int i = 1; i < 500; ++i) {
        m[i] = "x";                      // forces many rehashes
    }
    CHECK(&first == m.find(0));          // same object, never moved
    CHECK(first == "zero");
}

TEST(map_word_count) {
    const char* words[] = {"the", "cat", "sat", "on", "the", "mat", "the"};
    HashMap<std::string, int> counts;
    for (const char* w : words) {
        ++counts[w];
    }
    CHECK(counts.at("the") == 3);
    CHECK(counts.at("cat") == 1);
    CHECK(counts.size() == 5);
}
