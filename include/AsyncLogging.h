#ifndef ASYNCLOGGING_H
#define ASYNCLOGGING_H

#include "CountDownLatch.h"
#include "Mutex.h"
#include "Thread.h"
#include "LogStream.h"

#include <atomic>
#include <vector>

class AsyncLogging : noncopyable
{
public:
	AsyncLogging(const std::string &basename, int flushInterval = 3);

	~AsyncLogging()
	{
		if (running_)
		{
			stop();
		}
	}

	void append(const char *logline, int len);

	void start()
	{
		running_ = true;
		thread_.start();
		latch_.wait();
	}

	void stop()
	{
		running_ = false;
		cond_.notify();
		thread_.join();
	}

private:
	void threadFunc();

	typedef FixedBuffer<kLargeBuffer> Buffer;
	typedef std::vector<std::unique_ptr<Buffer>> BufferVector;
	typedef BufferVector::value_type BufferPtr;

	const int flushInterval_;
	std::atomic<bool> running_;
	const std::string basename_;
	Thread thread_;
	MutexLock mutex_;
	Condition cond_;
	CountDownLatch latch_;
	BufferPtr currentBuffer_;
	BufferPtr nextBuffer_;
	BufferVector buffers_;
};

#endif
