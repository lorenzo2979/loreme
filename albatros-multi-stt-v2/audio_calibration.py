"""ALBATROS Speech+ audio calibration — reversible, offline, PCM16 mono.

Processing never repairs words lost at capture. Always retain original audio
in memory and expose calibrated as a separate candidate; do not overwrite.
No device access, no cloud, no training, no file writes.
"""
from array import array
from dataclasses import dataclass
import math
import sys

@dataclass(frozen=True)
class AudioReport:
    duration_s: float
    rms_dbfs: float
    peak_dbfs: float
    dc_offset: float
    clipped_pct: float
    gain_db: float
    action: str


@dataclass(frozen=True)
class Calibrated:
    original: object
    processed: object
    before: AudioReport
    after: AudioReport


def _unpack(audio):
    if (audio.rate, audio.width, audio.channels) != (16000, 2, 1):
        raise ValueError("Sono accettati solo PCM16 mono a 16 kHz")
    if not isinstance(audio.pcm, bytes) or not audio.pcm or len(audio.pcm) % 2:
        raise ValueError("Campioni PCM non validi")
    if len(audio.pcm) > 16000*2*30:
        raise ValueError("Audio oltre 30 secondi")
    a = array("h")
    a.frombytes(audio.pcm)
    if sys.byteorder != "little":
        a.byteswap()
    return a


def _dbfs(value):
    return round(20 * math.log10(max(value / 32768, 1e-10)), 2)


def _report(samples, gain_db=0.0, action="original"):
    count = len(samples)
    mean = sum(samples)/count
    rms = math.sqrt(sum(x*x for x in samples)/count)
    peak = max(abs(x) for x in samples)
    clipped = 100 * sum(abs(x) >= 32760 for x in samples)/count
    return AudioReport(round(count/16000, 3), _dbfs(rms), _dbfs(peak),
                       round(mean/32768, 5), round(clipped, 3),
                       round(gain_db, 2), action)


def calibrate(audio, target_rms_dbfs=-24, max_gain_db=12):
    """Remove slow DC drift and scale only moderately, never aggressively.

    Peak limiter here is gain budgeting, not nonlinear compression; saturated
    recordings cannot be restored and are left untouched.
    """
    if not -35 <= target_rms_dbfs <= -18 or not 0 <= max_gain_db <= 12:
        raise ValueError("Parametri DSP fuori dai limiti sicuri")
    original = _unpack(audio)
    before = _report(original)
    if before.clipped_pct >= 1.0:
        return Calibrated(audio, audio, before, _report(original, action="clipped_unmodified"))
    if before.rms_dbfs < -50:
        return Calibrated(audio, audio, before, _report(original, action="near_silence_unmodified"))

    # First-order DC blocking filter at ~20 Hz, output stays in the PCM range.
    pole = math.exp(-2*math.pi*20/16000)
    prev_x = prev_y = 0.0
    filtered = []
    for value in original:
        y = value - prev_x + pole*prev_y
        filtered.append(y)
        prev_x, prev_y = value, y
    rms = math.sqrt(sum(x*x for x in filtered)/len(filtered))
    peak = max(abs(x) for x in filtered)
    if rms < 1 or peak < 1:
        return Calibrated(audio, audio, before, _report(original, action="near_silence_unmodified"))
    desired = 32768 * (10 ** (target_rms_dbfs/20)) / rms
    lower = 10 ** (-6/20)
    upper = 10 ** (max_gain_db/20)
    # Reserve ~1dB of headroom: no new clipping from amplification.
    scale = min(max(desired, lower), upper, (32767*10**(-1/20))/peak)
    # Positive gain allowed only if the input is well below the desired level.
    gain_db = 20*math.log10(scale)
    adjusted = array("h", (max(-32768, min(32767, round(x*scale))) for x in filtered))
    exported = array("h", adjusted)
    if sys.byteorder != "little":
        exported.byteswap()
    processed = type(audio)(exported.tobytes(), audio.rate, audio.width, audio.channels)
    after = _report(adjusted, gain_db=gain_db, action="dc_block_and_gain")
    return Calibrated(audio, processed, before, after)
