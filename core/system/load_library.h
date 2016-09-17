#ifndef CORE_SYSTEM_LOAD_LIBRARY_H_
#define CORE_SYSTEM_LOAD_LIBRARY_H_

#include "core/base/status.h"

using std::string;

namespace mr {

Status LoadLibrary(const char* library_filename, void** handle);
Status GetSymbolFromLibrary(void* handle,
		            const char* symbol_name,
			    void** symbol);
string FormatLibraryFileName(const string& name, const string& version);

} // namespace mr
#endif // CORE_SYSTEM_LOAD_LIBRARY_H_
