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
typedef std::function<void()> TimerCallback;

template<typename CLASS, typename... ARGS>
class WeakCallback
{
 public:

  WeakCallback(const std::weak_ptr<CLASS>& object,
               const std::function<void (CLASS*, ARGS...)>& function)
    : object_(object), function_(function)
  {
  }

  void operator()(ARGS&&... args) const
  {
    std::shared_ptr<CLASS> ptr(object_.lock());
    if (ptr)
    {
      function_(ptr.get(), std::forward<ARGS>(args)...);
    }
  }

 private:

  std::weak_ptr<CLASS> object_;
  std::function<void (CLASS*, ARGS...)> function_;
};

template<typename CLASS, typename... ARGS>
WeakCallback<CLASS, ARGS...> makeWeakCallback(const std::shared_ptr<CLASS>& object,
                                              void (CLASS::*function)(ARGS...))
{
  return WeakCallback<CLASS, ARGS...>(object, function);
}

#endif