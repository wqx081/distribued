#include "core/base/mem.h"

#include <stdlib.h>
#include <string.h>

namespace mr {

void* aligned_malloc(size_t size, int minimum_alignment) {
  void* ptr = NULL;
  const int required_alignment = sizeof(void*);
  if (minimum_alignment < required_alignment) return malloc(size);
  if (posix_memalign(&ptr, minimum_alignment, size) != 0)
    return NULL;
  else
    return ptr;
  }
   
void aligned_free(void* aligned_memory) { free(aligned_memory); }

} // namespace mr
