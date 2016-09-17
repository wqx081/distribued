#ifndef TENSORFLOW_LIB_IO_PATH_H_
#define TENSORFLOW_LIB_IO_PATH_H_


#include "core/base/status.h"
#include "core/strings/string_piece.h"

using std::string;

namespace mr {
class StringPiece;
namespace io {
namespace internal {
string JoinPathImpl(std::initializer_list<StringPiece> paths);
}

// Utility routines for processing filenames

// Join multiple paths together, without introducing unnecessary path
// separators.
// For example:
//
//  Arguments                  | JoinPath
//  ---------------------------+----------
//  '/foo', 'bar'              | /foo/bar
//  '/foo/', 'bar'             | /foo/bar
//  '/foo', '/bar'             | /foo/bar
//
// Usage:
// string path = io::JoinPath("/mydir", filename);
// string path = io::JoinPath(FLAGS_test_srcdir, filename);
// string path = io::JoinPath("/full", "path", "to", "filename);
template <typename... T>
string JoinPath(const T&... args) {
  return internal::JoinPathImpl({args...});
}

// Return true if path is absolute.
bool IsAbsolutePath(StringPiece path);

// Returns the part of the path before the final "/".  If there is a single
// leading "/" in the path, the result will be the leading "/".  If there is
// no "/" in the path, the result is the empty prefix of the input.
StringPiece Dirname(StringPiece path);

// Returns the part of the path after the final "/".  If there is no
// "/" in the path, the result is the same as the input.
StringPiece Basename(StringPiece path);

// Returns the part of the basename of path after the final ".".  If
// there is no "." in the basename, the result is empty.
StringPiece Extension(StringPiece path);

// Collapse duplicate "/"s, resolve ".." and "." path elements, remove
// trailing "/".
//
// NOTE: This respects relative vs. absolute paths, but does not
// invoke any system calls (getcwd(2)) in order to resolve relative
// paths with respect to the actual working directory.  That is, this is purely
// string manipulation, completely independent of process state.
string CleanPath(StringPiece path);

}  // namespace io
}  // namespace tensorflow

#endif  // TENSORFLOW_LIB_IO_PATH_H_
