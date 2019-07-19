#ifndef HttpAnalysis_H
#define HttpAnalysis_H

#include <unordered_map>
#include <string>

class Buffer;

enum HttpMethod
{
  METHOD_GET,
  METHOD_HEAD,
  METHOD_INVALID
};
enum HttpVersion
{
  HTTP_10 = 0,
  HTTP_11 = 1,
  UNKNOWN
};

class HttpMessage
{
public:
  HttpMessage()
      : method_(METHOD_INVALID),
        version_(UNKNOWN)
  {
  }

  void setVersion(HttpVersion v)
  {
    version_ = v;
  }

  HttpVersion getVersion() const
  {
    return version_;
  }

  bool setMethod(const char *begin, const char *end);

  HttpMethod method() const
  {
    return method_;
  }

  void setfilepath(const char *start, const char *end)
  {
    if (start == end)
    {
      filepath_ = "index.html";
      return;
    }
    filepath_.assign(start, end);
  }

  const std::string &filepath() const
  {
    return filepath_;
  }

  void setQuery(const char *start, const char *end)
  {
    query_.assign(start, end);
  }

  const std::string &query() const
  {
    return query_;
  }

  void addHeader(const char *start, const char *colon, const char *end);
  std::string getHeader(const std::string &field) const;

  void swap(HttpMessage &that);

private:
  HttpMethod method_;
  HttpVersion version_;
  std::string filepath_;
  std::string query_;
  std::unordered_map<std::string, std::string> headers_;
};

class HttpAnalysis
{
public:
  enum HttpRequestParseState
  {
    IWantRequestLine,
    IWantHeaders,
    IWantBody,
    ImOK
  };

  HttpAnalysis()
      : state_(IWantRequestLine)
  {
  }

  bool gotAll() const
  {
    return state_ == ImOK;
  }

  void reset()
  {
    state_ = IWantRequestLine;
    HttpMessage dummy;
    request_.swap(dummy);
  }

  HttpMessage& getRequest()
  {
    return request_;
  }

  bool parseRequest(Buffer *buf);

private:
  bool processRequestLine(const char *begin, const char *end);

  HttpRequestParseState state_;
  HttpMessage request_;
};

#endif