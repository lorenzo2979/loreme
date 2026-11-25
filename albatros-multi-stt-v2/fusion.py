"""ALBATROS Speech+ word-level fusion (experimental).

Combine corroborated words from independent ASR model families.
Never manufacture words or pretend a majority of related Whisper deployments
constitutes independent confirmation. Ambiguous positions are rejected.
"""
from dataclasses import dataclass
from difflib import SequenceMatcher
from collections import defaultdict
from agent import Result, normalize, conflicts

FAMILIES = {
    "speech_to_phrase": "stp",
    "whisper_local": "whisper",
    "groq": "whisper",
    "gemini": "gemini",
}


@dataclass(frozen=True)
class FusionVerdict:
    status: str
    text: str
    reason: str
    disagreements: tuple = ()
    sources: tuple = ()


def fuse(results):
    """Fuse word choices only where aligned and verified by two model families.

    Conservative first version: requires *equal token counts* for word-level
    combination, thus avoids guessing how to align insertions or omissions.
    No answer if any token position is uncertain or critical intents disagree.
    """
    valid = [r for r in results
             if isinstance(r, Result) and r.engine in FAMILIES and normalize(r.text)]
    # One result per engine so repetition cannot create a fake majority.
    by_engine = {r.engine: r for r in valid}
    valid = list(by_engine.values())
    if len({FAMILIES[r.engine] for r in valid}) < 2:
        return FusionVerdict("retry", "", "insufficient_independent_families")

    for i, a in enumerate(valid):
        for b in valid[i+1:]:
            if conflicts(a.text, b.text):
                return FusionVerdict("retry", "", "critical_disagreement")

    sequences = [normalize(r.text).split() for r in valid]
    lengths = {len(t) for t in sequences}
    if len(lengths) != 1:
        return FusionVerdict("retry", "", "word_alignment_ambiguous")

    chosen = []
    ambiguous = []
    evidence = set()
    for pos, _ in enumerate(sequences[0]):
        votes = defaultdict(set)
        origin = defaultdict(list)
        for result, words in zip(valid, sequences):
            word = words[pos]
            votes[word].add(FAMILIES[result.engine])
            origin[word].append(result.engine)
        candidates = [word for word, families in votes.items() if len(families) >= 2]
        if len(candidates) != 1:
            ambiguous.append(pos)
            continue
        selected = candidates[0]
        chosen.append(selected)
        evidence.update(origin[selected])

    if ambiguous:
        return FusionVerdict("retry", "", "uncertain_words",
                             tuple(ambiguous), tuple(sorted(evidence)))
    # Every word is an actual recognized token from a provider.
    # We cannot yet acoustically verify the resulting assembled sentence.
    return FusionVerdict("accepted", " ".join(chosen), "word_level_agreement",
                         (), tuple(sorted(evidence)))
