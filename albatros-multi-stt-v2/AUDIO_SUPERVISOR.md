# Experimental intelligent audio supervisor
This is the initial adaptive policy, not a trained language model. It profiles
the captured mono PCM (quiet / tonal / broadband / uncertain, heuristic),
routes a bounded subset of filters and blends, protects speech frames and peaks,
and compares local Whisper hypotheses and confidence. The source is immutable.
Different filter outputs from one Whisper model are correlated evidence, NOT
independent STT engines. Only an alternative corroborated by two distinct
nonidentical filtered waveforms may replace the original; conflicting critical
tokens or uncertain alternatives preserve original. No text is fabricated.
ENABLE_AUDIO_SUPERVISOR=true and ENABLE_WHISPER=true opt in to the experimental
local selector before the existing Multi-STT judge; neither default is changed.
The container still needs models/dependencies; do not deploy until validated.
No microphone hardware control, AEC, or actual changes to Alexa/Home Assistant.
Verified feedback accumulates aggregate outcome counts only; requires an
explicit verified reference. Minimum 12 observations before bounded re-ranking.
The benchmark learns on the old 10 recordings (50 simulated conditions), freezes
feedback, then attempts 24 distinct FLEURS Italian test recordings, each clean
and mixed with recorded kitchen/public-square DEMAND noise (72 conditions).
This is not actual far-field paired recording: mixing real noise is synthetic
mixture. FLEURS download and DEMAND archive availability require CI validation.
Sources: Google FLEURS CC BY 4.0, DEMAND Thiemann/Ito/Vincent (Zenodo 1227121).
Do not redistribute the downloaded source audio; verify current licenses before
commercial use. Artifacts include metrics and aggregate feedback, no transcripts.
No learning from unverified hypotheses, and no test references used to choose.
Next: separate real satellite recordings, latency and Multi-STT families testing,
explicit human feedback persistence and hardware control after permission/testing.

## Revision 1.1
First development run preserved original in 50/50 conditions (50 word errors);
held-out parquet download exceeded 750 MB. Use dataset-viewer rows of it_it/test,
fetch only needed audio assets with bounded individual downloads. Dataset viewer
availability and audio assets are not yet validated. Keep DEMAND noise mixtures.
Single-filter experimental alternative requires a small edit, no critical-token
conflict, strong recognizer margin (.35) on tonal/broadband input or a .15 margin
with verified profile/variant feedback. Multi-variant margin relaxed slightly.
Feedback threshold 10 observations (not 10 independent speakers). Report
oracle_diagnostic separately as reference-based theoretical ceiling, never as
runtime output. New tests for weak and supported single alternatives. No deployment.
