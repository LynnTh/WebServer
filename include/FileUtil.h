#ifndef FILEUTIL_H
#define FILEUTIL_H

#include "noncopyable.h"
#include <string>

class FileWriter : noncopyable
{
public:
  explicit FileWriter(std::string filename);
  ~FileWriter();
  void append(const char *logline, const size_t len); // 实际写入文件的函数
  void flush();

private:
  size_t write(const char *logline, const size_t len);

  FILE *fp_;
  char buf[64 * 1024];
};

#endif
