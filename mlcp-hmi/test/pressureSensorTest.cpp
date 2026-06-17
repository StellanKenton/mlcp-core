#include "device/pressure/pressureSensor.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace {

namespace fs = std::filesystem;

void writeTextFile(const fs::path& path, const std::string& text)
{
    std::ofstream output(path);
    assert(output);
    output << text;
    assert(output);
}

void writePressureFiles(const fs::path& directory)
{
    fs::create_directories(directory);
    writeTextFile(directory / "raw", "1536\n");
    writeTextFile(directory / "voltage_uv", "900000\n");
    writeTextFile(directory / "voltage_mv", "900\n");
    writeTextFile(directory / "pressure_kpa", "150\n");
    writeTextFile(directory / "pressure_range", "0-300 kPa @ 0-1800000 uV\n");
}

void testReadsPressureSnapshot()
{
    const fs::path tempDirectory = fs::temp_directory_path() / "mlcp_pressure_test";
    fs::remove_all(tempDirectory);
    writePressureFiles(tempDirectory);

    const mlcp::hmi::device::pressure::PressureSensor sensor(tempDirectory.string());
    const auto snapshot = sensor.readSnapshot();

    assert(snapshot.raw == 1536);
    assert(snapshot.voltageMicrovolt == 900000U);
    assert(snapshot.voltageMillivolt == 900U);
    assert(snapshot.pressureKpa == 150U);
    assert(sensor.readPressureRange() == "0-300 kPa @ 0-1800000 uV\n");

    fs::remove_all(tempDirectory);
}

void testFindsPressureDirectoryUnderPlatformRoot()
{
    const fs::path tempDirectory = fs::temp_directory_path() / "mlcp_pressure_find_test";
    fs::remove_all(tempDirectory);

    const fs::path deviceDirectory = tempDirectory / "rumipressure";
    writePressureFiles(deviceDirectory);

    const std::string found =
        mlcp::hmi::device::pressure::findRumiPressureDirectoryUnder(tempDirectory.string());

    assert(found == deviceDirectory.string());
    fs::remove_all(tempDirectory);
}

void testRejectsInvalidPressureValue()
{
    const fs::path tempDirectory = fs::temp_directory_path() / "mlcp_pressure_invalid_test";
    fs::remove_all(tempDirectory);
    writePressureFiles(tempDirectory);
    writeTextFile(tempDirectory / "pressure_kpa", "bad\n");

    const mlcp::hmi::device::pressure::PressureSensor sensor(tempDirectory.string());

    bool hasException = false;
    try {
        static_cast<void>(sensor.readPressureKpa());
    } catch (const std::runtime_error&) {
        hasException = true;
    }

    assert(hasException);
    fs::remove_all(tempDirectory);
}

} // namespace

int main()
{
    testReadsPressureSnapshot();
    testFindsPressureDirectoryUnderPlatformRoot();
    testRejectsInvalidPressureValue();

    return 0;
}
