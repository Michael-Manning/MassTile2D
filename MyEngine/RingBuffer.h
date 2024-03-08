#pragma once

#include <iostream>
#include <vector>
#include <numeric>

template<typename T>
class RingBuffer {
public:

    RingBuffer()  
        : buffer_(0), head_(0), tail_(0), full_(false) {}

    explicit RingBuffer(size_t capacity)
        : buffer_(capacity), head_(0), tail_(0), full_(false) {}

    bool push(const T& item) {
        if (full_) {
            return false; // Buffer is full
        }
        buffer_[head_] = item;
        head_ = (head_ + 1) % buffer_.size();
        full_ = head_ == tail_;
        return true;
    }

    bool pop(T& item) {
        if (isEmpty()) {
            return false; // Buffer is empty
        }
        item = buffer_[tail_];
        tail_ = (tail_ + 1) % buffer_.size();
        full_ = false;
        return true;
    }

    T peek() const {
        if (isEmpty()) {
            throw std::runtime_error("Cannot peek into an empty buffer");
        }
        return buffer_[tail_];
    }

    bool isEmpty() const {
        return (!full_ && (head_ == tail_));
    }

    bool isFull() const {
        return full_;
    }

    size_t capacity() const {
        return buffer_.size();
    }

    size_t size() const {
        if (full_) return buffer_.size();
        if (head_ >= tail_) {
            return head_ - tail_;
        }
        return buffer_.size() + head_ - tail_;
    }
    
    // assumes in. Made for a specific use case in tile world class
    void iota() {
        std::iota(buffer_.begin(), buffer_.end(), 0);
        head_ = 0;
        tail_ = 0;
        full_ = true;
    }

private:
    std::vector<T> buffer_;
    size_t head_, tail_;
    bool full_;
};
