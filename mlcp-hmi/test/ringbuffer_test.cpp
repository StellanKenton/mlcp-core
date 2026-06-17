#include "common/utils/ringbuffer/ringbuffer.h"

#include <cstdlib>
#include <stdexcept>

namespace {

using mlcp::common::utils::RingBuffer;

void expect(const bool condition)
{
    if (!condition) {
        std::abort();
    }
}

void testPushPeekAndPop()
{
    RingBuffer buffer(3U);

    expect(buffer.isEmpty());
    expect(!buffer.isFull());
    expect(buffer.capacity() == 3U);
    expect(buffer.size() == 0U);
    expect(!buffer.peek().has_value());

    expect(buffer.push(10));
    expect(buffer.push(20));
    expect(buffer.peek().value() == 10);
    expect(buffer.size() == 2U);

    expect(buffer.pop().value() == 10);
    expect(buffer.pop().value() == 20);
    expect(!buffer.pop().has_value());
    expect(buffer.isEmpty());
}

void testFullBufferRejectsNewValue()
{
    RingBuffer buffer(2U);

    expect(buffer.push(1));
    expect(buffer.push(2));
    expect(buffer.isFull());
    expect(!buffer.push(3));
    expect(buffer.pop().value() == 1);
    expect(buffer.pop().value() == 2);
}

void testWrapAroundOrder()
{
    RingBuffer buffer(3U);

    expect(buffer.push(1));
    expect(buffer.push(2));
    expect(buffer.push(3));
    expect(buffer.pop().value() == 1);
    expect(buffer.push(4));

    expect(buffer.pop().value() == 2);
    expect(buffer.pop().value() == 3);
    expect(buffer.pop().value() == 4);
    expect(buffer.isEmpty());
}

void testClear()
{
    RingBuffer buffer(2U);

    expect(buffer.push(7));
    expect(buffer.push(8));
    buffer.clear();

    expect(buffer.isEmpty());
    expect(buffer.size() == 0U);
    expect(!buffer.peek().has_value());
    expect(buffer.push(9));
    expect(buffer.pop().value() == 9);
}

void testRejectsZeroCapacity()
{
    bool hasException = false;

    try {
        RingBuffer buffer(0U);
    } catch (const std::invalid_argument&) {
        hasException = true;
    }

    expect(hasException);
}

} // namespace

int main()
{
    testPushPeekAndPop();
    testFullBufferRejectsNewValue();
    testWrapAroundOrder();
    testClear();
    testRejectsZeroCapacity();

    return 0;
}
