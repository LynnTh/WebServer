#include "HttpAnalysis.h"
#include "Buffer.h"

#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <iostream>

pthread_once_t MimeType::once_control = PTHREAD_ONCE_INIT;
std::unordered_map<std::string, std::string> MimeType::mime;

void MimeType::init()
{
  mime[".html"] = "text/html";
  mime[".avi"] = "video/x-msvideo";
  mime[".bmp"] = "image/bmp";
  mime[".c"] = "text/plain";
  mime[".doc"] = "application/msword";
  mime[".gif"] = "image/gif";
  mime[".gz"] = "application/x-gzip";
  mime[".htm"] = "text/html";
  mime[".ico"] = "image/x-icon";
  mime[".jpg"] = "image/jpeg";
  mime[".png"] = "image/png";
  mime[".txt"] = "text/plain";
  mime[".mp3"] = "audio/mp3";
  mime[".css"] = "text/css";
  mime[".js"] = "application/x-javascript";
  mime[".ttf"] = "application/octet-stream";
  mime["default"] = "text/html";
}

std::string MimeType::getMime(const std::string &suffix)
{
  pthread_once(&once_control, MimeType::init);
  if (mime.find(suffix) == mime.end())
    return mime["default"];
  else
    return mime[suffix];
}

HttpAnalysis::HttpAnalysis()
    : state_(kExpectRequestLine)
{
}

HttpAnalysis::~HttpAnalysis()
{}

bool HttpAnalysis::setMethod(const char *begin, const char *end)
{
  std::string m(begin, end);
  if (m == "GET")
  {
    message_.method_ = METHOD_GET;
  }
  else if (m == "HEAD")
  {
    message_.method_ = METHOD_HEAD;
  }
  else
  {
    message_.method_ = METHOD_INVALID;
  }
  return message_.method_ != METHOD_INVALID;
}

void HttpAnalysis::setPath(const char *begin, const char *end)
{
  message_.path_.assign(begin, end);
}

void HttpAnalysis::setFilename(const std::string &filename)
{
  message_.filename_ = filename;
}

void HttpAnalysis::setQuery(const char *begin, const char *end)
{
  message_.query_.assign(begin, end);
}

void HttpAnalysis::setVersion(HttpVersion version)
{
  message_.version_ = version;
}

void HttpAnalysis::setAlive()
{
  message_.keep_alive_ = false;
  if (receiveHeaders_.find("Connection") != receiveHeaders_.end() &&
      (receiveHeaders_["Connection"] == "Keep-Alive" || receiveHeaders_["Connection"] == "keep-alive"))
  {
    message_.keep_alive_ = true;
  }
  else if (message_.version_ == HTTP_11 && receiveHeaders_.find("Connection") == receiveHeaders_.end())
  {
    message_.keep_alive_ = true;
  }
}

bool HttpAnalysis::processRequestLine(const char *begin, const char *end)
{
  bool succeed = false;
  const char *start = begin;
  const char *space = std::find(start, end, ' ');
  if (space != end && setMethod(start, space))
  {
    start = space + 1;
    space = std::find(start, end, ' ');
    if (space != end)
    {
      const char *question = std::find(start, space, '?');
      if (question != space)
      {
        setPath(start, question);
        setQuery(question, space);
      }
      else
      {
        setPath(start, space);
      }
      size_t pos = message_.path_.find("/");
      std::string filename = message_.path_.substr(pos + 1);
      if (filename.empty())
      {
        filename = "index.html";
      }
      setFilename(filename);
      start = space + 1;
      succeed = end - start == 8 && std::equal(start, end - 1, "HTTP/1.");
      if (succeed)
      {
        if (*(end - 1) == '1')
        {
          setVersion(HTTP_11);
        }
        else if (*(end - 1) == '0')
        {
          setVersion(HTTP_10);
        }
        else
        {
          succeed = false;
        }
      }
    }
  }
  return succeed;
}

void HttpAnalysis::addHeader(const char *start, const char *colon, const char *end)
{
  std::string field(start, colon);
  ++colon;
  while (colon < end && isspace(*colon))
  {
    ++colon;
  }
  std::string value(colon, end);
  while (!value.empty() && isspace(value[value.size() - 1]))
  {
    value.resize(value.size() - 1);
  }
  receiveHeaders_[field] = value;
}

bool HttpAnalysis::parseRequest(Buffer *buf)
{
  bool ok = false;
  bool hasMore = true;
  while (hasMore)
  {
    if (state_ == kExpectRequestLine)
    {
      const char *crlf = buf->findCRLF();
      if (crlf != NULL)
      {
        ok = processRequestLine(buf->peek(), crlf);
        if (ok)
        {
          buf->retrieveUntil(crlf + 2);
          state_ = kExpectHeaders;
        }
        else
        {
          hasMore = false;
        }
      }
      else
      {
        hasMore = false;
      }
    }
    else if (state_ == kExpectHeaders)
    {
      const char *crlf = buf->findCRLF();
      if (crlf)
      {
        const char *colon = std::find(buf->peek(), crlf, ':');
        if (colon != crlf)
        {
          addHeader(buf->peek(), colon, crlf);
        }
        else
        {
          state_ = kGotAll;
          hasMore = true;
        }
        buf->retrieveUntil(crlf + 2);
      }
      else
      {
        hasMore = false;
      }
    }
    else if (state_ == kGotAll)
    {
      setAlive();
      setStatusCode(k200Ok);
      hasMore = false;
    }
  }
  return ok;
}

bool HttpAnalysis::findFile()
{
  struct stat sbuf;
  std::string fullpath = message_.filename_;
  if (stat(fullpath.c_str(), &sbuf) < 0)
  {
    return false;
  }
  int src_fd = ::open(fullpath.c_str(), O_RDONLY, 0);
  if (src_fd < 0)
  {
    return false;
  }
  void *mmapRet = mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
  ::close(src_fd);
  if (mmapRet == (void *)-1)
  {
    return false;
  }
  char *src_addr = static_cast<char *>(mmapRet);
  body_ = std::string(src_addr, src_addr + sbuf.st_size);
  munmap(mmapRet, sbuf.st_size);

  int dot_pos = message_.filename_.find('.');
  std::string filetype;
  if (dot_pos < 0)
    filetype = MimeType::getMime("default");
  else
    filetype = MimeType::getMime(message_.filename_.substr(dot_pos));
  setContentType(filetype);

  return true;
}

void HttpAnalysis::appendToBuffer(Buffer *output)
{
  char buf[32];
  std::string mess = makeStatueMessage(statusCode_); // 待设置
  snprintf(buf, sizeof buf, "HTTP/1.%d %d ", message_.version_, statusCode_);
  output->append(buf, strlen(buf));
  output->append(mess);
  output->append("\r\n");

  if (!message_.keep_alive_)
  {
    output->append("Connection: close\r\n");
  }
  else
  {
    snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", body_.size());
    output->append(buf);
    output->append("Connection: Keep-Alive\r\n");
  }

  for (const auto &header : sendHeaders_)
  {
    output->append(header.first);
    output->append(": ");
    output->append(header.second);
    output->append("\r\n");
  }
  if (message_.method_ == METHOD_GET)
  {
    output->append("\r\n");
    output->append(body_);
  }
}

std::string HttpAnalysis::makeStatueMessage(HttpStatusCode code)
{
  if (code == k200Ok)
  {
    return "OK";
  }
  else if (code == k301MovedPermanently)
  {
    return "Moved Permanently";
  }
  else if (code == k400BadRequest)
  {
    return "Bad Request";
  }
  else if (code == k404NotFound)
  {
    return "Not Found";
  }
  else
  {
    return "Unknow";
  }
}