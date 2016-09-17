#ifndef CORE_BASE_MEM_H_
#define CORE_BASE_MEM_H_

#include <stdint.h>
#include <stddef.h>

namespace mr {

void* aligned_malloc(size_t size, int minimum_alignment);
void aligned_free(void* aligned_memory);

} // namespace mr
#endif // CORE_BASE_MEM_H_
