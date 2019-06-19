#ifndef LOGFILE_H
#define LOGFILE_H

#include "noncopyable.h"

#include <memory>

/* 只在日志线程运行，无需锁 */

class FileWriter;

static const int InitcheckEveryN = 1024; // 默认每append 1024次就调用FileWriter
static const int InitRollFileN = 40960;

class LogFile : noncopyable
{
public:
  LogFile(const std::string basename, int checkEveryN = InitcheckEveryN, int RollFileN = InitRollFileN);
  ~LogFile();

  void append(const char *logline, int len);
  void flush();
  void rollFile();

private:
  std::string getLogFileName(time_t *now);

  const std::string basename_;
  const int checkEveryN_;
  const int RollFileN_;

  int count_checkEveryN;
  int count_RollFileN;
  std::unique_ptr<FileWriter> file_;
};

#endif
