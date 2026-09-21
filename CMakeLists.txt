cmake_minimum_required(VERSION 3.0)

project(uart)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmax-errors=5")

set(CMAKE_BUILD_TYPE Debug)

include(FetchContent)

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

FetchContent_MakeAvailable(json)
FetchContent_MakeAvailable(cxxopts)

set(SOURCES
    ${CMAKE_SOURCE_DIR}/src/device_handler.cpp
    ${CMAKE_SOURCE_DIR}/src/configuration_file.cpp

) 

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(cli ${CMAKE_SOURCE_DIR}/src/cli.cpp ${SOURCES})
target_link_libraries(cli PRIVATE nlohmann_json::nlohmann_json cxxopts::cxxopts)

# add_executable(test ${CMAKE_SOURCE_DIR}/src/test.cpp ${SOURCES})
# target_link_libraries(test PRIVATE cxxopts::cxxopts)