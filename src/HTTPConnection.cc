#include "HTTPConnection.h"
#include "Channel.h"
#include "Logging.h"
#include "EventLoop.h"

#include <errno.h>

HTTPConnection::HTTPConnection(EventLoop *loop, std::string name, int sockfd)
    : loop_(loop),
      name_(name),
      state_(kConnecting),
      acceptfd_(sockfd),
      channel_(new Channel(loop_, sockfd)),
      inputBuffer_()
{
  channel_->setReadCallback(
      std::bind(&HTTPConnection::handleRead, this));
  // channel_->setWriteCallback(
  //     std::bind(&HTTPConnection::handleWrite, this));
  channel_->setCloseCallback(
      std::bind(&HTTPConnection::handleClose, this));
}

void HTTPConnection::handleRead()
{
  int saveErrno = 0;
  ssize_t n = inputBuffer_.readFd(acceptfd_, &saveErrno);
  if (n > 0)
  {
    messageCallback_(shared_from_this(), &inputBuffer_);
  }
  else if (n == 0)
  {
    handleClose();
  }
  else
  {
    errno = saveErrno;
    LOG_INFO << "HTTPConnection::handleRead";
  }
}

void HTTPConnection::handleClose()
{
  loop_->assertInLoopThread();
  LOG_INFO << "HTTPConnection::handleClose state = " << state_;
  assert(state_ == KConnected || state_ == KDisconnected);
  setState(KDisconnected);
  channel_->disableAll();
  closeCallback_(shared_from_this());
}

void HTTPConnection::connectionEstablished()
{
  loop_->assertInLoopThread();
  assert(state_ == kConnecting);
  setState(KConnected);
  channel_->enableReading();

  connectionCallback_(shared_from_this());
}

void HTTPConnection::connectionDestroyed()
{
  loop_->assertInLoopThread();
  if (state_ == KConnected)
  {
    setState(KDisconnected);
    channel_->disableAll();

    // connectionCallback_(shared_from_this());
  }
  channel_->remove();
}