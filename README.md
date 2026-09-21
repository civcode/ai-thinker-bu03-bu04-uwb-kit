# AI-Thinker BU03 / BU04 UWB Kit Configuration Tool

CLI utility for communicating with the AI-Thinker BU03 / BU04 UWB kit over a serial connection.

## Features

- Read device configuration
- Export configuration to JSON
- Update device parameters
- Manage TWR and PDOA settings
- Add and remove TWR tags
- Save, restart, and restore the device

## Requirements

- CMake 3.20+
- A C++20 compiler
- A connected serial device such as `/dev/ttyUSB0`

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

## Tests

Unit tests (Catch2, fetched with CMake `FetchContent`) are configured by default and need
no hardware:

```bash
ctest --test-dir build --output-on-failure      # or: ctest --test-dir build -L unit
```

Hardware-in-the-loop tests are opt-in and require a connected BU03/BU04:

```bash
cmake -S . -B build -DENABLE_HIL_TESTS=ON -DHIL_SERIAL_DEVICE=/dev/ttyUSB0
cmake --build build -j
ctest --test-dir build -L hil --output-on-failure
```

HIL runtime overrides:

- `UWB_HIL_SERIAL_DEVICE` - serial device path (overrides `HIL_SERIAL_DEVICE`)
- `UWB_HIL_REQUIRE_AVAILABILITY=1` - make a missing HIL device fail instead of skip
- `UWB_HIL_CHATTER_WINDOW_MS` - how long each HIL case listens for unsolicited device output before trusting the port; `0` disables the check (fastest run, only for a restored/quiet device)
- `UWB_HIL_REMEASURE_CHATTER=1` - re-measure device chatter in every case instead of once per test binary
- `UWB_HIL_ENABLE_STATEFUL=1` - allow state-changing HIL tests (reserved for future tests)
- `UWB_HIL_ENABLE_DESTRUCTIVE=1` - allow destructive HIL tests (reserved for future tests)

The read-only cases never write to the device and expect a quiet one. A module holding a
configuration it cannot bring up (e.g. a PDOA network it never joins) streams GBK status text on
the same UART as the AT protocol, which breaks `DeviceHandler`'s one-read-per-command flow. Put
the device in the quiet state first:

```bash
./build/cli --device /dev/ttyUSB0 --export backup.json   # keep the current configuration
./build/cli --device /dev/ttyUSB0 --restore             # stops the unsolicited output
```

Each case opens the port, reads `AT+GETUWBMODE` with its own framing (so the mode is reported even
on a chatty device, and the answer's `OK` line is consumed rather than left for the next reader),
then listens for `UWB_HIL_CHATTER_WINDOW_MS` (default 1200 ms). If that listen sees unsolicited
output, the cases skip with the measurement and the remedy above instead of failing. The measured
behaviour and the `HandleComm()` framing issue it exposes are recorded in
`docs/testing/catch2-unit-test-spec.md` §7.1.

That listen is where most of the HIL run time goes — about 12 s for the five cases, because
`catch_discover_tests` gives each case its own process. On a device you have just restored, run
`UWB_HIL_CHATTER_WINDOW_MS=0 ctest --test-dir build -L hil` for the same assertions in about 6 s.
The remaining time is the product's own pacing: a 5 ms per byte write throttle plus an unconditional
100 ms sleep after every command, eight commands for `GetDeviceConfiguration()`.

Changing the UWB mode (`./build/cli --uwb_mode 0`) is not a way to get a quiet port: it restarts
the module, drops the serial link for a second, leaves the status output running, and is not
persistent unless `--save` is given as well.

Disable the test targets entirely with `-DBUILD_TESTING=OFF`.
See `docs/testing/catch2-unit-test-spec.md` for the test plan.

## Usage

Show help:

```bash
./build/cli --help
```

Print the device configuration:

```bash
./build/cli --print
```

Use a specific serial device:

```bash
./build/cli --device /dev/ttyUSB0 --print
```

Export the configuration to a JSON file:

```bash
./build/cli --export device_config.json
```

## Common options

- `--device <path>`: serial device path
- `--print`: print device configuration as JSON
- `--export <file>`: export configuration to a JSON file
- `--uwb_mode <int>`: set UWB mode
- `--dev_id <int>`: set device ID
- `--dev_role <int>`: set device role
- `--dev_channel <int>`: set device channel
- `--dev_rate <int>`: set device rate
- `--twr_tag_cap <int>`: set TWR tag capacity
- `--twr_antenna_delay <int>`: set TWR antenna delay
- `--twr_kalman_filter_enable <int>`: enable or disable TWR Kalman filter
- `--twr_kalman_Q <double>`: set TWR Kalman Q
- `--twr_kalman_R <double>`: set TWR Kalman R
- `--twr_correction_a <double>`: set TWR correction parameter A
- `--twr_correction_b <double>`: set TWR correction parameter B
- `--twr_positioning_enable <int>`: enable or disable positioning
- `--twr_positioning_dimension <int>`: set positioning dimension
- `--pdoa_d_list <int>`: set PDOA D list
- `--pdoa_k_list <int>`: set PDOA K list
- `--pdoa_net <int>`: set PDOA network ID
- `--pdoa_anchor_id <int>`: set PDOA anchor ID
- `--pdoa_uart_rate <int>`: set PDOA UART rate
- `--pdoa_filter_enable <int>`: enable or disable PDOA filter
- `--pdoa_user_cmd <int>`: set PDOA user command
- `--add_tag <string>`: add a TWR tag
- `--delete_tag <string>`: remove a TWR tag
- `--save`: save device configuration
- `--restart`: restart the device
- `--restore`: restore factory settings

## Notes

- The tool defaults to `/dev/ttyUSB0` if no device is specified.
- The device is accessed at `115200` baud.
- Some actions update configuration on the device immediately; use them carefully.

## Example

Read and export configuration in one command:

```bash
./build/cli --device /dev/ttyUSB0 --print --export device_config.json
```

## License

Add your license here.
