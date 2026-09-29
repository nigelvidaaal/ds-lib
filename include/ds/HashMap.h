#pragma once

#include <cstddef>
#include <functional>
#include <list>
#include <stdexcept>
#include <utility>
#include <vector>

namespace ds {

// A hash map using separate chaining: an array of buckets, where each bucket
// is a list of (key, value) pairs whose keys hashed to that bucket.
//
// Buckets use std::vector and std::list on purpose. Because those already
// manage their own memory, this class needs NO destructor, copy, or move
// functions of its own. That's the "Rule of Zero" (see README).
//
// Average cost of insert / find / erase is O(1), as long as the load factor
// (elements per bucket) stays bounded. The map doubles its bucket count
// whenever the load factor passes kMaxLoadFactor.
template <typename K, typename V, typename Hash = std::hash<K>>
class HashMap {
public:
    explicit HashMap(std::size_t initialBuckets = 8)
        : buckets_(initialBuckets == 0 ? 1 : initialBuckets) {}

    // ---------- modifiers ----------
    void insert(const K& key, const V& value);  // adds, or overwrites if key exists
    bool erase(const K& key);                   // returns true if something was removed
    V& operator[](const K& key);                // inserts V{} if key is missing, like std::unordered_map
    void clear() noexcept;                      // removes all entries, keeps bucket count

    // ---------- lookup ----------
    V* find(const K& key);                      // nullptr if missing
    const V* find(const K& key) const;
    bool contains(const K& key) const { return find(key) != nullptr; }
    V& at(const K& key);                        // throws std::out_of_range if missing
    const V& at(const K& key) const;

    // ---------- iteration (order is unspecified, as in any hash map) ----------
    // Usage: map.forEach([](const std::string& k, int v) { ... });
    template <typename Fn>
    void forEach(Fn fn) const {
        for (const Bucket& bucket : buckets_) {
            for (const auto& entry : bucket) {
                fn(entry.first, entry.second);
            }
        }
    }

    // ---------- size and stats ----------
    std::size_t size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }
    std::size_t bucketCount() const noexcept { return buckets_.size(); }
    double loadFactor() const noexcept { return static_cast<double>(size_) / static_cast<double>(buckets_.size()); }

    static constexpr double kMaxLoadFactor = 0.75;

private:
    using Entry = std::pair<K, V>;
    using Bucket = std::list<Entry>;

    std::size_t bucketIndex(const K& key) const;
    std::size_t bucketIndex(const K& key, std::size_t bucketCount) const;
    Entry* findEntry(const K& key);
    void rehash(std::size_t newBucketCount);
    void rehashIfNeeded();

    std::vector<Bucket> buckets_;
    std::size_t size_ = 0;
    Hash hasher_;
};

// =====================================================================
// Definitions
// =====================================================================

// ---------- private helpers ----------

template <typename K, typename V, typename Hash>
std::size_t HashMap<K, V, Hash>::bucketIndex(const K& key, std::size_t bucketCount) const {
    // hasher_ returns any size_t; modulo squashes it into [0, bucketCount).
    return hasher_(key) % bucketCount;
}

template <typename K, typename V, typename Hash>
std::size_t HashMap<K, V, Hash>::bucketIndex(const K& key) const {
    return bucketIndex(key, buckets_.size());
}

template <typename K, typename V, typename Hash>
typename HashMap<K, V, Hash>::Entry* HashMap<K, V, Hash>::findEntry(const K& key) {
    // Only the key's own bucket can contain it, so we search just that list.
    for (Entry& entry : buckets_[bucketIndex(key)]) {
        if (entry.first == key) {
            return &entry;
        }
    }
    return nullptr;
}

template <typename K, typename V, typename Hash>
void HashMap<K, V, Hash>::rehashIfNeeded() {
    if (loadFactor() > kMaxLoadFactor) {
        rehash(buckets_.size() * 2);
    }
}

template <typename K, typename V, typename Hash>
void HashMap<K, V, Hash>::rehash(std::size_t newBucketCount) {
    std::vector<Bucket> newBuckets(newBucketCount);
    for (Bucket& bucket : buckets_) {
        // splice relinks each list node into its new bucket without copying
        // or reallocating the key/value pair. References to values stay valid.
        while (!bucket.empty()) {
            Bucket& target = newBuckets[bucketIndex(bucket.front().first, newBucketCount)];
            target.splice(target.end(), bucket, bucket.begin());
        }
    }
    buckets_.swap(newBuckets);
}

// ---------- modifiers ----------

template <typename K, typename V, typename Hash>
void HashMap<K, V, Hash>::insert(const K& key, const V& value) {
    if (Entry* existing = findEntry(key)) {
        existing->second = value;   // key already present: overwrite
        return;
    }
    buckets_[bucketIndex(key)].emplace_back(key, value);
    ++size_;
    rehashIfNeeded();
}

template <typename K, typename V, typename Hash>
bool HashMap<K, V, Hash>::erase(const K& key) {
    Bucket& bucket = buckets_[bucketIndex(key)];
    for (auto it = bucket.begin(); it != bucket.end(); ++it) {
        if (it->first == key) {
            bucket.erase(it);
            --size_;
            return true;
        }
    }
    return false;
}

template <typename K, typename V, typename Hash>
V& HashMap<K, V, Hash>::operator[](const K& key) {
    if (Entry* existing = findEntry(key)) {
        return existing->second;
    }
    Bucket& bucket = buckets_[bucketIndex(key)];
    bucket.emplace_back(key, V{});
    V& value = bucket.back().second;
    ++size_;
    // Safe to rehash AFTER taking the reference: rehash() splices list nodes
    // rather than copying them, so `value` still points at the same object.
    rehashIfNeeded();
    return value;
}

template <typename K, typename V, typename Hash>
void HashMap<K, V, Hash>::clear() noexcept {
    for (Bucket& bucket : buckets_) {
        bucket.clear();
    }
    size_ = 0;
}

// ---------- lookup ----------

template <typename K, typename V, typename Hash>
V* HashMap<K, V, Hash>::find(const K& key) {
    Entry* entry = findEntry(key);
    return entry ? &entry->second : nullptr;
}

template <typename K, typename V, typename Hash>
const V* HashMap<K, V, Hash>::find(const K& key) const {
    // Reuse the non-const version. Safe because findEntry doesn't modify anything.
    return const_cast<HashMap*>(this)->find(key);
}

template <typename K, typename V, typename Hash>
V& HashMap<K, V, Hash>::at(const K& key) {
    if (V* value = find(key)) {
        return *value;
    }
    throw std::out_of_range("HashMap::at key not found");
}

template <typename K, typename V, typename Hash>
const V& HashMap<K, V, Hash>::at(const K& key) const {
    if (const V* value = find(key)) {
        return *value;
    }
    throw std::out_of_range("HashMap::at key not found");
}

} // namespace ds
