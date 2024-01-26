#pragma once

#include <condition_variable> 
#include <iostream> 
#include <mutex> 
#include <queue> 

template <typename T>
class ConcurrentQueue {
private:
    // Underlying queue 
    std::queue<T> m_queue;

    // mutex for thread synchronization 
    std::mutex m_mutex;

    // Condition variable for signaling 
    std::condition_variable m_cond;

    bool m_cancelled = false;


public:

    size_t size() {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

    void push(T item)
    {
        std::unique_lock<std::mutex> lock(m_mutex);

        assert(m_cancelled == false);

        m_queue.push(item);

        // signal queue not empty
        m_cond.notify_one();
    }

    bool wait_and_pop(T& value)
    {
        std::unique_lock<std::mutex> lock(m_mutex);

        // wait until queue is not empty 
        m_cond.wait(lock, [this]() { return !m_queue.empty() || m_cancelled; });

        if (m_cancelled) {
            m_cancelled = false;  // reset the cancel flag for future waits
            return false;
        }

        value = m_queue.front();
        m_queue.pop();

        return true;
    }

    void cancel_wait() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cancelled = true;
        m_cond.notify_all();  // wake up all waiting threads
    }

};