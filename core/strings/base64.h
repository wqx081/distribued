#ifndef TENSORFLOW_LIB_STRINGS_B64_H_
#define TENSORFLOW_LIB_STRINGS_B64_H_

#include <string>
#include "core/base/status.h"

namespace mr {

/// \brief Converts data into web-safe base64 encoding.
///
/// See https://en.wikipedia.org/wiki/Base64
Status Base64Encode(StringPiece data, bool with_padding, 
		    std::string* encoded);
Status Base64Encode(StringPiece data, 
		    std::string* encoded);  // with_padding=false.

/// \brief Converts data from web-safe base64 encoding.
///
/// See https://en.wikipedia.org/wiki/Base64
Status Base64Decode(StringPiece data, 
		    std::string* decoded);

}  // namespace mr

#endif  // TENSORFLOW_LIB_STRINGS_B64_H_
