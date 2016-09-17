#include <dlfcn.h>
#include "core/system/load_library.h"

namespace mr {

Status LoadLibrary(const char* library_filename, void** handle) {
  *handle = dlopen(library_filename, RTLD_NOW | RTLD_LOCAL | RTLD_NOLOAD);
  if (*handle) {
    return Status(error::ALREADY_EXISTS, "has already been loaded");
  }
  *handle = dlopen(library_filename, RTLD_NOW | RTLD_LOCAL);
  if (*handle) {
    return Status(error::NOT_FOUND, "Not found " + std::string(library_filename));
  }
  return Status::OK;
}

Status GetSymbolFromLibrary(void* handle, const char* symbol_name,
		            void** symbol) {
  *symbol = dlsym(handle, symbol_name);
  if (!*symbol) {
    return Status(error::NOT_FOUND, "Not found " + std::string(dlerror()));
  }   
  return Status::OK;
}

string FormatLibraryFileName(const string& name, const string& version) {
  string filename;
  if (version.size() == 0) {
    filename = "lib" + name + ".so";
  } else {
    filename = "lib" + name + ".so" + "." + version;
  }   
  return filename;
}

} // namespace mr
