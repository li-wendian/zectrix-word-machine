#!/usr/bin/env python3
"""Generate the built-in Kaoyan vocabulary deck include."""

from __future__ import annotations

import argparse
import hashlib
import html
import json
import re
import tempfile
from pathlib import Path


SOURCE_KEY = "5530考研词汇词频排序表"
DEFAULT_TATOEBA_PATH = (
    Path(tempfile.gettempdir()) / "tatoeba_lxs" / "Tatoeba Chinese-English vocabulary.txt"
)


FUNCTION_EXAMPLES = {
    "the": ("The evidence supports the author's claim.", "证据支持作者的观点。"),
    "be": ("The answer may be different after new evidence appears.", "有了新证据后，答案可能会不同。"),
    "a": ("A careful note can save time during review.", "一条认真的笔记能在复习时节省时间。"),
    "to": ("She returned to the article for more clues.", "她回到文章中寻找更多线索。"),
    "of": ("The cause of the change was still unclear.", "这种变化的原因仍不清楚。"),
    "and": ("The chart is clear and useful.", "这张图表清楚而且有用。"),
    "in": ("The key idea appears in the first paragraph.", "关键观点出现在第一段。"),
    "have": ("Students have different ways to remember new words.", "学生有不同方法记忆新词。"),
    "that": ("The writer argues that habits shape success.", "作者认为习惯会影响成功。"),
    "it": ("It takes time to build confidence.", "建立信心需要时间。"),
    "for": ("This exercise is useful for quick review.", "这个练习对快速复习有用。"),
    "on": ("The report focuses on education.", "这份报告关注教育。"),
    "they": ("They compared the two answers carefully.", "他们仔细比较了两个答案。"),
    "you": ("You can mark the sentence before reading the options.", "你可以先标出句子再看选项。"),
    "with": ("She solved the problem with a simple example.", "她用一个简单例子解决了问题。"),
    "as": ("As the data grows, the pattern becomes clearer.", "随着数据增加，规律变得更清楚。"),
    "their": ("Their results were different from the first study.", "他们的结果不同于第一项研究。"),
    "by": ("The passage was organized by topic.", "这篇文章按主题组织。"),
    "not": ("The answer is not as simple as it looks.", "答案并不像看起来那么简单。"),
    "he": ("He checked the source before quoting it.", "他引用前先核对来源。"),
    "from": ("The idea came from a recent discussion.", "这个想法来自最近的一次讨论。"),
    "at": ("She looked at the title before reading.", "她阅读前先看标题。"),
    "will": ("This habit will make review easier.", "这个习惯会让复习更轻松。"),
    "more": ("The second paragraph gives more detail.", "第二段给出了更多细节。"),
    "do": ("Do the easy questions first.", "先做简单题。"),
    "we": ("We need evidence before drawing a conclusion.", "下结论前我们需要证据。"),
    "passage": ("The passage compares two views on work.", "这篇文章比较了两种关于工作的观点。"),
    "this": ("This example makes the rule easier to remember.", "这个例子让规则更容易记住。"),
    "or": ("Choose evidence or explanation according to the question.", "根据题目选择证据或解释。"),
    "can": ("A short review can improve long-term memory.", "简短复习能提高长期记忆。"),
    "i": ("I marked the keyword before answering.", "我答题前标出了关键词。"),
    "one": ("One example is enough to show the pattern.", "一个例子足以说明规律。"),
    "but": ("The text is short, but the idea is complex.", "文章很短，但观点很复杂。"),
    "question": ("The question asks for the writer's attitude.", "题目询问作者态度。"),
    "people": ("People learn faster when examples feel familiar.", "例子熟悉时，人们学得更快。"),
    "what": ("What matters is the reason behind the choice.", "重要的是选择背后的原因。"),
    "there": ("There is a clue in the final sentence.", "最后一句里有一个线索。"),
    "well": ("A well written note can prevent confusion.", "写得好的笔记可以避免混淆。"),
    "about": ("The article is about changes in reading habits.", "这篇文章讲的是阅读习惯的变化。"),
    "answer": ("The answer depends on the context.", "答案取决于语境。"),
}


NOUN_TEMPLATES = [
    ("The class discussed {word} after reading the paragraph.", "读完这段后，课堂讨论了“{meaning}”。"),
    ("A question about {word} appeared in reading practice.", "阅读练习里出现了一道关于“{meaning}”的题。"),
    ("The paragraph connects {word} with everyday life.", "这一段把“{meaning}”和日常生活联系起来。"),
    ("The report treats {word} as a key part of the problem.", "报告把“{meaning}”看作问题的关键部分。"),
    ("The note explains why {word} matters in this context.", "这条笔记解释了“{meaning}”为什么在语境中重要。"),
    ("The author introduces {word} before giving evidence.", "作者先引出“{meaning}”，再给出证据。"),
    ("The example of {word} made the idea easier to remember.", "这个关于“{meaning}”的例子让观点更容易记住。"),
    ("The final paragraph returns to {word}.", "最后一段又回到了“{meaning}”。"),
    ("We compared two views on {word}.", "我们比较了两种关于“{meaning}”的看法。"),
    ("The article uses {word} to frame the debate.", "文章用“{meaning}”来组织这场讨论。"),
    ("This passage turns {word} into a practical issue.", "这篇文章把“{meaning}”变成了一个实际问题。"),
    ("The teacher asked us to define {word} in our own words.", "老师让我们用自己的话定义“{meaning}”。"),
]


ADJ_TEMPLATES = [
    ("{article} {word} detail often changes the answer.", "一个“{meaning}”的细节常常会改变答案。"),
    ("The article gives {article_lc} {word} view of the issue.", "文章给出了一个“{meaning}”的视角。"),
    ("{article} {word} example is easier to remember.", "一个“{meaning}”的例子更容易记住。"),
    ("The final sentence makes the attitude {word}.", "最后一句让态度显得“{meaning}”。"),
    ("The teacher chose {article_lc} {word} case for review.", "老师选了一个“{meaning}”的案例来复习。"),
    ("{article} {word} reason can support the conclusion.", "一个“{meaning}”的理由能够支撑结论。"),
    ("The chart makes {article_lc} {word} difference visible.", "图表让一个“{meaning}”的差异变得明显。"),
    ("{article} {word} phrase may signal the author's opinion.", "一个“{meaning}”的短语可能提示作者观点。"),
]


ADV_TEMPLATES = [
    ("She read the paragraph {word} before answering.", "她在答题前“{meaning}”地读了这段话。"),
    ("The argument changes {word} in the last paragraph.", "论点在最后一段“{meaning}”地发生变化。"),
    ("He explained the clue {word} during review.", "复习时，他“{meaning}”地解释了这个线索。"),
    ("The writer {word} connects the example to the topic.", "作者“{meaning}”地把例子和主题联系起来。"),
    ("The answer becomes clearer when you read {word}.", "当你“{meaning}”地阅读时，答案会更清楚。"),
    ("The note reminds learners to compare ideas {word}.", "这条笔记提醒学习者“{meaning}”地比较观点。"),
]


STUDY_TEMPLATES = [
    ("I wrote \"{word}\" beside a clue so I could remember it later.", "我把“{word}”写在线索旁，方便之后记住它的意思：{meaning}。"),
    ("When \"{word}\" appears in a question, I check the surrounding sentence first.", "题目里出现“{word}”时，我会先看它周围的句子；这里可记作：{meaning}。"),
    ("The notebook keeps \"{word}\" next to a short personal hint.", "笔记把“{word}”和个人提示放在一起，帮助记住：{meaning}。"),
    ("I connect \"{word}\" with a real scene instead of memorizing it alone.", "我把“{word}”和真实场景联系起来，不只死记：{meaning}。"),
    ("A quick review of \"{word}\" before sleep makes it easier to recall.", "睡前快速复习“{word}”，能更容易想起它：{meaning}。"),
    ("The teacher asked us to underline \"{word}\" and explain it in our own words.", "老师让我们划出“{word}”，再用自己的话解释：{meaning}。"),
    ("I place \"{word}\" in a short reading note after each practice test.", "每次练习后，我把“{word}”放进阅读笔记里：{meaning}。"),
    ("Seeing \"{word}\" in a fresh sentence helps break rote memory.", "在新句子里见到“{word}”，能打破机械记忆：{meaning}。"),
    ("The review card for \"{word}\" starts with one clear scene.", "“{word}”的复习卡从一个清楚场景开始：{meaning}。"),
    ("I compare \"{word}\" with a similar word before choosing an answer.", "选择答案前，我会把“{word}”和近似词比较：{meaning}。"),
    ("The margin note turns \"{word}\" into a memory hook.", "页边笔记把“{word}”变成记忆钩子：{meaning}。"),
    ("A fresh example for \"{word}\" keeps the review from feeling mechanical.", "给“{word}”换一个新例子，复习就不那么机械：{meaning}。"),
    ("I say \"{word}\" once, then link it to the sentence around it.", "我先读一遍“{word}”，再把它和前后句连起来：{meaning}。"),
    ("The word \"{word}\" is easier to keep when it has a small story.", "给“{word}”配上小故事后，它更容易记住：{meaning}。"),
    ("I mark \"{word}\" with a star when it explains the author's point.", "当“{word}”能说明作者观点时，我会给它做星标：{meaning}。"),
    ("A personal example makes \"{word}\" less distant during review.", "个人化例子会让“{word}”在复习时不再陌生：{meaning}。"),
]


NOUN_SUFFIXES = (
    "tion", "sion", "ment", "ness", "ity", "ship", "hood", "ism", "ist",
    "ance", "ence", "age", "ery", "ure", "dom", "cy", "ty", "or", "er",
    "sis", "ogy", "ics", "phy",
)
ADJ_SUFFIXES = (
    "able", "ible", "al", "ial", "ical", "ic", "ive", "ous", "ful", "less",
    "ary", "ory", "ent", "ant", "ish",
)
NOT_ADVERB_LY = {"family", "supply", "reply", "apply", "imply", "comply", "ally", "belly", "fly", "july"}
ADJ_HINTS = (
    "的", "重要", "主要", "明显", "相同", "不同", "复杂", "简单", "可能",
    "必要", "有效", "严重", "普通", "特殊", "积极", "消极", "传统", "现代",
)

TOKEN_PATTERN = re.compile(r"[A-Za-z]+(?:'[A-Za-z]+)?")
FONT_PATTERN = re.compile(r"<font[^>]*>(.*?)</font>", re.IGNORECASE | re.DOTALL)
LI_PATTERN = re.compile(r"<li>(.*?)</li>", re.IGNORECASE | re.DOTALL)
TAG_PATTERN = re.compile(r"<.*?>")


def c_escape(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')


def strip_html(value: str) -> str:
    return html.unescape(TAG_PATTERN.sub("", value)).strip()


def make_simplifier():
    try:
        from opencc import OpenCC

        return OpenCC("t2s").convert
    except Exception:
        return lambda value: value


def clean_chinese(value: str, simplify) -> str:
    value = simplify(strip_html(value))
    value = value.replace(" ", "")
    value = re.sub(r"\s+", "", value)
    return value


def clean_english(value: str) -> str:
    value = strip_html(value)
    value = re.sub(r"\s+", " ", value)
    return value


def sentence_score(sentence: str) -> int:
    score = abs(len(sentence) - 55)
    score += sentence.count(",") * 5
    score += sentence.count(";") * 8
    score += sentence.count("?") * 2
    score += sentence.count("!") * 2
    if not sentence[:1].isupper():
        score += 10
    if "Tom" in sentence or "Mary" in sentence:
        score += 4
    return score


def load_tatoeba_examples(path: Path, words: set[str]) -> dict[str, list[tuple[int, str, str]]]:
    if not path.exists():
        return {}

    simplify = make_simplifier()
    examples: dict[str, list[tuple[int, str, str]]] = {}
    for line in path.open(encoding="utf-8", errors="replace"):
        parts = line.rstrip("\n").split("\t")
        if len(parts) < 2:
            continue
        html_text = parts[1]
        if "Google-translate" in html_text:
            continue
        match = FONT_PATTERN.search(html_text)
        if not match:
            continue
        translation = clean_chinese(match.group(1), simplify)
        if not (4 <= len(translation) <= 80):
            continue
        for li_text in LI_PATTERN.findall(html_text):
            if "<i>" in li_text or "Transl. Note" in li_text:
                continue
            sentence = clean_english(li_text)
            if not (12 <= len(sentence) <= 120):
                continue
            if any(ch.isdigit() for ch in sentence):
                continue
            tokens = {token.lower() for token in TOKEN_PATTERN.findall(sentence)}
            for token in tokens & words:
                examples.setdefault(token, []).append((sentence_score(sentence), sentence, translation))

    for candidates in examples.values():
        candidates.sort(key=lambda item: (item[0], item[1], item[2]))
    return examples


def short_meaning(meaning: str) -> str:
    parts = [part.strip() for part in re.split(r"[、，,；;。/（）()]+", meaning) if part.strip()]
    if not parts:
        return meaning.strip()
    result = parts[0]
    return result[:14]


def pick_index(word: str, rank: int, count: int) -> int:
    digest = hashlib.blake2s(f"{rank}:{word}".encode("utf-8"), digest_size=4).digest()
    return int.from_bytes(digest, "little") % count


def article(word: str, *, lower: bool = False) -> str:
    value = "an" if word[:1].lower() in "aeiou" else "a"
    return value if lower else value.capitalize()


def classify(word: str, meaning: str) -> str:
    lower = word.lower()
    if lower in FUNCTION_EXAMPLES:
        return "fixed"
    if lower.endswith("ly") and lower not in NOT_ADVERB_LY:
        return "adv"
    if lower.endswith(NOUN_SUFFIXES):
        return "noun"
    if lower.endswith(ADJ_SUFFIXES):
        return "adj"
    if any(hint in meaning for hint in ADJ_HINTS):
        return "adj"
    return "study"


def choose_source_example(
    word: str,
    examples: dict[str, list[tuple[int, str, str]]],
    used_sentences: set[str],
) -> tuple[str, str] | None:
    candidates = examples.get(word.lower())
    if not candidates:
        return None
    for _, sentence, translation in candidates:
        if sentence not in used_sentences:
            used_sentences.add(sentence)
            return sentence, translation
    _, sentence, translation = candidates[0]
    return sentence, translation


def build_template_example(word: str, rank: int, meaning: str) -> tuple[str, str]:
    lower = word.lower()
    kind = classify(word, meaning)
    hint = short_meaning(meaning)
    values = {"word": word, "meaning": hint, "article": article(word), "article_lc": article(word, lower=True)}
    if kind == "fixed":
        return FUNCTION_EXAMPLES[lower]
    if kind == "noun":
        template = NOUN_TEMPLATES[pick_index(word, rank, len(NOUN_TEMPLATES))]
    elif kind == "adj":
        template = ADJ_TEMPLATES[pick_index(word, rank, len(ADJ_TEMPLATES))]
    elif kind == "adv":
        template = ADV_TEMPLATES[pick_index(word, rank, len(ADV_TEMPLATES))]
    else:
        template = STUDY_TEMPLATES[pick_index(word, rank, len(STUDY_TEMPLATES))]
        values["meaning"] = meaning
    return template[0].format(**values), template[1].format(**values)


def generate(source: Path, output: Path, examples_source: Path | None) -> None:
    data = json.loads(source.read_text(encoding="utf-8"))
    rows = data[SOURCE_KEY]
    words = {str(row["单词"]).lower() for row in rows}
    examples = load_tatoeba_examples(examples_source, words) if examples_source else {}
    used_sentences: set[str] = set()
    lines = [
        "// Generated from exam-data/NETEMVocabulary release v7.1.",
        "// Source: https://github.com/exam-data/NETEMVocabulary/blob/master/netem_full_list.json",
        "// Data license: CC BY-NC-SA 4.0. See KAOYAN_DECK_SOURCE.md.",
        "// Examples prefer Tatoeba Chinese-English Vocabulary, then fall back to local varied templates.",
    ]
    for row in rows:
        rank = int(row["序号"])
        freq = int(row["词频"])
        word = str(row["单词"])
        meaning = str(row["释义"] or "")
        source_example = choose_source_example(word, examples, used_sentences)
        example, translation = source_example if source_example else build_template_example(word, rank, meaning)
        phonetic = f"rank {rank}  freq {freq}"
        lines.append(
            f'    {{{rank}, kDeckId, "{c_escape(word)}", "{c_escape(phonetic)}", '
            f'"{c_escape(meaning)}", "{c_escape(example)}", "{c_escape(translation)}"}},'
        )
    output.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--source",
        type=Path,
        default=Path(tempfile.gettempdir()) / "netem_full_list.json",
        help="Path to upstream netem_full_list.json.",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("main/vocabulary/kaoyan_full_deck.inc"),
        help="Path to generated C++ include file.",
    )
    parser.add_argument(
        "--examples-source",
        type=Path,
        default=DEFAULT_TATOEBA_PATH if DEFAULT_TATOEBA_PATH.exists() else None,
        help="Optional Tatoeba Chinese-English vocabulary text file.",
    )
    args = parser.parse_args()
    generate(args.source, args.output, args.examples_source)


if __name__ == "__main__":
    main()
