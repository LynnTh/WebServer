#ifndef BUFFER_H
#define BUFFER_H

#include <algorithm>
#include <vector>
#include <assert.h>
#include <string.h>
#include <string>

class Buffer
{
public:
  static const size_t kCheapPrepend = 8;
  static const size_t kInitialSize = 1024;

  explicit Buffer(size_t InitialSize = kInitialSize)
      : buffer_(InitialSize),
        readerIndex_(kCheapPrepend),
        writerIndex_(kCheapPrepend)
  {
  }

  size_t readableBytes() const
  {
    return writerIndex_ - readerIndex_;
  }

  size_t writableBytes() const
  {
    return buffer_.size() - writerIndex_;
  }

  size_t prependableBytes() const
  {
    return readerIndex_;
  }

  const char *peek() const
  {
    return begin() + readerIndex_;
  }

  const char* findCRLF() const
  {
    const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF+2);
    return crlf == beginWrite() ? NULL : crlf;
  }

  const char* findCRLF(const char* start) const
  {
    assert(peek() <= start);
    assert(start <= beginWrite());
    const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF+2);
    return crlf == beginWrite() ? NULL : crlf;
  }

  void retrieve(size_t len)
  {
    assert(len <= readableBytes());
    if (len < readableBytes())
    {
      readerIndex_ += len;
    }
    else
    {
      retrieveAll();
    }
  }

  void retrieveAll()
  {
    readerIndex_ = kCheapPrepend;
    writerIndex_ = kCheapPrepend;
  }

  void retrieveUntil(const char* end)
  {
    assert(peek() <= end);
    assert(end <= beginWrite());
    retrieve(end - peek());
  }

  std::string retrieveAllAsString()
  {
    return retrieveAsString(readableBytes());
  }

  std::string retrieveAsString(size_t len)
  {
    assert(len <= readableBytes());
    std::string result(peek(), len);
    retrieve(len);
    return result;
  }

  void append(const char * /*restrict*/ data, size_t len)
  {
    ensureWritableBytes(len);
    std::copy(data, data + len, beginWrite());
    hasWritten(len);
  }

  void append(const void * /*restrict*/ data, size_t len)
  {
    append(static_cast<const char *>(data), len);
  }

  void append(const std::string& data)
  {
    append(data.data(),data.size());
  }

  void append(const std::string&& data)
  {
    append(data.data(),data.size());
  }

  void ensureWritableBytes(size_t len)
  {
    if (writableBytes() < len)
    {
      makeSpace(len);
    }
    assert(writableBytes() >= len);
  }

  char *beginWrite()
  {
    return begin() + writerIndex_;
  }

  const char *beginWrite() const
  {
    return begin() + writerIndex_;
  }

  void hasWritten(size_t len)
  {
    assert(len <= writableBytes());
    writerIndex_ += len;
  }

  ssize_t readFd(int fd, int *savedErrno);

private:
  char *begin()
  {
    return &*buffer_.begin();
  }

  const char *begin() const
  {
    return &*buffer_.begin();
  }

  void makeSpace(size_t len)
  {
    if (writableBytes() + prependableBytes() < len + kCheapPrepend)
    {
      buffer_.resize(writerIndex_ + len);
    }
    else
    {

      assert(kCheapPrepend < readerIndex_);
      size_t readable = readableBytes();
      std::copy(begin() + readerIndex_,
                begin() + writerIndex_,
                begin() + kCheapPrepend);
      readerIndex_ = kCheapPrepend;
      writerIndex_ = readerIndex_ + readable;
      assert(readable == readableBytes());
    }
  }

  std::vector<char> buffer_;
  size_t readerIndex_;
  size_t writerIndex_;

  static const char kCRLF[];
};

#endif