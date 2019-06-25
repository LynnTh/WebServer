#include <iostream>
#include <string>
#include <memory>

#include "Buffer.h"
#include "Server.h"
#include "Logging.h"
#include "EventLoop.h"

using namespace std;

int main()
{
  string logPath = "./WebServer.log";
  Logger::setLogFileName(logPath);
  LOG_INFO << "main start.";
  EventLoop loop;
  Server server(&loop,1,1234);
  server.start();
  loop.loop();

  return 0;
}