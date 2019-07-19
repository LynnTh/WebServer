#include "HttpResponse.h"
#include "Buffer.h"

#include <unordered_map>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

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

void HttpResponse::appendToBuffer(Buffer *output, HttpMessage &message)
{
  char buf[32];
  setStatusCode(k200Ok);
  std::string mess = makeStatueMessage(statusCode_); // 待设置
  snprintf(buf, sizeof buf, "HTTP/1.%d %d ", message.getVersion(), statusCode_);
  output->append(buf, strlen(buf));
  output->append(mess);
  output->append("\r\n");

  if (closeConnection_)
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
  if (message.method() == METHOD_GET)
  {
    output->append("\r\n");
    output->append(body_);
  }
  else if (message.method() == METHOD_HEAD)
  {
    output->append("\r\n");
  }
}

bool HttpResponse::findFile(HttpMessage &message)
{
  struct stat sbuf;
  std::string fullpath = message.filepath();
  // for test
  if (fullpath == "hello")
  {
    body_ = "Hello World";
    return true;
  }

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

  int dot_pos = fullpath.find('.');
  std::string filetype;
  if (dot_pos < 0)
    filetype = MimeType::getMime("default");
  else
    filetype = MimeType::getMime(fullpath.substr(dot_pos));
  setContentType(filetype);

  return true;
}

std::string HttpResponse::makeStatueMessage(HttpStatusCode code)
{
  if (code == k200Ok)
  {
    return " OK";
  }
  else if (code == k301MovedPermanently)
  {
    return " Moved Permanently";
  }
  else if (code == k400BadRequest)
  {
    return " Bad Request";
  }
  else if (code == k404NotFound)
  {
    return " Not Found";
  }
  else
  {
    return " Unknow";
  }
}