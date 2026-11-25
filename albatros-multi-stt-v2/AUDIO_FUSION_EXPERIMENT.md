# V2 second revision — WebRTC and observation adding
Run 45705173493: eight jobs succeeded; WebRTC plugin unavailable on Ubuntu 24.04.
Use OHF-Voice webrtc-noise-gain 1.2.3 (same API as Wyoming Satellite) instead.
AEC is not implemented because playback reference is not supplied.
DeepFilterNet previous quiet-condition WER was 355 errors on 66 reference words.
Try capped pre-gain (up to 18 dB), reverse scale, and 12 dB attenuation limit.
The gain and limit are experimental, not evidence of repair.
For every filter compare original, 100% filtered, and 25/50/75% filtered blends.
Bound alignment search to 50ms, reject low correlation; rejected mixes use original
and count the rejection. Missing aligned edges use original samples.
All configurations are measured against the same baseline within each job.
Cross-job baseline can vary. Original data is ten Italian recordings, five
simulated conditions, Whisper Base only. No test-set-driven deployment or
automatic supervisor selection; independent held-out real audio remains necessary.
No Alexa firmware or Home Assistant changes.
