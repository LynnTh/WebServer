#include "LogFile.h"
#include "Mutex.h"
#include "FileUtil.h"

#include <assert.h>
#include <stdio.h>
#include <time.h>

LogFile::LogFile(const std::string basename, int checkEveryN, int RollFileN)
    : basename_(basename),
      checkEveryN_(checkEveryN),
      RollFileN_(RollFileN),
      count_checkEveryN(0),
      count_RollFileN(0)
{
  // assert(basename.find('/') == std::string::npos);
  rollFile();
}

LogFile::~LogFile()
{
}

void LogFile::flush()
{
  file_->flush();
}

void LogFile::append(const char *logline, int len)
{
  file_->append(logline, len);
  count_checkEveryN++;
  count_RollFileN++;
  if (count_checkEveryN >= checkEveryN_)
  {
    count_checkEveryN = 0;
    file_->flush();
  }
  if (count_RollFileN >= RollFileN_)
  {
    count_RollFileN = 0;
    rollFile();
  }
}

void LogFile::rollFile()
{
  time_t now = 0;
  std::string filename = getLogFileName(&now);
  file_.reset(new FileWriter(filename));
}

std::string LogFile::getLogFileName(time_t *now)
{
  std::string filename;
  filename.reserve(basename_.size() + 64);
  filename = basename_;

  char timebuf[32];
  struct tm tm;
  *now = time(NULL);
  gmtime_r(now, &tm);
  strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm);
  filename += timebuf;
  filename += ".log";

  return filename;
}