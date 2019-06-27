#ifndef TIMER_H
#define TIMER_H

#include "noncopyable.h"
#include "Callbacks.h"
#include "Timestamp.h"
#include "Channel.h"
#include "Atomic.h"

#include <memory>
#include <queue>
#include <deque>
#include <vector>
#include <map>

class EventLoop;

static AtomicInt64 sumN_;

class TimeNode : noncopyable
{
public:
  TimeNode(TimerCallback cb, Timestamp when, double interval)
      : callback_(std::move(cb)),
        expiration_(when),
        interval_(interval),
        repeat_(interval > 0.0),
        deleted_(false),
        id_(sumN_.incrementAndGet())
  {
  }

  void run()
  {
    callback_();
  }

  Timestamp expiration() const
  {
    return expiration_;
  }

  bool repeat() const
  {
    return repeat_;
  }

  void setDeleted()
  {
    deleted_ = true;
  }

  bool isDeleted() const
  {
    return deleted_;
  }

  int64_t id() const
  {
    return id_;
  }

  void restart(Timestamp now);

private:
  TimerCallback callback_;
  Timestamp expiration_;
  const double interval_;
  const bool repeat_;
  bool deleted_;
  const int64_t id_;
};

typedef std::shared_ptr<TimeNode> TimeNodePtr;

struct TimeNodeCmp
{
  bool operator()(TimeNodePtr &p1, TimeNodePtr &p2)
  {
    return p1->expiration() > p2->expiration();
  }
};

class Timer : noncopyable
{
public:
  Timer(EventLoop *loop);
  ~Timer();

  int addTimer(TimerCallback cb, Timestamp when, double interval);
  void removeTimer(int id);

private:
  typedef std::priority_queue<TimeNodePtr, std::deque<TimeNodePtr>, TimeNodeCmp> TimeNodeQueue;

  void addTimerInLoop(TimeNodePtr timenode);
  void removeInLoop(int id);
  void handleRead();
  std::vector<TimeNodePtr> getExpired(Timestamp now);
  void reset(const std::vector<TimeNodePtr> &expired, Timestamp now);
  bool insert(TimeNodePtr timenode);

  EventLoop *loop_;
  const int timerfd_;
  std::shared_ptr<Channel> timerfdChannel_;
  TimeNodeQueue queue_;
  std::map<int, TimeNodePtr> timers_;
};

#endif