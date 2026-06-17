#include "service/serviceHost.h"

#include <cassert>
#include <stdexcept>
#include <string>

namespace {

class FakeService : public mlcp::hmi::service::IService {
public:
    explicit FakeService(const char* serviceName)
        : serviceName_(serviceName)
    {
    }

    const char* name() const override
    {
        return serviceName_;
    }

    void start() override
    {
        ++startCount;
    }

    void stop() override
    {
        ++stopCount;
    }

    void tick() override
    {
    }

    int startCount {0};
    int stopCount {0};

private:
    const char* serviceName_;
};

class FailingStartService : public FakeService {
public:
    FailingStartService()
        : FakeService("failingStartService")
    {
    }

    void start() override
    {
        throw std::runtime_error("start failed");
    }
};

void testStartsAndStopsRegisteredServices()
{
    mlcp::hmi::service::ServiceHost host;
    FakeService first("firstService");
    FakeService second("secondService");

    host.add(first);
    host.add(second);
    host.add(first);

    host.startAll();
    host.stopAll();

    assert(first.startCount == 1);
    assert(second.startCount == 1);
    assert(first.stopCount == 1);
    assert(second.stopCount == 1);

    const auto snapshot = host.snapshot();
    assert(snapshot.serviceCount == 2U);
    assert(snapshot.startCount == 2U);
    assert(snapshot.stopCount == 2U);
    assert(snapshot.modules.size() == 2U);
    assert(snapshot.modules[0].state == mlcp::hmi::service::ServiceModuleState::stopped);
    assert(snapshot.modules[0].startCount == 1U);
    assert(snapshot.modules[0].stopCount == 1U);
}

void testContinuesAfterStartFailure()
{
    mlcp::hmi::service::ServiceHost host;
    FailingStartService failing;
    FakeService healthy("healthyService");

    host.add(failing);
    host.add(healthy);
    host.startAll();

    assert(healthy.startCount == 1);

    const auto snapshot = host.snapshot();
    assert(snapshot.serviceCount == 2U);
    assert(snapshot.startCount == 1U);
    assert(snapshot.lastError.find("failingStartService start failed") != std::string::npos);
    assert(snapshot.modules[0].state == mlcp::hmi::service::ServiceModuleState::failed);
    assert(snapshot.modules[0].lastError.find("failingStartService start failed")
           != std::string::npos);
    assert(snapshot.modules[1].state == mlcp::hmi::service::ServiceModuleState::running);
}

void testRemoveAndClearServices()
{
    mlcp::hmi::service::ServiceHost host;
    FakeService service("service");

    host.add(service);
    host.remove(service);
    assert(host.snapshot().serviceCount == 0U);

    host.add(service);
    host.clear();
    assert(host.snapshot().serviceCount == 0U);
}

} // namespace

int main()
{
    testStartsAndStopsRegisteredServices();
    testContinuesAfterStartFailure();
    testRemoveAndClearServices();

    return 0;
}
