cmake_minimum_required(VERSION 3.14)

project(mlcp-hmi VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Quick REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Quick REQUIRED)
find_package(Threads REQUIRED)

set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "" FORCE)
set(SPDLOG_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/spdlog EXCLUDE_FROM_ALL)

set(PROJECT_SOURCES
    app/main.cpp
    bridge/fanSettingsMapper.cpp
    bridge/fanSettingsMapper.h
    bridge/fanSettingsUiState.h
    bridge/fanSettingsViewModel.cpp
    bridge/fanSettingsViewModel.h
    bridge/hmiBackend.h
    bridge/lifecycleMapper.cpp
    bridge/lifecycleMapper.h
    bridge/lifecycleUiState.h
    bridge/lifecycleViewModel.cpp
    bridge/lifecycleViewModel.h
    bridge/uiError.h
    common/utils/ringbuffer/ringbuffer.cpp
    common/utils/ringbuffer/ringbuffer.h
    core/state_machine/system_state/systemStateMachine.cpp
    core/state_machine/system_state/systemStateMachine.h
    device/fan/fanController.cpp
    device/fan/fanController.h
    device/pressure/pressureSensor.cpp
    device/pressure/pressureSensor.h
    device/sysinfo/systemInfoReader.cpp
    device/sysinfo/systemInfoReader.h
    device/temp/temperatureSensor.cpp
    device/temp/temperatureSensor.h
    persistence/storage/exampleServiceConfigStore.cpp
    persistence/storage/exampleServiceConfigStore.h
    persistence/storage/tempServiceConfigStore.cpp
    persistence/storage/tempServiceConfigStore.h
    system/appcontrol/applicationServiceRegistry.cpp
    system/appcontrol/applicationServiceRegistry.h
    system/appcontrol/applicationWiring.cpp
    system/appcontrol/applicationWiring.h
    system/appcontrol/appcontrol.cpp
    system/appcontrol/appcontrol.h
    system/appcontrol/deviceRegistry.cpp
    system/appcontrol/deviceRegistry.h
    system/appcontrol/runtimeTaskRegistry.cpp
    system/appcontrol/runtimeTaskRegistry.h
    service/lifecycle/lifecycle.cpp
    service/lifecycle/lifecycle.h
    service/example/exampleSampler.cpp
    service/example/exampleSampler.h
    service/example/exampleService.cpp
    service/example/exampleService.h
    service/selftest/selfTestFlow.cpp
    service/selftest/selfTestFlow.h
    service/service.h
    service/serviceHost.cpp
    service/serviceHost.h
    system/runtime/runtime.cpp
    system/runtime/runtime.h
    service/temp/tempService.cpp
    service/temp/tempService.h
    service/temp/temperatureSampler.cpp
    service/temp/temperatureSampler.h
    service/log/logger.cpp
    service/log/logger.h
    system/diagnostic/systemInfoMonitor.cpp
    system/diagnostic/systemInfoMonitor.h
    ui/qml.qrc
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(mlcp-hmi
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET mlcp-hmi APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(mlcp-hmi SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(mlcp-hmi
          ${PROJECT_SOURCES}
        )
    endif()
endif()

target_include_directories(mlcp-hmi
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(mlcp-hmi
  PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_link_libraries(mlcp-hmi
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Quick
        Threads::Threads
        spdlog::spdlog
)

set_target_properties(mlcp-hmi PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_import_qml_plugins(mlcp-hmi)
    qt_finalize_executable(mlcp-hmi)
endif()

enable_testing()

add_executable(ringbuffer_test
    common/utils/ringbuffer/ringbuffer.cpp
    common/utils/ringbuffer/ringbuffer.h
    test/ringbuffer_test.cpp
)

target_include_directories(ringbuffer_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME ringbuffer_test COMMAND ringbuffer_test)

add_executable(runtime_test
    service/log/logger.cpp
    service/log/logger.h
    system/runtime/runtime.cpp
    system/runtime/runtime.h
    test/runtime_test.cpp
)

target_include_directories(runtime_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(runtime_test
    PRIVATE
        Threads::Threads
        spdlog::spdlog
)

add_test(NAME runtime_test COMMAND runtime_test)

add_executable(systemStateMachineTest
    core/state_machine/system_state/systemStateMachine.cpp
    core/state_machine/system_state/systemStateMachine.h
    test/systemStateMachineTest.cpp
)

target_include_directories(systemStateMachineTest
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME systemStateMachineTest COMMAND systemStateMachineTest)

add_executable(lifecycleTest
    core/state_machine/system_state/systemStateMachine.cpp
    core/state_machine/system_state/systemStateMachine.h
    service/lifecycle/lifecycle.cpp
    service/lifecycle/lifecycle.h
    service/log/logger.cpp
    service/log/logger.h
    test/lifecycleTest.cpp
)

target_include_directories(lifecycleTest
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(lifecycleTest
    PRIVATE
        spdlog::spdlog
)

add_test(NAME lifecycleTest COMMAND lifecycleTest)

add_executable(selfTestFlowTest
    service/lifecycle/lifecycle.h
    service/log/logger.cpp
    service/log/logger.h
    service/selftest/selfTestFlow.cpp
    service/selftest/selfTestFlow.h
    test/selfTestFlowTest.cpp
)

target_include_directories(selfTestFlowTest
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(selfTestFlowTest
    PRIVATE
        spdlog::spdlog
)

add_test(NAME selfTestFlowTest COMMAND selfTestFlowTest)

add_executable(tempServiceTest
    service/service.h
    service/temp/tempService.cpp
    service/temp/tempService.h
    service/log/logger.cpp
    service/log/logger.h
    test/tempServiceTest.cpp
)

target_include_directories(tempServiceTest
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(tempServiceTest
    PRIVATE
        spdlog::spdlog
)

add_test(NAME tempServiceTest COMMAND tempServiceTest)

add_executable(tempServiceConfigStoreTest
    persistence/storage/tempServiceConfigStore.cpp
    persistence/storage/tempServiceConfigStore.h
    service/temp/tempService.h
    test/tempServiceConfigStoreTest.cpp
)

target_include_directories(tempServiceConfigStoreTest
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME tempServiceConfigStoreTest COMMAND tempServiceConfigStoreTest)

add_executable(temperatureSamplerTest
    service/temp/temperatureSampler.cpp
    service/temp/temperatureSampler.h
    test/temperatureSamplerTest.cpp
)

target_include_directories(temperatureSamplerTest
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME temperatureSamplerTest COMMAND temperatureSamplerTest)

add_executable(serviceHostTest
    service/service.h
    service/serviceHost.cpp
    service/serviceHost.h
    service/log/logger.cpp
    service/log/logger.h
    test/serviceHostTest.cpp
)

target_include_directories(serviceHostTest
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(serviceHostTest
    PRIVATE
        spdlog::spdlog
)

add_test(NAME serviceHostTest COMMAND serviceHostTest)

add_executable(systemInfoReaderTest
    device/sysinfo/systemInfoReader.cpp
    device/sysinfo/systemInfoReader.h
    test/systemInfoReaderTest.cpp
)

target_include_directories(systemInfoReaderTest
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(systemInfoReaderTest
    PRIVATE
        Threads::Threads
)

add_test(NAME systemInfoReaderTest COMMAND systemInfoReaderTest)

add_executable(systemInfoMonitorTest
    device/sysinfo/systemInfoReader.h
    system/diagnostic/systemInfoMonitor.cpp
    system/diagnostic/systemInfoMonitor.h
    test/systemInfoMonitorTest.cpp
)

target_include_directories(systemInfoMonitorTest
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME systemInfoMonitorTest COMMAND systemInfoMonitorTest)

add_executable(pressureSensorTest
    device/pressure/pressureSensor.cpp
    device/pressure/pressureSensor.h
    test/pressureSensorTest.cpp
)

target_include_directories(pressureSensorTest
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME pressureSensorTest COMMAND pressureSensorTest)

add_executable(fanSettingsMapperTest
    bridge/fanSettingsMapper.cpp
    bridge/fanSettingsMapper.h
    bridge/fanSettingsUiState.h
    bridge/uiError.h
    service/temp/tempService.h
    test/fanSettingsMapperTest.cpp
)

target_include_directories(fanSettingsMapperTest
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME fanSettingsMapperTest COMMAND fanSettingsMapperTest)
