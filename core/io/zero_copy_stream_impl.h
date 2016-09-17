// This file contains common implementations of the interfaces defined in
// zero_copy_stream.h which are only included in the full (non-lite)
// protobuf library.  These implementations include Unix file descriptors
// and C++ iostreams.  See also:  zero_copy_stream_impl_lite.h

#ifndef GOOGLE_PROTOBUF_IO_ZERO_COPY_STREAM_IMPL_H__
#define GOOGLE_PROTOBUF_IO_ZERO_COPY_STREAM_IMPL_H__

#include <string>
#include <iosfwd>

#include "core/io/zero_copy_stream.h"
#include "core/io/zero_copy_stream_impl_lite.h"

using std::istream;
using std::ostream;

namespace mr {

// ===================================================================

// A ZeroCopyInputStream which reads from a file descriptor.
//
// FileInputStream is preferred over using an ifstream with IstreamInputStream.
// The latter will introduce an extra layer of buffering, harming performance.
// Also, it's conceivable that FileInputStream could someday be enhanced
// to use zero-copy file descriptors on OSs which support them.
class FileInputStream : public ZeroCopyInputStream {
 public:
  // Creates a stream that reads from the given Unix file descriptor.
  // If a block_size is given, it specifies the number of bytes that
  // should be read and returned with each call to Next().  Otherwise,
  // a reasonable default is used.
  explicit FileInputStream(int file_descriptor, int block_size = -1);
  ~FileInputStream();

  // Flushes any buffers and closes the underlying file.  Returns false if
  // an error occurs during the process; use GetErrno() to examine the error.
  // Even if an error occurs, the file descriptor is closed when this returns.
  bool Close();

  // By default, the file descriptor is not closed when the stream is
  // destroyed.  Call SetCloseOnDelete(true) to change that.  WARNING:
  // This leaves no way for the caller to detect if close() fails.  If
  // detecting close() errors is important to you, you should arrange
  // to close the descriptor yourself.
  void SetCloseOnDelete(bool value) { copying_input_.SetCloseOnDelete(value); }

  // If an I/O error has occurred on this file descriptor, this is the
  // errno from that error.  Otherwise, this is zero.  Once an error
  // occurs, the stream is broken and all subsequent operations will
  // fail.
  int GetErrno() { return copying_input_.GetErrno(); }

  // implements ZeroCopyInputStream ----------------------------------
  bool Next(const void** data, int* size);
  void BackUp(int count);
  bool Skip(int count);
  int64_t ByteCount() const;

 private:
  class CopyingFileInputStream : public CopyingInputStream {
   public:
    CopyingFileInputStream(int file_descriptor);
    ~CopyingFileInputStream();

    bool Close();
    void SetCloseOnDelete(bool value) { close_on_delete_ = value; }
    int GetErrno() { return errno_; }

    // implements CopyingInputStream ---------------------------------
    int Read(void* buffer, int size);
    int Skip(int count);

   private:
    // The file descriptor.
    const int file_;
    bool close_on_delete_;
    bool is_closed_;

    // The errno of the I/O error, if one has occurred.  Otherwise, zero.
    int errno_;

    // Did we try to seek once and fail?  If so, we assume this file descriptor
    // doesn't support seeking and won't try again.
    bool previous_seek_failed_;

    DISALLOW_COPY_AND_ASSIGN(CopyingFileInputStream);
  };

  CopyingFileInputStream copying_input_;
  CopyingInputStreamAdaptor impl_;

  DISALLOW_COPY_AND_ASSIGN(FileInputStream);
};

// ===================================================================

// A ZeroCopyOutputStream which writes to a file descriptor.
//
// FileOutputStream is preferred over using an ofstream with
// OstreamOutputStream.  The latter will introduce an extra layer of buffering,
// harming performance.  Also, it's conceivable that FileOutputStream could
// someday be enhanced to use zero-copy file descriptors on OSs which
// support them.
class FileOutputStream : public ZeroCopyOutputStream {
 public:
  // Creates a stream that writes to the given Unix file descriptor.
  // If a block_size is given, it specifies the size of the buffers
  // that should be returned by Next().  Otherwise, a reasonable default
  // is used.
  explicit FileOutputStream(int file_descriptor, int block_size = -1);
  ~FileOutputStream();

  // Flushes any buffers and closes the underlying file.  Returns false if
  // an error occurs during the process; use GetErrno() to examine the error.
  // Even if an error occurs, the file descriptor is closed when this returns.
  bool Close();

  // Flushes FileOutputStream's buffers but does not close the
  // underlying file. No special measures are taken to ensure that
  // underlying operating system file object is synchronized to disk.
  bool Flush();

  // By default, the file descriptor is not closed when the stream is
  // destroyed.  Call SetCloseOnDelete(true) to change that.  WARNING:
  // This leaves no way for the caller to detect if close() fails.  If
  // detecting close() errors is important to you, you should arrange
  // to close the descriptor yourself.
  void SetCloseOnDelete(bool value) { copying_output_.SetCloseOnDelete(value); }

  // If an I/O error has occurred on this file descriptor, this is the
  // errno from that error.  Otherwise, this is zero.  Once an error
  // occurs, the stream is broken and all subsequent operations will
  // fail.
  int GetErrno() { return copying_output_.GetErrno(); }

  // implements ZeroCopyOutputStream ---------------------------------
  bool Next(void** data, int* size);
  void BackUp(int count);
  int64_t ByteCount() const;

 private:
  class CopyingFileOutputStream : public CopyingOutputStream {
   public:
    CopyingFileOutputStream(int file_descriptor);
    ~CopyingFileOutputStream();

    bool Close();
    void SetCloseOnDelete(bool value) { close_on_delete_ = value; }
    int GetErrno() { return errno_; }

    // implements CopyingOutputStream --------------------------------
    bool Write(const void* buffer, int size);

   private:
    // The file descriptor.
    const int file_;
    bool close_on_delete_;
    bool is_closed_;

    // The errno of the I/O error, if one has occurred.  Otherwise, zero.
    int errno_;

    DISALLOW_COPY_AND_ASSIGN(CopyingFileOutputStream);
  };

  CopyingFileOutputStream copying_output_;
  CopyingOutputStreamAdaptor impl_;

  DISALLOW_COPY_AND_ASSIGN(FileOutputStream);
};

// ===================================================================

// A ZeroCopyInputStream which reads from a C++ istream.
//
// Note that for reading files (or anything represented by a file descriptor),
// FileInputStream is more efficient.
class IstreamInputStream : public ZeroCopyInputStream {
 public:
  // Creates a stream that reads from the given C++ istream.
  // If a block_size is given, it specifies the number of bytes that
  // should be read and returned with each call to Next().  Otherwise,
  // a reasonable default is used.
  explicit IstreamInputStream(istream* stream, int block_size = -1);
  ~IstreamInputStream();

  // implements ZeroCopyInputStream ----------------------------------
  bool Next(const void** data, int* size);
  void BackUp(int count);
  bool Skip(int count);
  int64_t ByteCount() const;

 private:
  class CopyingIstreamInputStream : public CopyingInputStream {
   public:
    CopyingIstreamInputStream(istream* input);
    ~CopyingIstreamInputStream();

    // implements CopyingInputStream ---------------------------------
    int Read(void* buffer, int size);
    // (We use the default implementation of Skip().)

   private:
    // The stream.
    istream* input_;

    DISALLOW_COPY_AND_ASSIGN(CopyingIstreamInputStream);
  };

  CopyingIstreamInputStream copying_input_;
  CopyingInputStreamAdaptor impl_;

  DISALLOW_COPY_AND_ASSIGN(IstreamInputStream);
};

// ===================================================================

// A ZeroCopyOutputStream which writes to a C++ ostream.
//
// Note that for writing files (or anything represented by a file descriptor),
// FileOutputStream is more efficient.
class OstreamOutputStream : public ZeroCopyOutputStream {
 public:
  // Creates a stream that writes to the given C++ ostream.
  // If a block_size is given, it specifies the size of the buffers
  // that should be returned by Next().  Otherwise, a reasonable default
  // is used.
  explicit OstreamOutputStream(std::ostream* stream, int block_size = -1);
  ~OstreamOutputStream();

  // implements ZeroCopyOutputStream ---------------------------------
  bool Next(void** data, int* size);
  void BackUp(int count);
  int64_t ByteCount() const;

 private:
  class CopyingOstreamOutputStream : public CopyingOutputStream {
   public:
    CopyingOstreamOutputStream(std::ostream* output);
    ~CopyingOstreamOutputStream();

    // implements CopyingOutputStream --------------------------------
    bool Write(const void* buffer, int size);

   private:
    // The stream.
    std::ostream* output_;

    DISALLOW_COPY_AND_ASSIGN(CopyingOstreamOutputStream);
  };

  CopyingOstreamOutputStream copying_output_;
  CopyingOutputStreamAdaptor impl_;

  DISALLOW_COPY_AND_ASSIGN(OstreamOutputStream);
};

// ===================================================================

// A ZeroCopyInputStream which reads from several other streams in sequence.
// ConcatenatingInputStream is unable to distinguish between end-of-stream
// and read errors in the underlying streams, so it assumes any errors mean
// end-of-stream.  So, if the underlying streams fail for any other reason,
// ConcatenatingInputStream may do odd things.  It is suggested that you do
// not use ConcatenatingInputStream on streams that might produce read errors
// other than end-of-stream.
class ConcatenatingInputStream : public ZeroCopyInputStream {
 public:
  // All streams passed in as well as the array itself must remain valid
  // until the ConcatenatingInputStream is destroyed.
  ConcatenatingInputStream(ZeroCopyInputStream* const streams[], int count);
  ~ConcatenatingInputStream();

  // implements ZeroCopyInputStream ----------------------------------
  bool Next(const void** data, int* size);
  void BackUp(int count);
  bool Skip(int count);
  int64_t ByteCount() const;


 private:
  // As streams are retired, streams_ is incremented and count_ is
  // decremented.
  ZeroCopyInputStream* const* streams_;
  int stream_count_;
  int64_t bytes_retired_;  // Bytes read from previous streams.

  DISALLOW_COPY_AND_ASSIGN(ConcatenatingInputStream);
};

// ===================================================================

// A ZeroCopyInputStream which wraps some other stream and limits it to
// a particular byte count.
class LimitingInputStream : public ZeroCopyInputStream {
 public:
  LimitingInputStream(ZeroCopyInputStream* input, int64_t limit);
  ~LimitingInputStream();

  // implements ZeroCopyInputStream ----------------------------------
  bool Next(const void** data, int* size);
  void BackUp(int count);
  bool Skip(int count);
  int64_t ByteCount() const;


 private:
  ZeroCopyInputStream* input_;
  int64_t limit_;  // Decreases as we go, becomes negative if we overshoot.
  int64_t prior_bytes_read_;  // Bytes read on underlying stream at construction

  DISALLOW_COPY_AND_ASSIGN(LimitingInputStream);
};

// ===================================================================

}  // namespace google
#endif  // GOOGLE_PROTOBUF_IO_ZERO_COPY_STREAM_IMPL_H__
