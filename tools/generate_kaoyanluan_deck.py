#!/usr/bin/env python3
"""Generate a compact firmware include from local Kaoyan JSON decks."""

from __future__ import annotations

import argparse
import json
import re
from collections import OrderedDict
from dataclasses import dataclass, field
from pathlib import Path


WORD_KEY = "\u5355\u8bcd"
TRANS_KEY = "\u91ca\u4e49"

MAX_MEANINGS = 3
MAX_EXAMPLES = 2
MAX_MEANING_BYTES = 190
MAX_EXAMPLE_BYTES = 430
MAX_TRANSLATION_BYTES = 250
MAX_PHONETIC_BYTES = 60


@dataclass
class DeckEntry:
    word: str
    phonetic: str = ""
    meanings: list[tuple[str, str]] = field(default_factory=list)
    examples: list[tuple[str, str]] = field(default_factory=list)


def c_escape(value: str) -> str:
    return (
        value.replace("\\", "\\\\")
        .replace("\n", "\\n")
        .replace("\r", "")
        .replace('"', '\\"')
    )


def normalize_space(value: str) -> str:
    value = value.replace("\u200b", "")
    value = value.replace("\r", " ").replace("\n", " ").replace("\t", " ")
    value = value.replace("\u2018", "'").replace("\u2019", "'")
    value = value.replace("\u201c", '"').replace("\u201d", '"')
    value = re.sub(r"\s+", " ", value)
    return value.strip()


def clean_phonetic(value: str) -> str:
    value = normalize_space(value).strip("[]/ ")
    if not value or re.search(r"[\u4e00-\u9fff]", value):
        return ""
    return value


def normalize_pos(value: str) -> str:
    pos = normalize_space(value).lower().strip(".")
    aliases = {
        "a": "adj",
        "adjective": "adj",
        "adjectival": "adj",
        "ad": "adv",
        "adverb": "adv",
        "noun": "n",
        "verb": "v",
        "vt": "v",
        "vi": "v",
        "modal": "aux",
        "auxiliary": "aux",
        "preposition": "prep",
        "pronoun": "pron",
        "conjunction": "conj",
        "interjection": "int",
        "numeral": "num",
    }
    return aliases.get(pos, pos)


def split_translation(value: str) -> list[str]:
    normalized = normalize_space(value)
    parts = [part.strip(" ，,、") for part in re.split(r"[;；,，、]+", normalized)]
    return [part for part in parts if part]


def strip_note(value: str) -> str:
    value = re.sub(r"\s*\(=\s*.*?\)\s*", " ", value)
    value = re.sub(r"\s*\([^)]{1,28}\)\s*", " ", value)
    value = re.sub(r"^\s*\.\.\.\s*", "", value)
    return normalize_space(value)


def limit_utf8(value: str, max_bytes: int) -> str:
    encoded = value.encode("utf-8")
    if len(encoded) <= max_bytes:
        return value
    suffix = "..."
    budget = max(0, max_bytes - len(suffix))
    out = []
    used = 0
    for ch in value:
        size = len(ch.encode("utf-8"))
        if used + size > budget:
            break
        out.append(ch)
        used += size
    return "".join(out).rstrip() + suffix


def nested_content(item: dict) -> dict:
    return item.get("content", {}).get("word", {}).get("content", {})


def direct_content(item: dict) -> dict:
    return item if "word" in item else nested_content(item)


def extract_word(item: dict, content: dict) -> str:
    return normalize_space(str(item.get("word") or item.get("headWord") or content.get(WORD_KEY) or ""))


def build_phonetic(content: dict) -> str:
    for key in ("usphone", "ukphone", "phone"):
        value = clean_phonetic(str(content.get(key) or ""))
        if value:
            return limit_utf8(f"[{value}]", MAX_PHONETIC_BYTES)
    for key in ("us", "uk"):
        value = clean_phonetic(str(content.get(key) or ""))
        if value:
            return limit_utf8(f"[{value}]", MAX_PHONETIC_BYTES)
    return ""


def extract_meanings(content: dict) -> list[tuple[str, str]]:
    meanings: list[tuple[str, str]] = []
    seen: set[tuple[str, str]] = set()
    for item in content.get("trans") or []:
        pos = normalize_pos(str(item.get("pos") or ""))
        tran = normalize_space(str(item.get("tranCn") or ""))
        if not tran:
            continue
        for part in split_translation(tran):
            key = (pos, part)
            if key in seen:
                continue
            seen.add(key)
            meanings.append(key)
    for item in content.get("translations") or []:
        pos = normalize_pos(str(item.get("type") or ""))
        tran = normalize_space(str(item.get("translation") or ""))
        if not tran:
            continue
        for part in split_translation(tran):
            key = (pos, part)
            if key in seen:
                continue
            seen.add(key)
            meanings.append(key)
    return meanings


def build_meaning(meanings: list[tuple[str, str]]) -> str:
    groups: "OrderedDict[str, list[str]]" = OrderedDict()
    for pos, tran in meanings:
        values = groups.setdefault(pos, [])
        if tran not in values:
            values.append(tran)

    lines = []
    for pos, values in groups.items():
        if len(lines) >= MAX_MEANINGS:
            break
        text = "；".join(values)
        lines.append(f"{pos}. {text}" if pos else text)
    return limit_utf8("\n".join(lines), MAX_MEANING_BYTES)


def example_score(sentence: str, index: int) -> tuple[int, int]:
    score = 0
    if len(sentence) < 18:
        score += 18
    if not re.search(r"[.!?]$", sentence):
        score += 12
    if not sentence[:1].isupper():
        score += 6
    score += abs(len(sentence) - 62) // 5
    return score, index


def usable_sentence(sentence: str) -> bool:
    if not sentence:
        return False
    if sentence.startswith("{") or sentence.startswith("["):
        return False
    if "GLOSS" in sentence or "COLLOINEXA" in sentence:
        return False
    return bool(re.search(r"[A-Za-z]", sentence))


def extract_examples(content: dict) -> list[tuple[str, str]]:
    raw_examples = (content.get("sentence", {}).get("sentences") or []) + (content.get("sentences") or [])
    examples: list[tuple[str, str]] = []
    for index, item in enumerate(raw_examples):
        sentence = strip_note(str(item.get("sContent") or item.get("sentence") or ""))
        translation = normalize_space(str(item.get("sCn") or item.get("translation") or ""))
        if not usable_sentence(sentence) or not translation:
            continue
        examples.append((sentence, translation))
    return examples


def build_examples(examples: list[tuple[str, str]]) -> tuple[str, str]:
    candidates: list[tuple[tuple[int, int], str, str]] = []
    seen: set[tuple[str, str]] = set()
    for index, (sentence, translation) in enumerate(examples):
        key = (sentence.lower(), translation)
        if key in seen:
            continue
        seen.add(key)
        candidates.append((example_score(sentence, index), sentence, translation))
    candidates.sort(key=lambda item: item[0])
    selected = candidates[:MAX_EXAMPLES]
    interleaved = []
    for i, (_, sentence, translation) in enumerate(selected, 1):
        interleaved.append(f"{i}. {sentence}\n   {translation}")
    return (
        limit_utf8("\n".join(interleaved), MAX_EXAMPLE_BYTES),
        "",
    )


def merge_sources(sources: list[Path]) -> list[DeckEntry]:
    entries: "OrderedDict[str, DeckEntry]" = OrderedDict()
    for source in sources:
        rows = json.loads(source.read_text(encoding="utf-8"))
        for item in rows:
            content = direct_content(item)
            word = extract_word(item, content)
            if not word:
                continue
            key = word.lower()
            entry = entries.get(key)
            if entry is None:
                entry = DeckEntry(word=word)
                entries[key] = entry

            if not entry.phonetic:
                entry.phonetic = build_phonetic(content)

            existing_meanings = set(entry.meanings)
            for meaning in extract_meanings(content):
                if meaning not in existing_meanings:
                    entry.meanings.append(meaning)
                    existing_meanings.add(meaning)

            existing_examples = {(sentence.lower(), translation) for sentence, translation in entry.examples}
            for sentence, translation in extract_examples(content):
                example_key = (sentence.lower(), translation)
                if example_key not in existing_examples:
                    entry.examples.append((sentence, translation))
                    existing_examples.add(example_key)
    return list(entries.values())


def generate(sources: list[Path], output: Path) -> None:
    entries = merge_sources(sources)
    lines = [
        "// Generated from the local KaoYan_1/2/3 JSON decks.",
        "// Meanings are capped at 3 items; examples are capped at 2 sentence pairs.",
    ]
    for idx, entry in enumerate(entries, 1):
        meaning = build_meaning(entry.meanings)
        example, translation = build_examples(entry.examples)
        lines.append(
            f'    {{{idx}, kDeckId, "{c_escape(entry.word)}", "{c_escape(entry.phonetic)}", '
            f'"{c_escape(meaning)}", "{c_escape(example)}", "{c_escape(translation)}"}},'
        )
    output.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    repo_root = Path(__file__).resolve().parents[1]
    repo_data_sources = [
        repo_root / "data" / "KaoYan_1.json",
        repo_root / "data" / "KaoYan_2.json",
        repo_root / "data" / "KaoYan_3.json",
    ]
    parent_sources = [
        repo_root.parent / "KaoYan_1.json",
        repo_root.parent / "KaoYan_2.json",
        repo_root.parent / "KaoYan_3.json",
    ]
    default_sources = repo_data_sources if all(path.exists() for path in repo_data_sources) else parent_sources
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, nargs="+", default=default_sources)
    parser.add_argument("--output", type=Path, default=repo_root / "main/vocabulary/kaoyan_full_deck.inc")
    args = parser.parse_args()
    generate(args.source, args.output)


if __name__ == "__main__":
    main()
