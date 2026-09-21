# BU03 / BU04 AT Protocol Reference

This is an implementation-oriented AT protocol reference for the CLI in this repository.

**Protocol baseline:** AI-Thinker BU03 / BU04 AT Command Manual **V1.0.6**.  
**Compatibility notes:** AI-Thinker BU03 / BU04 AT Command Manual **V1.0.7**.

The CLI names are used as the primary parameter names. Vendor terminology is retained only where it is needed to match the wire protocol.

> Important: the English V1.0.6 PDF circulating for this device is not fully equivalent to the current official Chinese V1.0.6 document. In particular, the official V1.0.6 document contains `AT+SETMCUMODE` and `AT+GETMCUMODE`, while the English V1.0.6 PDF jumps directly from `AT+SETDEV` to the PDOA section. The official Ai-Thinker documents should therefore be treated as the protocol authority.

## Version compatibility summary

| Area | V1.0.6 baseline | V1.0.7 change / clarification | Repository impact |
|---|---|---|---|
| Firmware mapping | Official site associates AT V1.0.6 with firmware V1.0.0. | Official site associates AT V1.0.7 with firmware V1.0.1. | Version-aware validation is useful if both firmware families are supported. |
| `AT+SETCFG` ID | `ID` documented as `0..10`. | Anchor `0..8`, tag `0..100`. | Do not document the wider role-specific ranges as V1.0.6 behavior. |
| `AT+SETCFG` rate | Fourth field is fixed to `1`; only 6.8M documented. | `Rate=0` = 850K, `Rate=1` = 6.8M. | `--dev_rate 0` is a V1.0.7 feature. Keep `1` as the V1.0.6-safe value. |
| `AT+SETCFG` group | Fifth field exists; group `0..255`; tags should use `0`. | Same, with clearer syntax and behavior. | CLI has no `--dev_group`; preserve the existing value when rewriting the tuple or document any fixed/default value. |
| `AT+SAVE` | Described only as saving configuration. | Explicitly writes RAM to Flash and resets after about 100 ms. | On V1.0.7, treat `SAVE` as the final command in a transaction and tolerate the reset/disconnect. |
| `AT+RESTART` | Reset/restart. | Explicitly says unsaved RAM settings are lost. | No command syntax change. |
| `AT+SETUWBMODE` | `0` TWR, `1` PDOA; save after setting. | Explicitly says the change is not applied until `AT+SAVE`. | Existing separate `--save` model is compatible; document that `--uwb_mode` alone only stages the change. |
| `AT+GETDEV` / `AT+SETDEV` | Same 9-field `SETDEV` tuple; `GETDEV` payload is not documented in the English V1.0.6 example. | V1.0.7 documents full `getdev ...` and `setdev ...` payloads and field meanings. | A parser for the detailed payload is V1.0.7-spec-backed; V1.0.6 support may rely on observed device behavior. |
| MCU timing | Present in the current official V1.0.6 (`SETMCUMODE` / `GETMCUMODE`), but absent from the English V1.0.6 PDF. | Same commands, with clearer naming/response. | Not a V1.0.7-only feature. Only expose it if L4 low-power tag interoperability is needed. |
| PDOA config tuple | `PDOASETCFG=1,1,3333,1,100,0,0` and `gtcfg` readback are shown, but field meanings are mostly undocumented. | Defines `Dlist,Klist,Panid,Ancid,Rate,Filter,Usercmd`. | Existing CLI field names match V1.0.7 clarifications; mark those semantics as clarified in V1.0.7, not guaranteed by V1.0.6 text. |
| PDOA offsets | `PDOAOFF` and `RNGOFF` already exist. Units are not defined. | Angle is degrees; range offset is mm. | Missing CLI options are a V1.0.6 coverage gap, not a V1.0.7 addition. |
| PDOA filter | `FILTER=1` example only. | `0` off, `1` on, default on. | Current boolean CLI naming is consistent; V1.0.7 supplies the formal meaning. |
| PDOA output format | V1.0.6 official Chinese text states `0` JSON, `1` Hex. | Same. | No compatibility issue. |
| `ADDTAG` refresh fields | Five fields already exist. | V1.0.7 adds that `multfast` and `multslow` must be non-zero. | Add validation if desired; syntax is unchanged. |

## CLI argument to AT protocol mapping

| CLI option | AT command | Wire argument | V1.0.6 meaning / accepted values | V1.0.7 difference | Notes |
|---|---|---|---|---|---|
| `--uwb_mode <int>` | `AT+SETUWBMODE=<mode>` | `mode` | `0` = TWR, `1` = PDOA; save after setting. | Same; V1.0.7 explicitly says the switch does not take effect until `AT+SAVE`. | Safe on both versions. |
| `--dev_id <int>` | `AT+SETCFG=<ID>,<Role>,<CH>,<Rate>,<Group>` | `ID` | `0..10` | Anchor `0..8`; tag `0..100` | Validation should be version-aware if V1.0.7 is supported. |
| `--dev_role <int>` | `AT+SETCFG=...` | `Role` | `0` = tag, `1` = anchor | Same | Part of complete `SETCFG` tuple. |
| `--dev_channel <int>` | `AT+SETCFG=...` | `CH` | `0` = channel 9, `1` = channel 5 | Same | Part of complete `SETCFG` tuple. |
| `--dev_rate <int>` | `AT+SETCFG=...` | `Rate` | V1.0.6 documents only `1` = 6.8M. | `0` = 850K, `1` = 6.8M | `0` should be treated as V1.0.7-only unless verified on a particular V1.0.6 firmware build. |
| *(no CLI option)* | `AT+SETCFG=...` | `Group` | `0..255`; tags use `0`. | Same | Required fifth field. Preserve it when changing another device field. |
| `--twr_tag_cap <int>` | `AT+SETDEV=<cap>,...` | `cap` | Tag capacity; English V1.0.6 also parenthetically calls it tag refresh rate. | V1.0.7 simply calls it tag capacity. | Range/units are not specified. |
| `--twr_antenna_delay <int>` | `AT+SETDEV=...,<antdelay>,...` | `antdelay` | Antenna-delay parameter. | Clarified as written to both RX and TX delay. | Integer in the examples. |
| `--twr_kalman_filter_enable <int>` | `AT+SETDEV=...,<kalman_enable>,...` | `kalman_enable` | Kalman enable field; exact values not enumerated in V1.0.6 English text. | `0` off, `1` on. | Current CLI boolean model matches V1.0.7. |
| `--twr_kalman_Q <double>` | `AT+SETDEV=...` | `kalman_Q` | Kalman Q parameter. | Clarified as process noise Q. | Floating point. |
| `--twr_kalman_R <double>` | `AT+SETDEV=...` | `kalman_R` | Kalman R parameter. | Clarified as measurement noise R. | Floating point. |
| `--twr_correction_a <double>` | `AT+SETDEV=...` | `para_a` | Correction parameter a. | Clarified as distance-calibration coefficient a. | Floating point. |
| `--twr_correction_b <double>` | `AT+SETDEV=...` | `para_b` | Correction parameter b. | Clarified as distance-calibration coefficient b. | Floating point. |
| `--twr_positioning_enable <int>` | `AT+SETDEV=...` | `pos_enable` | Positioning enable field. | Same wording, still no full value range. | Do not invent additional values. |
| `--twr_positioning_dimension <int>` | `AT+SETDEV=...` | `pos_dimen` | Positioning dimension setting. | Same; values still not enumerated. | Do not invent dimension codes. |
| `--pdoa_d_list <int>` | `AT+PDOASETCFG=<Dlist>,...` | `Dlist` | Present in the 7-value tuple/readback; V1.0.6 does not define its semantics. | Discovery-list capacity limit; vendor says use `1`. | Treat the semantic label as V1.0.7 clarification. |
| `--pdoa_k_list <int>` | `AT+PDOASETCFG=...,<Klist>,...` | `Klist` | Present in tuple/readback; semantics not defined. | Paired-list capacity limit; vendor says use `1`. | Treat the semantic label as V1.0.7 clarification. |
| `--pdoa_net <int>` | `AT+PDOASETCFG=...,<Panid>,...` | `Panid` | Example input `3333` reads back as `Net:0D05`, consistent with decimal input / hexadecimal readback. | V1.0.7 explicitly documents this. | `3333 decimal = 0x0D05`. |
| `--pdoa_anchor_id <int>` | `AT+PDOASETCFG=...,<Ancid>,...` | `Ancid` | Present as `AncID` in readback; no formal range in V1.0.6. | Described as anchor short address; vendor says use `1`. | Avoid claiming a wider range without device testing. |
| `--pdoa_uart_rate <int>` | `AT+PDOASETCFG=...,<Rate>,...` / `AT+UARTRATE=<rate>` | `Rate` | Vendor calls it serial-port rate; example value is `100`; units are not defined. | Same; default shown as `100`. | Do not describe this as UART baud rate. |
| `--pdoa_filter_enable <int>` | `AT+PDOASETCFG=...,<Filter>,...` / `AT+FILTER=<enable>` | `Filter` | Filter field; `FILTER=1` shown. | `0` off, `1` on; default on. | Anchor/PDOA setting. |
| `--pdoa_user_cmd <int>` | `AT+PDOASETCFG=...,<Usercmd>` / `AT+USER_CMD=<fmt>` | `Usercmd` | `0` JSON, `1` Hex format in official V1.0.6. | Same, described as hexadecimal binary. | Anchor/PDOA setting. |
| `--add_tag <string>` | `AT+ADDTAG=<addr64>,<addr16>,<multfast>,<multslow>,<mode>` | complete payload | Five fields already defined in V1.0.6. | V1.0.7 adds that `multfast` and `multslow` must not be zero. | Use `AT+SAVE` when persistence is required. |
| `--delete_tag <string>` | `AT+DELTAG=<addr>` | `addr` | Long/64-bit tag address. | Same; response format is documented more precisely. | Anchor only. |
| `--save` | `AT+SAVE` | - | Save configuration. | Explicitly writes RAM to Flash and resets after about 100 ms. | Put last in a write transaction on V1.0.7. |
| `--restart` | `AT+RESTART` | - | Reset/restart. | Explicitly does not save pending RAM settings. | No syntax change. |
| `--restore` | `AT+RESTORE` | - | Restore factory mode/settings. | Clarified as restore defaults and reset. | Destructive to current settings. |
| `--print` | read commands | - | Host-side aggregate read. | V1.0.7 documents more read payloads. | Parser should tolerate version differences. |
| `--export [file]` | read commands | - | Host-side aggregate read and JSON export. | Same | Not an AT command. |
| `--device <path>` | host-side only | - | Serial device path. | Same | Not an AT command. |

## Raw AT command table

| Category | AT command | V1.0.6 syntax / behavior | V1.0.7 delta |
|---|---|---|---|
| General | `AT` | Parser test. | No material change. |
| General | `AT+GETVER` | Returns software version. | Response format is explicitly documented as `getver software:<version>`. |
| General | `AT+SAVE` | Save configuration. | Explicit RAM-to-Flash write + reset after about 100 ms. |
| General | `AT+RESTART` | Reset/restart. | Explicitly does not save pending RAM changes. |
| General | `AT+RESTORE` | Restore factory mode/settings. | Explicitly restores defaults and resets. |
| General | `AT+GETWORKMODE` | `0` normal, `1` production test. | Same; low-power-firmware limitation is stated explicitly. |
| General | `AT+SETWORKMODE=<mode>` | `0` normal, `1` production test. | Same. |
| Device | `AT+GETCFG` | Returns `ID,Role,CH,Rate,Group`. | Field meanings are explicitly enumerated. |
| Device | `AT+SETCFG=<ID>,<Role>,<CH>,<Rate>,<Group>` | V1.0.6 notation fixes `Rate` to literal `1`; `ID=0..10`; `Group=0..255`. | `Rate` becomes `0/1`; ID becomes role-dependent (`0..8` anchor, `0..100` tag); immediate TWR start is documented. |
| Production | `AT+GETSENSOR` | BU03 supported, BU04 hardware unsupported. | Response fields and meanings are documented. |
| Production | `AT+TESTLED=<mode>` | `1` start, `0` stop. | No material change. |
| Production | `AT+TESTOLED=<text>` | OLED test string. | Syntax/meaning made explicit. |
| Production | `AT+DISTANCE` | Ranging result query. | Explicit `distance:<dis>` response definition. |
| TWR | `AT+GETDEV` | Command exists; V1.0.6 English example only shows `OK`. | Full `getdev cap:...` response documented. |
| TWR | `AT+SETDEV=<cap>,<antdelay>,<kalman_enable>,<kalman_Q>,<kalman_R>,<para_a>,<para_b>,<pos_enable>,<pos_dimen>` | Same 9-field tuple. | Field meanings and `setdev ...` response clarified; explicit save note. |
| TWR | `AT+SETMCUMODE=<mode>` | Present in current official V1.0.6: `0` F1 standard tag, `1` L4 low-power tag. | Same, described as MCU communication timing. |
| TWR | `AT+GETMCUMODE` | Present in current official V1.0.6. | Response `mcu_mode:<mode>` documented. |
| PDOA | `AT+DECA$` | PDOA PC software certification/info. | Device/version/build/driver payload fields documented. |
| PDOA | `AT+GETDLIST` | Get discovery list, anchor only. | 64-bit address response schema documented. |
| PDOA | `AT+GETKLIST` | Get pairing list, anchor only. | Slot/address/refresh/mode schema documented. |
| PDOA | `AT+ADDTAG=<addr64>,<addr16>,<multfast>,<multslow>,<mode>` | Five tag fields. | `multfast` and `multslow` must be non-zero; response schema documented. |
| PDOA | `AT+DELTAG=<addr>` | Delete tag by long address. | Response schema documented. |
| PDOA | `AT+PDOAOFF=<value>` | Angle correction. | Value is degrees; persistence note added. |
| PDOA | `AT+RNGOFF=<value>` | Distance correction. | Value is millimetres; persistence note added. |
| PDOA | `AT+FILTER=<enable>` | Filtering; example uses `1`. | `0` off, `1` on, default on. |
| PDOA | `AT+UARTRATE=<rate>` | Vendor calls it serial-port rate; example `100`. | Default `100` documented; units still not specified. |
| PDOA | `AT+USER_CMD=<fmt>` | `0` JSON, `1` Hex in official V1.0.6. | Same. |
| PDOA | `AT+PDOASETCFG=<Dlist>,<Klist>,<Panid>,<Ancid>,<Rate>,<Filter>,<Usercmd>` | Seven-value example and `gtcfg` readback exist, but semantics are mostly implicit. | All seven fields formally named and described. |
| PDOA | `AT+PDOAGETCFG` | `gtcfg Dlist:... KList:... Net:... AncID:... Rate:... Filter:... UserCmd:... pdoaOffset:... rngOffset:...` shown. | Same structure, now fully described. |
| Algorithm | `AT+SETUWBMODE=<mode>` | `0` TWR, `1` PDOA; save after setting. | Explicitly does not switch until `AT+SAVE`. |
| Algorithm | `AT+GETUWBMODE` | Query current algorithm; response format not defined in V1.0.6 English document. | `twr_pdoa_mode:<mode>` documented. |

## Composite command argument order

These setters use positional tuples. When the CLI modifies one field, it should preserve the other fields rather than silently replacing them with guessed defaults.

| Command | Positional payload | V1.0.6-safe notes |
|---|---|---|
| `AT+SETCFG` | `ID, Role, CH, Rate, Group` | Use `Rate=1` for strict V1.0.6 compatibility. Preserve `Group` because the CLI does not expose it directly. |
| `AT+SETDEV` | `cap, antdelay, kalman_enable, kalman_Q, kalman_R, para_a, para_b, pos_enable, pos_dimen` | Shape is common to V1.0.6 and V1.0.7. |
| `AT+PDOASETCFG` | `Dlist, Klist, Panid, Ancid, Rate, Filter, Usercmd` | Shape is already present in V1.0.6; V1.0.7 supplies formal field definitions. |
| `AT+ADDTAG` | `addr64, addr16, multfast, multslow, mode` | Shape is common; non-zero refresh-multiplier validation is explicitly documented only in V1.0.7. |

## CLI coverage gaps relative to V1.0.6

These are not V1.0.7 additions; they already exist in the V1.0.6 protocol but are not exposed by the current CLI shown in the repository help output.

| Protocol field / command | Current CLI status | Suggested treatment |
|---|---|---|
| `Group` in `AT+SETCFG` | No `--dev_group`. | Preserve the read value when editing another `dev_*` field; optionally expose `--dev_group`. |
| `AT+SETMCUMODE` / `AT+GETMCUMODE` | No MCU-mode option. | Optional unless the project needs F1-anchor/L4-low-power-tag interoperability. |
| `AT+PDOAOFF` | No angle-offset option. | Optional if PDOA calibration should be controllable from the CLI. |
| `AT+RNGOFF` | No range-offset option. | Optional if PDOA distance calibration should be controllable from the CLI. |
| Work-mode / production-test commands | Not exposed. | Reasonable to omit from a configuration-focused CLI. |

## Compatibility rules for the implementation

1. Treat **V1.0.6 as the default contract** for existing repository behavior.
2. For V1.0.6, validate `--dev_rate` as `1` only unless actual hardware testing demonstrates otherwise.
3. For V1.0.7, allow `--dev_rate 0` and use the role-dependent ID ranges.
4. Keep `SETCFG`, `SETDEV`, and `PDOASETCFG` tuple updates read-modify-write. In particular, do not lose `Group` simply because the CLI has no `--dev_group` option.
5. Do not send another configuration command after `AT+SAVE` on V1.0.7; it resets after the save.
6. Parse read responses by labels rather than fixed spacing. V1.0.7 adds richer payloads but retains the same command family.
7. Do not require `SETMCUMODE` for normal F1-to-F1 operation; default mode `0` is the standard path. Add it when L4 low-power tag support is a project requirement.
8. Do not call `pdoa_uart_rate` a baud-rate setting. The vendor uses a value such as `100` and does not define conventional baud units in either spec.
9. Preserve unknown or undocumented values on read-modify-write operations rather than normalizing them to guessed defaults.

## Version detection

The current Ai-Thinker UWB documentation page associates:

- firmware **V1.0.0** with AT manual **V1.0.6**;
- firmware **V1.0.1** with AT manual **V1.0.7**.

`AT+GETVER` is therefore the natural first version check. The manuals contain stale example version strings, so the actual device response should be treated as authoritative. If a future firmware reports a version not in the table, prefer conservative V1.0.6 validation unless the newer behavior has been verified.
