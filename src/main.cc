#include <iostream>
#include <string>
#include <memory>
#include <unistd.h>
#include <getopt.h>

#include "Buffer.h"
#include "Server.h"
#include "Logging.h"
#include "EventLoop.h"

using namespace std;

int main(int argc, char *argv[])
{
  int threadnums = 4;
  int port = 80;
  string logPath = "./WebServer.log";

  int opt;
  const char *str = "t:p:";
  while ((opt = getopt(argc, argv, str)) != -1)
  {
    switch (opt)
    {
    case 't':
    {
      threadnums = atoi(optarg);
      break;
    }
    case 'p':
    {
      port = atoi(optarg);
      if (port < 1 || port > 65535)
      {
        cout << "Wrong port." << endl;
        abort();
      }
      break;
    }
    default:
      break;
    }
  }

  Logger::setLogFileName(logPath);
  LOG_INFO << "main start.";
  EventLoop loop;
  Server server(&loop, threadnums, port, 8);
  server.start();
  loop.loop();

  return 0;
}