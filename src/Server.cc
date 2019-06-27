#include "Server.h"
#include "Utile.h"
#include "Logging.h"
#include "HTTPConnection.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string>

Server::Server(EventLoop *loop, int threadnum, int port)
    : loop_(loop),
      threadnum_(threadnum),
      eventLoopThreadPool_(new EventLoopThreadPool(loop_, threadnum_)),
      started_(false),
      port_(port),
      listenFd_(socket_bind_listen(port_)),
      acceptChannel_(new Channel(loop_, listenFd_)),
      idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)),
      nextConnId_(1),
      wheel_()
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

  // char buf[256];
  // if (!getcwd(buf, sizeof buf))
  // {
  //   LOG_FATAL << "getcwd failed";
  // }
  // path_.assign(buf);
  // path_ += "/";
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
      ::close(accept_fd);
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
    NodePtr node(new Node(conn));
    wheel_.last().insert(node);
    WeakNodePtr weakNode(node);
    conn->setNode(weakNode);
  }
  else
  {
    LOG_INFO << "Node use_count = " << conn->getNode().use_count();
  }
}

void Server::onMessage(const HTTPConnectionPtr &conn, Buffer *buf)
{
  if (!conn->httpanalysis_.parseRequest(buf))
  {
    if (conn->httpanalysis_.version() == HTTP_10)
    {
      conn->send("HTTP/1.0 400 Bad Request\r\n\r\n");
    }
    else
    {
      conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
    }
    conn->shutdown();
    return;
  }
  if (!conn->httpanalysis_.findFile())
  {
    if (conn->httpanalysis_.version() == HTTP_10)
    {
      conn->send("HTTP/1.0 404 Not Found!\r\n\r\n");
    }
    else
    {
      conn->send("HTTP/1.1 404 Not Found!\r\n\r\n");
    }
    conn->shutdown();
    return;
  }

  if (conn->httpanalysis_.gotAll())
  {
    Buffer buf;
    conn->httpanalysis_.appendToBuffer(&buf);
    conn->send(&buf);
    conn->httpanalysis_.reset();

    WeakNodePtr weakNode = conn->getNode();
    NodePtr node(weakNode.lock());
    if (node)
    {
      wheel_.last().insert(node);
    }
  }
}

void Server::onTimer()
{
  wheel_.push(Bucket());
}