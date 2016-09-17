#ifndef MR_CORE_FILE_SYSTEM_H_
#define MR_CORE_FILE_SYSTEM_H_

#include <stdint.h>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/base/macros.h"
#include "core/base/status.h"
#include "core/strings/string_piece.h"

namespace mr {

struct FileStatistics {
 int64_t length = -1;
 int64_t mtime_nsec = 0;
 bool is_directory = false;

 FileStatistics() {}
 ~FileStatistics() {}
};

class RandomAccessFile;
class ReadOnlyMemoryRegion;
class WritableFile;

class FileSystem {
 public:
  FileSystem() {}
  virtual ~FileSystem();

  virtual Status NewRandomAccessFile(
		  const std::string& fname,
		  std::unique_ptr<RandomAccessFile>* result) = 0;
  virtual Status NewWritableFile(
		  const std::string& fname,
		  std::unique_ptr<WritableFile>* result) = 0;
  virtual Status NewAppendableFile(
		  const std::string& fname,
		  std::unique_ptr<WritableFile>* result) = 0;
  virtual Status NewReadOnlyMemoryRegionFromFile(
		  const std::string& fname,
		  std::unique_ptr<ReadOnlyMemoryRegion>* result) = 0;
  virtual bool FileExists(const std::string& fname) = 0;
  virtual Status GetChildren(const std::string& dir,
		                   std::vector<std::string>* result) = 0;
  virtual Status Stat(const std::string& fname,
                            FileStatistics* stat) = 0;
  
  virtual Status DeleteFile(const std::string& fname) = 0;
  virtual Status CreateDir(const std::string& dirname) = 0;
  virtual Status DeleteDir(const std::string& dirname) = 0;
  virtual Status GetFileSize(const std::string& fname, uint64_t* ret)=0;
  virtual Status RenameFile(const std::string& src,
		                  const std::string& target) = 0;
  virtual std::string TranslateName(const std::string& name) const;
  virtual Status IsDirectory(const std::string& fname);
};

class RandomAccessFile {
 public:
  RandomAccessFile() {}
  virtual ~RandomAccessFile();

  virtual Status Read(uint64_t offset, size_t n, 
		            StringPiece* result,
			    char* scratch) const = 0;
 private:
  DISALLOW_COPY_AND_ASSIGN(RandomAccessFile);
};

class WritableFile {
 public:
  WritableFile() {}
  virtual ~WritableFile();

  virtual Status Append(const StringPiece& data) = 0;
  virtual Status Close() = 0;
  virtual Status Flush() = 0;
  virtual Status Sync() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(WritableFile);
};

class ReadOnlyMemoryRegion {
 public:
  ReadOnlyMemoryRegion() {}
  virtual ~ReadOnlyMemoryRegion() = default;
  virtual const void* data() = 0;
  virtual uint64_t length() = 0;
};

//
class FileSystemRegistry {
 public:
  typedef std::function<FileSystem*()> Factory;

  virtual ~FileSystemRegistry();
  virtual Status Register(const std::string& scheme,
		                Factory factory) = 0;
  virtual FileSystem* Lookup(const std::string& scheme) = 0;
  virtual Status GetRegisteredFileSystemSchemes(
		  std::vector<std::string>* schemes) = 0;
};

std::string GetSchemeFromURI(const std::string& name);
std::string GetNameFromURI(const std::string& name);

} // namespace mr
#endif // MR_CORE_FILE_SYSTEM_H_
