#pragma once

#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace ds {

// A singly linked list with a tail pointer, so push_front and push_back are
// both O(1). Supports range-based for through a forward iterator.
//
// Invariants:
//   - head_ == nullptr  <=>  tail_ == nullptr  <=>  size_ == 0
//   - tail_->next == nullptr whenever the list is non-empty
//   - following next from head_ reaches tail_ after exactly size_ - 1 steps
template <typename T>
class LinkedList {
private:
    struct Node {
        T value;
        Node* next = nullptr;

        explicit Node(const T& v) : value(v) {}
        explicit Node(T&& v) : value(std::move(v)) {}
    };

    // One iterator template serves both iterator (IsConst = false) and
    // const_iterator (IsConst = true).
    template <bool IsConst>
    class Iter {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = std::conditional_t<IsConst, const T*, T*>;
        using reference = std::conditional_t<IsConst, const T&, T&>;

        Iter() = default;
        explicit Iter(Node* node) : node_(node) {}

        // Lets an iterator convert to a const_iterator, not the other way round.
        template <bool WasConst, typename = std::enable_if_t<IsConst && !WasConst>>
        Iter(const Iter<WasConst>& other) : node_(other.node_) {}

        reference operator*() const { return node_->value; }
        pointer operator->() const { return &node_->value; }

        Iter& operator++() {            // ++it
            node_ = node_->next;
            return *this;
        }
        Iter operator++(int) {          // it++
            Iter old = *this;
            node_ = node_->next;
            return old;
        }

        bool operator==(const Iter& other) const { return node_ == other.node_; }
        bool operator!=(const Iter& other) const { return node_ != other.node_; }

    private:
        friend class LinkedList;
        template <bool> friend class Iter;   // lets Iter<false> -> Iter<true> read node_
        Node* node_ = nullptr;
    };

public:
    using iterator = Iter<false>;
    using const_iterator = Iter<true>;

    // ---------- construction and destruction (Rule of Five) ----------
    LinkedList() = default;

    LinkedList(const LinkedList& other);
    LinkedList& operator=(const LinkedList& other);
    LinkedList(LinkedList&& other) noexcept;
    LinkedList& operator=(LinkedList&& other) noexcept;
    ~LinkedList();

    // ---------- modifiers ----------
    void push_front(const T& value);
    void push_back(const T& value);
    void push_back(T&& value);
    void pop_front();                   // throws std::out_of_range if empty
    bool remove(const T& value);        // removes first match, returns true if found
    void clear() noexcept;
    void reverse() noexcept;            // reverses the list in place, O(n), no allocation

    // ---------- access ----------
    T& front();                         // throws std::out_of_range if empty
    T& back();                          // throws std::out_of_range if empty
    bool contains(const T& value) const;

    // ---------- size ----------
    std::size_t size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }

    // ---------- iteration ----------
    iterator begin() noexcept { return iterator(head_); }
    iterator end() noexcept { return iterator(nullptr); }
    const_iterator begin() const noexcept { return const_iterator(head_); }
    const_iterator end() const noexcept { return const_iterator(nullptr); }

    // Kept for convenience: list.forEach([](const int& x) { ... });
    template <typename Fn>
    void forEach(Fn fn) const {
        for (const T& value : *this) {
            fn(value);
        }
    }

    void swap(LinkedList& other) noexcept;

private:
    void linkBack(Node* node) noexcept;

    Node* head_ = nullptr;
    Node* tail_ = nullptr;
    std::size_t size_ = 0;
};

// =====================================================================
// Definitions
// =====================================================================

// ---------- private helpers ----------

template <typename T>
void LinkedList<T>::linkBack(Node* node) noexcept {
    if (tail_ == nullptr) {
        head_ = node;       // list was empty: new node is both head and tail
    } else {
        tail_->next = node;
    }
    tail_ = node;
    ++size_;
}

// ---------- Rule of Five ----------

template <typename T>
LinkedList<T>::LinkedList(const LinkedList& other) {
    try {
        for (const T& value : other) {
            push_back(value);
        }
    } catch (...) {
        clear();   // the destructor won't run if the constructor throws
        throw;
    }
}

template <typename T>
LinkedList<T>& LinkedList<T>::operator=(const LinkedList& other) {
    // Copy-and-swap: safe on self-assignment and if copying throws.
    if (this != &other) {
        LinkedList copy(other);
        swap(copy);
    }
    return *this;
}

template <typename T>
LinkedList<T>::LinkedList(LinkedList&& other) noexcept
    : head_(other.head_), tail_(other.tail_), size_(other.size_) {
    other.head_ = nullptr;
    other.tail_ = nullptr;
    other.size_ = 0;
}

template <typename T>
LinkedList<T>& LinkedList<T>::operator=(LinkedList&& other) noexcept {
    if (this != &other) {
        clear();
        head_ = other.head_;
        tail_ = other.tail_;
        size_ = other.size_;
        other.head_ = nullptr;
        other.tail_ = nullptr;
        other.size_ = 0;
    }
    return *this;
}

template <typename T>
LinkedList<T>::~LinkedList() {
    clear();
}

template <typename T>
void LinkedList<T>::swap(LinkedList& other) noexcept {
    std::swap(head_, other.head_);
    std::swap(tail_, other.tail_);
    std::swap(size_, other.size_);
}

// ---------- modifiers ----------

template <typename T>
void LinkedList<T>::push_front(const T& value) {
    Node* node = new Node(value);
    node->next = head_;
    head_ = node;
    if (tail_ == nullptr) {
        tail_ = node;       // list was empty
    }
    ++size_;
}

template <typename T>
void LinkedList<T>::push_back(const T& value) {
    linkBack(new Node(value));
}

template <typename T>
void LinkedList<T>::push_back(T&& value) {
    linkBack(new Node(std::move(value)));
}

template <typename T>
void LinkedList<T>::pop_front() {
    if (head_ == nullptr) {
        throw std::out_of_range("LinkedList::pop_front on empty list");
    }
    Node* old = head_;
    head_ = head_->next;
    if (head_ == nullptr) {
        tail_ = nullptr;    // removed the last node
    }
    delete old;
    --size_;
}

template <typename T>
bool LinkedList<T>::remove(const T& value) {
    Node* prev = nullptr;
    for (Node* cur = head_; cur != nullptr; prev = cur, cur = cur->next) {
        if (!(cur->value == value)) {
            continue;
        }

        // Unlink cur. Either it's the head (no prev) or prev skips over it.
        if (prev == nullptr) {
            head_ = cur->next;
        } else {
            prev->next = cur->next;
        }
        // If cur was the tail, the new tail is the node before it
        // (nullptr if cur was the only node).
        if (cur == tail_) {
            tail_ = prev;
        }

        delete cur;
        --size_;
        return true;
    }
    return false;
}

template <typename T>
void LinkedList<T>::clear() noexcept {
    Node* cur = head_;
    while (cur != nullptr) {
        Node* next = cur->next;   // save before deleting cur
        delete cur;
        cur = next;
    }
    head_ = nullptr;
    tail_ = nullptr;
    size_ = 0;
}

template <typename T>
void LinkedList<T>::reverse() noexcept {
    // Classic three-pointer reversal: flip each next pointer as we walk.
    Node* prev = nullptr;
    Node* cur = head_;
    tail_ = head_;          // the old head becomes the new tail
    while (cur != nullptr) {
        Node* next = cur->next;
        cur->next = prev;
        prev = cur;
        cur = next;
    }
    head_ = prev;
}

// ---------- access ----------

template <typename T>
T& LinkedList<T>::front() {
    if (head_ == nullptr) {
        throw std::out_of_range("LinkedList::front on empty list");
    }
    return head_->value;
}

template <typename T>
T& LinkedList<T>::back() {
    if (tail_ == nullptr) {
        throw std::out_of_range("LinkedList::back on empty list");
    }
    return tail_->value;
}

template <typename T>
bool LinkedList<T>::contains(const T& value) const {
    for (const T& item : *this) {
        if (item == value) {
            return true;
        }
    }
    return false;
}

} // namespace ds
