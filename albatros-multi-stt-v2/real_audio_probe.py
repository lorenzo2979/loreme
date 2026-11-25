"""One public Italian speech sample, tested clean and noisy.

External sample is downloaded at runtime and never committed. For diagnostic
research only: not proof of reliability on diverse users or devices.
"""
import io
import json
import os
import urllib.request
import wave

import numpy as np
from faster_whisper import WhisperModel

from audio_calibration import calibrate
from italian_quality import analyze
from wyoming_server import Audio

SOURCE = "https://raw.githubusercontent.com/Nexdata-AI/347-Hours-Italian-Speech-Data-Collected-by-Mobile-Phone/main/"
NAME = "T0263G0002S0001"


def download(name):
    with urllib.request.urlopen(SOURCE + name, timeout=30) as stream:
        return stream.read(500000)


def read_wav(blob):
    with wave.open(io.BytesIO(blob), "rb") as f:
        if (f.getframerate(), f.getnchannels(), f.getsampwidth()) != (16000, 1, 2):
            raise ValueError("Formato inatteso")
        return Audio(f.readframes(f.getnframes()), 16000, 2, 1)


def noise_add(audio, snr_db=10):
    sample = np.frombuffer(audio.pcm, dtype="<i2").astype(np.float64)
    rng = np.random.default_rng(25112026)  # same noise each run
    noise = rng.normal(size=len(sample))
    noise -= noise.mean()
    noise /= max(np.sqrt(np.mean(noise*noise)), 1e-9)
    signal_rms = np.sqrt(np.mean(sample*sample))
    noisy = sample + noise*signal_rms/10**(snr_db/20)
    # No normalisation based on the ground-truth words.
    max_peak = np.max(np.abs(noisy))
    if max_peak > 32000:
        noisy = noisy * 32000/max_peak
    return Audio(np.rint(noisy).astype("<i2").tobytes(),16000,2,1)


def decode(model, audio):
    pcm = np.frombuffer(audio.pcm, dtype="<i2").astype(np.float32)/32768.0
    segments,_ = model.transcribe(pcm, language="it", beam_size=5,
                                  condition_on_previous_text=False,
                                  vad_filter=False)
    return " ".join(s.text.strip() for s in segments).strip()


def main():
    raw = download(NAME + ".wav")
    reference = download(NAME + ".txt").decode("utf-8-sig").strip()
    clean = read_wav(raw)
    noisy = noise_add(clean)
    model = WhisperModel(os.getenv("WHISPER_MODEL", "tiny"), device="cpu",
                         compute_type="int8")
    for label, audio in [("pulito",clean),("rumore_10dB",noisy)]:
        calibrated = calibrate(audio)
        print(f"--- {label} ---")
        print("Reference:", reference)
        print("Audio report before:",calibrated.before)
        print("Audio report after:",calibrated.after)
        for variant, entry in [("originale",audio),("calibrato",calibrated.processed)]:
            hypothesis = decode(model, entry)
            metrics = analyze(reference, hypothesis)
            print(json.dumps({"case":label,"variant":variant,
                              "hypothesis":hypothesis,
                              "wer":metrics["wer"],"cer":metrics["cer"],
                              "exact_match":metrics["exact_match"],
                              "critical_differences":metrics["critical_differences"]},
                             ensure_ascii=False))
    print("NOTE: single sample and tiny STT, not validated ALBATROS Multi-STT end to end")


if __name__ == "__main__":
    main()
