#include "Server.h"
#include "Utile.h"
#include "Logging.h"
#include "HttpResponse.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string>

Server::Server(EventLoop *loop, int threadnum, int port, int idleSeconds)
    : loop_(loop),
      threadnum_(threadnum),
      eventLoopThreadPool_(new EventLoopThreadPool(loop_, threadnum_)),
      started_(false),
      port_(port),
      listenFd_(socket_bind_listen(port_)),
      acceptChannel_(new Channel(loop_, listenFd_)),
      idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)),
      nextConnId_(1),
      idleSeconds_(idleSeconds)
{
  handle_for_sigpipe();
  if (setSocketNonBlocking(listenFd_) < 0)
  {
    LOG_FATAL << "set socket non block failed";
  }
  acceptChannel_->setReadCallback(std::bind(&Server::handNewConn, this));
  acceptChannel_->enableReading();
  setConnectionCallback(std::bind(&Server::onConnection, this, _1));
  setMessageCallback(std::bind(&Server::onMessage, this, _1, _2));
  loop_->clock(1.0, std::bind(&Server::onTimer, this));
}

Server::~Server()
{
  loop_->assertInLoopThread();
  for (auto &item : connections_)
  {
    HTTPConnectionPtr conn(item.second);
    item.second.reset();
    conn->getLoop()->runInLoop(
        std::bind(&HTTPConnection::connectionDestroyed, conn));
  }
}

void Server::start()
{
  eventLoopThreadPool_->start();
  started_ = true;
}

void Server::handNewConn()
{
  loop_->assertInLoopThread();
  struct sockaddr_in client_addr;
  memset(&client_addr, 0, sizeof(struct sockaddr_in));
  socklen_t client_addr_len = sizeof(client_addr);
  int accept_fd = accept(listenFd_, (struct sockaddr *)&client_addr, &client_addr_len);
  if (accept_fd > 0)
  {
    std::string conn_name = "HTTPConnection" + std::to_string(nextConnId_);
    nextConnId_++;
    LOG_INFO << "New connection from " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port);

    // 设为非阻塞模式
    if (setSocketNonBlocking(accept_fd) < 0)
    {
      LOG_INFO << "Set non block failed!";
      return;
    }

    setSocketNodelay(accept_fd);

    EventLoop *ioloop = eventLoopThreadPool_->getNextLoop();
    HTTPConnectionPtr conn(new HTTPConnection(ioloop, conn_name, accept_fd));
    connections_[conn_name] = conn;
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setCloseCallback(std::bind(&Server::removeConnection, this, _1));
    ioloop->runInLoop(std::bind(&HTTPConnection::connectionEstablished, conn));
  }
  else
  {
    LOG_INFO << "accept_fd error";
    if (errno == EMFILE)
    {
      ::close(idleFd_);
      idleFd_ = accept(accept_fd, NULL, NULL);
      ::close(idleFd_);
      idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
    }
  }
}

void Server::removeConnection(const HTTPConnectionPtr &conn)
{
  loop_->runInLoop(std::bind(&Server::removeConnectionInLoop, this, conn));
}

void Server::removeConnectionInLoop(const HTTPConnectionPtr &conn)
{
  loop_->assertInLoopThread();
  size_t n = connections_.erase(conn->name());
  (void)n;
  assert(n == 1);
  EventLoop *ioLoop = conn->getLoop();
  ioLoop->queueInLoop(
      std::bind(&HTTPConnection::connectionDestroyed, conn));
}

void Server::onConnection(const HTTPConnectionPtr &conn)
{
  if (conn->connected())
  {
    Node node;
    node.lastReceiveTime = Timestamp::now();
    {
      MutexLockGuard lock(mutex_);
      connectionList_.push_back(conn);
      node.position = --connectionList_.end();
    }
    conn->setNode(node);
  }
  else
  {
    const Node &node = conn->getNode();
    {
      MutexLockGuard lock(mutex_);
      connectionList_.erase(node.position);
    }    
  }
}

void Server::onMessage(const HTTPConnectionPtr &conn, Buffer *buf)
{
  HttpAnalysis &ana = conn->getAnalysis();
  HttpMessage &request = ana.getRequest();
  if (!ana.parseRequest(buf))
  {
    if (request.getVersion() == HTTP_10)
    {
      conn->send("HTTP/1.0 400 Bad Request\r\n\r\n");
    }
    else
    {
      conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
    }
    conn->shutdown();
    conn->forceCloseWithDelay(3.5);
    return;
  }
  const std::string &connection = request.getHeader("Connection");
  bool close = connection == "close" ||
               (request.getVersion() == HTTP_10 && connection != "Keep-Alive");
  HttpResponse response(close);
  if (!response.findFile(request))
  {
    if (request.getVersion() == HTTP_10)
    {
      conn->send("HTTP/1.0 404 Not Found!\r\n\r\n");
    }
    else
    {
      conn->send("HTTP/1.1 404 Not Found!\r\n\r\n");
    }
    conn->shutdown();
    conn->forceCloseWithDelay(3.5);
    return;
  }

  if (ana.gotAll())
  {
    Buffer buf;
    response.appendToBuffer(&buf, request);
    conn->send(&buf);
    if (response.closeConnection())
    {
      const Node &node = conn->getNode();
      {
        MutexLockGuard lock(mutex_);
        connectionList_.erase(node.position);
      }
      conn->shutdown();
      conn->forceCloseWithDelay(3.5);
    }
    else
    {
      Node &node = conn->getNode();
      node.lastReceiveTime = Timestamp::now();
      {
        MutexLockGuard lock(mutex_);
        connectionList_.splice(connectionList_.end(), connectionList_, node.position);
        assert(node.position == --connectionList_.end());
      }
    }
  }
}

void Server::onTimer()
{
  Timestamp now = Timestamp::now();
  MutexLockGuard lock(mutex_);
  for (WeakConnectionList::iterator it = connectionList_.begin();
       it != connectionList_.end();)
  {
    HTTPConnectionPtr conn = it->lock();
    if (conn)
    {
      Node &n = conn->getNode();
      double age = timeDifference(now, n.lastReceiveTime);
      if (age > idleSeconds_)
      {
        if (conn->connected())
        {
          conn->shutdown();
          // LOG_INFO << "shutting down " << conn->name();
          conn->forceCloseWithDelay(3.5); // > round trip of the whole Internet.
        }
      }
      else if (age < 0)
      {
        // LOG_WARN << "Time jump";
        n.lastReceiveTime = now;
      }
      else
      {
        break;
      }
      ++it;
    }
    else
    {
      // LOG_WARN << "Expired";
      it = connectionList_.erase(it);
    }
  }
}