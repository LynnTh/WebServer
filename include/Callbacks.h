#ifndef CALLBACKS_H
#define CALLBACKS_H

#include <functional>
#include <memory>

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

class Buffer;
class HTTPConnection;
class EventLoop;

typedef std::function<void(EventLoop *)> ThreadInitCallback;
typedef std::shared_ptr<HTTPConnection> HTTPConnectionPtr;
typedef std::function<void(const HTTPConnectionPtr &)> CloseCallback;
typedef std::function<void(const HTTPConnectionPtr &)> ConnectionCallback;
typedef std::function<void(const HTTPConnectionPtr &, Buffer *)> MessageCallback;

#endif