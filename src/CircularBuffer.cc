#include "CircularBuffer.h"
#include "Callbacks.h"
#include "HTTPConnection.h"

Node::Node(const WeakHttpConnectionPtr &weakconn)
      : weakconn_(weakconn)
  {
  }

Node::~Node()
  {
    HTTPConnectionPtr conn = weakconn_.lock();
    if (conn)
    {
      conn->shutdown();
    }
  }