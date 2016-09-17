#include "core/io/zero_copy_stream.h"
#include "core/base/logging.h"

namespace mr {

ZeroCopyInputStream::~ZeroCopyInputStream() {}
ZeroCopyOutputStream::~ZeroCopyOutputStream() {}


bool ZeroCopyOutputStream::WriteAliasedRaw(const void* /* data */,
                                           int /* size */) {
  LOG(FATAL) << "This ZeroCopyOutputStream doesn't support aliasing. "
                "Reaching here usually means a ZeroCopyOutputStream "
                "implementation bug.";
  return false;
}

}  // namespace mr
