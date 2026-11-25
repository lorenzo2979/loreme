"""Experimental ALBATROS Speech+ adaptive spectral denoising V2.

Non-generative, offline, reversible. Works with PCM16 mono / 16 kHz.
The algorithm uses noise estimates from quiet frames, a conservative gain
floor and smoothed time-frequency masks. No lost speech can be recreated.
"""
import numpy as np
from wyoming_server import Audio


def denoise_v2(audio, strength=1.0, floor=0.22):
    if (audio.rate, audio.width, audio.channels) != (16000, 2, 1):
        raise ValueError("PCM16 mono 16 kHz richiesto")
    if not isinstance(audio.pcm, bytes) or len(audio.pcm) % 2 or not 8000 <= len(audio.pcm) <= 960000:
        raise ValueError("Audio non valido o durata fuori limite")
    if not 0.25 <= strength <= 2.0 or not 0.10 <= floor <= 0.75:
        raise ValueError("Parametri DSP fuori limite")
    samples = np.frombuffer(audio.pcm, dtype="<i2").astype(np.float64) / 32768.0
    if np.max(np.abs(samples)) < 1e-4:
        return audio  # do not amplify or transform silence
    nfft, hop = 512, 128
    pad = nfft
    x = np.pad(samples, (pad, pad))
    window = np.hanning(nfft+1)[:-1]
    positions = np.arange(0, len(x)-nfft+1, hop)
    frames = np.stack([x[p:p+nfft]*window for p in positions])
    spectrum = np.fft.rfft(frames, axis=1)
    magnitude = np.abs(spectrum)
    # Use lowest-energy frames for robust stationary-noise estimation. Not
    # equivalent to a real VAD; may include quiet phonemes, so be conservative.
    energy = np.mean(frames**2, axis=1)
    quiet = energy <= np.quantile(energy, 0.25)
    noise = np.median(magnitude[quiet], axis=0)
    # Frequency smoothing avoids narrow-bin musical noise.
    noise = (np.roll(noise, 1) + 2*noise + np.roll(noise, -1))/4
    noise[0] = (noise[0]+noise[1])/2
    noise[-1] = (noise[-1]+noise[-2])/2

    raw_gain = 1.0 - strength * (noise[None,:] / np.maximum(magnitude, 1e-8))**1.2
    mask = np.clip(raw_gain, floor, 1.0)
    # When a frame is clearly dominated by speech energy, preserve more of it.
    quiet_rms = np.sqrt(np.median(energy[quiet]) + 1e-12)
    speech_weight = np.clip(np.sqrt(energy+1e-12)/(5*quiet_rms+1e-9), 0, 1)
    mask = mask*(1 - 0.45*speech_weight[:,None]) + 0.45*speech_weight[:,None]
    # Smooth along both axes; no frame lookahead beyond immediate neighbours.
    if len(mask) >= 3:
        smooth = mask.copy()
        smooth[1:-1] = (mask[:-2] + 2*mask[1:-1] + mask[2:])/4
        mask = smooth
    mask[:,1:-1] = (mask[:,:-2] + 2*mask[:,1:-1] + mask[:,2:])/4

    output = np.zeros(len(x), dtype=np.float64)
    weights = np.zeros(len(x), dtype=np.float64)
    reconstructed = np.fft.irfft(spectrum * mask, n=nfft, axis=1) * window
    for p, block in zip(positions, reconstructed):
        output[p:p+nfft] += block
        weights[p:p+nfft] += window*window
    y = output / np.maximum(weights, 1e-8)
    y = np.clip(y[pad:pad+len(samples)], -0.9999, 0.9999)
    return Audio(np.rint(y*32768).astype("<i2").tobytes(),
                 audio.rate, audio.width, audio.channels)
