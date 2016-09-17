// Printf variants that place their output in a C++ string.
//
// Usage:
//      string result = strings::Printf("%d %s\n", 10, "hello");
//      strings::SPrintf(&result, "%d %s\n", 10, "hello");
//      strings::Appendf(&result, "%d %s\n", 20, "there");

#ifndef TENSORFLOW_LIB_STRINGS_STRINGPRINTF_H_
#define TENSORFLOW_LIB_STRINGS_STRINGPRINTF_H_

#include <stdarg.h>
#include <string>

#include "core/base/macros.h"

using std::string;

namespace mr {
namespace strings {

extern string Printf(const char* format, ...)
    PRINTF_ATTRIBUTE(1, 2);

extern void Appendf(string* dst, const char* format, ...)
    PRINTF_ATTRIBUTE(2, 3);

extern void Appendv(string* dst, const char* format, va_list ap);

}  // namespace strings
}  // namespace tensorflow

#endif  // TENSORFLOW_LIB_STRINGS_STRINGPRINTF_H_
