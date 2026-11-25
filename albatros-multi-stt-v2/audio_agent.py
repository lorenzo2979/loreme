"""ALBATROS Audio Agent V1: conservative offline preprocessing.
No device writes, generative reconstruction, recording persistence or network.
Noise-floor estimates are heuristics, not true speech SNR or quality scores.
"""
from dataclasses import dataclass
import math
from mic_diagnostics import diagnose
from audio_calibration import calibrate

@dataclass(frozen=True)
class AudioPreparation:
    original: object
    selected: object
    reason: str
    noise_floor_dbfs: float | None = None
    dynamics_db: float | None = None

def prepare(audio):
    """Keep length and format unchanged; fall back to source on DSP failure."""
    diagnostic = diagnose(audio)
    if diagnostic.status not in ("ok", "quiet"):
        return AudioPreparation(audio, audio, diagnostic.status)
    try:
        import numpy as np
        from noise_reduction_v2 import denoise_v2
        samples = np.frombuffer(audio.pcm, dtype="<i2").astype(np.float64) / 32768
        n = 320  # 20 ms
        frames = samples[:len(samples)//n*n].reshape(-1,n)
        rms = np.sqrt(np.mean(frames**2, axis=1))
        low, high = np.quantile(rms, [0.20, 0.85])
        def db(v):
            return 20*math.log10(max(float(v),1e-10))
        floor_db, dynamics = db(low), db(high)-db(low)
        # Only try denoising if there are relatively quiet and loud frames
        # plus substantial background energy. Not a true speech detector.
        candidate, reason = audio, "original"
        if -45 <= floor_db <= -18 and 8 <= dynamics <= 30:
            filtered = denoise_v2(audio, strength=0.7, floor=0.45)
            y = np.frombuffer(filtered.pcm,dtype="<i2").astype(np.float64)/32768
            yrms = np.sqrt(np.mean(y[:len(frames)*n].reshape(-1,n)**2, axis=1))
            quiet = rms <= low
            loud = rms >= high
            quiet_delta = db(np.median(yrms[quiet]))-db(np.median(rms[quiet]))
            loud_delta = db(np.median(yrms[loud]))-db(np.median(rms[loud]))
            # Require some measured noise-floor reduction and limited
            # attenuation of loud frames. These do NOT prove preserved words.
            if quiet_delta <= -0.8 and -4 <= loud_delta <= 1:
                candidate, reason = filtered, "denoised"
        if diagnostic.status == "quiet" or abs(float(samples.mean())) > 0.015:
            gain = calibrate(candidate, target_rms_dbfs=-24,max_gain_db=6)
            if gain.after.action == "dc_block_and_gain":
                candidate, reason = gain.processed, reason + "+calibrated"
        check = diagnose(candidate)
        if (len(candidate.pcm)!=len(audio.pcm)
            or (candidate.rate,candidate.width,candidate.channels)!=(audio.rate,audio.width,audio.channels)
            or check.status not in ("ok","quiet")
            or (check.clipped_pct or 0) > (diagnostic.clipped_pct or 0)):
            return AudioPreparation(audio,audio,"quality_fallback",floor_db,dynamics)
        return AudioPreparation(audio,candidate,reason,floor_db,dynamics)
    except (ImportError, ValueError, RuntimeError, ArithmeticError):
        return AudioPreparation(audio,audio,"dsp_fallback")
