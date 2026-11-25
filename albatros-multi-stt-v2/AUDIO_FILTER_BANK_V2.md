# ALBATROS Audio Agent V2 — filter bank experiment
Eight candidate audio processors and Silero diagnostics: V1, GTCRN, DPDFNet,
DeepFilterNet, RNNoise, WebRTC NS/AGC, FFmpeg and digital AGC.
All process identical source independently; no blind cascades. Optional dependencies
are installed in separate CI jobs; a missing backend fails its job, not silently
substituted. This is not yet a production selector or a deployed service.
The original stays intact; length and format checks reject altered-length output.
Silero reports regions only and never removes samples.
AEC requires synchronized playback PCM, unavailable in current Wyoming input.
ESP-SR hardware processing, capture timing, physical gain and multi-mic routing
require reading the known-good Alexa firmware/config and hardware testing; unchanged.
The benchmark repeats the original ten Italian recordings with five conditions,
using one STT. These are exploratory repeated samples, not independent validation.
Next: evaluate real room recordings and implement conservative Multi-STT candidate
selection without reference text. Check upstream models and licenses before release.
No firmware or production changes. External sample licensing per existing benchmark.
