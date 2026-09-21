cmake_minimum_required(VERSION 3.20)

project(uart LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmax-errors=5")

set(CMAKE_BUILD_TYPE Debug)

include(FetchContent)
# include(CTest) provides the BUILD_TESTING option (default ON) and enable_testing()
include(CTest)

# ---------------------------------------------------------------------------
# Test options (docs/testing/catch2-unit-test-spec.md §2.3)
# ---------------------------------------------------------------------------
option(ENABLE_HIL_TESTS "Build hardware-in-the-loop test target (requires a real BU03/BU04 device)" OFF)
set(HIL_SERIAL_DEVICE "" CACHE STRING "Serial device path used by HIL tests, e.g. /dev/ttyUSB0")

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------

# Fetch nlohmann/json
FetchContent_Declare(
  json
  URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
)
FetchContent_Declare(
  cxxopts
  GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
  GIT_TAG v3.3.1
)

# Catch2 v3 is fetched (not vendored, not system-installed) and only when the
# test targets are actually configured.
if(BUILD_TESTING)
  # Keep Catch2's own self-tests/examples out of this build.
  set(CATCH_BUILD_TESTING OFF CACHE BOOL "" FORCE)
  set(CATCH_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(CATCH_INSTALL_DOCS OFF CACHE BOOL "" FORCE)

  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.16.0
    GIT_SHALLOW TRUE
  )
endif()

FetchContent_MakeAvailable(json)
FetchContent_MakeAvailable(cxxopts)
if(BUILD_TESTING)
  FetchContent_MakeAvailable(Catch2)
endif()

# ---------------------------------------------------------------------------
# Production executable
# ---------------------------------------------------------------------------

set(SOURCES
    ${CMAKE_SOURCE_DIR}/src/device_handler.cpp
    ${CMAKE_SOURCE_DIR}/src/configuration_file.cpp

)

add_executable(cli ${CMAKE_SOURCE_DIR}/src/cli.cpp ${SOURCES})
target_include_directories(cli PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(cli PRIVATE nlohmann_json::nlohmann_json cxxopts::cxxopts)

# ---------------------------------------------------------------------------
# Test targets (unit_tests, hil_tests) - see tests/CMakeLists.txt
# ---------------------------------------------------------------------------
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

# add_executable(test ${CMAKE_SOURCE_DIR}/src/test.cpp ${SOURCES})
# target_link_libraries(test PRIVATE cxxopts::cxxopts)
