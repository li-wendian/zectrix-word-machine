# Zectrix Word Machine / Zectrix 单词机固件

**暂时只支持离线使用，有计划开发联网功能。**

这是 Zectrix S3 4.2 英寸电子墨水屏单词机的 ESP-IDF 固件工程。仓库根目录就是可直接编译的固件项目，不包含上一级目录里的本地整理文件。

当前固件内置考研英语词库，支持单词卡片复习、顺序/随机背词、每日目标、复习进度保存、NFC 入口链接、Wi-Fi 配网和 OTA 配置。

## 中文使用说明

完整固件使用说明见：

```text
docs/USER_GUIDE.zh-CN.md
```

说明内容包括：

- 首次上电后的屏幕信息。
- 上键、下键、确认键的短按/长按功能。
- 背词正面、背面、完成页和设置页的操作方式。
- 复习规则和每日目标。
- 如何准备自己的 JSON 单词库。
- 如何生成 `main/vocabulary/kaoyan_full_deck.inc`。
- 如何修改词库名称、重新编译、刷机和清空旧进度。

## 按键速查

| 按键 | GPIO | 背词界面短按 | 背词界面长按 |
| --- | --- | --- | --- |
| 上键 | GPIO39 | 上一张；如果在背面，则当前词稍后再看 | 与下键在约 1.4 秒内先后长按可进入设置页 |
| 下键 | GPIO18 | 下一张；如果在背面，则当前词稍后再看 | 与上键在约 1.4 秒内先后长按可进入设置页 |
| 确认键 | GPIO0 | 正面翻到背面；背面标记为认识并进入下一张 | 当前词稍后再看；设置页中长按退出 |

当前版本没有单独的“答错”键。不认识的词可以用上键、下键或长按确认键延后复习。

## 更换自己的单词库

词库当前是编译进固件的，核心文件是：

```text
main/vocabulary/kaoyan_full_deck.inc
```

最常用流程：

1. 准备 UTF-8 JSON 词库，例如 `data/my_words.json`。
2. 执行生成脚本：

```bash
python tools/generate_kaoyanluan_deck.py \
  --source data/my_words.json \
  --output main/vocabulary/kaoyan_full_deck.inc
```

3. 修改 `main/vocabulary/deck_store.cc` 中的 `deck_id`、`manifest_.name` 和 `manifest_.version`。
4. 重新编译并刷机。
5. 如果新词库和旧词库差异很大，擦除 NVS 或整片擦除后再刷机，避免旧进度错配。

详细 JSON 字段示例和清空进度命令见 `docs/USER_GUIDE.zh-CN.md`。

## 硬件目标

- Board: `zectrix-s3-epaper-4.2`
- MCU: ESP32-S3
- Display: 400 x 300 e-paper
- Audio codec: ES8311
- RTC: PCF8563
- NFC: GT23SC6699
- Flash layout: 16 MB, `partitions/v2/16m.csv`

## 编译环境

- ESP-IDF 5.4 或更新版本
- Python 3
- Windows 下建议使用 ESP-IDF PowerShell、Git Bash 或 WSL

先加载 ESP-IDF 环境：

```bash
. "$IDF_PATH/export.sh"
```

开发编译：

```bash
idf.py set-target esp32s3
idf.py build
```

刷机和串口监视：

```bash
idf.py -p COM5 flash monitor
```

把 `COM5` 换成你的实际串口。

## 打包发布

可选本地默认配置写在 `.env`，该文件不会提交到 Git：

```bash
cp .env.example .env
```

打包：

```bash
./build.sh
```

常用参数：

```bash
./build.sh --no-rebuild
./build.sh --ota-url https://your.example/ota/
./build.sh zectrix-s3-epaper-4.2 zectrix-s3-epaper-4.2
```

打包结果在 `releases/`，合并烧录文件在 `build/merged-binary.bin`。这两个目录都已被 `.gitignore` 排除。

## 目录结构

- `main/`: 固件应用、板级支持、显示、音频、单词复习逻辑和本地组件。
- `main/boards/zectrix-s3-epaper-4.2/`: 当前硬件的引脚、电源、RTC、NFC、墨水屏、充电检测和工厂测试支持。
- `main/vocabulary/`: 内置词库、复习状态和调度逻辑。
- `docs/`: 使用说明和参考资料。
- `partitions/`: ESP-IDF 分区表。
- `scripts/`: 发布打包和素材处理工具。
- `tools/`: 单词库生成工具。

## License

MIT. See `LICENSE`.
