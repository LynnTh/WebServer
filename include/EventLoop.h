#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include "noncopyable.h"
#include "CurrentThread.h"
#include "Mutex.h"
#include "Callbacks.h"

#include <memory>
#include <vector>
#include <functional>

class Channel;
class EPoller;
class Timer;

class EventLoop : noncopyable
{
public:
  typedef std::function<void()> Functor;

  EventLoop();
  ~EventLoop();

  void loop();
  void assertInLoopThread()
  {
    if (!isInLoopThread())
    {
      abort();
    }
  }
  void quit();

  bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

  // 管理Channel
  void updateChannel(std::shared_ptr<Channel> channel); // 根据Channel's index自动更新Channel关注的事件
  void removeChannel(std::shared_ptr<Channel> channel);

  void wakeup();
  void queueInLoop(Functor cb);
  void runInLoop(Functor cb);

  int clock(double interval, TimerCallback cb);

private:
  typedef std::vector<std::shared_ptr<Channel>> ChannelList;

  void handleWakeup(); // loop唤醒事件
  void doPendingFunctors();

  bool looping_;
  bool quit_;
  bool callingPendingFunctors_;
  const pid_t threadId_;
  std::unique_ptr<EPoller> poller_;
  std::unique_ptr<Timer> timer_;
  int WakeupFd_;
  std::shared_ptr<Channel> wakeupChannel_;
  ChannelList activeChannels_;
  MutexLock mutex_;
  std::vector<Functor> pendingFunctors_;
};

#endif