"""Autocalibration controller for ALBATROS Speech+ (simulation only).

Suggests bounded parameter changes after repeated diagnostic evidence.
Applies only to an isolated configuration in memory; accepts/rolls back
a trial against measured quality. No remote device, ESPHome or HA writes.
"""
from dataclasses import dataclass, replace

@dataclass(frozen=True)
class MicSettings:
    gain_db: float = 0.0
    pre_roll_ms: int = 400
    end_silence_ms: int = 700

@dataclass(frozen=True)
class Adjustment:
    name: str
    old_value: float | int
    new_value: float | int
    reason: str

LIMITS = {
    "gain_db": (-12.0, 12.0),
    "pre_roll_ms": (200, 1200),
    "end_silence_ms": (400, 2000),
}

class AutoCalibrator:
    """Requires at least three consistent clips and a measured A/B comparison."""
    def __init__(self, settings=None):
        self.settings = settings or MicSettings()
        self.pending = None

    def propose(self, diagnostics):
        if self.pending is not None:
            raise RuntimeError("Valutare la modifica precedente prima di procedere")
        samples = list(diagnostics)
        if len(samples) < 3:
            return None
        # No tuning on malformed/silent/unusable recordings.
        if any(d.status in ("invalid_audio","unsupported_format","near_silence",
                            "too_short","too_long") for d in samples):
            return None
        clipped = sum(d.status == "clipping" for d in samples)
        quiet = sum(d.status == "quiet" for d in samples)
        start = sum(bool(d.possible_cut_start) for d in samples)
        end = sum(bool(d.possible_cut_end) for d in samples)
        threshold = (2*len(samples)+2)//3  # ceil(2n/3)
        if clipped >= threshold:
            choice = ("gain_db", -2.0, "Saturazione ripetuta")
        elif quiet >= threshold and clipped == 0:
            choice = ("gain_db", 2.0, "Livello basso ripetuto")
        elif start >= threshold:
            choice = ("pre_roll_ms", 100, "Possibile perdita ripetuta dell'inizio frase")
        elif end >= threshold:
            choice = ("end_silence_ms", 150, "Possibile perdita ripetuta della fine frase")
        else:
            return None
        key, step, reason = choice
        current = getattr(self.settings, key)
        lower, upper = LIMITS[key]
        new = max(lower, min(upper, current + step))
        if new == current:
            return None
        self.pending = Adjustment(key, current, new, reason)
        return self.pending

    def candidate(self):
        """Temporary isolated settings, never writes to a physical device."""
        if self.pending is None:
            return self.settings
        return replace(self.settings, **{self.pending.name:self.pending.new_value})

    def evaluate(self, baseline_errors, candidate_errors, baseline_critical=0,
                 candidate_critical=0):
        """Commit trial only on demonstrated improvement with no critical regression.

        All scores must be nonnegative numeric evaluation results from a
        consistent reference dataset; caller controls the A/B test.
        """
        if self.pending is None:
            raise RuntimeError("Nessun esperimento in corso")
        scores = [baseline_errors,candidate_errors,baseline_critical,candidate_critical]
        if any(type(x) not in (int,float) or x < 0 or x != x for x in scores):
            raise ValueError("Punteggi non validi")
        improved = candidate_errors < baseline_errors and candidate_critical <= baseline_critical
        if improved:
            self.settings = self.candidate()
        self.pending = None
        return "accepted" if improved else "rolled_back"
