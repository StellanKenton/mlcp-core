#include "device/sysinfo/systemInfoReader.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace {

namespace fs = std::filesystem;

void writeTextFile(const fs::path& path, const std::string& text)
{
    std::ofstream output(path);
    assert(output);
    output << text;
    assert(output);
}

void testReadsMemoryInfoFromProcFormat()
{
    const fs::path tempDirectory = fs::temp_directory_path() / "mlcp_sysinfo_test";
    fs::create_directories(tempDirectory);

    const fs::path statPath = tempDirectory / "stat";
    const fs::path meminfoPath = tempDirectory / "meminfo";

    writeTextFile(statPath, "cpu  100 0 50 850 0 0 0 0 0 0\n");
    writeTextFile(meminfoPath,
                  "MemTotal:       1000 kB\n"
                  "MemFree:         200 kB\n"
                  "MemAvailable:    750 kB\n");

    const mlcp::hmi::device::sysinfo::SystemInfoReader reader(
        statPath.string(), meminfoPath.string());
    const auto memory = reader.readMemoryInfo();

    assert(memory.totalKb == 1000U);
    assert(memory.availableKb == 750U);
    assert(memory.usedKb == 250U);
    assert(memory.usagePercent == 25.0);

    fs::remove_all(tempDirectory);
}

void testRejectsInvalidMemoryInfo()
{
    const fs::path tempDirectory = fs::temp_directory_path() / "mlcp_sysinfo_invalid_test";
    fs::create_directories(tempDirectory);

    const fs::path statPath = tempDirectory / "stat";
    const fs::path meminfoPath = tempDirectory / "meminfo";

    writeTextFile(statPath, "cpu  100 0 50 850 0 0 0 0 0 0\n");
    writeTextFile(meminfoPath, "MemAvailable: 100 kB\n");

    const mlcp::hmi::device::sysinfo::SystemInfoReader reader(
        statPath.string(), meminfoPath.string());

    bool hasException = false;
    try {
        static_cast<void>(reader.readMemoryInfo());
    } catch (const std::runtime_error&) {
        hasException = true;
    }

    assert(hasException);
    fs::remove_all(tempDirectory);
}

void testReadsCurrentSystemSnapshot()
{
    const mlcp::hmi::device::sysinfo::SystemInfoReader reader;
    const auto snapshot = reader.readSnapshot(std::chrono::milliseconds(10));

    assert(snapshot.memory.totalKb > 0U);
    assert(snapshot.memory.availableKb <= snapshot.memory.totalKb);
    assert(snapshot.cpuUsagePercent >= 0.0);
    assert(snapshot.cpuUsagePercent <= 100.0);
}

} // namespace

int main()
{
    testReadsMemoryInfoFromProcFormat();
    testRejectsInvalidMemoryInfo();
    testReadsCurrentSystemSnapshot();

    return 0;
}
