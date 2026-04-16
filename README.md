# Zectrix Word Machine / Zectrix 单词机

ESP-IDF firmware for the Zectrix S3 4.2-inch e-paper vocabulary device. It boots into a compact word-review experience with local Kaoyan vocabulary data, e-paper UI rendering, button input, audio hardware support, RTC, NFC, battery/charge detection, Wi-Fi provisioning, and OTA hooks.

Recommended GitHub repository name: `zectrix-word-machine`.

## Hardware Target

- Board: `zectrix-s3-epaper-4.2`
- MCU: ESP32-S3
- Display: 400 x 300 e-paper panel
- Audio codec: ES8311
- RTC: PCF8563
- NFC: GT23SC6699
- Flash layout: 16 MB, `partitions/v2/16m.csv`

## Requirements

- ESP-IDF 5.4 or newer
- Python 3
- Git Bash, WSL, Linux, macOS shell, or ESP-IDF shell for `build.sh`

Install ESP-IDF first, then export the environment before building:

```bash
. "$IDF_PATH/export.sh"
```

## Build

```bash
idf.py set-target esp32s3
idf.py build
```

Flash and monitor:

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

On Windows, replace `/dev/ttyUSB0` with your serial port, for example `COM5`.

## User Guide

See `docs/USER_GUIDE.zh-CN.md` for Chinese usage instructions, button behavior, settings, review rules, and vocabulary deck replacement steps.

## Release Package

Optional local defaults live in `.env`; this file is ignored by git.

```bash
cp .env.example .env
./build.sh
```

Useful variants:

```bash
./build.sh --no-rebuild
./build.sh --ota-url https://your.example/ota/
./build.sh zectrix-s3-epaper-4.2 zectrix-s3-epaper-4.2
```

Release zips are written to `releases/` and merged flashing binaries are generated under `build/`. Both are ignored so GitHub keeps only source files.

Set `SYNC_LOCAL_OUTPUTS=true` in `.env` only when you want `build.sh` to copy the merged firmware to a local management dashboard or OTA bin directory.

## Vocabulary Deck

The committed `main/vocabulary/kaoyan_full_deck.inc` lets the firmware build without redistributing raw source JSON. To regenerate it from local source data:

```bash
python tools/generate_kaoyanluan_deck.py \
  --source ../KaoYan_1.json ../KaoYan_2.json ../KaoYan_3.json \
  --output main/vocabulary/kaoyan_full_deck.inc
```

The generator also looks for `data/KaoYan_1.json`, `data/KaoYan_2.json`, and `data/KaoYan_3.json` when those files are kept inside the firmware repo.

## Project Layout

- `main/`: firmware application, board support, display, audio, vocabulary logic, and local components.
- `main/boards/zectrix-s3-epaper-4.2/`: board pinout, power, RTC, NFC, e-paper, charge, and factory-test support.
- `main/vocabulary/`: built-in vocabulary deck and review scheduler.
- `partitions/`: ESP-IDF partition tables.
- `scripts/`: release packaging and asset helper scripts.
- `tools/`: vocabulary deck generation tools.

## License

MIT. See `LICENSE`.
