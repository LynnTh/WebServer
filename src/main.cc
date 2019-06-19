#include <iostream>
#include <string>
#include <memory>

#include "Buffer.h"
#include "Server.h"
#include "Logging.h"
#include "EventLoop.h"

using namespace std;

class EchoServer
{
public:
  EchoServer(EventLoop *loop)
      : server_(loop, 4, 1234)
  {
    server_.setConnectionCallback(bind(&EchoServer::onConnection, this, _1));
    server_.setMessageCallback(bind(&EchoServer::onMessage, this, _1, _2));
  }

  void start()
  {
    server_.start();
  }

private:
  void onConnection(const HTTPConnectionPtr &conn)
  {
    cout << "onConnection" << endl;
  }

  void onMessage(const HTTPConnectionPtr &conn, Buffer *buf)
  {
    cout << "onMessage" << endl;
  }

  Server server_;
};

int main()
{
  string logPath = "./WebServer.log";
  Logger::setLogFileName(logPath);
  LOG_INFO << "main start.";
  EventLoop loop;
  EchoServer echoserver(&loop);
  echoserver.start();
  loop.loop();

  return 0;
}