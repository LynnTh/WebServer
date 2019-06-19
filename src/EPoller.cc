#include "EPoller.h"
#include "Logging.h"
#include "Channel.h"

#include <assert.h>

const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;

EPoller::EPoller(EventLoop *loop)
    : ownerLoop_(loop),
      epollfd_(epoll_create1(EPOLL_CLOEXEC)),
      events_(EventListSize)
{
    assert(epollfd_ > 0);
}

void EPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int savedErrno = errno;
    if (numEvents > 0)
    {
        LOG_INFO << numEvents << " events happended";
        fillActiveChannels(numEvents, activeChannels);
        if (static_cast<size_t>(numEvents) == events_.size())
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if (numEvents < 0)
    {
        if (savedErrno != EINTR)
        {
            errno = savedErrno;
            LOG_INFO << "EPollPoller::poll()";
        }
    }
}

void EPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) 
{
    assert(static_cast<size_t>(numEvents) <= events_.size());
    for (int i = 0; i < numEvents; i++)
    {
        // Channel *channel_tmp = static_cast<Channel *>(events_[i].data.ptr);
        // std::shared_ptr<Channel> channel(channel_tmp);
        int fd = events_[i].data.fd;
        std::shared_ptr<Channel> channel = channels_[fd];
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

void EPoller::updateChannel(std::shared_ptr<Channel> channel)
{
    assertInLoopThread();
    const int index = channel->index();
    if (index == kNew || index == kDeleted)
    {
        int fd = channel->fd();
        if (index == kNew)
        {
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel;
        }
        else // index == kDeleted
        {
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }

        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else
    {
        int fd = channel->fd();
        (void)fd;
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        assert(index == kAdded);
        if (channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EPoller::removeChannel(std::shared_ptr<Channel> channel)
{
    assertInLoopThread();
    int fd = channel->fd();
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    assert(channel->isNoneEvent());
    int index = channel->index();
    assert(index == kAdded || index == kDeleted);
    size_t n = channels_.erase(fd);
    (void)n;
    assert(n == 1);

    if (index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

void EPoller::update(int operation, std::shared_ptr<Channel> channel)
{
    struct epoll_event event;
    memset(&event, 0, sizeof event);
    event.events = channel->events();
    // event.data.ptr = channel;
    int fd = channel->fd();
    event.data.fd = fd;
    epoll_ctl(epollfd_, operation, fd, &event);
}