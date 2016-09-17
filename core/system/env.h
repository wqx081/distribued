#ifndef MR_CORE_SYSTEM_ENV_H_
#define MR_CORE_SYSTEM_ENV_H_

#include <stdint.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>

#include "core/base/status.h"
#include "core/strings/string_piece.h"
#include "core/files/file_system.h"
#include "core/base/macros.h"

using std::string;

namespace mr {

class Thread;
struct ThreadOptions;

class Env {
 public:
  Env();
  virtual ~Env() = default;

  static Env* Default();
  
  virtual Status GetFileSystemForFile(const string& fname,
		                      FileSystem** result);
  virtual Status GetRegisteredFileSystemSchemes(std::vector<string>* schemes);
  virtual Status RegisterFileSystem(const string& scheme,
		                    FileSystemRegistry::Factory factory);
  
  Status NewRandomAccessFile(const string& fname,
		             std::unique_ptr<RandomAccessFile>* result);
  Status NewWritableFile(const string& fname,
		         std::unique_ptr<WritableFile>* result);
  Status NewAppendableFile(const string& fname,
		           std::unique_ptr<WritableFile>* result);
  Status NewReadOnlyMemoryRegionFromFile(
		  const string& fname,
		  std::unique_ptr<ReadOnlyMemoryRegion>* result);
  bool FileExists(const string& fname);
  Status GetChildren(const string& dir, std::vector<string>* result);
  Status DeleteFile(const string& fname);
  Status DeleteRecursively(const string& dirname,
		           int64_t* undeleted_files,
			   int64_t* undeleted_dirs);
  Status RecursivelyCreateDir(const string& dirname);
  Status CreateDir(const string& dirname);
  Status DeleteDir(const string& dirname);
  Status Stat(const string& fname, FileStatistics* stat);
  Status IsDirectory(const string& fname);
  Status GetFileSize(const string& fname, uint64_t* file_size);
  Status RenameFile(const string& src, const string& target);

  virtual uint64_t NowMicros() = 0;
  virtual uint64_t NowSeconds() { return NowMicros() / 1000000L; }
  virtual void SleepForMicroseconds(int64_t micros) = 0;

  virtual Thread* StartThread(const ThreadOptions& thread_options,
		              const string& name,
			      std::function<void()> fn) = 0;
  virtual void SchedClosure(std::function<void()> closure) = 0;
  virtual void SchedClosureAfter(int64_t micros,
		                 std::function<void()> closure) = 0;

  virtual Status LoadLibrary(const char* library_filename, void** handle) = 0;
  virtual Status GetSymbolFromLibrary(void* handle,
		                      const char* symbol_name,
				      void** symbol) = 0;
 private:
  DISALLOW_COPY_AND_ASSIGN(Env);

  std::unique_ptr<FileSystemRegistry> file_system_registry_;
};

class EnvProxy : public Env {
 public:
  explicit EnvProxy(Env* t) : target_(t) {}
  virtual ~EnvProxy();

  Env* target() const { return target_; }

  Status GetFileSystemForFile(const string& fname,
		              FileSystem** result) override {
    return target_->GetFileSystemForFile(fname, result);
  }

  Status GetRegisteredFileSystemSchemes(
		  std::vector<string>* schemes) override {
    return target_->GetRegisteredFileSystemSchemes(schemes);
  }

  Status RegisterFileSystem(const string& scheme,
		            FileSystemRegistry::Factory factory) override {
    return target_->RegisterFileSystem(scheme, factory);
  }

  uint64_t NowMicros() override { return target_->NowMicros(); }

  void SleepForMicroseconds(int64_t micros) override {
    target_->SleepForMicroseconds(micros);
  }

  Thread* StartThread(const ThreadOptions& thread_options,
		      const string& name,
		      std::function<void()> fn) override {
    return target_->StartThread(thread_options, name, fn);
  }

  void SchedClosure(std::function<void()> closure) override {
    target_->SchedClosure(closure);
  }

  void SchedClosureAfter(int64_t micros, 
		         std::function<void()> closure) override {
    target_->SchedClosureAfter(micros, closure);
  }

  Status LoadLibrary(const char* library_filename, void** handle) override {
    return target_->LoadLibrary(library_filename, handle);
  }

  Status GetSymbolFromLibrary(void* handle,
		              const char* symbol_name,
			      void** symbol) override {
    return target_->GetSymbolFromLibrary(handle, symbol_name, symbol);
  }
 private:
  Env* target_;
};

class Thread {
 public:
  Thread() {}
  virtual ~Thread();

 private:
  DISALLOW_COPY_AND_ASSIGN(Thread);
};

struct ThreadOptions {
  size_t stack_size = 0;
  size_t guard_size = 0;
};

Status ReadFileToString(Env* env, const string& fname, string* data);
Status WriteStringToFile(Env* env, const string& fname,
		         const StringPiece& data);

namespace register_file_system {

template <typename Factory>
struct Register {
  Register(Env* env, const string& scheme) {
    env->RegisterFileSystem(scheme,
		            []() -> FileSystem* { return new Factory; });
  }
};

} // namespace register_file_system
} // namespace mr

#define REGISTER_FILE_SYSTEM_ENV(env, scheme, factory) \
  REGISTER_FILE_SYSTEM_UNIQ_HULPER(__COUNTER__, env, scheme, factory)
#define REGISTER_FILE_SYSTEM_UNIQ_HULPER(ctr, env, scheme, factory) \
  REGISTER_FILE_SYSTEM_UNIQ(ctr, env, scheme, factory)
#define REGISTER_FILE_SYSTEM_UNIQ(ctr, env, scheme, factory)    \
  static ::mr::register_file_system::Register<factory>          \
    register_ff##ctr = ::mr::register_file_system::Register<factory>(env, scheme)

#define REGISTER_FILE_SYSTEM(scheme, factory) \
  REGISTER_FILE_SYSTEM_ENV(Env::Default(), scheme, factory);

#endif // MR_CORE_SYSTEM_ENV_H_
