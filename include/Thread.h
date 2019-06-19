#ifndef _THREAD_H_
#define _THREAD_H_

#include "Atomic.h"
#include "CountDownLatch.h"

#include <functional>
#include <memory>
#include <pthread.h>
#include <string>

class Thread : noncopyable
{
public:
  typedef std::function<void()> ThreadFunc;

  explicit Thread(ThreadFunc, const std::string &name = std::string());
  ~Thread();

  void start();
  int join(); // return pthread_join()

  bool started() const { return started_; }
  pid_t tid() const { return tid_; }
  const std::string &name() const { return name_; }

  static int numCreated() { return numCreated_.get(); }

private:
  void setDefaultName();

  bool started_;
  bool joined_;
  pthread_t pthreadId_; // Thread ID
  pid_t tid_;           // Process ID
  ThreadFunc func_;
  std::string name_;
  CountDownLatch latch_;

  static AtomicInt32 numCreated_;
};

#endif