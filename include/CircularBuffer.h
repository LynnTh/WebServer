#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

#include <cstdint>
#include <cstddef>
#include <array>
#include <memory>
#include <unordered_set>

class HTTPConnection;

typedef std::weak_ptr<HTTPConnection> WeakHttpConnectionPtr;

struct Node
{
  explicit Node(const WeakHttpConnectionPtr &weakconn);

  ~Node();

  WeakHttpConnectionPtr weakconn_;
};

typedef std::shared_ptr<Node> NodePtr;
typedef std::weak_ptr<Node> WeakNodePtr;
typedef std::unordered_set<NodePtr> Bucket;

template<size_t S>
class CircularBuffer
{
public:
  CircularBuffer()
      : buffer(),
        size_(S),
        tail(0)
  {
  }

  CircularBuffer(const CircularBuffer &) = delete;
  CircularBuffer(CircularBuffer &&) = delete;

  CircularBuffer &operator=(const CircularBuffer &) = delete;
  CircularBuffer &operator=(CircularBuffer &&) = delete;

  /*
        Adds an element to the end of buffer: 
        the operation returns `false` if the addition caused overwriting an existing element.
    */
  void push(Bucket value)
  {
    if (++tail == size_)
    {
      tail = 0;
    }
    buffer[tail] = value;
  }

  /*
	    Returns the element at the end of the buffer.
	*/
  Bucket& last()
  {
    return buffer[tail];
  }

private:
  std::array<Bucket, S> buffer;
  const size_t size_;
  size_t tail;
};

typedef CircularBuffer<8> TimeWheel;

#endif