#ifndef LOGGING_H
#define LOGGING_H

#include "LogStream.h"

#include <string>
#include <pthread.h>

class AsyncLogging;

class Logger
{
public:
  enum LogLevel
  {
    INFO,
    FATAL,
  };

  Logger(const char *fileName, int line);
  Logger(const char *fileName, int line, LogLevel level);
  ~Logger();
  LogStream &stream() { return impl_.stream_; }

  static void setLogFileName(std::string fileName)
  {
    logFileName_ = fileName;
  }
  static std::string getLogFileName()
  {
    return logFileName_;
  }

private:
  class Impl
  {
  public:
    Impl(const char *fileName, int line);
    void formatTime();

    LogStream stream_;
    int line_;
    std::string basename_;
  };

  Impl impl_;
  LogLevel level_;
  static std::string logFileName_;
};

#define LOG_INFO Logger(__FILE__, __LINE__).stream()
#define LOG_FATAL Logger(__FILE__, __LINE__, Logger::FATAL).stream()

#endif