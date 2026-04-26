#pragma once
#ifndef MEIKA_SPAN
#define MEIKA_SPAN

#include <array>
#include <vector>

#include <cstddef>
#include <type_traits>

namespace meika {
template <typename T> struct span {
    // simulate STL style
    typedef T element_type;
    typedef T *pointer;
    typedef const T *const_pointer;
    typedef T &reference;
    typedef const T &const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
    typedef pointer iterator;
    typedef const_pointer const_iterator;
    pointer data_;
    size_type size_;

  public:
    // constructors
    span() noexcept : data_(nullptr), size_(0) {}
    span(pointer ptr, size_type count) noexcept : data_(ptr), size_(count) {}

    // vector
    template <typename U, typename = typename std::enable_if<
                              std::is_convertible<U *, T *>::value>::type>
    span(std::vector<U> &v) noexcept : data_(v.data()), size_(v.size()) {}
    template <typename U, typename = typename std::enable_if<
                              std::is_convertible<const U *, T *>::value>::type>
    span(const std::vector<U> &v) noexcept : data_(v.data()), size_(v.size()) {}

    // array
    template <typename U, std::size_t N,
              typename = typename std::enable_if<
                  std::is_convertible<U *, T *>::value>::type>
    span(std::array<U, N> &arr) noexcept : data_(arr.data()), size_(N) {}
    template <typename U, std::size_t N,
              typename = typename std::enable_if<
                  std::is_convertible<const U *, T *>::value>::type>
    span(const std::array<T, N> &arr) noexcept : data_(arr.data()), size_(N) {}

    // c-style array
    template <typename U, std::size_t N,
              typename = typename std::enable_if<
                  std::is_convertible<U *, T *>::value>::type>
    span(U (&arr)[N]) noexcept : data_(arr), size_(N) {}
    template <typename U, std::size_t N,
              typename = typename std::enable_if<
                  std::is_convertible<const U *, T *>::value>::type>
    span(const U (&arr)[N]) noexcept : data_(arr), size_(N) {}

    // iterators
    iterator begin() const noexcept { return data_; }
    pointer end() const noexcept { return data_ + size_; }

    // visit
    reference operator[](size_type idx) const noexcept { return data_[idx]; }
    reference front() const noexcept { return data_[0]; }
    reference back() const noexcept { return data_[size_ - 1]; }

    // properties
    pointer data() const noexcept { return data_; }
    size_type size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }

    // subview
    span subspan(size_type offset, size_type count = -1) const noexcept {
        if (offset > size_) offset = size_;
        if (count == static_cast<size_type>(-1) || offset + count > size_) {
            count = size_ - offset;
        }
        return span(data_ + offset, count);
    }
};
} // namespace meika

#endif
