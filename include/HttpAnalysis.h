#ifndef HTTPANALYSIS_H
#define HTTPANALYSIS_H

#include "noncopyable.h"

#include <string>
#include <map>
#include <unordered_map>
#include <string.h>

enum HttpRequestParseState
{
  kExpectRequestLine,
  kExpectHeaders,
  kGotAll,
};

enum HttpMethod
{
  METHOD_GET,
  METHOD_HEAD,
  METHOD_INVALID
};

enum HttpVersion
{
  HTTP_10 = 0,
  HTTP_11 = 1
};

struct HttpMessage
{
  HttpMethod method_;
  HttpVersion version_;
  std::string path_;
  std::string filename_;
  std::string query_;
  bool keep_alive_;
};

enum HttpStatusCode
{
  kUnknown,
  k200Ok = 200,
  k301MovedPermanently = 301,
  k400BadRequest = 400,
  k404NotFound = 404,
};

class Buffer;

class MimeType
{
private:
    static void init();
    static std::unordered_map<std::string, std::string> mime;
    MimeType();
    MimeType(const MimeType &m);

public:
    static std::string getMime(const std::string &suffix);

private:
    static pthread_once_t once_control;
};

class HttpAnalysis : noncopyable
{
public:
  HttpAnalysis();
  ~HttpAnalysis();

  // 接收
  void reset()
  {
    state_ = kExpectRequestLine;
    memset(&message_, 0, sizeof(HttpMessage));
    receiveHeaders_.clear();
  }
  const HttpMessage &message()
  {
    return message_;
  }
  bool gotAll() const
  {
    return state_ == kGotAll;
  }
  const HttpVersion version()
  {
    return message_.version_;
  }

  bool parseRequest(Buffer *buf);
  bool findFile();

  // 发送
  void appendToBuffer(Buffer *buf);
  void setContentType(const std::string &contentType)
  {
    addHeader("Content-Type", contentType);
  }
  void addHeader(const std::string &key, const std::string &value)
  {
    sendHeaders_[key] = value;
  }
  void setStatusCode(HttpStatusCode code)
  {
    statusCode_ = code;
  }
  // void setCloseConnection(bool on)
  // {
  //   closeConnection_ = on;
  // }
  bool analysis(const HttpMessage &message);

private:
  // 接收
  bool processRequestLine(const char *begin, const char *end);
  bool setMethod(const char *begin, const char *end);
  void setPath(const char *begin, const char *end);
  void setFilename(const std::string &filename);
  void setQuery(const char *begin, const char *end);
  void setVersion(HttpVersion version);
  void setAlive();
  void addHeader(const char *start, const char *colon, const char *end);

  // 发送
  std::string makeStatueMessage(HttpStatusCode code);

  HttpRequestParseState state_;
  HttpMessage message_;
  std::map<std::string, std::string> receiveHeaders_;

  std::map<std::string, std::string> sendHeaders_;
  HttpStatusCode statusCode_;
  std::string body_;
};

#endif