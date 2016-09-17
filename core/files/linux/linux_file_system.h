#ifndef TENSORFLOW_CORE_PLATFORM_POSIX_POSIX_FILE_SYSTEM_H_
#define TENSORFLOW_CORE_PLATFORM_POSIX_POSIX_FILE_SYSTEM_H_

#include "core/files/file_system.h"

using std::string;

namespace mr {

class LinuxFileSystem : public FileSystem {
 public:
  LinuxFileSystem() {}

  ~LinuxFileSystem() {}

  Status NewRandomAccessFile(const string& filename,
      std::unique_ptr<RandomAccessFile>* result) override;
  Status NewWritableFile(const string& fname,
                         std::unique_ptr<WritableFile>* result) override;
  Status NewAppendableFile(const string& fname,
                           std::unique_ptr<WritableFile>* result) override;
  Status NewReadOnlyMemoryRegionFromFile(
      const string& filename,
      std::unique_ptr<ReadOnlyMemoryRegion>* result) override;

  bool FileExists(const string& fname) override;
  Status GetChildren(const string& dir, std::vector<string>* result) override;
  Status Stat(const string& fname, FileStatistics* stats) override;
  Status DeleteFile(const string& fname) override;
  Status CreateDir(const string& name) override;
  Status DeleteDir(const string& name) override;
  Status GetFileSize(const string& fname, uint64_t* size) override;
  Status RenameFile(const string& src, const string& target) override;
};

Status IOError(const string& context, int err_number);

class LocalLinuxFileSystem : public LinuxFileSystem {
 public:
  string TranslateName(const string& name) const override {
    return GetNameFromURI(name);
  }
};

}  // namespace mr

#endif  // TENSORFLOW_CORE_PLATFORM_POSIX_POSIX_FILE_SYSTEM_H_
