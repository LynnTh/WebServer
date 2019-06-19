#include "AsyncLogging.h"
#include "LogFile.h"
#include "Logging.h"

#include <stdio.h>
#include <assert.h>

AsyncLogging::AsyncLogging(const std::string &basename,
						   int flushInterval)
	: flushInterval_(flushInterval),
	  running_(false),
	  basename_(basename),
	  thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"),
	  mutex_(),
	  cond_(mutex_),
	  latch_(1),
	  currentBuffer_(new Buffer),
	  nextBuffer_(new Buffer),
	  buffers_()
{
	assert(basename_.size() > 1);
	currentBuffer_->bzero();
	nextBuffer_->bzero();
	buffers_.reserve(16);
}

void AsyncLogging::append(const char *logline, int len)
{
	MutexLockGuard lock(mutex_);
	if (currentBuffer_->avail() > len)
	{
		currentBuffer_->append(logline, len);
	}
	else
	{
		buffers_.push_back(std::move(currentBuffer_));

		if (nextBuffer_)
		{
			currentBuffer_ = std::move(nextBuffer_);
		}
		else
		{
			currentBuffer_.reset(new Buffer); // Rarely happens
		}
		currentBuffer_->append(logline, len);
		cond_.notify();
	}
}

void AsyncLogging::threadFunc()
{
	assert(running_ == true);
	latch_.countDown();
	LogFile output(basename_);
	BufferPtr newBuffer1(new Buffer);
	BufferPtr newBuffer2(new Buffer);
	newBuffer1->bzero();
	newBuffer2->bzero();
	BufferVector buffersToWrite;
	buffersToWrite.reserve(16);
	while (running_)
	{
		assert(newBuffer1 && newBuffer1->length() == 0);
		assert(newBuffer2 && newBuffer2->length() == 0);
		assert(buffersToWrite.empty());

		{
			MutexLockGuard lock(mutex_);
			if (buffers_.empty())
			{
				cond_.waitForSeconds(flushInterval_);
			}
			buffers_.push_back(std::move(currentBuffer_));
			currentBuffer_ = std::move(newBuffer1);
			buffersToWrite.swap(buffers_);
			if (!nextBuffer_)
			{
				nextBuffer_ = std::move(newBuffer2);
			}
		}

		assert(!buffersToWrite.empty());

		if (buffersToWrite.size() > 25)
		{
			buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
		}

		for (const auto &buffer : buffersToWrite)
		{
			output.append(buffer->data(), buffer->length());
		}

		if (buffersToWrite.size() > 2)
		{
			buffersToWrite.resize(2);
		}

		if (!newBuffer1)
		{
			assert(!buffersToWrite.empty());
			newBuffer1 = std::move(buffersToWrite.back());
			buffersToWrite.pop_back();
			newBuffer1->reset();
		}

		if (!newBuffer2)
		{
			assert(!buffersToWrite.empty());
			newBuffer2 = std::move(buffersToWrite.back());
			buffersToWrite.pop_back();
			newBuffer2->reset();
		}

		buffersToWrite.clear();
		output.flush();
	}
	output.flush();
}