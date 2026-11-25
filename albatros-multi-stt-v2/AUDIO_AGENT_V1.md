# Audio Agent V1 — experimental
Enable using ENABLE_AUDIO_AGENT=true when numpy is installed. Default off.
PCM16 mono 16 kHz; original retained in RAM during preprocessing. Conservative
noise reduction V2 and bounded digital gain, fallback on bad quality. No audio
files saved, no hardware gain, capture timing or VAD configuration changes.
All enabled STT engines receive the same selected audio. This initial mode
does not yet perform original-vs-filtered STT comparisons or WER-based selection.
Noise heuristics and synthetic tests do not establish real speech accuracy.
Dockerfile remains an unvalidated architecture draft. Do not deploy yet.
Next: multi-clip Italian clean/noisy WER/CER comparison and device-side controls.
