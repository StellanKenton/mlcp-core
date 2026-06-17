#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>

namespace mlcp::hmi::service::example {

struct ExampleSamplerSnapshot {
    std::optional<double> latestValue;
    std::uint64_t sampleCount {0U};
    std::string lastError;
};

class ExampleSampler {
public:
    using ValueReader = std::function<double()>;

    explicit ExampleSampler(ValueReader valueReader);

    void sample();
    double latestValueOrThrow() const;
    ExampleSamplerSnapshot snapshot() const;

private:
    ValueReader valueReader_;
    mutable std::mutex mutex_;
    std::optional<double> latestValue_;
    std::uint64_t sampleCount_ {0U};
    std::string lastError_;
};

} // namespace mlcp::hmi::service::example
