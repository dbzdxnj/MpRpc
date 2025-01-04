#pragma once

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

// 异步写日志的日志队列
template<typename T>
class LockQueue
{
public:
    // 多个线程都会写日志
    void push(const T &data)
    {
        std::lock_guard<std::mutex> lock(m_mutex_);
        m_queue_.push(data);
        m_condvariable_.notify_one();
    }

    // 一个线程读日志文件
    T pop()
    {
        std::unique_lock<std::mutex> lock(m_mutex_);
        while (m_queue_.empty())
        {
            // 日志队列为空，线程进入wait状态
            m_condvariable_.wait(lock);
        }
        T data = m_queue_.front();
        m_queue_.pop();
        return data;
    }
private:
    std::queue<T> m_queue_;
    std::mutex m_mutex_;
    std::condition_variable m_condvariable_;
};