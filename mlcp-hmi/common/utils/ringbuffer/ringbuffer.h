#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace mlcp::common::utils {

class RingBuffer {
public:
    explicit RingBuffer(std::size_t capacity);

    bool push(std::int32_t value);
    std::optional<std::int32_t> pop();
    std::optional<std::int32_t> peek() const;

    void clear();

    bool isEmpty() const;
    bool isFull() const;
    std::size_t size() const;
    std::size_t capacity() const;

private:
    std::vector<std::int32_t> buffer_;
    std::size_t head_;
    std::size_t tail_;
    std::size_t size_;
};

} // namespace mlcp::common::utils
