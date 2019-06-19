#ifndef _MUTEX_H_
#define _MUTEX_H_

#include "CurrentThread.h"
#include "noncopyable.h"

#include <assert.h>
#include <pthread.h>

class MutexLock : noncopyable {
private:
    pthread_mutex_t mutex_;
    pid_t holder_;

public:
    MutexLock()
        : holder_(0)
    {
        pthread_mutex_init(&mutex_,NULL);
    }

    ~MutexLock(){
        assert(holder_ == 0);
        pthread_mutex_destroy(&mutex_);
    }

    bool isLockedbyThisThread() const
    {
        return holder_ == CurrentThread::tid();
    }

    void assertLocked() const
    {
        assert(isLockedbyThisThread());
    }

    void lock()
    {
        pthread_mutex_lock(&mutex_);
        assignHolder();
    }

    void unlock()
    {
        unassignHolder();
        pthread_mutex_unlock(&mutex_);
    }

    pthread_mutex_t* getPthreadMutex() /* non-const */
    {
        return &mutex_;
    }

private:
    friend class Condition;

    class UnassignGuard : noncopyable
    {
    public:
        explicit UnassignGuard(MutexLock& owner)
            : owner_(owner)
        {
            owner_.unassignHolder();
        }

        ~UnassignGuard()
        {
            owner_.assignHolder();
        }

    private:
        MutexLock &owner_;
    };

    void unassignHolder()
    {
        holder_ = 0;
    }

    void assignHolder()
    {
        holder_ = CurrentThread::tid();
    }
};

class MutexLockGuard : noncopyable
{
public:
    explicit MutexLockGuard(MutexLock& mutex)
        : mutex_(mutex)
    {
        mutex_.lock();
    }

    ~MutexLockGuard()
    {
        mutex_.unlock();
    }
    
private:
    MutexLock& mutex_;
};

#endif