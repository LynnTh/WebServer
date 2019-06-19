#ifndef _CONDITION_H_
#define _CONDITION_H_

#include "Mutex.h"

#include <pthread.h>

class Condition : noncopyable
{
private:
  MutexLock &mutex_;
  pthread_cond_t pcond_;

public:
  Condition(MutexLock &mutex)
      : mutex_(mutex)
  {
    pthread_cond_init(&pcond_, NULL);
  }

  ~Condition()
  {
    pthread_cond_destroy(&pcond_);
  }

  void wait()
  {
    MutexLock::UnassignGuard ug(mutex_);
    pthread_cond_wait(&pcond_, mutex_.getPthreadMutex());
  }

  bool waitForSeconds(double seconds); // returns true if time out, false otherwise.

  void notify()
  {
    pthread_cond_signal(&pcond_);
  }

  void notifyall()
  {
    pthread_cond_broadcast(&pcond_);
  }
};

#endif