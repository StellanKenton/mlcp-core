#include "service/example/exampleSampler.h"

#include <exception>
#include <stdexcept>
#include <utility>

namespace mlcp::hmi::service::example {

ExampleSampler::ExampleSampler(ValueReader valueReader)
    : valueReader_(std::move(valueReader))
{
    if (!valueReader_) {
        throw std::invalid_argument("example value reader is required");
    }
}

void ExampleSampler::sample()
{
    try {
        const double value = valueReader_();
        std::lock_guard<std::mutex> lock(mutex_);
        latestValue_ = value;
        ++sampleCount_;
        lastError_.clear();
    } catch (const std::exception& error) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            lastError_ = error.what();
        }
        throw;
    } catch (...) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            lastError_ = "unknown exception";
        }
        throw;
    }
}

double ExampleSampler::latestValueOrThrow() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!lastError_.empty()) {
        throw std::runtime_error(lastError_);
    }
    if (latestValue_.has_value()) {
        return *latestValue_;
    }

    throw std::runtime_error("example sample is not ready");
}

ExampleSamplerSnapshot ExampleSampler::snapshot() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    ExampleSamplerSnapshot result;
    result.latestValue = latestValue_;
    result.sampleCount = sampleCount_;
    result.lastError = lastError_;
    return result;
}

} // namespace mlcp::hmi::service::example
