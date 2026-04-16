# Kaoyan Deck Source

The built-in Kaoyan deck is generated from local user-provided JSON files:

- `KaoYan_1.json`
- `KaoYan_2.json`
- `KaoYan_3.json`

The generated firmware include file is `kaoyan_full_deck.inc` and is committed so the firmware can build without the raw JSON files. To keep the firmware small and the e-paper page readable, the generator stores only the compact card fields:

1. Head word.
2. One available phonetic string.
3. Up to three Chinese meanings.
4. Up to two English example sentences, each immediately followed by its Chinese translation.

- Generator: `tools/generate_kaoyanluan_deck.py`
- Generated word count: 5047 after duplicate words are merged.

Example regeneration command from the firmware project root:

```bash
python tools/generate_kaoyanluan_deck.py \
  --source ../KaoYan_1.json ../KaoYan_2.json ../KaoYan_3.json \
  --output main/vocabulary/kaoyan_full_deck.inc
```
