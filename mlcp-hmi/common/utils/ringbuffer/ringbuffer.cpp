#include "common/utils/ringbuffer/ringbuffer.h"

#include <stdexcept>

namespace mlcp::common::utils {

RingBuffer::RingBuffer(const std::size_t capacity)
    : buffer_(capacity)
    , head_(0U)
    , tail_(0U)
    , size_(0U)
{
    if (capacity == 0U) {
        throw std::invalid_argument("ring buffer capacity must be greater than zero");
    }
}

bool RingBuffer::push(const std::int32_t value)
{
    if (isFull()) {
        return false;
    }

    buffer_[tail_] = value;
    tail_ = (tail_ + 1U) % buffer_.size();
    ++size_;

    return true;
}

std::optional<std::int32_t> RingBuffer::pop()
{
    if (isEmpty()) {
        return std::nullopt;
    }

    const std::int32_t value = buffer_[head_];
    head_ = (head_ + 1U) % buffer_.size();
    --size_;

    return value;
}

std::optional<std::int32_t> RingBuffer::peek() const
{
    if (isEmpty()) {
        return std::nullopt;
    }

    return buffer_[head_];
}

void RingBuffer::clear()
{
    head_ = 0U;
    tail_ = 0U;
    size_ = 0U;
}

bool RingBuffer::isEmpty() const
{
    return size_ == 0U;
}

bool RingBuffer::isFull() const
{
    return size_ == buffer_.size();
}

std::size_t RingBuffer::size() const
{
    return size_;
}

std::size_t RingBuffer::capacity() const
{
    return buffer_.size();
}

} // namespace mlcp::common::utils
