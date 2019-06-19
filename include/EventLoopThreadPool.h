#ifndef EVENTLOOPTHREADPOOL_H
#define EVENTLOOPTHREADPOOL_H

#include "noncopyable.h"
#include "Callbacks.h"

#include <vector>
#include <memory>
#include <functional>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
public:
  EventLoopThreadPool(EventLoop *baseLoop, int numThreads);
  ~EventLoopThreadPool();

  void start();
  EventLoop *getNextLoop();

private:
  EventLoop *baseLoop_;
  bool started_;
  const int numThreads_;
  int next_;
  std::vector<std::unique_ptr<EventLoopThread>> threads_;
  std::vector<EventLoop *> loops_;
};

#endif