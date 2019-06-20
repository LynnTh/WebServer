#include "HTTPConnection.h"
#include "Channel.h"
#include "Logging.h"
#include "EventLoop.h"

#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>

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

void HTTPConnection::shutdown()
{
  if (state_ == KConnected)
  {
    setState(KDisconnecting);
    loop_->runInLoop(std::bind(&HTTPConnection::shutdownInLoop, this));
  }
}

void HTTPConnection::shutdownInLoop()
{
  if (::shutdown(acceptfd_, SHUT_WR) < 0)
  {
    LOG_INFO << "sockets::shutdownWrite error";
  }
}

void HTTPConnection::send(const std::string &message)
{
  if (state_ == KConnected)
  {
    loop_->runInLoop(std::bind(&HTTPConnection::sendInLoop, this, message));
  }
}

void HTTPConnection::sendInLoop(const std::string &message)
{
  loop_->assertInLoopThread();
  ssize_t n = 0;
  if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
  {
    n = ::write(channel_->fd(), message.data(), message.size());
    if (n >= 0)
    {
      LOG_INFO << "HTTPConnection::sendInLoop directly write message";
      if (static_cast<size_t>(n) < message.size())
      {
        LOG_INFO << "I am going to write more data";
      }
    }
    else
    {
      n = 0;
      if (errno != EWOULDBLOCK)
      {
        LOG_INFO << "HTTPConnection::sendInLoop Error";
      }
    }
  }
  if (static_cast<size_t>(n) < message.size())
  {
    outputBuffer_.append(message.data() + n, message.size() - n);
    if (!channel_->isWriting())
    {
      channel_->enableWriting();
    }
  }
}

void HTTPConnection::handleWrite()
{
  loop_->assertInLoopThread();
  if (channel_->isWriting())
  {
    ssize_t n = ::write(channel_->fd(), outputBuffer_.peek(), outputBuffer_.readableBytes());
    if (n > 0)
    {
      outputBuffer_.retrieve(n);
      if (outputBuffer_.readableBytes() == 0)
      {
        channel_->disableWriting();
        if (state_ == KDisconnecting)
        {
          shutdownInLoop();
        }
      }
      else
      {
        LOG_INFO << "I am going to write more data";
      }
    }
    else
    {
      LOG_INFO << "HTTPConnection::handleWrite Error";
    }
  }
}