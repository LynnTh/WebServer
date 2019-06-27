#include "EventLoop.h"
#include "Channel.h"
#include "EPoller.h"
#include "Logging.h"
#include "Timer.h"
#include "Timestamp.h"

#include <assert.h>
#include <sys/eventfd.h>
#include <unistd.h>

__thread EventLoop *t_loopInThisThread = 0;
const int kPollTimeMs = 10000;

int createEventfd()
{
  int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evtfd < 0)
  {
    LOG_FATAL << "Failed in eventfd";
  }
  return evtfd;
}

EventLoop::EventLoop()
    : looping_(false),
      quit_(true),
      callingPendingFunctors_(false),
      threadId_(CurrentThread::tid()),
      poller_(new EPoller(this)),
      timer_(new Timer(this)),
      WakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(this, WakeupFd_))
{
  LOG_INFO << "EventLoop created " << this << " in thread " << threadId_;
  if (t_loopInThisThread)
  {
    LOG_FATAL << "Another EventLoop exists in this thread.";
  }
  else
  {
    t_loopInThisThread = this;
  }
  wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleWakeup, this));
  wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
  assert(!looping_);
  wakeupChannel_->disableAll();
  wakeupChannel_->remove();
  t_loopInThisThread = 0;
}

void EventLoop::quit()
{
  quit_ = true;
  if (!isInLoopThread())
  {
    wakeup();
  }
}

void EventLoop::loop()
{
  assert(!looping_);
  assertInLoopThread();
  looping_ = true;
  quit_ = false;

  while (!quit_)
  {
    activeChannels_.clear();
    poller_->poll(kPollTimeMs, &activeChannels_);
    for (ChannelList::iterator it = activeChannels_.begin(); it != activeChannels_.end(); it++)
    {
      (*it)->handleEvent();
    }
    doPendingFunctors();
  }
  looping_ = false;
}

void EventLoop::updateChannel(std::shared_ptr<Channel> channel)
{
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  poller_->updateChannel(channel);
}

void EventLoop::removeChannel(std::shared_ptr<Channel> channel)
{
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  poller_->removeChannel(channel);
}

void EventLoop::wakeup()
{
  uint64_t tag = 1;
  ssize_t n = ::write(WakeupFd_, &tag, sizeof tag);
  if (n != sizeof tag)
  {
    LOG_INFO << "ERROR : EventLoop::wakeup() writes " << n << " bytes instead of 8";
  }
}

void EventLoop::handleWakeup()
{
  uint64_t tag = 1;
  ssize_t n = ::read(WakeupFd_, &tag, sizeof tag);
  if (n != sizeof tag)
  {
    LOG_INFO << "ERROR : EventLoop::wakeup() reads " << n << " bytes instead of 8";
  }
}

void EventLoop::runInLoop(EventLoop::Functor cb)
{
  if (isInLoopThread())
    cb();
  else
    queueInLoop(std::move(cb));
}

void EventLoop::queueInLoop(EventLoop::Functor cb)
{
  {
    MutexLockGuard lock(mutex_);
    pendingFunctors_.push_back(std::move(cb));
  }
  if (!isInLoopThread() || callingPendingFunctors_)
  {
    wakeup();
  }
}

void EventLoop::doPendingFunctors()
{
  std::vector<Functor> functionList;
  callingPendingFunctors_ = true;
  {
    MutexLockGuard lock(mutex_);
    functionList.swap(pendingFunctors_);
  }
  for (size_t i = 0; i < functionList.size(); i++)
  {
    functionList[i]();
  }
  callingPendingFunctors_ = false;
}

int EventLoop::clock(double interval, TimerCallback cb)
{
  Timestamp time(addTime(Timestamp::now(), interval));
  return timer_->addTimer(std::move(cb), time, interval);
}