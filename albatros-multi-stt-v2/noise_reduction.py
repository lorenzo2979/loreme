"""Experimental spectral noise reduction, offline, always preserves source.

Numpy implementation. Only operate on PCM16 mono 16 kHz, 0.25–30 s.
Noise spectrum estimated from 20th percentile across overlapping frames.
It is NOT a solution to clipping, echo, or missing beginning/end words.
"""
import numpy as np
from wyoming_server import Audio


def denoise(audio, strength=1.3, floor=0.12):
    if (audio.rate,audio.width,audio.channels)!=(16000,2,1):
        raise ValueError("PCM16 16kHz mono richiesto")
    if not isinstance(audio.pcm, bytes) or len(audio.pcm)%2 or not 8000<=len(audio.pcm)<=960000:
        raise ValueError("Audio non valido o durata non consentita")
    if not 0.5<=strength<=3.0 or not 0.05<=floor<=0.7:
        raise ValueError("Parametri di riduzione rumore non validi")
    original = np.frombuffer(audio.pcm, dtype="<i2").astype(np.float64)/32768
    n_fft, hop = 512, 128
    length = len(original)
    padded = np.pad(original, (n_fft, n_fft), mode="constant")
    # A nonzero synthesis window avoids division by zero at boundaries.
    win = np.hanning(n_fft+1)[:-1]
    positions = np.arange(0,len(padded)-n_fft+1,hop)
    matrix = np.array([np.fft.rfft(padded[p:p+n_fft]*win) for p in positions])
    magnitudes = np.abs(matrix)
    # Quieter frames approximate stationary noise; this is an estimate.
    noise = np.quantile(magnitudes, 0.20, axis=0)
    mask = np.maximum(floor, 1.0 - strength*noise[None,:]/
                      np.maximum(magnitudes,1e-10))
    # Mild time smoothing makes fewer musical artifacts.
    if len(mask)>2:
        mask[1:-1] = (mask[:-2] + 2*mask[1:-1] + mask[2:])/4
    result = np.zeros(len(padded))
    weights = np.zeros(len(padded))
    for i,p in enumerate(positions):
        frame = np.fft.irfft(matrix[i]*mask[i], n=n_fft)*win
        result[p:p+n_fft] += frame
        weights[p:p+n_fft] += win*win
    restored = (result/np.maximum(weights, 1e-8))[n_fft:n_fft+length]
    restored = np.clip(restored,-0.9999,0.9999)
    pcm = np.rint(restored*32768).astype("<i2").tobytes()
    return Audio(pcm, audio.rate, audio.width, audio.channels)
