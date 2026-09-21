# Test Specification (Catch2 + Hardware-in-the-Loop)

## Project
AI-Thinker BU03 / BU04 UWB Kit Configuration Tool

## Document purpose
Define a **detailed automated test plan** for this project.

This document covers two complementary layers:
1. **Unit tests** using Catch2 with mocks/fakes
2. **Hardware-in-the-loop (HIL) tests** using a real connected BU03/BU04 device

This document is a **specification only**.

- No tests are implemented yet.
- No CMake test target is implemented yet.
- The goal is to define structure, scope, test cases, fixtures, and expected behavior before writing tests.

---

## 1) Test strategy overview

The project communicates with a serial-connected hardware device using AT commands. Because of that, a complete quality strategy must include both:

### 1.1 Unit tests
Unit tests validate:
- JSON serialization/deserialization
- configuration file loading/saving
- command formatting
- response parsing
- orchestration logic
- CLI helper logic

Unit tests must:
- run without real hardware
- be deterministic
- use mocked/fake UART transport
- be suitable for CI

### 1.2 Hardware-in-the-loop (HIL) tests
HIL tests validate:
- actual UART communication with a physical device
- compatibility with real firmware responses
- behavior across read/write command sequences
- state-changing commands and subsequent readback verification
- assumptions made by unit tests against real firmware behavior

HIL tests are not a replacement for unit tests; they complement them.

---

## 2) Catch2 and CMake requirements

## 2.1 Catch2 acquisition
Catch2 shall be:
- fetched by **CMake FetchContent**
- integrated at the **project level**
- built as part of the project build configuration
- available to all test targets through standard target linkage

### Requirement
Future CMake integration shall follow this model:
- declare Catch2 using `FetchContent_Declare(...)`
- make it available with `FetchContent_MakeAvailable(...)`
- create one or more test executables linked against `Catch2::Catch2WithMain` or equivalent
- register tests with CTest

### Non-goals
- Do not vendor Catch2 manually into the repository.
- Do not require a system-installed Catch2 package.

## 2.2 Test target organization
Planned target structure:
- `cli` — production executable
- `unit_tests` — Catch2-based unit test executable(s)
- `hil_tests` — Catch2-based HIL test executable(s), optionally gated behind a CMake option

## 2.3 Suggested future CMake options
These are specification-level requirements only:
- `BUILD_TESTING=ON/OFF`
- `ENABLE_HIL_TESTS=ON/OFF`
- `HIL_SERIAL_DEVICE=/dev/ttyUSB0` (or equivalent cache variable)

HIL tests must be opt-in by default unless the environment is explicitly configured for them.

---

## 3) Scope and boundaries

## 3.1 In scope for unit tests
1. `src/device_handler.cpp`
2. `src/configuration_file.cpp`
3. `include/configuration/serialization.hpp`
4. Pure helper logic currently in `src/cli.cpp`:
   - `SameDeviceParameters`
   - `SameTwrParameters`
   - `SamePdoaParameters`
   - `ParseSetCfg`
5. Default helper behavior in `include/transport/iuart.hpp` (`readText`, `writeText`) using a fake implementation

## 3.2 In scope for HIL tests
1. Real command/response behavior against a connected BU03/BU04 device
2. Read-only configuration retrieval flows
3. Controlled state-changing flows with readback verification
4. Save/restart workflows when explicitly enabled
5. Device detection/selection by configured serial path

## 3.3 Out of scope for unit tests
1. Real serial I/O timing on physical hardware
2. OS-level USB enumeration behavior
3. Firmware correctness beyond observed response parsing
4. End-to-end shell scripting around the CLI unless refactored into testable code

## 3.4 Out of scope for HIL tests
1. Destructive tests that may permanently brick hardware
2. Factory reset tests unless explicitly approved and isolated
3. High-volume soak testing unless later added as a separate test plan

---

## 4) Proposed test architecture

## 4.1 Proposed directory tree

```text
tests/
  CMakeLists.txt
  test_main.cpp
  fakes/
    fake_uart.hpp
  fixtures/
    temp_file_fixture.hpp
    hil_fixture.hpp
  unit/
    device_handler_tests.cpp
    configuration_file_tests.cpp
    serialization_tests.cpp
    iuart_default_method_tests.cpp
    cli_helper_tests.cpp
  hil/
    cli_hil_tests.cpp
    device_handler_hil_tests.cpp
```

## 4.2 Core test doubles for unit tests

### `FakeUart`
Required capabilities:
- queue scripted read responses (`std::string` + byte count)
- capture `write()` / `writeSlow()` payloads in order
- control failures (`write=false`, timeout by returning 0 bytes)
- expose last command and full command history
- optionally capture timeout values passed to `read`

## 4.3 Fixtures

### Temp-file fixture
Required capabilities:
- create isolated temp directory per test
- write valid or malformed JSON test files
- auto cleanup

### HIL fixture
Required capabilities:
- obtain serial device path from config/env/CMake cache
- open real UART safely
- optionally snapshot device configuration before mutation
- support best-effort restore of original state after tests
- skip tests cleanly if no hardware is available

## 4.4 Conventions
- Naming style: `TEST_CASE("[Component] ...", "[tag]")`
- Tags:
  - `[unit]`
  - `[hil]`
  - `[serialization]`
  - `[config-file]`
  - `[device-handler]`
  - `[cli-helper]`
  - `[error-path]`
  - `[read-only]`
  - `[stateful]`
- Floating-point comparisons: `Catch::Approx`
- Each implemented test should reference a spec ID, e.g. `UT-DH-023`, `HIL-CLI-004`

---

## 5) Unit test specification

## 5.1 `serialization.hpp`

## 5.1.1 `gbk_to_utf8`
- **UT-SER-001**: Empty string -> empty string
- **UT-SER-002**: ASCII input remains equivalent text
- **UT-SER-003**: Valid GBK bytes convert to expected UTF-8
- **UT-SER-004**: Invalid GBK sequence throws `std::runtime_error`

## 5.1.2 Struct JSON mapping (`to_json` / `from_json`)
For each type:
- `WorkMode`
- `UwbMode`
- `DeviceParameters`
- `SensorData`
- `TwrParameters`
- `PdoaParameters`
- `TagParameters`
- `PdoaMisc`
- `DeviceConfiguration`
- `DeviceConfigurationPatch`

Required coverage:
- **UT-SER-010..019**: `to_json` contains expected keys/values
- **UT-SER-020..029**: `from_json` restores equivalent object
- **UT-SER-030..039**: Missing required fields throw JSON exception

Additional required checks:
- **UT-SER-040**: `DeviceConfiguration::to_json` includes `timestamp` in format `%Y-%m-%d %H:%M:%S`
- **UT-SER-041**: `DeviceConfigurationPatch::to_json` omits absent optionals
- **UT-SER-042**: `DeviceConfigurationPatch::from_json` with `null` resets optional
- **UT-SER-043**: `PdoaMisc::to_json` routes strings through `gbk_to_utf8`
- **UT-SER-044**: `PdoaMisc::to_json` propagates GBK conversion exceptions

---

## 5.2 `configuration_file.cpp`

## 5.2.1 `Load`
- **UT-CFG-001**: Valid file loads full `DeviceConfiguration`
- **UT-CFG-002**: Nonexistent file throws runtime_error with file path
- **UT-CFG-003**: Malformed JSON throws runtime_error with parse context
- **UT-CFG-004**: Valid JSON missing required fields throws runtime_error

## 5.2.2 `LoadPatch`
- **UT-CFG-010**: Valid patch with `deviceParam` only loads correctly
- **UT-CFG-011**: Valid patch with `twrParam` only loads correctly
- **UT-CFG-012**: Patch with explicit `null` resets optionals
- **UT-CFG-013**: Nonexistent file throws runtime_error
- **UT-CFG-014**: Malformed patch JSON throws runtime_error

## 5.2.3 `Save` / `GetJsonString`
- **UT-CFG-020**: `Save` writes parseable JSON to disk
- **UT-CFG-021**: Save to unwritable location throws runtime_error
- **UT-CFG-022**: `GetJsonString` returns parseable JSON
- **UT-CFG-023**: `GetJsonString` reflects input object values

---

## 5.3 `device_handler.cpp`

## 5.3.1 Internal parsing behavior

### `ExtractErrorCode`
- **UT-DH-001**: response containing `OK` -> `kSuccess`
- **UT-DH-002**: response containing `ERR` (without OK) -> `kError`
- **UT-DH-003**: response containing neither -> `kUnexpectedResponse`
- **UT-DH-004**: response containing both `OK` and `ERR` -> current behavior `kSuccess` due to check order

### `ParseResponse`
- **UT-DH-010**: Leading CR/LF/NUL are trimmed
- **UT-DH-011**: Response truncates at first `\r\n` and appends trailing comma
- **UT-DH-012**: OK response returns `kSuccess`
- **UT-DH-013**: ERR response returns `kError`
- **UT-DH-014**: Unknown response returns `kUnexpectedResponse`

### `ExtractDataString`
- **UT-DH-020**: Extracts substring between prefix and delimiter
- **UT-DH-021**: Missing prefix -> `kUnexpectedResponse`
- **UT-DH-022**: Missing delimiter -> `kUnexpectedResponse`
- **UT-DH-023**: Multiple prefix occurrences -> first-match behavior

### `HandleComm`
- **UT-DH-030**: Writes command and reads response
- **UT-DH-031**: Zero bytes read -> `kTimeout`
- **UT-DH-032**: `returnRawResponse=true` bypasses parse and returns raw response
- **UT-DH-033**: Parsed OK -> `kSuccess` and normalized response
- **UT-DH-034**: Parsed ERR/unknown -> current behavior `kUnexpectedResponse`
- **UT-DH-035**: Inter-command wait logic preserves sequencing; no strict wall-clock assertion required

## 5.3.2 Getter command methods
For each getter verify:
1. exact command string sent
2. success-path parsing
3. timeout/error propagation
4. malformed response behavior

### Methods in scope
- `GetAt`
- `GetVer`
- `GetWorkMode`
- `GetCfg`
- `GetSensor`
- `GetDistance`
- `GetDev`
- `GetPdoaCfg`
- `GetUwbMode`
- `GetDeca`
- `GetDList`
- `GetKList`

Representative cases:
- **UT-DH-100**: `GetAt` sends `AT\r\n`
- **UT-DH-101**: `GetVer` parses `getver software:<v>,`
- **UT-DH-102**: `GetWorkMode` parses integer after `workmode:`
- **UT-DH-103..106**: `GetCfg` parses ID/Role/CH/Rate and fails on missing/malformed token
- **UT-DH-110..116**: `GetSensor` parses floats and rejects malformed numeric values
- **UT-DH-120..123**: `GetDistance` parses `distance: <x>,`
- **UT-DH-130..138**: `GetDev` parses all TWR fields and handles malformed field content
- **UT-DH-140..149**: `GetPdoaCfg` parses all PDOA fields including hex `Net`
- **UT-DH-150**: `GetUwbMode` parse success
- **UT-DH-151**: `GetUwbMode` malformed integer path
- **UT-DH-160**: `GetDeca` raw response path + error code extraction
- **UT-DH-161**: `GetDList` raw response path
- **UT-DH-162**: `GetKList` raw response path

Known-behavior capture tests:
- **UT-DH-170**: `GetDList` currently returns `kSuccess` even if extracted status indicates error
- **UT-DH-171**: `GetKList` currently returns `kSuccess` even if extracted status indicates error

## 5.3.3 Setter/action command methods
Verify command formatting and return mapping:
- **UT-DH-200**: `SetCfg` -> `AT+SETCFG=id,role,ch,rate\r\n`
- **UT-DH-201**: `AddTag` -> `AT+ADDTAG=a64,a16,F,S,M\r\n`
- **UT-DH-202**: `DelTag` -> `AT+DELTAG=a64\r\n`
- **UT-DH-203**: `SetWorkMode` -> `AT+SETWORKMODE=<n>\r\n`
- **UT-DH-204**: `SetPdoaOffset` -> `AT+PDOAOFF=<n>\r\n`
- **UT-DH-205**: `SetRngOffset` -> `AT+RNGOFF=<n>\r\n`
- **UT-DH-206**: `SetPdoaCfg` -> full `AT+PDOASETCFG=...\r\n`
- **UT-DH-207**: `SetUwbMode` -> `AT+SETUWBMODE=<n>\r\n`
- **UT-DH-208**: `TestOled` sends `AT+TESTOLED\r\n`
- **UT-DH-209**: `TestLed(state)` currently ignores `state` and sends `AT+TESTLED\r\n`

### `SetDev` special path
- **UT-DH-220**: Command formatting for `AT+SETDEV=...`
- **UT-DH-221**: Read timeout -> `kTimeout`
- **UT-DH-222**: Parse OK -> `kSuccess`
- **UT-DH-223**: Parse ERR/unknown -> parse result

## 5.3.4 `GetDeviceConfiguration` orchestration
- **UT-DH-300**: Full happy path populates all fields
- **UT-DH-301..307**: Failure at each earlier step propagates failure
- **UT-DH-308**: If `GetPdoaCfg` fails, current code logs failure but still returns success (capture current behavior)
- **UT-DH-309**: `pdoaMisc` receives raw deca/dlist/klist responses

---

## 5.4 `iuart.hpp` default helper behavior
Using a fake derived transport:
- **UT-IUART-001**: `readText` clears prior string content
- **UT-IUART-002**: `readText` resizes string to actual byte count
- **UT-IUART-003**: `readText` forwards timeout values to `read`
- **UT-IUART-004**: `writeText` delegates to `writeSlow` with 5ms delay
- **UT-IUART-005**: `writeText` forwards exact payload bytes

---

## 5.5 CLI helper logic (`src/cli.cpp`)

> Note: these helpers currently live in the same translation unit as `main()`. For clean unit testing, they should be moved into a separate header/source pair before test implementation.

### Equality helpers
- **UT-CLI-001**: `SameDeviceParameters` true for identical objects
- **UT-CLI-002**: `SameDeviceParameters` false when any field differs
- **UT-CLI-003**: `SameTwrParameters` true/false coverage across all fields
- **UT-CLI-004**: `SamePdoaParameters` true/false coverage across all fields

### `ParseSetCfg`
- **UT-CLI-010**: Parses 4 integers successfully
- **UT-CLI-011**: `_` preserves current value in each position
- **UT-CLI-012**: Mixed explicit values and `_`
- **UT-CLI-013**: Non-integer token throws runtime_error
- **UT-CLI-014**: Too few fields throws runtime_error
- **UT-CLI-015**: Too many fields throws runtime_error
- **UT-CLI-016**: Negative values are parsed and applied
- **UT-CLI-017**: Tokens with unsupported whitespace fail (capture current behavior)

---

## 6) Hardware-in-the-loop (HIL) test specification

## 6.1 HIL guiding principles
1. HIL tests must be clearly separated from unit tests.
2. HIL tests must be opt-in.
3. HIL tests must prefer read-only operations by default.
4. State-changing tests must:
   - capture original device state first
   - restore state after test when feasible
   - be marked `[stateful]`
5. Dangerous commands (`restore`, persistent writes) must be additionally gated.

## 6.2 HIL environment requirements
Required before running HIL tests:
- supported BU03 or BU04 device connected via USB
- known serial device path (for example `/dev/ttyUSB0`)
- user has permission to access the serial port
- device is in a firmware state compatible with the CLI protocol

Optional future controls:
- environment variable: `UWB_HIL_SERIAL_DEVICE`
- environment variable: `UWB_HIL_ENABLE_STATEFUL=1`
- environment variable: `UWB_HIL_ENABLE_DESTRUCTIVE=1`

## 6.3 HIL read-only tests
These are safe baseline tests and should be the first implemented.

### Real connectivity and protocol sanity
- **HIL-DH-001**: open configured serial port successfully
- **HIL-DH-002**: `GetAt` receives valid response from real device
- **HIL-DH-003**: `GetVer` returns non-empty version string
- **HIL-DH-004**: `GetWorkMode` returns parseable integer
- **HIL-DH-005**: `GetCfg` returns parseable device configuration
- **HIL-DH-006**: `GetDev` returns parseable TWR configuration
- **HIL-DH-007**: `GetUwbMode` returns parseable value
- **HIL-DH-008**: `GetPdoaCfg` returns parseable PDOA configuration
- **HIL-DH-009**: `GetDeca` returns raw response containing recognizable status
- **HIL-DH-010**: `GetDList` returns non-empty/raw decodable response
- **HIL-DH-011**: `GetKList` returns non-empty/raw decodable response
- **HIL-DH-012**: `GetDeviceConfiguration` completes successfully and yields a coherent aggregate object

### CLI read-only behavior
- **HIL-CLI-001**: `./build/cli --help` exits successfully
- **HIL-CLI-002**: `./build/cli --device <port> --print` exits successfully and prints JSON
- **HIL-CLI-003**: `./build/cli --device <port> --export <tmpfile>` writes JSON file
- **HIL-CLI-004**: exported JSON re-loads via `ConfigurationFile::Load`

## 6.4 HIL stateful non-destructive tests
These require readback verification and restoration where possible.

### UWB mode
- **HIL-DH-100**: read current UWB mode, set same mode again, verify readback
- **HIL-DH-101**: if alternate mode is safe, switch mode, verify, restore original mode

### Device config parameters
- **HIL-DH-110**: read current config, set same config, verify success
- **HIL-DH-111**: change one safe field, verify readback, restore original
- **HIL-DH-112**: change multiple safe fields, verify readback, restore original

### TWR config
- **HIL-DH-120**: read current TWR config, set same config, verify success
- **HIL-DH-121**: change one safe TWR field, verify readback, restore original

### PDOA config
- **HIL-DH-130**: read current PDOA config, set same config, verify success
- **HIL-DH-131**: change one safe PDOA field, verify readback, restore original

### Save/restart
- **HIL-DH-140**: `Save` succeeds after a known safe config write
- **HIL-DH-141**: `Restart` succeeds and device becomes responsive again after reboot wait

## 6.5 HIL risky/destructive tests
These must be disabled by default.

- **HIL-DH-200**: `Restore` factory settings succeeds and device remains reachable
- **HIL-DH-201**: full save/restart/restore cycle

These tests must only run when explicitly enabled and only on non-production hardware.

## 6.6 HIL negative-path tests
Where safe and possible:
- **HIL-DH-300**: open invalid serial path -> expected failure
- **HIL-DH-301**: issue command when device is disconnected -> expected timeout/open failure
- **HIL-DH-302**: unsupported argument via CLI returns non-zero exit

## 6.7 HIL observability requirements
Future HIL implementation should capture:
- serial device path used
- raw responses for failed tests
- timing around restart/wait-sensitive operations
- before/after config snapshots for stateful tests

---

## 7) Risk-driven bug-capture tests

These tests intentionally document current behavior, including behavior that may later be changed.

1. **UT-DH-170/171**: `GetDList` / `GetKList` currently return success even if extracted status indicates error.
2. **UT-DH-308**: `GetDeviceConfiguration` can return success even if PDOA config retrieval fails.
3. **UT-DH-209**: `TestLed(state)` ignores the `state` parameter.
4. **Integration/HIL note**: CLI prints selected `--device`, but currently opens the hardcoded default `device` variable instead of the parsed option.
5. **Integration/HIL note**: code checks `result.count("set_cfg")` but no `set_cfg` option is declared.

These should be preserved as explicit tests or tracked issues before refactoring.

---

## 8) Non-functional expectations

## 8.1 Unit suite
- deterministic
- hardware-independent
- suitable for CI
- target runtime: under 2 seconds

## 8.2 HIL suite
- clearly opt-in
- fail fast when device is unavailable
- provide useful logs for diagnosis
- avoid unnecessary persistent state changes

---

## 9) Definition of done for future implementation

1. Catch2 integrated using **CMake FetchContent at project level**.
2. Unit test targets build with the main project.
3. HIL test target exists and is gated by CMake option.
4. Unit tests cover serialization, config files, DeviceHandler parsing/formatting, and CLI helpers.
5. HIL tests cover read-only workflows first.
6. Stateful HIL tests implement restoration or explicit safeguards.
7. Tests are runnable through CTest.
8. CI runs unit tests automatically; HIL tests run only in suitable environments.

---

## 10) Explicitly deferred

1. Long-duration soak tests
2. Throughput/latency benchmarks
3. Multi-device parallel HIL testing
4. Pseudo-terminal based integration tests (may be added later between unit and HIL layers)

---

## Appendix A: Example UART response fixtures for unit tests

- `GETCFG OK`: `"getcfg ID:1,Role:2,CH:5,Rate:0,\r\nOK\r\n"`
- `GETDEV OK`: `"cap:8 anndelay:16385,kalman_enable:1,kalman_Q:0.1,kalman_R:0.2,para_a:1.0,para_b:2.0,pos_enable:1,pos_dimen:2,\r\nOK\r\n"`
- `PDOAGETCFG OK`: `"Dlist:1 KList:2 Net:1A AncID:3 Rate:4 Filter:1 UserCmd:0 pdoaOffset:10 rngOffset:20,\r\nOK\r\n"`
- generic error: `"ERR\r\n"`
- timeout simulation: fake UART read returns `0`

## Appendix B: Implementation notes for later

Before writing tests, consider small refactors to improve testability:
1. move CLI helper functions out of `src/cli.cpp` into dedicated utility files
2. optionally expose/internalize parser helpers more cleanly
3. ensure parsed `--device` value is actually used when constructing `Uart`
4. add explicit CLI option definition for `set_cfg` if intended
