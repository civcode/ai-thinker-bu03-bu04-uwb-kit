# Test targets for the BU03/BU04 configuration tool.
# Layout and naming follow docs/testing/catch2-unit-test-spec.md §2.2 and §4.1.
#
# tests/
#   fakes/fake_uart.hpp             FakeUart test double (§4.2) - implemented
#   fixtures/hil_fixture.hpp        HIL device path + gating fixture (§4.3) - implemented
#   fixtures/temp_file_fixture.hpp  deferred until the UT-CFG-* cases are written
#   unit/*.cpp                      unit cases (UT-SER-*, UT-CFG-*, UT-DH-*, UT-IUART-*, UT-CLI-*)
#   hil/*.cpp                       HIL cases (HIL-DH-*, HIL-CLI-*)
#
# No tests/test_main.cpp: Catch2::Catch2WithMain supplies main(), which the spec
# allows explicitly (§2.1). Add Catch2::Catch2 + a custom main only if needed later.

# catch_discover_tests() comes from Catch2's extras (installed via FetchContent).
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)

set(TEST_SOURCES
    ${PROJECT_SOURCE_DIR}/src/device_handler.cpp
    ${PROJECT_SOURCE_DIR}/src/configuration_file.cpp
)

# ---------------------------------------------------------------------------
# unit_tests - Catch2, hardware-independent, CI-safe
# ---------------------------------------------------------------------------
add_executable(unit_tests
    unit/smoke_tests.cpp
    ${TEST_SOURCES}
)

target_include_directories(unit_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}          # lets tests include "fakes/..." and "fixtures/..."
)

target_link_libraries(unit_tests PRIVATE
    nlohmann_json::nlohmann_json
    Catch2::Catch2WithMain               # Catch2 provides main() for this target
)

catch_discover_tests(unit_tests PROPERTIES LABELS "unit")

# ---------------------------------------------------------------------------
# hil_tests - opt-in, requires a physical BU03/BU04 on a serial port
# ---------------------------------------------------------------------------
if(ENABLE_HIL_TESTS)
    add_executable(hil_tests
        hil/read_only_hil_tests.cpp
        ${TEST_SOURCES}
    )

    target_include_directories(hil_tests PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    # Device path: -DHIL_SERIAL_DEVICE=/dev/ttyUSB0, overridable at runtime by
    # the UWB_HIL_SERIAL_DEVICE environment variable (spec §6.2).
    target_compile_definitions(hil_tests PRIVATE HIL_SERIAL_DEVICE="${HIL_SERIAL_DEVICE}")

    target_link_libraries(hil_tests PRIVATE
        nlohmann_json::nlohmann_json
        Catch2::Catch2WithMain
    )

    catch_discover_tests(hil_tests PROPERTIES LABELS "hil")
endif()
