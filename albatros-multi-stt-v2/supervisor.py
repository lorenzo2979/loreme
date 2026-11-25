"""ALBATROS Speech+ Supervisor — decision layer, no generative rewrites.

Reads one PCM stream, checks basic integrity, evaluates independent ASR outputs.
It does not transcribe audio itself, invoke external services, or control HA.
"""
from array import array
from dataclasses import dataclass
import math
import sys

from agent import Result, judge, conflicts, normalize


@dataclass(frozen=True)
class Verdict:
    status: str    # accepted / retry / rejected
    text: str      # never AI-generated
    reason: str    # machine-readable diagnostic
    engine: str = ""


def inspect_audio(audio):
    """Check common audio defects before transcription (not a full VAD)."""
    if (audio.rate, audio.width, audio.channels) != (16000, 2, 1):
        return "unsupported_format"
    pcm = audio.pcm
    if not isinstance(pcm, bytes) or len(pcm) % 2 or len(pcm) == 0:
        return "invalid_pcm"
    seconds = len(pcm) / 32000.0
    if seconds < 0.25:
        return "too_short"
    if seconds > 30:
        return "too_long"
    samples = array("h")
    samples.frombytes(pcm)
    if sys.byteorder != "little":
        samples.byteswap()
    # Sampled diagnostics for long utterances to keep the memory bounded.
    stride = max(1, len(samples) // 48000)
    subset = samples[::stride]
    peak = max(abs(v) for v in subset)
    if peak < 100:
        return "near_silence"
    clipped = sum(abs(v) >= 32760 for v in subset) / len(subset)
    if clipped > 0.05:
        return "clipping"
    rms = math.sqrt(sum(v*v for v in subset)/len(subset))
    if rms < 100:
        return "low_volume"
    return "ok"


def decide(results):
    """Conservative acceptance: require independent agreement; no invented words."""
    valid = [r for r in results if isinstance(r, Result) and normalize(r.text)]
    if not valid:
        return Verdict("retry", "", "no_transcript")
    if len({r.engine for r in valid}) < 2:
        return Verdict("retry", "", "insufficient_independent_engines")
    for i, first in enumerate(valid):
        for other in valid[i+1:]:
            if conflicts(first.text, other.text):
                return Verdict("retry", "", "critical_disagreement")
    selected = judge(valid)
    if selected is None:
        return Verdict("retry", "", "low_agreement")
    # Exact original words from a selected engine, not rewritten by a model.
    return Verdict("accepted", selected.text, "independent_agreement", selected.engine)
