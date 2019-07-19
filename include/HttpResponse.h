#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include "HttpAnalysis.h"

#include <string>
#include <map>
#include <unordered_map>

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

class HttpResponse
{
public:
  enum HttpStatusCode
  {
    kUnknown,
    k200Ok = 200,
    k301MovedPermanently = 301,
    k400BadRequest = 400,
    k404NotFound = 404,
  };

  explicit HttpResponse(bool close)
      : statusCode_(kUnknown),
        closeConnection_(close)
  {
  }

  void appendToBuffer(Buffer *buf, HttpMessage &message);
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
  bool closeConnection() const
  {
    return closeConnection_;
  }
  bool findFile(HttpMessage &message);

private:
  std::string makeStatueMessage(HttpStatusCode code);

  std::map<std::string, std::string> sendHeaders_;
  HttpStatusCode statusCode_;
  bool closeConnection_;
  std::string body_;
};

#endif