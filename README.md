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

- CMake 3.0+
- A C++20 compiler
- A connected serial device such as `/dev/ttyUSB0`

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

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
