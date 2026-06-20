#ifndef OWNEDC_HPP
#define OWNEDC_HPP

#include "ownedc.h"
#include <utility>
#include <new>

namespace ownedc {

template <typename T>
class owned_ptr {
private:
    T* ptr_;

public:
    // Construct from raw pointer (assumes already allocated via owner_malloc)
    explicit owned_ptr(T* p = nullptr) : ptr_(p) {}

    // Allocate a new object natively and call its constructor
    template <typename... Args>
    static owned_ptr<T> make(Args&&... args) {
        T* raw = static_cast<T*>(owner_malloc(sizeof(T)));
        if (raw) {
            new (raw) T(std::forward<Args>(args)...);
        }
        return owned_ptr<T>(raw);
    }

    // Move semantics (transfer ownership)
    owned_ptr(owned_ptr&& other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }

    owned_ptr& operator=(owned_ptr&& other) noexcept {
        if (this != &other) {
            if (ptr_) owner_free(ptr_);
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    // Disable copy
    owned_ptr(const owned_ptr&) = delete;
    owned_ptr& operator=(const owned_ptr&) = delete;

    ~owned_ptr() {
        if (ptr_) {
            ptr_->~T();
            owner_free(ptr_);
        }
    }

    T* get() const { return ptr_; }
    T& operator*() const { return *ptr_; }
    T* operator->() const { return ptr_; }
    
    // Release ownership manually
    T* release() {
        T* p = ptr_;
        ptr_ = nullptr;
        return p;
    }
};

template <typename T>
class borrowed_ptr {
private:
    T* ptr_;

public:
    explicit borrowed_ptr(T* p) : ptr_(p) {
        if (ptr_) ownership_borrow(ptr_);
    }

    borrowed_ptr(const owned_ptr<T>& owner) : ptr_(owner.get()) {
        if (ptr_) ownership_borrow(ptr_);
    }

    // Copy semantics for borrows
    borrowed_ptr(const borrowed_ptr& other) : ptr_(other.ptr_) {
        if (ptr_) ownership_borrow(ptr_);
    }

    borrowed_ptr& operator=(const borrowed_ptr& other) {
        if (this != &other) {
            if (ptr_) ownership_release(ptr_);
            ptr_ = other.ptr_;
            if (ptr_) ownership_borrow(ptr_);
        }
        return *this;
    }

    ~borrowed_ptr() {
        if (ptr_) {
            ownership_release(ptr_);
        }
    }

    T* get() const { return ptr_; }
    T& operator*() const { return *ptr_; }
    T* operator->() const { return ptr_; }
};

} // namespace ownedc

#endif // OWNEDC_HPP
