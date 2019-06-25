#include "Timer.h"
#include "Logging.h"
#include "EventLoop.h"

#include <functional>
#include <sys/timerfd.h>
#include <unistd.h>

int createTimerfd()
{
  int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  if (timerfd < 0)
  {
    LOG_FATAL << "Failed in timerfd_create";
  }
  return timerfd;
}

struct timespec howMuchTimeFromNow(Timestamp when)
{
  int64_t microseconds = when.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();
  if (microseconds < 100)
  {
    microseconds = 100;
  }
  struct timespec ts;
  ts.tv_sec = static_cast<time_t>(
      microseconds / Timestamp::kMicroSecondsPerSecond);
  ts.tv_nsec = static_cast<long>(
      (microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
  return ts;
}

void readTimerfd(int timerfd, Timestamp now)
{
  uint64_t howmany;
  ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
  LOG_INFO << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
  if (n != sizeof howmany)
  {
    LOG_INFO << "ERROR : TimerQueue::handleRead() reads " << n << " bytes instead of 8";
  }
}

void resetTimerfd(int timerfd, Timestamp expiration)
{
  struct itimerspec newValue;
  struct itimerspec oldValue;
  memset(&newValue, 0, sizeof newValue);
  memset(&oldValue, 0, sizeof oldValue);
  newValue.it_value = howMuchTimeFromNow(expiration);
  int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
  if (ret)
  {
    LOG_INFO << "ERROR : timerfd_settime()";
  }
}

TimeNode::TimeNode(TimerCallback cb, Timestamp when, double interval)
    : callback_(std::move(cb)),
      expiration_(when),
      interval_(interval),
      repeat_(interval > 0.0),
      deleted_(false),
      id_(sumN_.incrementAndGet())
{
}

void TimeNode::restart(Timestamp now)
{
  if (repeat_)
  {
    expiration_ = addTime(now, interval_);
  }
  else
  {
    expiration_ = Timestamp::invalid();
  }
}

Timer::Timer(EventLoop *loop)
    : loop_(loop),
      timerfd_(createTimerfd()),
      timerfdChannel_(loop_, timerfd_),
      queue_()
{
  timerfdChannel_.setReadCallback(std::bind(&Timer::handleRead, this));
  timerfdChannel_.enableReading();
}

Timer::~Timer()
{
  timerfdChannel_.disableAll();
  timerfdChannel_.remove();
  ::close(timerfd_);
}

int Timer::addTimer(TimerCallback cb, Timestamp when, double interval)
{
  TimeNodePtr timenodeptr(new TimeNode(std::move(cb), when, interval));
  loop_->runInLoop(std::bind(&Timer::addTimerInLoop, this, timenodeptr));
  return timenodeptr->id();
}

void Timer::removeTimer(int id)
{
  loop_->runInLoop(std::bind(&Timer::removeInLoop,this,id));
}

void Timer::addTimerInLoop(TimeNodePtr timenode)
{
  loop_->assertInLoopThread();
  bool earliestChanged = insert(timenode);
  timers_[timenode->id()] = timenode;

  if (earliestChanged)
  {
    resetTimerfd(timerfd_, timenode->expiration());
  }
}

void Timer::removeInLoop(int id)
{
  loop_->assertInLoopThread();
  if(timers_.find(id) != timers_.end()){
    timers_[id]->setDeleted();
  }
}

void Timer::handleRead()
{
  loop_->assertInLoopThread();
  Timestamp now(Timestamp::now());
  readTimerfd(timerfd_, now);
  std::vector<TimeNodePtr> expiredTimer = getExpired(now);

  for (const TimeNodePtr &it : expiredTimer)
  {
    it->run();
  }

  reset(expiredTimer, now);
}

std::vector<TimeNodePtr> Timer::getExpired(Timestamp now)
{
  std::vector<TimeNodePtr> result;
  while (!queue_.empty())
  {
    TimeNodePtr tp = std::move(queue_.top());
    if (tp->isDeleted())
    {
      timers_.erase(tp->id());
      queue_.pop();
    }
    else if(tp->expiration() <= now)
    {
      if(!tp->repeat()){
        timers_.erase(tp->id());
      }
      result.push_back(std::move(tp));
      queue_.pop();
    }
    else
      break;
  }
  return result;
}

void Timer::reset(const std::vector<TimeNodePtr> &expired, Timestamp now)
{
  Timestamp nextExpire;

  for (const TimeNodePtr& it : expired)
  {
    if (it->repeat() && !it->isDeleted())
    {
      it->restart(now);
      insert(it);
    }
  }

  if (!queue_.empty())
  {
    nextExpire = queue_.top()->expiration();
  }

  if (nextExpire.valid())
  {
    resetTimerfd(timerfd_, nextExpire);
  }
}

bool Timer::insert(TimeNodePtr timenode)
{
  loop_->assertInLoopThread();
  bool earliestChanged = false;
  Timestamp when = timenode->expiration();
  if (queue_.empty() || when < queue_.top()->expiration())
  {
    earliestChanged = true;
  }
  queue_.push(timenode);
  
  return earliestChanged;
}