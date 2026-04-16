#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 加载 .env 配置（在参数解析前，使 .env 中的值可作为默认值）
if [[ -f "$PROJECT_DIR/.env" ]]; then
  # shellcheck disable=SC1091
  source "$PROJECT_DIR/.env"
  echo "[INFO] 已加载 .env 配置"
fi

DEFAULT_BOARD="zectrix-s3-epaper-4.2"
DEFAULT_OTA_URL="${DEFAULT_OTA_URL:-https://ota.zectrix.com/zectrix-word-machine/ota/}"

REBUILD=true
OTA_URL="$DEFAULT_OTA_URL"

POSITIONAL_ARGS=()

while [[ $# -gt 0 ]]; do
  case "$1" in
    --rebuild)
      REBUILD=true
      shift
      ;;
    --no-rebuild)
      REBUILD=false
      shift
      ;;
    --ota-url)
      if [[ $# -lt 2 ]]; then
        echo "[ERROR] --ota-url 需要传入地址" >&2
        exit 1
      fi
      OTA_URL="$2"
      shift 2
      ;;
    -h|--help)
      cat <<'EOF'
用法:
  ./build.sh [--no-rebuild] [--ota-url URL] [board_type] [build_name]

示例:
  ./build.sh
  ./build.sh --no-rebuild
  ./build.sh --ota-url https://ota.zectrix.com/zectrix-word-machine/ota/
  ./build.sh --no-rebuild zectrix-s3-epaper-4.2 zectrix-s3-epaper-4.2

默认值:
  board_type = zectrix-s3-epaper-4.2
  build_name = 与 board_type 相同
  ota_url = .env 中的 DEFAULT_OTA_URL 或 https://ota.zectrix.com/zectrix-word-machine/ota/

参数:
  --no-rebuild   跳过 fullclean，增量编译（默认执行 fullclean 完整重编译）
  --ota-url URL  指定打包写入的 CONFIG_OTA_URL
EOF
      exit 0
      ;;
    --)
      shift
      while [[ $# -gt 0 ]]; do
        POSITIONAL_ARGS+=("$1")
        shift
      done
      ;;
    -*)
      echo "[ERROR] 未知参数: $1" >&2
      exit 1
      ;;
    *)
      POSITIONAL_ARGS+=("$1")
      shift
      ;;
  esac
done

if [[ ${#POSITIONAL_ARGS[@]} -gt 2 ]]; then
  echo "[ERROR] 参数过多，仅支持 [board_type] [build_name]" >&2
  exit 1
fi

BOARD_TYPE="${POSITIONAL_ARGS[0]:-$DEFAULT_BOARD}"
BUILD_NAME="${POSITIONAL_ARGS[1]:-$BOARD_TYPE}"

if [[ -z "$OTA_URL" ]]; then
  echo "[ERROR] OTA 地址不能为空" >&2
  exit 1
fi

load_idf_env() {
  if command -v idf.py >/dev/null 2>&1; then
    return 0
  fi

  if [[ -n "${IDF_PATH:-}" && -f "${IDF_PATH}/export.sh" ]]; then
    # shellcheck disable=SC1090
    source "${IDF_PATH}/export.sh"
  fi

  if command -v idf.py >/dev/null 2>&1; then
    return 0
  fi

  for export_sh in "$HOME/esp/esp-idf/export.sh" "/root/esp/esp-idf/export.sh"; do
    if [[ -f "$export_sh" ]]; then
      # shellcheck disable=SC1090
      source "$export_sh"
      break
    fi
  done

  if ! command -v idf.py >/dev/null 2>&1; then
    echo "[ERROR] 未检测到 idf.py，请先安装 ESP-IDF 并执行 source export.sh" >&2
    exit 1
  fi
}

if ! command -v python3 >/dev/null 2>&1; then
  echo "[ERROR] 未检测到 python3，请先安装 Python 3" >&2
  exit 1
fi

PROJECT_NAME="$(python3 - "$PROJECT_DIR/CMakeLists.txt" <<'PY'
import re
import sys
from pathlib import Path

text = Path(sys.argv[1]).read_text(encoding="utf-8")
match = re.search(r"^\s*project\(([^)\s]+)", text, re.MULTILINE)
print(match.group(1) if match else "zectrix_word_machine")
PY
)"

cd "$PROJECT_DIR"
load_idf_env

BOARD_CONFIG_DIR="main/boards/${BOARD_TYPE}"
BOARD_CONFIG_PATH="${BOARD_CONFIG_DIR}/config.json"
TEMP_CONFIG_NAME="config.build.sh.$$.json"
TEMP_CONFIG_PATH="${BOARD_CONFIG_DIR}/${TEMP_CONFIG_NAME}"

if [[ ! -f "$BOARD_CONFIG_PATH" ]]; then
  echo "[ERROR] 未找到板型配置文件: $BOARD_CONFIG_PATH" >&2
  exit 1
fi

cleanup_temp_config() {
  if [[ -f "$TEMP_CONFIG_PATH" ]]; then
    rm -f "$TEMP_CONFIG_PATH"
  fi
}

trap cleanup_temp_config EXIT

python3 - "$BOARD_CONFIG_PATH" "$TEMP_CONFIG_PATH" "$BUILD_NAME" "$OTA_URL" \
  "${DEFAULT_WIFI_SSID:-}" "${DEFAULT_WIFI_PASSWORD:-}" <<'PY'
import json
import os
import sys

config_path, output_path, build_name, ota_url, wifi_ssid, wifi_password = sys.argv[1:7]

with open(config_path, "r", encoding="utf-8") as f:
    config = json.load(f)

build = None
for item in config.get("builds", []):
    if item.get("name") == build_name:
        build = item
        break

if build is None:
    print(f"[ERROR] 在 {config_path} 中未找到 build_name={build_name}", file=sys.stderr)
    sys.exit(1)

sdkconfig_append = build.setdefault("sdkconfig_append", [])

# 需要注入的 sdkconfig 键值对
injections = {
    "CONFIG_OTA_URL": f'"{ota_url}"',
}
if wifi_ssid:
    injections["CONFIG_DEFAULT_WIFI_SSID"] = f'"{wifi_ssid}"'
    injections["CONFIG_DEFAULT_WIFI_PASSWORD"] = f'"{wifi_password}"'

for key, value in injections.items():
    entry = f"{key}={value}"
    for i, existing in enumerate(sdkconfig_append):
        if isinstance(existing, str) and existing.startswith(f"{key}="):
            sdkconfig_append[i] = entry
            break
    else:
        sdkconfig_append.append(entry)

with open(output_path, "w", encoding="utf-8") as f:
    json.dump(config, f, ensure_ascii=False, indent=4)
    f.write("\n")
PY

if [[ "$REBUILD" == "true" ]]; then
  echo "[INFO] --rebuild 已启用，执行 fullclean 并删除旧发布包"
  idf.py fullclean
  shopt -s nullglob
  old_zips=(releases/v*_${BUILD_NAME}.zip)
  if [[ ${#old_zips[@]} -gt 0 ]]; then
    rm -f "${old_zips[@]}"
  fi
  shopt -u nullglob
fi

if [[ -n "${DEFAULT_WIFI_SSID:-}" ]]; then
  echo "[INFO] 预设WiFi: $DEFAULT_WIFI_SSID"
fi

echo "[INFO] 开始打包: board_type=$BOARD_TYPE, build_name=$BUILD_NAME, ota_url=$OTA_URL"
python3 scripts/release.py "$BOARD_TYPE" --config "$TEMP_CONFIG_NAME" --name "$BUILD_NAME"

LATEST_ZIP="$(ls -1t releases/v*_${BUILD_NAME}.zip 2>/dev/null | head -n 1 || true)"

if [[ -z "$LATEST_ZIP" ]]; then
  echo "[ERROR] 打包完成但未找到 releases/v*_${BUILD_NAME}.zip" >&2
  exit 1
fi

ZIP_SIZE="$(stat -c%s "$LATEST_ZIP")"
BIN_PATH="build/merged-binary.bin"
BIN_SIZE="0"

if [[ -f "$BIN_PATH" ]]; then
  BIN_SIZE="$(stat -c%s "$BIN_PATH")"
fi

echo "[OK] 打包完成"
echo "[OK] ZIP: $LATEST_ZIP (${ZIP_SIZE} bytes)"
echo "[OK] BIN: $BIN_PATH (${BIN_SIZE} bytes)"

# ── 可选：同步固件到本地管理后台/OTA 目录 ──
if [[ -f "$BIN_PATH" && "${SYNC_LOCAL_OUTPUTS:-false}" == "true" ]]; then
  MANAGER_NEXT_DIR="${MANAGER_NEXT_DIR:-$PROJECT_DIR/../main/manager-next}"
  CHANGELOG_SOURCE="$PROJECT_DIR/CHANGELOG.md"
  if [[ -f "$PROJECT_DIR/changelog.md" ]]; then
    CHANGELOG_SOURCE="$PROJECT_DIR/changelog.md"
  fi
  FW_VERSION="$(basename "$LATEST_ZIP" .zip | sed "s/_${BUILD_NAME}$//")"
  FW_TIME="$(date '+%Y-%m-%d %H:%M:%S')"

  for target_dir in "$MANAGER_NEXT_DIR/public/firmware" "$MANAGER_NEXT_DIR/out/firmware"; do
    mkdir -p "$target_dir"
    cp "$BIN_PATH" "$target_dir/merged-binary.bin"
    if [[ -f "$CHANGELOG_SOURCE" ]]; then
      cp "$CHANGELOG_SOURCE" "$target_dir/changelog.md"
    fi
    cat > "$target_dir/firmware-info.json" <<FWEOF
{
  "version": "$FW_VERSION",
  "board": "$BOARD_TYPE",
  "buildName": "$BUILD_NAME",
  "buildTime": "$FW_TIME",
  "fileSize": $BIN_SIZE,
  "fileName": "merged-binary.bin"
}
FWEOF
  done

  echo "[OK] 已同步固件及更新日志到管理后台 (public + out)"

  # ── 复制 OTA 固件到 data/bin/，供设备自动更新 ──
  OTA_BIN="build/${PROJECT_NAME}.bin"
  OTA_BIN_DIR="${OTA_BIN_DIR:-$PROJECT_DIR/../main/zectrix-server/data/bin}"
  if [[ -f "$OTA_BIN" ]]; then
    mkdir -p "$OTA_BIN_DIR"
    OTA_BIN_NAME="${BUILD_NAME}_${FW_VERSION#v}.bin"
    cp "$OTA_BIN" "$OTA_BIN_DIR/$OTA_BIN_NAME"
    echo "[OK] 已复制 OTA 固件: data/bin/$OTA_BIN_NAME ($(stat -c%s "$OTA_BIN") bytes)"
  else
    echo "[WARN] 未找到 $OTA_BIN，跳过 OTA 固件复制"
  fi
elif [[ -f "$BIN_PATH" ]]; then
  echo "[INFO] 未启用本地输出同步（如需同步，设置 SYNC_LOCAL_OUTPUTS=true）"
fi
