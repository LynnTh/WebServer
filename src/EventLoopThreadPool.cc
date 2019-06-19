#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"
#include "EventLoop.h"
#include "Logging.h"

#include <stdio.h>

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, int numThreads)
    : baseLoop_(baseLoop),
      started_(false),
      numThreads_(numThreads),
      next_(0)
{
  if (numThreads <= 0)
  {
    LOG_FATAL << "EventLoopThreadPool : numThreads <= 0";
  }
}

EventLoopThreadPool::~EventLoopThreadPool()
{
  LOG_INFO << "EventLoopThreadPool over.";
}

void EventLoopThreadPool::start()
{
  assert(!started_);
  baseLoop_->assertInLoopThread();
  started_ = true;
  for (int i = 0; i < numThreads_; i++)
  {
    EventLoopThread *t = new EventLoopThread();
    threads_.push_back(std::unique_ptr<EventLoopThread>(t));
    loops_.push_back(t->startLoop());
  }
}

EventLoop *EventLoopThreadPool::getNextLoop()
{
  baseLoop_->assertInLoopThread();
  assert(started_);
  EventLoop *loop = baseLoop_;
  if (!loops_.empty())
  {
    loop = loops_[next_];
    next_ = (next_ + 1) % numThreads_;
  }
  return loop;
}