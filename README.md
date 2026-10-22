# RL78/F24 CAN boot-loader application

Boot-aware application for RL78/F24 R7F124FPJ5xFB, paired with
`..\RL78_F24_Boot_loader_CAN`. The image starts at `0x2000` and expects Boot
handoff, rather than standalone reset installation.

INFO health page `0x1B` reports APP_READY after initialization, CAN setup
and an observed timer IRQ. Tool checks live App version/build/size after END,
complementing Boot CRC validation. See [update confirmation](https://released.github.io/slide_isp_tool_for_RL78/#update-confirmation)
and the [porting guide (zh-TW)](https://released.github.io/slide_boot_loader_for_RL78/#boot-app-porting).

## Public examples and checkout layout

- [CAN Transfer benchmark](https://github.com/released/RL78_F24_CAN_Transfer).
- [Required sibling Boot project](https://github.com/released/RL78_F24_Boot_loader_CAN).
- [This App project](https://github.com/released/RL78_F24_Boot_loader_CAN_app).

Keep the exact repository names under the same parent; remove `-main` after
ZIP extraction. App uses its own SC output and the Boot board sources,
image/CAN/RAM contracts, parser and build helpers. App alone is not a
self-contained checkout. The [Boot README](https://github.com/released/RL78_F24_Boot_loader_CAN#readme)
owns wiring, full Flash geometry, Boot decisions and recovery.

## App-only IRQ extension

Edit `app_image_config.h` and `app_irq_handlers.h`. Pre-build generates
`ftable.inc`, `app_identity.h`, `app_irq_table.inc` and the
`ftable_RL78_F24.asm` wrapper. Inspect these outputs, but edit their C inputs.
Post-build rejects stale linked metadata/vectors after configuration changes.

App release settings belong to `app_image_config.h`. Shared
`image_metadata_config.h` owns format/MCU/Flash/policy/IRQ ABI;
`can_boot_config.h` owns CAN and `boot_app_handoff.h` owns no-init RAM.
`image_metadata.c/.h` is compiled independently for App parsing and Boot
validation, with expected values from `image_metadata_profile.c`.
See the [C-header workshop](https://released.github.io/slide_boot_loader_for_RL78/#device-metadata-vector-workshop)
and [metadata C APIs](https://released.github.io/slide_boot_loader_for_RL78/#device-metadata-parser).

For each added normal interrupt:

1. Find the physical vector byte address in this MCU `iodefine.h` and hardware
   interrupt-vector table.
2. Register its C source in this App CS+ project.
3. Define one global `#pragma interrupt Name` ISR without `vect=`.
4. Add the vector/function pair to `APP_IRQ_HANDLERS(X)`.
5. Initialize the peripheral, clear pending causes and enable used IRQs;
   clean rebuild/update App.

The sample maps `0x2E` (`INTTM01`, TAU0 channel 1) to `Device_TimerIsr`.
Boot owns physical vectors; the App table supplies RETI destinations.
Boot/App may use the same ISR name because they are separate linked images.
Unused sources stay masked; default RETI does not clear peripheral causes.
See [interrupt ownership](https://released.github.io/slide_boot_loader_for_RL78/#device-irq-ownership).

## Memory layout

| Range | Purpose |
| --- | --- |
| `0x00000..0x01FFF` | Reserved Boot; outside the update BIN |
| `0x02000..0x020FF` | 64 four-byte App branch entries |
| `0x02100..0x021FF` | 256-byte metadata region, format 6 |
| `0x02100..0x0217F` | 128-byte defined core |
| `0x02180..0x021FF` | 128 reserved bytes, `0xFF` |
| `0x02200..linked end` | App startup/code/constants |
| Final four image bytes | Little-endian CRC32 |
| After image end through `0x3FBFF` | Outside this image when below capacity |
| `0x3FC00..0x3FFFF` | Protected last 1 KiB |

App slot = `0x2000 + physical_vector_byte_address * 2`.
Capacity is `0x3DC00` (252928) bytes; map-derived actual size is four-byte
aligned and includes table, metadata and CRC.

## Runtime identity and boot provenance

`APP_RUNTIME_LOG_TAG` in `app_runtime_log.h` is an observation tag independent
of the metadata semantic version. Select `"V1"` or `"V2"` and clean rebuild
App for a distinguishable update test.

```text
F24 CAN DEVICE APP: boot-validated; X1=4M CPU/CAN40
app image tag: V1
CAN update: completed
log(timer)-V1: 0
log(timer)-V1: 1
```

`boot-validated` means Boot supplied the one-shot signature at `0xFF714`.
`CAN update: completed` appears only when the separate signature at `0xFF718`
records a completed update. App consumes/clears both before board
initialization; missing handoff requests Boot. The decimal counter starts
at zero and prints once per second in main, not the timer ISR.

## CAN boot entry

Classical CAN defaults to 500 kbit/s: P10 TX, P11 RX, P12 LOW.
UART0 P15/P16 is diagnostic output at 115200. Clock is X1 4 MHz ->
PLL 80 MHz -> CPU/CAN 40 MHz. CAN requests are polled; TAU0 channel 1 uses IRQ.

For analyzer discovery send standard `0x700`, DLC 2, `07 00`; App replies
on `0x701`, DLC 8, INFO page 0 with application state. ENTER `0x06`, DLC 1,
is acknowledged before recording the request at `0xFF71C` and resetting.
Boot consumes it and starts the valid-App inactivity window.
Tool operation is `Discover Target` -> `Enter Boot` -> `Start Device Update`;
discovery already reporting Boot skips ENTER. The [operation guide](https://released.github.io/slide_isp_tool_for_RL78/#mcu-can-device)
covers the 30-second pre-START timeout and re-discovery.

The paired Boot `can_boot_config.h` owns IDs/rate declarations;
`common/f24_device_config.h` owns timing overrides. For a different bitrate,
update the matching Boot/App configuration and follow the [250K/500K/1M steps](https://released.github.io/slide_boot_loader_for_RL78/#device-can-bitrate).
Host Link baud and diagnostic UART baud are independent settings.

`APP_IMAGE_VERSION_CONTROL` defaults to `0U`. An installed valid App with `1U`
allows ordinary same/newer App semantic versions. Boot/App releases are
independent; minimum Boot and IRQ/layout contracts establish compatibility.
See [version policy and maintenance](https://released.github.io/slide_isp_tool_for_RL78/#force-old-version).
CRC and version ordering are not cryptographic authentication.

## Build and artifacts

Use Windows PowerShell 5.1/.NET and CS+/CC-RL, without Python or SRecord.
From this App directory:

```powershell
..\RL78_F24_Boot_loader_CAN\build.ps1 -Kind app -AppProject .
```

For initial combined images, build Boot first; `build.ps1 -Kind all` from
Boot builds Transfer, Boot and App. An App-only release does not rebuild Boot.
MTPJ pre/post-build hooks run the paired `tools/prepare_projects.ps1` and
`tools/package_images.ps1`.

Packaging validates linked S-record/map, metadata and ISR targets, derives
the aligned image length, patches metadata size and appends CRC32.
No manual image-size define is needed. Clean rebuild after source/header
changes instead of repackaging stale outputs.

| File | Purpose |
| --- | --- |
| `DefaultBuild/RL78_F24_Boot_loader_CAN_app_update.bin` | Tool CAN App update |
| `DefaultBuild/flash_app_variable_CRC.hex` | Packaged App-only programmer image |
| `DefaultBuild/flash2000_3FBFF.hex` | Same App HEX named for the allowed partition; populated length remains variable |
| `DefaultBuild/app_build_report.html` | Offline actual BIN metadata, byte addresses, CRC/SHA-256 and vector table |
| Root `boot_app.hex` / `boot_app.bin` | Combined initial installation, generated when validated Boot MOT/map are available |
| `DefaultBuild/RL78_F24_Boot_loader_CAN_app.mot` / `.map` | Raw compiler/debug/packaging inputs; not Tool update images |

The packager excludes device-file physical/debug records from raw MOT.
Use packaged HEX or combined HEX for programmer installation, and update BIN
for Tool. Git distribution retains only the update BIN in DefaultBuild;
rebuild to recreate reports/HEX/compiler outputs. Root combined images remain.
See [publication and recovery](https://github.com/released/simple_renesas_isp_tool/blob/main/docs/GITHUB_PUBLISH_zh-TW.md)
and the [browser build summary](https://released.github.io/slide_boot_loader_for_RL78/#device-app-build-report).

## FF-filled App test images

After a successful App build, double-click `generate_test_bins.bat`, or run
`powershell -NoProfile -ExecutionPolicy Bypass -File .\generate_test_bins.ps1`
from this App directory. The matching sibling Boot supplies
`tools/app_config.ps1`; no Python, SRecord or MCU compiler invocation
is needed.

The script reads the packaged `DefaultBuild/*_update.bin` and creates a new
`test_bins/<batch>/` with test BINs, `manifest.csv` and `skipped.csv`.
Default Flash budgets are 16/32/64/128/256/512 KiB, including Boot; the actual
BIN subtracts the Boot reserve and respects metadata capacity. With the current
App, lengths are 8/24/56/120/247 KiB; the last 1 KiB remains protected. An App
that no longer fits a budget is skipped rather than truncated. One KiB is 1024 bytes.

App code and vectors remain unchanged. FF padding starts at the original CRC
location; metadata image_size at BIN `0x118..0x11B` is updated, and the new final
four bytes contain little-endian CRC32. Version/build/policy remain unchanged.
The original BIN is never overwritten. Test outputs are ignored by Git and
can be regenerated after each App build; both generator scripts are shared.
Use the existing Tool CAN Upgrade workflow and keep its JSON operation report.
See the [test-image guide](https://released.github.io/slide_boot_loader_for_RL78/#device-test-images).

## Debugging with E2 Lite

App expects Boot to establish clock and handoff. For integrated testing,
program combined Boot/App, retain physical vectors/option bytes, load App
symbols and run from reset through Boot. Raw compiler records downloaded
over Boot-owned addresses are not the production App-update path.
See the [porting workflow](https://released.github.io/slide_boot_loader_for_RL78/#boot-app-porting)
for startup/linker and symbol-loading considerations.

## Verification requirements

- Keep `.app_vectors` at `0x2000..0x20FF`, metadata at `0x2100..0x21FF`
  and normal code at or above `0x2200`.
- Check all 64 branch destinations, mapped RETI handlers and default BRK RETB.
- Match linked identity with the editable headers; check image size, partition,
  reserved bytes and independently calculated CRC.
- After SC regeneration, reconcile App projections/hooks and no-`vect=` mapping.
- After a complete three-project clean build, run `test.ps1` from Boot for
  artifact/RAM RFD/metadata/vector/corruption tests. Optional native C/Tool
  models use `-ToolProject <Tool checkout>`.
- On hardware, confirm END, reset, APP_READY/live identity and the UART
  tag/counter. Tool automatically saves a Firmware update report.

Normal 500K update, CRC, App startup and UART counter are [verified](https://released.github.io/slide_isp_tool_for_RL78/#f24-device-update-example).
Other rates, IRQ mappings, stress, cancellation/disconnect and power loss
remain separate tests in the linked acceptance matrix.
