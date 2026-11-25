"""Offline Italian STT quality metrics: reproducible WER/CER and critical words.

No audio collection, network requests or dependency on third-party packages.
Comparison is against user-provided reference transcripts, not AI guesses.
"""
from collections import Counter
import json
import re
import unicodedata

CRITICAL_PAIRS = (
    ("accendi", "spegni"), ("apri", "chiudi"),
    ("attiva", "disattiva"), ("alza", "abbassa"),
    ("aumenta", "diminuisci"), ("si", "no"),
)
NEGATIONS = {"non", "mai", "nessuno", "niente", "senza"}
NUMBERS = {"zero", "uno", "una", "due", "tre", "quattro", "cinque",
           "sei", "sette", "otto", "nove", "dieci", "venti", "trenta"}


def tokens(text):
    if not isinstance(text, str):
        raise ValueError("Trascrizione non valida")
    normalized = unicodedata.normalize("NFKC", text).casefold()
    normalized = normalized.replace("’", "'")
    # Keep apostrophes within words, not punctuation.
    return re.findall(r"[^\W_]+(?:'[^\W_]+)?", normalized, flags=re.UNICODE)


def edit_distance(reference, hypothesis):
    """Levenshtein substitutions/deletions/insertions with deterministic ties."""
    previous = [(i, 0, 0, i) for i in range(len(hypothesis)+1)]
    for r in reference:
        current = [(previous[0][0]+1, previous[0][1], previous[0][2]+1, previous[0][3])]
        for j, h in enumerate(hypothesis, 1):
            substitution = previous[j-1]
            match = (substitution[0], substitution[1], substitution[2], substitution[3]) if r == h else (
                substitution[0]+1, substitution[1]+1, substitution[2], substitution[3])
            deletion = (previous[j][0]+1, previous[j][1], previous[j][2]+1, previous[j][3])
            insertion = (current[j-1][0]+1, current[j-1][1], current[j-1][2], current[j-1][3]+1)
            current.append(min((match, deletion, insertion), key=lambda v: (v[0], v[1], v[2], v[3])))
        previous = current
    distance, substitutions, deletions, insertions = previous[-1]
    return {"errors": distance, "substitutions": substitutions,
            "deletions": deletions, "insertions": insertions}


def critical_differences(reference, hypothesis):
    ref, hyp = tokens(reference), tokens(hypothesis)
    a, b = Counter(ref), Counter(hyp)
    warnings = []
    for x, y in CRITICAL_PAIRS:
        if a[x] != b[x] or a[y] != b[y]:
            warnings.append("opposti:" + x + "/" + y)
    for group, name in ((NEGATIONS, "negazione"), (NUMBERS, "numero_scritto")):
        if {w: a[w] for w in group if a[w]} != {w: b[w] for w in group if b[w]}:
            warnings.append(name)
    digits = lambda words: [w for w in words if any(c.isdigit() for c in w)]
    if digits(ref) != digits(hyp):
        warnings.append("cifre")
    return sorted(set(warnings))


def analyze(reference, transcript):
    expected, actual = tokens(reference), tokens(transcript)
    word_counts = edit_distance(expected, actual)
    ref_chars = list(" ".join(expected))
    hyp_chars = list(" ".join(actual))
    char_counts = edit_distance(ref_chars, hyp_chars)
    return {"reference_words": len(expected),
            "word_errors": word_counts["errors"],
            "wer": round(word_counts["errors"]/max(len(expected), 1), 4),
            "cer": round(char_counts["errors"]/max(len(ref_chars), 1), 4),
            "word_edits": word_counts,
            "critical_differences": critical_differences(reference, transcript),
            "exact_match": expected == actual}


def score_dataset(records):
    """Records: [{reference, hypotheses:{engine:text}, category:optional}]."""
    if not isinstance(records, list) or not records:
        raise ValueError("Serve una lista non vuota di prove")
    accum = {}
    for item in records:
        if not isinstance(item, dict) or not isinstance(item.get("reference"), str):
            raise ValueError("Ogni prova richiede reference")
        for engine, transcript in item.get("hypotheses", {}).items():
            metrics = analyze(item["reference"], transcript)
            stats = accum.setdefault(engine, {"samples":0, "words":0, "errors":0,
                                               "exact":0, "critical_cases":0,
                                               "critical_labels":Counter()})
            stats["samples"] += 1
            stats["words"] += metrics["reference_words"]
            stats["errors"] += metrics["word_errors"]
            stats["exact"] += int(metrics["exact_match"])
            stats["critical_cases"] += bool(metrics["critical_differences"])
            stats["critical_labels"].update(metrics["critical_differences"])
    return {engine: {"samples": s["samples"], "words": s["words"],
                     "word_errors": s["errors"],
                     "wer": round(s["errors"]/max(s["words"],1),4),
                     "exact_sentences_pct": round(100*s["exact"]/s["samples"],2),
                     "critical_cases": s["critical_cases"],
                     "critical_labels": dict(sorted(s["critical_labels"].items()))}
            for engine,s in sorted(accum.items())}


def main():
    import argparse
    parser = argparse.ArgumentParser(description="Valuta le trascrizioni italiane offline")
    parser.add_argument("dataset", help="JSON di riferimenti umani e trascrizioni STT")
    args = parser.parse_args()
    with open(args.dataset, encoding="utf-8") as stream:
        data = json.load(stream)
    print(json.dumps(score_dataset(data), ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
