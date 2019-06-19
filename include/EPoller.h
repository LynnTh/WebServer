#ifndef EPOLLER_H
#define EPOLLER_H

#include "noncopyable.h"
#include "EventLoop.h"

#include <sys/epoll.h>
#include <vector>
#include <unordered_map>
#include <memory>

class Channel;

class EPoller : noncopyable
{
public:
    typedef std::vector<std::shared_ptr<Channel>> ChannelList;

    EPoller(EventLoop* loop);
    ~EPoller() = default;

    void poll(int timeoutMs, ChannelList* activeChannels);
    void updateChannel(std::shared_ptr<Channel> channel);
    void removeChannel(std::shared_ptr<Channel> channel);
    void assertInLoopThread() {return ownerLoop_->assertInLoopThread();}

private:
    void fillActiveChannels(int numEvents, ChannelList* activeChannels);
    void update(int op, std::shared_ptr<Channel> channel);

    typedef std::vector<struct epoll_event> EventList;
    typedef std::unordered_map<int, std::shared_ptr<Channel>> ChannelMap;     // 映射,fd->channel

    static const int EventListSize = 16;

    EventLoop* ownerLoop_;
    int epollfd_;
    EventList events_;
    ChannelMap channels_;
};

#endif