#include <deque>
#include <vector>

#include <mutex>

#include "core/base/status.h"
#include "core/base/map_util.h"
#include "core/base/stl_util.h"
#include "core/system/env.h"
#include "core/base/logging.h"
#include "core/io/zero_copy_stream.h"
#include "core/io/path.h"

namespace mr {

class FileSystemRegistryImpl : public FileSystemRegistry {
 public:
  Status Register(const string& scheme, Factory factory) override;
  FileSystem* Lookup(const string& scheme) override;
  Status GetRegisteredFileSystemSchemes(std::vector<string>* schemes) override;

 private:
  mutable std::mutex mu_;
  mutable std::unordered_map<string, std::unique_ptr<FileSystem>> registry_;
};

Status FileSystemRegistryImpl::Register(const string& scheme,
		                        FileSystemRegistry::Factory factory) {
  std::lock_guard<std::mutex> l(mu_);
  if (!registry_.emplace(string(scheme),
			  std::unique_ptr<FileSystem>(factory())).second) {
    return Status(error::ALREADY_EXISTS, "File factory for " + scheme +
		    " already registered");
  }
  return Status::OK;
}

FileSystem* FileSystemRegistryImpl::Lookup(const string& scheme) {
  std::lock_guard<std::mutex> l(mu_);
  const auto found = registry_.find(scheme);
  if (found == registry_.end()) {
    return nullptr;
  }
  return found->second.get();
}

Status FileSystemRegistryImpl::GetRegisteredFileSystemSchemes(
		std::vector<string>* schemes) {
  std::lock_guard<std::mutex> l(mu_);
  for (const auto& e : registry_) {
    schemes->push_back(e.first);
  }
  return Status::OK;
}

////////////
Env::Env() : file_system_registry_(new FileSystemRegistryImpl) {}

Status Env::GetFileSystemForFile(const string& fname,
		                 FileSystem** result) {
  string scheme = GetSchemeFromURI(fname);
  FileSystem* file_system = file_system_registry_->Lookup(scheme);
  if (!file_system) {
    return Status(error::UNIMPLEMENTED, "File system scheme " + scheme +
		    " not implemented");
  }
  *result = file_system;
  return Status::OK;
}

Status Env::GetRegisteredFileSystemSchemes(
		std::vector<string>* schemes) {
  return file_system_registry_->GetRegisteredFileSystemSchemes(schemes);
}

Status Env::RegisterFileSystem(const string& scheme,
		               FileSystemRegistry::Factory factory) {
  return file_system_registry_->Register(scheme, factory);
}

Status Env::NewRandomAccessFile(const string& fname,
           std::unique_ptr<RandomAccessFile>* result) {
  FileSystem* fs;
  RETURN_IF_ERROR(GetFileSystemForFile(fname, &fs));
  return fs->NewRandomAccessFile(fname, result);
}

Status Env::NewReadOnlyMemoryRegionFromFile(
		const string& fname,
		std::unique_ptr<ReadOnlyMemoryRegion>* result) {
  FileSystem* fs;
  RETURN_IF_ERROR(GetFileSystemForFile(fname, &fs));
  return fs->NewReadOnlyMemoryRegionFromFile(fname, result);
}

Status Env::NewWritableFile(const string& fname,
		            std::unique_ptr<WritableFile>* result) {
  FileSystem* fs;
  RETURN_IF_ERROR(GetFileSystemForFile(fname, &fs));
  return fs->NewWritableFile(fname, result);
}
  
Status Env::NewAppendableFile(const string& fname,
		              std::unique_ptr<WritableFile>* result) {
  FileSystem* fs;
  RETURN_IF_ERROR(GetFileSystemForFile(fname, &fs));
  return fs->NewAppendableFile(fname, result);
}
  
bool Env::FileExists(const string& fname) {
  FileSystem* fs;
  if (!GetFileSystemForFile(fname, &fs).ok()) {
    return false;
  }
  return fs->FileExists(fname);
}
  
Status Env::GetChildren(const string& dir, std::vector<string>* result) {
  FileSystem* fs;
  RETURN_IF_ERROR(GetFileSystemForFile(dir, &fs));
  return fs->GetChildren(dir, result);
}
  
Status Env::DeleteFile(const string& fname) {
  FileSystem* fs;
  RETURN_IF_ERROR(GetFileSystemForFile(fname, &fs));
  return fs->DeleteFile(fname);
}

Status Env::RecursivelyCreateDir(const string& dirname) {
  FileSystem* fs;
  RETURN_IF_ERROR(GetFileSystemForFile(dirname, &fs));
  std::vector<StringPiece> sub_dirs;
  StringPiece remaining_dir(dirname);
  while (!fs->FileExists(remaining_dir.ToString()) 
         && !remaining_dir.empty())   {
    if (!remaining_dir.ends_with("/")) {
      sub_dirs.push_back(io::Basename(remaining_dir));
    }
    remaining_dir = io::Dirname(remaining_dir);
  }

  std::reverse(sub_dirs.begin(), sub_dirs.end());

  string built_path = remaining_dir.ToString();
  for (const StringPiece sub_dir : sub_dirs) {
       built_path = io::JoinPath(built_path, sub_dir);
    RETURN_IF_ERROR(fs->CreateDir(built_path));
  }
  return Status::OK;
}

Status Env::CreateDir(const string& dirname) {
  FileSystem* fs;
  RETURN_IF_ERROR(GetFileSystemForFile(dirname, &fs));
  return fs->CreateDir(dirname);
} 
  
Status Env::DeleteDir(const string& dirname) {
  FileSystem* fs;
  RETURN_IF_ERROR(GetFileSystemForFile(dirname, &fs));
  return fs->DeleteDir(dirname);
} 
  
Status Env::Stat(const string& fname, FileStatistics* stat) {
  FileSystem* fs;
  RETURN_IF_ERROR(GetFileSystemForFile(fname, &fs));
  return fs->Stat(fname, stat);
} 
  
Status Env::IsDirectory(const string& fname) {
  FileSystem* fs;
  RETURN_IF_ERROR(GetFileSystemForFile(fname, &fs));
  return fs->IsDirectory(fname);
} 

Status Env::DeleteRecursively(const string& dirname, 
		              int64_t* undeleted_files,
		              int64_t* undeleted_dirs) {
  CHECK_NOTNULL(undeleted_files);
  CHECK_NOTNULL(undeleted_dirs);
  FileSystem* fs;
  RETURN_IF_ERROR(GetFileSystemForFile(dirname, &fs));
  *undeleted_files = 0;
  *undeleted_dirs = 0;
  if (!FileExists(dirname)) {
    (*undeleted_dirs)++;
    return Status(error::NOT_FOUND, "Directory doesn't exist");
  } 
  std::deque<string> dir_q;      // Queue for the BFS
  std::vector<string> dir_list;  // List of all dirs discovered
  dir_q.push_back(dirname);
  Status ret;  // Status to be returned.

  while (!dir_q.empty()) {
    string dir = dir_q.front();
    dir_q.pop_front();
    dir_list.push_back(dir);
    std::vector<string> children;
    Status s = fs->GetChildren(dir, &children);
    ret.Update(s);
    if (!s.ok()) {
      (*undeleted_dirs)++;
      continue;
    } 
    for (const string& child : children) {
     const string child_path = io::JoinPath(dir, child);
     if (fs->IsDirectory(child_path).ok()) {
       dir_q.push_back(child_path);
     } else {
       Status del_status = fs->DeleteFile(child_path);
       ret.Update(del_status);
       if (!del_status.ok()) {
         (*undeleted_files)++;
         }
               }
           }
        }
  std::reverse(dir_list.begin(), dir_list.end());
  for (const string& dir : dir_list) {
    Status s = fs->DeleteDir(dir);
    ret.Update(s);
    if (!s.ok()) {
      (*undeleted_dirs)++;
    } 
  } 
  return Status::OK;
} 

Status Env::GetFileSize(const string& fname, uint64_t* file_size) {
  FileSystem* fs;
  RETURN_IF_ERROR(GetFileSystemForFile(fname, &fs));
  return fs->GetFileSize(fname, file_size);
}
  
Status Env::RenameFile(const string& src, const string& target) {
  FileSystem* src_fs;
  FileSystem* target_fs;
  RETURN_IF_ERROR(GetFileSystemForFile(src, &src_fs));
  RETURN_IF_ERROR(GetFileSystemForFile(target, &target_fs));
  if (src_fs != target_fs) {
    return Status(error::UNIMPLEMENTED, "Renaming " + src 
       +" to " +  target + " not implemented");
  }
  return src_fs->RenameFile(src, target);
}
  
Thread::~Thread() {}
    
EnvProxy::~EnvProxy() {}

Status ReadFileToString(Env* env, const string& fname, string* data) {
  uint64_t file_size;
  Status s = env->GetFileSize(fname, &file_size);
  if (!s.ok()) {
    return s;
  }
  std::unique_ptr<RandomAccessFile> file;
  s = env->NewRandomAccessFile(fname, &file);
  if (!s.ok()) {
    return s;
  }
  gtl::STLStringResizeUninitialized(data, file_size);
  char* p = gtl::string_as_array(data);
  StringPiece result;
  s = file->Read(0, file_size, &result, p);
  if (!s.ok()) {
    data->clear();
  } else if (result.size() != file_size) {
    s = Status(error::ABORTED, fname + " changed while reading " + 
		    std::to_string(file_size) + " vs. " +
		    std::to_string(result.size()));
    data->clear();
  } else if (result.data() == p) {
    ; // Data is already in the correct location
  } else {
    memmove(p, result.data(), result.size());
  }
  return s;
}

Status WriteStringToFile(Env* env, const string& fname,
		        const StringPiece& data) {
  std::unique_ptr<WritableFile> file;
  Status s = env->NewWritableFile(fname, &file);
  if (!s.ok()) {
    return s;
  }
  s = file->Append(data);
  if (s.ok()) {
    s = file->Close();
  }
  return s;
}

class FileStream : public ZeroCopyInputStream {
 public:
  explicit FileStream(RandomAccessFile* file) : file_(file), pos_(0) {}

  void BackUp(int count) override { pos_ -= count; }
  bool Skip(int count) override {
    pos_ += count;
    return true;
  }
  int64_t ByteCount() const override { return pos_; }
  Status status() const { return status_; }

  bool Next(const void** data, int* size) override {
    StringPiece result;
    Status s = file_->Read(pos_, kBufSize, &result, scratch_);
    if (result.empty()) {
      status_ = s;
      return false;
    }
    pos_ += result.size();
    *data = result.data();
    *size = result.size();
    return true;
  }
				     
 private:
  static const int kBufSize = 512 << 10;

  RandomAccessFile* file_;
  int64_t pos_;
  Status status_;
  char scratch_[kBufSize];
};

} // namespace mr
