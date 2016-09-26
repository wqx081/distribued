#ifndef EIGEN_FIXEDSIZEVECTOR_H
#define EIGEN_FIXEDSIZEVECTOR_H

#include "core/base/mem.h"

#include <glog/logging.h>

namespace eigen {

/** \class MaxSizeVector
  * \ingroup Core
  *
  * \brief The MaxSizeVector class.
  *
  * The %MaxSizeVector provides a subset of std::vector functionality.
  *
  * The goal is to provide basic std::vector operations when using
  * std::vector is not an option (e.g. on GPU or when compiling using
  * FMA/AVX, as this can cause either compilation failures or illegal
  * instruction failures).
  *
  * Beware: The constructors are not API compatible with these of
  * std::vector.
  */
template <typename T>
class MaxSizeVector {
 public:
  // Construct a new MaxSizeVector, reserve n elements.
  inline
  explicit MaxSizeVector(size_t n)
      : reserve_(n), size_(0),
        data_(static_cast<T*>(mr::aligned_malloc(n * sizeof(T), 16))) {
    for (size_t i = 0; i < n; ++i) { new (&data_[i]) T; }
  }

  // Construct a new MaxSizeVector, reserve and resize to n.
  // Copy the init value to all elements.
  inline
  MaxSizeVector(size_t n, const T& init)
      : reserve_(n), size_(n),
        data_(static_cast<T*>(mr::aligned_malloc(n * sizeof(T), 16))) {
    for (size_t i = 0; i < n; ++i) { new (&data_[i]) T(init); }
  }

  inline
  ~MaxSizeVector() {
    for (size_t i = 0; i < size_; ++i) {
      data_[i].~T();
    }
    mr::aligned_free(data_);
  }

  void resize(size_t n) {
    DCHECK(n <= reserve_);
    for (size_t i = size_; i < n; ++i) {
      new (&data_[i]) T;
    }
    for (size_t i = n; i < size_; ++i) {
      data_[i].~T();
    }
    size_ = n;
  }

  // Append new elements (up to reserved size).
  inline
  void push_back(const T& t) {
    DCHECK(size_ < reserve_);
    data_[size_++] = t;
  }

  inline
  const T& operator[] (size_t i) const {
    DCHECK(i < size_);
    return data_[i];
  }

  inline
  T& operator[] (size_t i) {
    DCHECK(i < size_);
    return data_[i];
  }

  inline
  T& back() {
    DCHECK(size_ > 0);
    return data_[size_ - 1];
  }

  inline
  const T& back() const {
    DCHECK(size_ > 0);
    return data_[size_ - 1];
  }

  inline
  void pop_back() {
    // NOTE: This does not destroy the value at the end the way
    // std::vector's version of pop_back() does.  That happens when
    // the Vector is destroyed.
    DCHECK(size_ > 0);
    size_--;
  }

  inline
  size_t size() const { return size_; }

  inline
  bool empty() const { return size_ == 0; }

  inline
  T* data() { return data_; }

  inline
  const T* data() const { return data_; }

  inline
  T* begin() { return data_; }

  inline
  T* end() { return data_ + size_; }

  inline
  const T* begin() const { return data_; }

  inline
  const T* end() const { return data_ + size_; }

 private:
  size_t reserve_;
  size_t size_;
  T* data_;
};

}  // namespace Eigen

#endif  // EIGEN_FIXEDSIZEVECTOR_H
