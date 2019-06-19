#ifndef SERVER_H
#define SERVER_H

#include "Callbacks.h"
#include "noncopyable.h"
#include "Channel.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"

#include <memory>
#include <map>

class EventLoopThreadPool;

/*
    在main函数中手动loop
*/
class Server : noncopyable
{
public:
    Server(EventLoop* loop, int threadnum, int port);
    ~Server();
    void start();
    void handNewConn();
    void setConnectionCallback(const ConnectionCallback& cb){
        connectionCallback_ = cb;
    }
    void setMessageCallback(const MessageCallback& cb){
        messageCallback_ = cb;
    }
    void removeConnection(const HTTPConnectionPtr& conn);
    void removeConnectionInLoop(const HTTPConnectionPtr& conn);
    // void handThisConn() { loop_->updateChannel(&acceptChannel_); }

private:
    static const int MAXFDS = 100000;

    typedef std::map<std::string, HTTPConnectionPtr> ConnectionMap;

    EventLoop *loop_;
    int threadnum_;
    std::shared_ptr<EventLoopThreadPool> eventLoopThreadPool_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    bool started_;
    int port_;
    int listenFd_;
    std::shared_ptr<Channel> acceptChannel_;
    int idleFd_;
    int nextConnId_;
    ConnectionMap connections_;
};

#endif