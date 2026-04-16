# Zectrix Word Machine / Zectrix 单词机固件

**暂时只支持离线使用，有计划开发联网功能。**

这是 Zectrix S3 4.2 英寸电子墨水屏单词机的 ESP-IDF 固件工程。仓库根目录就是可直接编译的固件项目，不包含上一级目录里的本地整理文件。

当前固件内置考研英语词库，主界面是单词卡片复习，支持顺序/随机背词、每日目标、复习进度保存和 NFC 入口链接。代码中保留了 Wi-Fi 配网与 OTA 相关配置基础，但当前使用方式以离线背词为主。

本文适用于 `zectrix-s3-epaper-4.2` 固件。

## 1. 首次使用

1. 给设备上电或刷入固件后，等待电子墨水屏刷新完成。
2. 屏幕顶部左侧显示当前词库名称，例如 `Kaoyan 5047 Local`；右侧显示今日进度，例如 `0/20 OK 0`。
3. 卡片正面只显示单词和音标；按确认键后进入背面，显示释义、例句和翻译。
4. 每天默认目标是 20 个单词。完成目标后会进入 `Done` 总结页。

进度会保存在 ESP32-S3 的 NVS 中，断电后仍会保留。更换完全不同的词库时，建议清空 NVS 或整片擦除后重新刷机，避免旧词库进度按索引错配到新词库。

## 2. 按键位置与名称

固件使用三个导航按键：

| 文档名称 | 固件名称 | GPIO | 说明 |
| --- | --- | --- | --- |
| 上键 | Up | GPIO39 | 向前切换或减少设置值 |
| 下键 | Down | GPIO18 | 向后切换或增加设置值；硬件上也与电源相关引脚复用 |
| 确认键 | Confirm | GPIO0 | 翻面、确认已掌握、切换设置项 |

短按是普通点击；长按阈值约为 1 秒。

## 3. 背词界面按键功能

### 卡片正面

| 操作 | 功能 |
| --- | --- |
| 短按确认键 | 翻到背面，查看释义、例句和翻译 |
| 短按上键 | 切到上一张待复习卡片 |
| 短按下键 | 切到下一张待复习卡片 |
| 长按确认键 | 当前单词稍后再看，不计入今日已复习 |
| 先后长按上键和下键 | 进入设置页；两次长按需要在约 1.4 秒内完成 |

### 卡片背面

| 操作 | 功能 |
| --- | --- |
| 短按确认键 | 标记为“认识/已掌握本次”，计入今日已复习和正确数，然后进入下一张 |
| 短按上键 | 当前单词稍后再看，并切到上一张 |
| 短按下键 | 当前单词稍后再看，并切到下一张 |
| 长按确认键 | 当前单词稍后再看，不计入今日已复习 |

当前版本没有单独的“答错”按键。遇到不认识或想再看一遍的词，使用上键、下键或长按确认键把它延后，固件会把它插回后面的队列中。

### 完成页

今日目标完成后，屏幕显示 `Done` 和今日统计。短按确认键会重新构建队列；如果当天目标已经完成，队列通常仍为空。可以进入设置页调高每日目标后继续学习。

## 4. 设置页

进入方式：在背词界面先后长按上键和下键，两次长按间隔不超过约 1.4 秒。

设置页当前有两个项目：

| 项目 | 含义 |
| --- | --- |
| `Order` | 背词顺序，`Sequential` 为顺序，`Random` 为随机 |
| `Daily target` | 每日目标数量，范围 1 到 200，默认 20 |

设置页按键：

| 操作 | 功能 |
| --- | --- |
| 短按确认键 | 在 `Order` 和 `Daily target` 之间切换选中项 |
| 短按上键 | 修改当前选中项；每日目标减 1 |
| 短按下键 | 修改当前选中项；每日目标加 1 |
| 长按上键 | 如果选中每日目标，则减 10；如果选中顺序，则切换顺序/随机 |
| 长按下键 | 如果选中每日目标，则加 10；如果选中顺序，则切换顺序/随机 |
| 长按确认键 | 退出设置页并返回背词队列 |

进入设置页时，固件会尝试执行一次同步逻辑，并在页脚显示最近同步状态。当前离线版本通常只需要关注本地设置项。

## 5. 复习规则

固件会优先安排到期复习词，再补充新词，直到达到当天目标。

当前调度规则：

1. 新词第一次按确认标记认识后，进入学习状态，1 天后复习。
2. 学习状态连续答对到一定次数后，进入复习状态，初始间隔约 3 天。
3. 复习状态继续答对时，间隔按倍数增长，最长限制为 180 天。
4. 连续正确次数达到条件且间隔足够长后，单词会进入已掌握状态。
5. 每日统计会按 RTC 日期重置；如果 RTC 不可用，固件会使用运行时间做备用日期。

## 6. 更换单词库

当前词库编译进固件，文件是：

```text
main/vocabulary/kaoyan_full_deck.inc
```

换词库的基本流程是：准备 JSON 数据，运行生成脚本，更新词库元信息，重新编译并刷机。

### 6.1 准备 JSON 数据

推荐每个单词一条记录。最小可用示例：

```json
[
  {
    "word": "abandon",
    "usphone": "əˈbændən",
    "trans": [
      {
        "pos": "v",
        "tranCn": "放弃；抛弃"
      }
    ],
    "sentence": {
      "sentences": [
        {
          "sContent": "Do not abandon your plan too early.",
          "sCn": "不要太早放弃你的计划。"
        }
      ]
    }
  }
]
```

字段说明：

| 字段 | 必填 | 说明 |
| --- | --- | --- |
| `word` | 是 | 单词本体 |
| `usphone` / `ukphone` / `phone` | 否 | 音标，生成器会优先使用可用项 |
| `trans` | 建议 | 释义数组，常用字段为 `pos` 和 `tranCn` |
| `translations` | 可选 | 另一种释义格式，常用字段为 `type` 和 `translation` |
| `sentence.sentences` | 可选 | 例句数组，常用字段为 `sContent` 和 `sCn` |
| `sentences` | 可选 | 另一种例句格式，常用字段为 `sentence` 和 `translation` |

如果从 Excel 或 CSV 整理词库，建议先导出为 UTF-8 JSON，再按上面的字段命名。释义和例句太长会被生成器截断，以适配 400 x 300 电子墨水屏。

### 6.2 生成固件词库文件

在固件目录执行：

```bash
python tools/generate_kaoyanluan_deck.py \
  --source data/my_words.json \
  --output main/vocabulary/kaoyan_full_deck.inc
```

也可以一次合并多个 JSON：

```bash
python tools/generate_kaoyanluan_deck.py \
  --source data/book_1.json data/book_2.json data/book_3.json \
  --output main/vocabulary/kaoyan_full_deck.inc
```

生成器会按单词小写去重，并保留先出现的数据，再合并后续文件中的释义和例句。

### 6.3 修改词库名称

生成后建议同步修改 `main/vocabulary/deck_store.cc` 中的元信息：

```cpp
constexpr const char* kDeckId = "my-deck-v1";

DeckStore::DeckStore() {
    manifest_.deck_id = kDeckId;
    manifest_.name = "My Deck";
    manifest_.language = "en-zh";
    manifest_.version = 1;
    manifest_.word_count = Count();
}
```

建议规则：

- `deck_id`：只用小写字母、数字、短横线，例如 `cet4-v1`、`ielts-core-v1`。
- `manifest_.name`：会显示在屏幕顶部，尽量短，例如 `CET4 Core`。
- `manifest_.version`：每次大幅换库或重排顺序时加 1。

### 6.4 重新编译和刷机

普通开发编译：

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p COM5 flash monitor
```

把 `COM5` 换成你的实际串口。

打包发布固件：

```bash
./build.sh
```

打包结果在 `releases/`，合并烧录文件在 `build/merged-binary.bin`。

### 6.5 清空旧进度

如果只是给原词库追加少量单词，并且原有顺序不变，可以保留进度。

如果换成完全不同的词库、重排词库顺序或删除大量单词，建议清空旧进度。最简单方式是整片擦除后重新刷机：

```bash
idf.py -p COM5 erase-flash
idf.py -p COM5 flash monitor
```

如果只想擦除 NVS 分区，可以按当前分区表擦除 `0x9000` 起始、大小 `0x4000` 的区域：

```bash
esptool.py --chip esp32s3 -p COM5 erase_region 0x9000 0x4000
```

注意：擦除 NVS 会清除背词进度、每日统计、设置项以及已保存的 Wi-Fi 信息。

## 7. 常见问题

### 屏幕顶部的 `0/20 OK 0` 是什么意思？

`0/20` 表示今天已经确认掌握 0 个，目标是 20 个；`OK 0` 表示今天确认正确的数量。

### 为什么不认识的词没有马上算错？

当前固件把“不认识”设计为“稍后再看”：在背面短按上/下，或长按确认键，会把当前词延后，不计入今日已复习。这样适合电子墨水屏的轻量复习流程。

### 换词库后屏幕还显示旧词库名怎么办？

检查 `main/vocabulary/deck_store.cc` 的 `manifest_.name` 是否修改，并确认已经重新编译、重新刷入新固件。

### 换词库后复习进度不对怎么办？

旧进度是按词库索引保存的。换库或重排后，请擦除 NVS 或整片擦除后重新刷机。

### JSON 里中文显示乱码怎么办？

确认 JSON 文件是 UTF-8 编码；生成命令和源码文件都按 UTF-8 读取。

## 8. 硬件目标

- Board: `zectrix-s3-epaper-4.2`
- MCU: ESP32-S3
- Display: 400 x 300 e-paper
- Audio codec: ES8311
- RTC: PCF8563
- NFC: GT23SC6699
- Flash layout: 16 MB, `partitions/v2/16m.csv`

## 9. 编译环境

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

## 10. 打包发布

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

## 11. 目录结构

- `main/`: 固件应用、板级支持、显示、音频、单词复习逻辑和本地组件。
- `main/boards/zectrix-s3-epaper-4.2/`: 当前硬件的引脚、电源、RTC、NFC、墨水屏、充电检测和工厂测试支持。
- `main/vocabulary/`: 内置词库、复习状态和调度逻辑。
- `docs/`: 独立使用说明和参考资料。
- `partitions/`: ESP-IDF 分区表。
- `scripts/`: 发布打包和素材处理工具。
- `tools/`: 单词库生成工具。

## 12. License

MIT. See `LICENSE`.
