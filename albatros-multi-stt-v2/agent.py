"""Conservative consensus judge for experimental ALBATROS Multi-STT.
This module does NOT perform STT itself or connect to Home Assistant.
"""
from dataclasses import dataclass
from difflib import SequenceMatcher
import re
import unicodedata

OPPOSITES = (
    ({"accendi", "attiva", "apri"}, {"spegni", "disattiva", "chiudi"}),
    ({"alza", "aumenta"}, {"abbassa", "diminuisci"}),
)
ORDER = ("speech_to_phrase", "whisper_local", "groq", "gemini")

@dataclass(frozen=True)
class Result:
    engine: str
    text: str

def normalize(text: str) -> str:
    text = unicodedata.normalize("NFKC", text).casefold().strip()
    return " ".join(re.findall(r"[^\W_]+", text, flags=re.UNICODE))

def conflicts(a: str, b: str) -> bool:
    left, right = set(normalize(a).split()), set(normalize(b).split())
    for x, y in OPPOSITES:
        if (left & x and right & y) or (left & y and right & x):
            return True
    nums_a = re.findall(r"\d+", normalize(a))
    nums_b = re.findall(r"\d+", normalize(b))
    return bool(nums_a and nums_b and nums_a != nums_b)

def judge(results: list[Result], threshold: float = .86) -> Result | None:
    """Return a transcript only with agreement from at least two engines."""
    valid = [r for r in results if r.engine in ORDER and normalize(r.text)]
    if len(valid) < 2:
        return None
    candidates = []
    for r in valid:
        score = sum(1 for other in valid if other.engine != r.engine
                    and not conflicts(r.text, other.text)
                    and SequenceMatcher(None, normalize(r.text),
                                        normalize(other.text)).ratio() >= threshold)
        if score:
            candidates.append((score, -ORDER.index(r.engine), r))
    if not candidates:
        return None
    candidates.sort(key=lambda t: (t[0], t[1]), reverse=True)
    best = candidates[0][2]
    # Conservative guard: any explicitly contradictory engine abstains.
    if any(conflicts(best.text, other.text) for other in valid if other != best):
        return None
    return best
