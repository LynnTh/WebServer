#include "HttpAnalysis.h"
#include "Buffer.h"

#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

/*
    HttpMessage
*/

bool HttpMessage::setMethod(const char *begin, const char *end)
{
  std::string m(begin, end);
  if (m == "GET")
  {
    method_ = METHOD_GET;
  }
  else if (m == "HEAD")
  {
    method_ = METHOD_HEAD;
  }
  else
  {
    method_ = METHOD_INVALID;
  }
  return method_ != METHOD_INVALID;
}

void HttpMessage::addHeader(const char *start, const char *colon, const char *end)
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
  headers_[field] = value;
}

std::string HttpMessage::getHeader(const std::string &field) const
{
  std::string result;
  auto it = headers_.find(field);
  if (it != headers_.end())
  {
    result = it->second;
  }
  return result;
}

void HttpMessage::swap(HttpMessage &that)
{
  std::swap(method_, that.method_);
  std::swap(version_, that.version_);
  filepath_.swap(that.filepath_);
  query_.swap(that.query_);
  headers_.swap(that.headers_);
}

/*
    HttpMessage over
*/

/*
    HttpAnalysis
*/

bool HttpAnalysis::processRequestLine(const char *begin, const char *end)
{
  bool succeed = false;
  const char *start = begin;
  const char *space = std::find(start, end, ' ');
  if (space != end && request_.setMethod(start, space))
  {
    start = space + 1;
    space = std::find(start, end, ' ');
    if (space != end)
    {
      const char *question = std::find(start, space, '?');
      if (question != space)
      {
        request_.setfilepath(start + 1, question);
        request_.setQuery(question, space);
      }
      else
      {
        request_.setfilepath(start + 1, space);
      }
      start = space + 1;
      succeed = end - start == 8 && std::equal(start, end - 1, "HTTP/1.");
      if (succeed)
      {
        if (*(end - 1) == '1')
        {
          request_.setVersion(HTTP_11);
        }
        else if (*(end - 1) == '0')
        {
          request_.setVersion(HTTP_10);
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

bool HttpAnalysis::parseRequest(Buffer *buf)
{
  bool ok = false;
  bool hasMore = true;
  while (hasMore)
  {
    if (state_ == IWantRequestLine)
    {
      const char *crlf = buf->findCRLF();
      if (crlf != NULL)
      {
        ok = processRequestLine(buf->peek(), crlf);
        if (ok)
        {
          buf->retrieveUntil(crlf + 2);
          state_ = IWantHeaders;
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
    else if (state_ == IWantHeaders)
    {
      const char *crlf = buf->findCRLF();
      if (crlf)
      {
        const char *colon = std::find(buf->peek(), crlf, ':');
        if (colon != crlf)
        {
          request_.addHeader(buf->peek(), colon, crlf);
        }
        else
        {
          state_ = IWantBody;
          hasMore = true;
        }
        buf->retrieveUntil(crlf + 2);
      }
      else
      {
        hasMore = false;
      }
    }
    else if (state_ == IWantBody)
    {
      // int content_length = -1;
      // std::string clen = request_.getHeader("Content-length");
      // if (!clen.empty())
      // {
      //   content_length = stoi(clen);
      // }
      // else
      // {
      //   ok = false;
      //   buf->retrieveAll();
      //   break;
      // }
      // buf->retrieve(content_length);
      buf->retrieveAll();
      hasMore = false;
      state_ = ImOK;
    }
  }
  return ok;
}

/*
    HttpAnalysis over
*/