#ifndef TENSORFLOW_LIB_STRINGS_NUMBERS_H_
#define TENSORFLOW_LIB_STRINGS_NUMBERS_H_
#include <string>

#include "core/strings/string_piece.h"

namespace mr {
namespace strings {

static const int kFastToBufferSize = 32;

char* FastInt32ToBufferLeft(int32_t i, char* buffer);    // at least 12 bytes
char* FastUInt32ToBufferLeft(uint32_t i, char* buffer);  // at least 12 bytes
char* FastInt64ToBufferLeft(int64_t i, char* buffer);    // at least 22 bytes
char* FastUInt64ToBufferLeft(uint64_t i, char* buffer);  // at least 22 bytes

char* DoubleToBuffer(double i, char* buffer);
char* FloatToBuffer(float i, char* buffer);

std::string FpToString(uint64_t fp);
bool StringToFp(const std::string& s, uint64_t* fp);

StringPiece Uint64ToHexString(uint64_t v, char* buf);
bool HexStringToUint64(const StringPiece& s, uint64_t* v);

bool safe_strto32(StringPiece str, int32_t* value);
bool safe_strtou32(StringPiece str, uint32_t* value);

bool safe_strto64(StringPiece str, int64_t* value);
bool safe_strtou64(StringPiece str, uint64_t* value);

bool safe_strtof(const char* str, float* value);
bool safe_strtod(const char* str, double* value);

// Converts from an int64_t representing a number of bytes to a
// human readable string representing the same number.
// e.g. 12345678 -> "11.77MiB".
std::string HumanReadableNumBytes(int64_t num_bytes);

}  // namespace strings
}  // namespace mr

#endif  // TENSORFLOW_LIB_STRINGS_NUMBERS_H_
