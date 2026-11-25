"""Diagnostic assistant for microphone calibration (offline, no device writes).

Only reports observations from PCM16/16kHz mono audio. Suggested changes
are hypotheses to verify by comparing recordings; does not alter hardware,
ESPHome, Home Assistant or recording pipelines.
"""
from array import array
from dataclasses import dataclass, asdict
import math
import sys


@dataclass(frozen=True)
class MicDiagnostic:
    status: str
    duration_s: float
    peak_dbfs: float | None
    rms_dbfs: float | None
    clipped_pct: float | None
    possible_cut_start: bool
    possible_cut_end: bool
    suggestions: tuple

    def to_dict(self):
        return asdict(self)


def _dbfs(value):
    return round(20 * math.log10(max(value / 32768.0, 1e-10)), 1)


def diagnose(audio):
    """PCM-only heuristic. No reliable SNR, echo or gain setting from one clip."""
    if (audio.rate, audio.width, audio.channels) != (16000, 2, 1):
        return MicDiagnostic("unsupported_format", 0, None, None, None, False, False,
                             ("Verificare il formato inviato allo STT: PCM16 mono a 16 kHz.",))
    if not isinstance(audio.pcm, bytes) or not audio.pcm or len(audio.pcm) % 2:
        return MicDiagnostic("invalid_audio", 0, None, None, None, False, False,
                             ("Verificare la registrazione PCM prima del riconoscimento.",))
    values = array("h")
    values.frombytes(audio.pcm)
    if sys.byteorder != "little":
        values.byteswap()
    duration = len(values) / 16000
    stride = max(1, len(values)//96000)
    sampled = values[::stride]
    peak = max(abs(v) for v in sampled)
    rms = math.sqrt(sum(int(x)*int(x) for x in sampled) / len(sampled))
    clipped = 100 * sum(abs(v) >= 32760 for v in sampled) / len(sampled)
    tips = []
    status = "ok"
    if duration < .4:
        status = "too_short"
        tips.append("Registrazione molto breve: controllare VAD e ritardo di attivazione.")
    elif duration > 30:
        status = "too_long"
        tips.append("Registrazione lunga: verificare la chiusura del VAD e i limiti di durata.")
    if peak < 100 or rms < 80:
        status = "near_silence"
        tips.append("Segnale quasi assente: verificare ingresso microfono, mute e instradamento audio.")
    elif clipped >= 1:
        status = "clipping"
        tips.append("Possibile saturazione: provare a ridurre il guadagno e ripetere la registrazione.")
    elif _dbfs(rms) < -32:
        status = "quiet"
        tips.append("Livello relativamente basso: verificare distanza, orientamento e guadagno.")
    # Boundary activity does not prove truncation: only suggests a follow-up.
    window = max(1, min(1600, len(values)//4))
    def energy(part):
        return math.sqrt(sum(int(x)*int(x) for x in part) / len(part))
    start = energy(values[:window])
    end = energy(values[-window:])
    boundary_threshold = max(500.0, rms * 1.8)
    start_risk = bool(duration >= .4 and start > boundary_threshold)
    end_risk = bool(duration >= .4 and end > boundary_threshold)
    if start_risk:
        tips.append("Audio già intenso all'inizio: possibile taglio della prima parola. Verificare pre-roll del VAD confrontando l'ascolto.")
    if end_risk:
        tips.append("Audio ancora intenso alla fine: possibile taglio dell'ultima parola. Verificare tempo di chiusura del VAD.")
    if status == "ok" and not tips:
        tips.append("Livello e formato plausibili. Rumore, eco e parole tagliate richiedono ascolto/registrazioni di riferimento.")
    return MicDiagnostic(status, round(duration, 3), _dbfs(peak),
                         _dbfs(rms), round(clipped, 3), start_risk, end_risk,
                         tuple(tips))
