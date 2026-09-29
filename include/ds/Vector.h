#pragma once

#include <cstddef>
#include <initializer_list>
#include <new>
#include <stdexcept>
#include <utility>

namespace ds {

// A growable array, like std::vector.
//
// Memory model: the buffer is allocated as RAW bytes (::operator new), not with
// new T[n]. Elements are constructed in place (placement new) only when they
// are added, and destroyed explicitly when removed. This means:
//   - T does not need a default constructor
//   - unused capacity holds no live objects, so nothing is constructed for nothing
//
// Invariants (true after every public function):
//   - data_ points to raw storage for capacity_ elements, or is nullptr when capacity_ == 0
//   - slots [0, size_) hold live objects; slots [size_, capacity_) are uninitialized
//   - size_ <= capacity_
template <typename T>
class Vector {
public:
    // ---------- construction and destruction (Rule of Five) ----------
    Vector() = default;
    Vector(std::size_t count, const T& value);
    Vector(std::initializer_list<T> init);

    Vector(const Vector& other);
    Vector& operator=(const Vector& other);
    Vector(Vector&& other) noexcept;
    Vector& operator=(Vector&& other) noexcept;
    ~Vector();

    // ---------- modifiers ----------
    void push_back(const T& value);
    void push_back(T&& value);
    template <typename... Args>
    T& emplace_back(Args&&... args);   // constructs the element in place
    void pop_back();                   // throws std::out_of_range if empty
    void clear() noexcept;             // destroys elements, keeps capacity
    void reserve(std::size_t newCapacity);

    // ---------- element access ----------
    T& operator[](std::size_t index) { return data_[index]; }
    const T& operator[](std::size_t index) const { return data_[index]; }
    T& at(std::size_t index);          // bounds-checked, throws std::out_of_range
    const T& at(std::size_t index) const;
    T& back();                         // throws std::out_of_range if empty

    // ---------- capacity ----------
    std::size_t size() const noexcept { return size_; }
    std::size_t capacity() const noexcept { return capacity_; }
    bool empty() const noexcept { return size_ == 0; }

    // ---------- iteration (raw pointers work as random-access iterators) ----------
    T* begin() noexcept { return data_; }
    T* end() noexcept { return data_ + size_; }
    const T* begin() const noexcept { return data_; }
    const T* end() const noexcept { return data_ + size_; }

    void swap(Vector& other) noexcept;

private:
    static T* allocate(std::size_t count);
    static void deallocate(T* ptr) noexcept;
    void destroyAll() noexcept;        // destroys live elements, frees buffer, resets to empty
    void growIfFull();

    T* data_ = nullptr;
    std::size_t size_ = 0;
    std::size_t capacity_ = 0;
};

// =====================================================================
// Definitions
// Templates live entirely in the header. See README "Why no .cpp files?"
// =====================================================================

// ---------- private helpers ----------

template <typename T>
T* Vector<T>::allocate(std::size_t count) {
    if (count == 0) {
        return nullptr;
    }
    // Raw, correctly-aligned bytes. No T objects are created here.
    return static_cast<T*>(::operator new(count * sizeof(T)));
}

template <typename T>
void Vector<T>::deallocate(T* ptr) noexcept {
    ::operator delete(ptr);
}

template <typename T>
void Vector<T>::destroyAll() noexcept {
    clear();
    deallocate(data_);
    data_ = nullptr;
    capacity_ = 0;
}

template <typename T>
void Vector<T>::growIfFull() {
    if (size_ == capacity_) {
        // Doubling makes push_back amortized O(1): the total cost of all the
        // copies across n pushes is at most about 2n.
        reserve(capacity_ == 0 ? 1 : capacity_ * 2);
    }
}

// ---------- constructors ----------

template <typename T>
Vector<T>::Vector(std::size_t count, const T& value) {
    reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        push_back(value);
    }
}

template <typename T>
Vector<T>::Vector(std::initializer_list<T> init) {
    reserve(init.size());
    for (const T& item : init) {
        push_back(item);
    }
}

// Rule-of-Five members below would be leaky if a push_back threw halfway
// through a constructor, because the destructor doesn't run for an object
// whose constructor didn't finish. The try/catch cleans up in that case.

template <typename T>
Vector<T>::Vector(const Vector& other) {
    reserve(other.size_);
    try {
        for (const T& item : other) {
            push_back(item);
        }
    } catch (...) {
        destroyAll();
        throw;
    }
}

template <typename T>
Vector<T>& Vector<T>::operator=(const Vector& other) {
    // Copy-and-swap: build the copy first, then swap it in. If copying throws,
    // *this is untouched. The old contents die with `copy`. Self-assignment is
    // handled automatically (it just makes a copy of itself).
    if (this != &other) {
        Vector copy(other);
        swap(copy);
    }
    return *this;
}

template <typename T>
Vector<T>::Vector(Vector&& other) noexcept
    : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
    // Steal the buffer, then leave `other` empty but valid.
    other.data_ = nullptr;
    other.size_ = 0;
    other.capacity_ = 0;
}

template <typename T>
Vector<T>& Vector<T>::operator=(Vector&& other) noexcept {
    if (this != &other) {
        destroyAll();
        data_ = other.data_;
        size_ = other.size_;
        capacity_ = other.capacity_;
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }
    return *this;
}

template <typename T>
Vector<T>::~Vector() {
    destroyAll();
}

template <typename T>
void Vector<T>::swap(Vector& other) noexcept {
    std::swap(data_, other.data_);
    std::swap(size_, other.size_);
    std::swap(capacity_, other.capacity_);
}

// ---------- modifiers ----------

template <typename T>
void Vector<T>::push_back(const T& value) {
    emplace_back(value);
}

template <typename T>
void Vector<T>::push_back(T&& value) {
    emplace_back(std::move(value));
}

template <typename T>
template <typename... Args>
T& Vector<T>::emplace_back(Args&&... args) {
    if (size_ == capacity_) {
        // Edge case: v.push_back(v[0]) passes a reference INTO our own buffer.
        // Growing would free that buffer before we read from it. So build the
        // new element first, then grow, then move it into place.
        T temp(std::forward<Args>(args)...);
        growIfFull();
        new (data_ + size_) T(std::move(temp));
    } else {
        // Placement new: construct a T in the already-allocated slot.
        new (data_ + size_) T(std::forward<Args>(args)...);
    }
    return data_[size_++];
}

template <typename T>
void Vector<T>::pop_back() {
    if (size_ == 0) {
        throw std::out_of_range("Vector::pop_back on empty vector");
    }
    --size_;
    data_[size_].~T();   // explicit destructor call; the memory stays allocated
}

template <typename T>
void Vector<T>::clear() noexcept {
    // Destroy in reverse order, mirroring how C++ destroys local variables.
    while (size_ > 0) {
        --size_;
        data_[size_].~T();
    }
}

template <typename T>
void Vector<T>::reserve(std::size_t newCapacity) {
    if (newCapacity <= capacity_) {
        return;   // never shrink
    }

    T* newData = allocate(newCapacity);
    std::size_t moved = 0;
    try {
        for (; moved < size_; ++moved) {
            // std::move_if_noexcept moves when the move can't throw, otherwise
            // copies. If a copy throws, the old buffer is still intact.
            new (newData + moved) T(std::move_if_noexcept(data_[moved]));
        }
    } catch (...) {
        for (std::size_t i = 0; i < moved; ++i) {
            newData[i].~T();
        }
        deallocate(newData);
        throw;
    }

    // Destroy the (now moved-from) originals and release the old buffer.
    for (std::size_t i = 0; i < size_; ++i) {
        data_[i].~T();
    }
    deallocate(data_);

    data_ = newData;
    capacity_ = newCapacity;
}

// ---------- element access ----------

template <typename T>
T& Vector<T>::at(std::size_t index) {
    if (index >= size_) {
        throw std::out_of_range("Vector::at index out of range");
    }
    return data_[index];
}

template <typename T>
const T& Vector<T>::at(std::size_t index) const {
    if (index >= size_) {
        throw std::out_of_range("Vector::at index out of range");
    }
    return data_[index];
}

template <typename T>
T& Vector<T>::back() {
    if (size_ == 0) {
        throw std::out_of_range("Vector::back on empty vector");
    }
    return data_[size_ - 1];
}

} // namespace ds
