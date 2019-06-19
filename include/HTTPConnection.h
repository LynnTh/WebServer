#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include "noncopyable.h"
#include "Callbacks.h"
#include "Buffer.h"

#include <memory>

class EventLoop;
class Channel;
class Timestamp;

class HTTPConnection : noncopyable,
                       public std::enable_shared_from_this<HTTPConnection>
{
public:
  HTTPConnection(EventLoop *loop, const std::string name, int sockfd);
  void setConnectionCallback(ConnectionCallback &cb)
  {
    connectionCallback_ = cb;
  }
  void setMessageCallback(MessageCallback &cb)
  {
    messageCallback_ = cb;
  }
  void setCloseCallback(const CloseCallback &cb)
  {
    closeCallback_ = cb;
  }
  void connectionEstablished(); // Server新建HTTPConnection时调用
  void connectionDestroyed();   // Server关闭HTTPConnection时调用
  const std::string name() const { return name_; }
  EventLoop *getLoop() const { return loop_; }

private:
  enum State
  {
    kConnecting,
    KConnected,
    KDisconnected,
    KDisconnecting
  };

  void setState(State s) { state_ = s; }
  void handleRead();
  // void handleWrite();
  void handleClose();

  EventLoop *loop_;
  const std::string name_;
  State state_;
  int acceptfd_;
  std::shared_ptr<Channel> channel_;
  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  CloseCallback closeCallback_;
  Buffer inputBuffer_;
};

#endif