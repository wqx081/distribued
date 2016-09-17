// StringPiece 主要用于减少参数传递拷贝以及strlen 函数的调用
//
//
// 它主要用作函数或者方法的参数类型
//
#ifndef CORE_STRINGS_STRING_PIECE_H_
#define CORE_STRINGS_STRING_PIECE_H_

#include <stddef.h>
#include <string.h>

#include <iosfwd>
#include <string>

namespace mr {

class StringPiece {
 public:
  // For Standard STL container
  using size_type = size_t;
  using value_type = std::string::value_type;
  using pointer = const value_type*;
  using reference = const value_type&;
  using const_reference = const value_type&;
  using difference_type = ptrdiff_t;
  using const_iterator = const value_type*;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  
  static const size_type npos;

 public:
  StringPiece() 
	  : ptr_(nullptr), length_(0) {}
  StringPiece(const value_type* str)
	  : ptr_(str),
	    length_((str == nullptr) ? 0 : std::string::traits_type::length(str)) {}
  StringPiece(const std::string& str)
	  : ptr_(str.data()),
	    length_(str.size()) {}
  StringPiece(const value_type* offset, size_type len)
	  : ptr_(offset),
	    length_(len) {}
  StringPiece(const std::string::const_iterator& begin,
              const std::string::const_iterator& end) {
    length_ = static_cast<size_t>(std::distance(begin, end));
    ptr_ = length_ > 0 ? &*begin : nullptr;
  }

  const value_type* data() const { return ptr_; }
  size_type size() const { return length_; }
  size_type length() const { return length_; }
  bool empty() const { return length_ == 0; }

  void clear() {
    ptr_ = nullptr;
    length_ = 0;
  }
  void set(const value_type* data, size_type len) {
    ptr_ = data;
    length_ = len;
  } 
  void set(const value_type* str) {
    ptr_ = str;
    length_ = str ? std::string::traits_type::length(str) : 0;
  }

  value_type operator[](size_t i) const { return ptr_[i]; }
  value_type front() const { return ptr_[0]; }
  value_type back() const { return ptr_[length_-1]; }

  void remove_prefix(size_type n) {
    ptr_ += n;
    length_ -= n;
  }
  void remove_suffix(size_type n) {
    length_ -= n;
  }

  int compare(const StringPiece& x) const {
    int r = memcmp(ptr_, x.ptr_, (length_ < x.length_ ? length_ : x.length_));
    if (r == 0) {
      if (length_ < x.length_) r = -1;
      else if (length_ > x.length_) r = +1;
    }
    return r;
  }  

  std::string as_string() const {
    return empty() ? std::string() : std::string(data(), size());
  }
  std::string ToString() const {
    return as_string();
  }

  const_iterator begin() const { return ptr_; }
  const_iterator end() const { return ptr_  + length_; }
  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(ptr_ + length_);
  }
  const_reverse_iterator rend() const {
    return const_reverse_iterator(ptr_);
  }

  size_type max_size() const { return length_; }
  size_type capacity() const { return length_; }

  static int wordmemcmp(const value_type* p,
		        const value_type* p1,
			size_type N) {
    return std::string::traits_type::compare(p, p1, N);
  }

  void CopyToString(std::string* target) const;
  void AppendToString(std::string* target) const;
  size_t copy(value_type* buf, size_type n, size_type pos=0) const;

  size_type find(const StringPiece& s, size_type pos = 0) const;
  size_type find(value_type c, size_type pos = 0) const;
  size_type rfind(const StringPiece& s, size_type pos=StringPiece::npos) const;
  size_type rfind(value_type c, size_type pos=StringPiece::npos) const;
  
  size_type find_first_of(const StringPiece& s,
		          size_type pos = 0) const;
  size_type find_first_of(value_type c, size_type pos = 0) const;

  size_type find_first_not_of(const StringPiece& s,
		              size_type pos = 0) const;
  size_type find_first_not_of(value_type c, size_type pos = 0) const;

  size_type find_last_of(const StringPiece& s, 
		         size_type pos=StringPiece::npos) const;
  size_type find_last_of(value_type c, size_type pos=StringPiece::npos) const;

  size_type find_last_not_of(const StringPiece& s,
		             size_type pos=StringPiece::npos) const;
  size_type find_last_not_of(value_type c, 
		             size_type pos=StringPiece::npos) const;

  bool Consume(StringPiece x) {
    if (starts_with(x)) {
      remove_prefix(x.size());
      return true;
    }
    return false;
  }

  StringPiece substr(size_type pos,
		     size_type n=StringPiece::npos) const;

  bool starts_with(const StringPiece& x) const {
    return ((this->length_ >= x.length_) &&
            (wordmemcmp(this->ptr_, x.ptr_, x.length_) == 0));
  }

  bool ends_with(const StringPiece& x) const {
    return ((this->length_ > x.length_) &&
            (wordmemcmp(this->ptr_ + (this->length_ - x.length_),
			x.ptr_,
			x.length_) == 0));
  }

 protected:
  const value_type* ptr_;
  size_type length_;
};

// Export
bool operator==(const StringPiece& x, const StringPiece& y);

inline bool operator!=(const StringPiece& x, const StringPiece& y) {
  return !(x == y);
}

inline bool operator<(const StringPiece& x, const StringPiece& y) {
  const int r = StringPiece::wordmemcmp(
		  x.data(), y.data(),
		  (x.size() < y.size() ? x.size() : y.size()));
  return ((r < 0) || ((r == 0) && (x.size() < y.size())));
}

inline bool operator>(const StringPiece& x, const StringPiece& y) {
  return y < x;
}

inline bool operator<=(const StringPiece& x, const StringPiece& y) {
  return !(x > y);
}

inline bool operator>=(const StringPiece& x, const StringPiece& y) {
  return !(x < y);
}

// Export
std::ostream& operator<<(std::ostream& o,
		         const StringPiece& piece);

// Hash
struct StringPieceHash {
  std::size_t operator()(const StringPiece& s) const {
    std::size_t result = 0;
    for (StringPiece::const_iterator i = s.begin();
         i != s.end(); ++i) {
      result = (result * 131) + *i;
    }
    return result;
  }
};

} // namespace mr
#endif // CORE_STRINGS_STRING_PIECE_H_
