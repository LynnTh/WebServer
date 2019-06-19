#ifndef _COUNTDOWNLATCH_H_
#define _COUNTDOWNLATCH_H_

#include "Mutex.h"
#include "Condition.h"

class CountDownLatch : noncopyable
{
public:
  explicit CountDownLatch(int count);
  ~CountDownLatch() = default;
  void wait();
  void countDown();
  int getCount() const;

private:
  mutable MutexLock mutex_;
  Condition condition_;
  int count_;
};

#endif