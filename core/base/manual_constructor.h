// ManualConstructor statically-allocates space in which to store some
// object, but does not initialize it.  You can then call the constructor
// and destructor for the object yourself as you see fit.  This is useful
// for memory management optimizations, where you want to initialize and
// destroy an object multiple times but only allocate it once.
//
// (When I say ManualConstructor statically allocates space, I mean that
// the ManualConstructor object itself is forced to be the right size.)

#ifndef TENSORFLOW_LIB_GTL_MANUAL_CONSTRUCTOR_H_
#define TENSORFLOW_LIB_GTL_MANUAL_CONSTRUCTOR_H_

#include <stddef.h>
#include <new>
#include <utility>

#include "core/base/macros.h"
#include "core/base/mem.h"

namespace mr {
namespace gtl {
namespace internal {

//
// Provides a char array with the exact same alignment as another type. The
// first parameter must be a complete type, the second parameter is how many
// of that type to provide space for.
//
//   LIB_GTL_ALIGNED_CHAR_ARRAY(struct stat, 16) storage_;
//
// Because MSVC and older GCCs require that the argument to their alignment
// construct to be a literal constant integer, we use a template instantiated
// at all the possible powers of two.
template <int alignment, int size>
struct AlignType {};
template <int size>
struct AlignType<0, size> {
  typedef char result[size];
};

#define LIB_GTL_ALIGN_ATTRIBUTE(X) __attribute__((aligned(X)))
#define LIB_GTL_ALIGN_OF(T) __alignof__(T)

#define LIB_GTL_ALIGNTYPE_TEMPLATE(X)                     \
  template <int size>                                        \
  struct AlignType<X, size> {                                \
    typedef LIB_GTL_ALIGN_ATTRIBUTE(X) char result[size]; \
  }

LIB_GTL_ALIGNTYPE_TEMPLATE(1);
LIB_GTL_ALIGNTYPE_TEMPLATE(2);
LIB_GTL_ALIGNTYPE_TEMPLATE(4);
LIB_GTL_ALIGNTYPE_TEMPLATE(8);
LIB_GTL_ALIGNTYPE_TEMPLATE(16);
LIB_GTL_ALIGNTYPE_TEMPLATE(32);
LIB_GTL_ALIGNTYPE_TEMPLATE(64);
LIB_GTL_ALIGNTYPE_TEMPLATE(128);
LIB_GTL_ALIGNTYPE_TEMPLATE(256);
LIB_GTL_ALIGNTYPE_TEMPLATE(512);
LIB_GTL_ALIGNTYPE_TEMPLATE(1024);
LIB_GTL_ALIGNTYPE_TEMPLATE(2048);
LIB_GTL_ALIGNTYPE_TEMPLATE(4096);
LIB_GTL_ALIGNTYPE_TEMPLATE(8192);

#define LIB_GTL_ALIGNED_CHAR_ARRAY(T, Size)                          \
  typename mr::gtl::internal::AlignType<LIB_GTL_ALIGN_OF(T), \
                                                sizeof(T) * Size>::result

#undef LIB_GTL_ALIGNTYPE_TEMPLATE
#undef LIB_GTL_ALIGN_ATTRIBUTE
}  // namespace internal
}  // namespace gtl

template <typename Type>
class ManualConstructor {
 public:
  // No constructor or destructor because one of the most useful uses of
  // this class is as part of a union, and members of a union cannot have
  // constructors or destructors.  And, anyway, the whole point of this
  // class is to bypass these.

  // Support users creating arrays of ManualConstructor<>s.  This ensures that
  // the array itself has the correct alignment.
  static void* operator new[](size_t size) {
    return aligned_malloc(size, LIB_GTL_ALIGN_OF(Type));
  }
  static void operator delete[](void* mem) { aligned_free(mem); }

  inline Type* get() { return reinterpret_cast<Type*>(space_); }
  inline const Type* get() const {
    return reinterpret_cast<const Type*>(space_);
  }

  inline Type* operator->() { return get(); }
  inline const Type* operator->() const { return get(); }

  inline Type& operator*() { return *get(); }
  inline const Type& operator*() const { return *get(); }

  inline void Init() { new (space_) Type; }

// Init() constructs the Type instance using the given arguments
// (which are forwarded to Type's constructor). In C++11, Init() can
// take any number of arguments of any type, and forwards them perfectly.
// On pre-C++11 platforms, it can take up to 11 arguments, and may not be
// able to forward certain kinds of arguments.
//
// Note that Init() with no arguments performs default-initialization,
// not zero-initialization (i.e it behaves the same as "new Type;", not
// "new Type();"), so it will leave non-class types uninitialized.
  template <typename... Ts>
  inline void Init(Ts&&... args) {                 // NOLINT
    new (space_) Type(std::forward<Ts>(args)...);  // NOLINT
  }

  inline void Destroy() { get()->~Type(); }

 private:
  LIB_GTL_ALIGNED_CHAR_ARRAY(Type, 1) space_;
};

#undef LIB_GTL_ALIGNED_CHAR_ARRAY
#undef LIB_GTL_ALIGN_OF

}  // namespace tensorflow

#endif  // TENSORFLOW_LIB_GTL_MANUAL_CONSTRUCTOR_H_
