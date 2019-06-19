#ifndef CHANNEL_H
#define CHANNEL_H

#include "noncopyable.h"

#include <functional>
#include <utility>
#include <memory>

class EventLoop;

class Channel : noncopyable, public std::enable_shared_from_this<Channel>
{
public:
  typedef std::function<void()> EventCallback;

  Channel(EventLoop *loop, int fd);
  ~Channel();

  void handleEvent();
  void setReadCallback(const EventCallback &cb)
  {
    readCallback_ = std::move(cb);
  }
  void setWriteCallback(const EventCallback &cb)
  {
    writeCallback_ = std::move(cb);
  }
  void setErrorCallback(const EventCallback &cb)
  {
    errorCallback_ = std::move(cb);
  }
  void setCloseCallback(const EventCallback &cb)
  {
    closeCallback_ = std::move(cb);
  }

  int fd() const { return fd_; }
  int events() const { return events_; }
  void set_revents(int revt) { revents_ = revt; }
  bool isNoneEvent() const { return events_ == kNoneEvent; }

  void enableReading()
  {
    events_ |= kReadEvent;
    update();
  }
  void enableWriting()
  {
    events_ |= kWriteEvent;
    update();
  }
  void disableWriting()
  {
    events_ &= ~kWriteEvent;
    update();
  }
  void disableAll()
  {
    events_ = kNoneEvent;
    update();
  }

  int index() { return index_; }
  void set_index(int idx) { index_ = idx; }

  EventLoop *ownerLoop() { return loop_; }
  void remove();

private:
  void update();

  static const int kNoneEvent;
  static const int kReadEvent;
  static const int kWriteEvent;

  EventLoop *loop_;
  const int fd_;
  int events_;  // 关心的IO事件
  int revents_; // 目前活动的事件
  int index_;
  bool eventHandling_;

  EventCallback readCallback_;
  EventCallback writeCallback_;
  EventCallback errorCallback_;
  EventCallback closeCallback_;
};

#endif