#ifndef TENSORFLOW_LIB_STRINGS_STRCAT_H_
#define TENSORFLOW_LIB_STRINGS_STRCAT_H_

#include <string>

#include "core/strings/string_piece.h"
#include "core/strings/numbers.h"
#include "core/base/macros.h"

using std::string;

namespace mr {
namespace strings {

enum PadSpec {
  NO_PAD = 1,
  ZERO_PAD_2,
  ZERO_PAD_3,
  ZERO_PAD_4,
  ZERO_PAD_5,
  ZERO_PAD_6,
  ZERO_PAD_7,
  ZERO_PAD_8,
  ZERO_PAD_9,
  ZERO_PAD_10,
  ZERO_PAD_11,
  ZERO_PAD_12,
  ZERO_PAD_13,
  ZERO_PAD_14,
  ZERO_PAD_15,
  ZERO_PAD_16,
};

struct Hex {
  uint64_t value;
  enum PadSpec spec;
  template <class Int>
  explicit Hex(Int v, PadSpec s = NO_PAD) : spec(s) {
    // Prevent sign-extension by casting integers to
    // their unsigned counterparts.
    static_assert(
        sizeof(v) == 1 || sizeof(v) == 2 || sizeof(v) == 4 || sizeof(v) == 8,
        "Unknown integer type");
    value = sizeof(v) == 1
                ? static_cast<uint8_t>(v)
                : sizeof(v) == 2 ? static_cast<uint16_t>(v)
                                 : sizeof(v) == 4 ? static_cast<uint32_t>(v)
                                                  : static_cast<uint64_t>(v);
  }
};

class AlphaNum {
 public:
  // No bool ctor -- bools convert to an integral type.
  // A bool ctor would also convert incoming pointers (bletch).

  AlphaNum(int i32)  // NOLINT(runtime/explicit)
      : piece_(digits_, FastInt32ToBufferLeft(i32, digits_) - &digits_[0]) {}
  AlphaNum(unsigned int u32)  // NOLINT(runtime/explicit)
      : piece_(digits_, FastUInt32ToBufferLeft(u32, digits_) - &digits_[0]) {}
  AlphaNum(long x)  // NOLINT(runtime/explicit)
      : piece_(digits_, FastInt64ToBufferLeft(x, digits_) - &digits_[0]) {}
  AlphaNum(unsigned long x)  // NOLINT(runtime/explicit)
      : piece_(digits_, FastUInt64ToBufferLeft(x, digits_) - &digits_[0]) {}
  AlphaNum(long long int i64)  // NOLINT(runtime/explicit)
      : piece_(digits_, FastInt64ToBufferLeft(i64, digits_) - &digits_[0]) {}
  AlphaNum(unsigned long long int u64)  // NOLINT(runtime/explicit)
      : piece_(digits_, FastUInt64ToBufferLeft(u64, digits_) - &digits_[0]) {}

  AlphaNum(float f)  // NOLINT(runtime/explicit)
      : piece_(digits_, strlen(FloatToBuffer(f, digits_))) {}
  AlphaNum(double f)  // NOLINT(runtime/explicit)
      : piece_(digits_, strlen(DoubleToBuffer(f, digits_))) {}

  AlphaNum(Hex hex);  // NOLINT(runtime/explicit)

  AlphaNum(const char *c_str) : piece_(c_str) {}   // NOLINT(runtime/explicit)
  AlphaNum(const StringPiece &pc) : piece_(pc) {}  // NOLINT(runtime/explicit)
  AlphaNum(const string &str)          // NOLINT(runtime/explicit)
      : piece_(str) {}

  StringPiece::size_type size() const { return piece_.size(); }
  const char *data() const { return piece_.data(); }
  StringPiece Piece() const { return piece_; }

 private:
  StringPiece piece_;
  char digits_[kFastToBufferSize];

  // Use ":" not ':'
  AlphaNum(char c);  // NOLINT(runtime/explicit)

  DISALLOW_COPY_AND_ASSIGN(AlphaNum);
};

extern AlphaNum gEmptyAlphaNum;

// ----------------------------------------------------------------------
// StrCat()
//    This merges the given strings or numbers, with no delimiter.  This
//    is designed to be the fastest possible way to construct a string out
//    of a mix of raw C strings, StringPieces, strings, bool values,
//    and numeric values.
//
//    Don't use this for user-visible strings.  The localization process
//    works poorly on strings built up out of fragments.
//
//    For clarity and performance, don't use StrCat when appending to a
//    string.  In particular, avoid using any of these (anti-)patterns:
//      str.append(StrCat(...))
//      str += StrCat(...)
//      str = StrCat(str, ...)
//    where the last is the worse, with the potential to change a loop
//    from a linear time operation with O(1) dynamic allocations into a
//    quadratic time operation with O(n) dynamic allocations.  StrAppend
//    is a better choice than any of the above, subject to the restriction
//    of StrAppend(&str, a, b, c, ...) that none of the a, b, c, ... may
//    be a reference into str.
// ----------------------------------------------------------------------

// For performance reasons, we have specializations for <= 4 args.
string StrCat(const AlphaNum &a);
string StrCat(const AlphaNum &a, const AlphaNum &b);
string StrCat(const AlphaNum &a, const AlphaNum &b,
              const AlphaNum &c);
string StrCat(const AlphaNum &a, const AlphaNum &b, const AlphaNum &c,
              const AlphaNum &d);

namespace internal {

// Do not call directly - this is not part of the public API.
string CatPieces(std::initializer_list<StringPiece> pieces);
void AppendPieces(string *dest, std::initializer_list<StringPiece> pieces);

}  // namespace internal

// Support 5 or more arguments
template <typename... AV>
string StrCat(const AlphaNum &a, const AlphaNum &b, const AlphaNum &c,
              const AlphaNum &d, const AlphaNum &e,
              const AV &... args);

template <typename... AV>
string StrCat(const AlphaNum &a, const AlphaNum &b, const AlphaNum &c,
              const AlphaNum &d, const AlphaNum &e, const AV &... args) {
  return internal::CatPieces({a.Piece(), b.Piece(), c.Piece(), d.Piece(),
                              e.Piece(),
                              static_cast<const AlphaNum &>(args).Piece()...});
}

// ----------------------------------------------------------------------
// StrAppend()
//    Same as above, but adds the output to the given string.
//    WARNING: For speed, StrAppend does not try to check each of its input
//    arguments to be sure that they are not a subset of the string being
//    appended to.  That is, while this will work:
//
//    string s = "foo";
//    s += s;
//
//    This will not (necessarily) work:
//
//    string s = "foo";
//    StrAppend(&s, s);
//
//    Note: while StrCat supports appending up to 26 arguments, StrAppend
//    is currently limited to 9.  That's rarely an issue except when
//    automatically transforming StrCat to StrAppend, and can easily be
//    worked around as consecutive calls to StrAppend are quite efficient.
// ----------------------------------------------------------------------

void StrAppend(string *dest, const AlphaNum &a);
void StrAppend(string *dest, const AlphaNum &a, const AlphaNum &b);
void StrAppend(string *dest, const AlphaNum &a, const AlphaNum &b,
               const AlphaNum &c);
void StrAppend(string *dest, const AlphaNum &a, const AlphaNum &b,
               const AlphaNum &c, const AlphaNum &d);

// Support 5 or more arguments
template <typename... AV>
inline void StrAppend(string *dest, const AlphaNum &a, const AlphaNum &b,
                      const AlphaNum &c, const AlphaNum &d, const AlphaNum &e,
                      const AV &... args) {
  internal::AppendPieces(dest,
                         {a.Piece(), b.Piece(), c.Piece(), d.Piece(), e.Piece(),
                          static_cast<const AlphaNum &>(args).Piece()...});
}

}  // namespace strings
}  // namespace mr

#endif  // TENSORFLOW_LIB_STRINGS_STRCAT_H_
