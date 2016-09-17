#include <sys/stat.h>

#include "core/base/status.h"
#include "core/base/map_util.h"
#include "core/base/stl_util.h"
#include "core/strings/scanner.h"
#include "core/strings/str_util.h"
#include "core/files/file_system.h"

namespace mr {

FileSystem::~FileSystem() {}

std::string FileSystem::TranslateName(const string& name) const {
  return "NOT_IMPLEMENTED_" + name;
}

Status FileSystem::IsDirectory(const string& name) {
  if (!FileExists(name)) {
    return Status(error::NOT_FOUND, "Path not found");
  }
  FileStatistics stat;
  RETURN_IF_ERROR(Stat(name, &stat));
  if (stat.is_directory) {
    return Status::OK;
  }
  return Status(error::FAILED_PRECONDITION, "Not a directory");
}

RandomAccessFile::~RandomAccessFile() {}

WritableFile::~WritableFile() {}

FileSystemRegistry::~FileSystemRegistry() {}

string GetSchemeFromURI(const string& name) {
  auto colon_loc = name.find(":");
  if (colon_loc != std::string::npos &&
      strings::Scanner(StringPiece(name.data(), colon_loc))
      .One(strings::Scanner::LETTER)
      .Many(strings::Scanner::LETTER_DIGIT_DOT)
      .GetResult()) {
    return name.substr(0, colon_loc);
  }
  return "";
}

string GetNameFromURI(const string& name) {
  string scheme = GetSchemeFromURI(name);
  if (scheme == "") {
    return name;
  }
  StringPiece filename{name.data() + scheme.length() + 1,
	               name.length() - scheme.length() -1 };
  if (filename[0] == '/' && filename[1] == '/') {
    return filename.substr(2).ToString();
  }
  return name;
}

} // namespace mr
