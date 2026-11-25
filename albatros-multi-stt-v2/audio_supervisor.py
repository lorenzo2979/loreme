"""Experimental local audio supervisor; original is always the fallback.

No reference transcript is provided to select(). One local ASR model provides
heuristic evidence, not independent ASR confirmation. Actual Multi-STT still
requires its own independent-family agreement. No physical microphone writes.
"""
from dataclasses import dataclass
from collections import Counter
import math
import numpy as np
from audio_filter_bank import samples, output, apply_filter, speech_segments
from audio_fusion import aligned
from mic_diagnostics import diagnose
from italian_quality import tokens, critical_differences, analyze

# Starting hypotheses from V2, not learned truths. Each profile can try any
# available engine through explicit exploration/verified feedback.
POLICY={
 "quiet":(("agc",1.),("webrtc",.25),("deepfilternet",.5),("albatros_v1",.5)),
 "tonal":(("ffmpeg",.5),("rnnoise",.5),("webrtc",.25),("dpdfnet",1.)),
 "broadband":(("dpdfnet",1.),("webrtc",.5),("deepfilternet",1.),("gtcrn",.75)),
 "uncertain":(("webrtc",.25),("albatros_v1",.5),("dpdfnet",1.),("gtcrn",.75)),
}
EXPLORATION=(("agc",1.),("ffmpeg",.5),("rnnoise",.5),("gtcrn",.75),("deepfilternet",1.),("dpdfnet",1.),("webrtc",.5),("albatros_v1",.5))

@dataclass(frozen=True)
class Profile:
    label: str
    rms_dbfs: float
    noise_floor_dbfs: float
    dynamics_db: float
    spectral_flatness: float
    tonal_fraction: float

@dataclass(frozen=True)
class Hearing:
    text: str
    score: float
    usable: bool

@dataclass(frozen=True)
class Option:
    key: str
    audio: object
    hearing: Hearing

@dataclass(frozen=True)
class Decision:
    audio: object
    selected: str
    reason: str
    profile: Profile
    original: object
    options: tuple
    errors: dict

def db(x):
    return float(20*np.log10(max(float(x),1e-10)))

def profile(audio):
    x=samples(audio)
    n=(len(x)//320)*320
    frames=x[:n].reshape(-1,320)
    rms=np.sqrt(np.mean(frames*frames,axis=1)+1e-12)
    floor=float(np.quantile(rms,.2))
    dyn=db(np.quantile(rms,.9))-db(floor)
    quiet=frames[rms<=np.quantile(rms,.25)]
    power=np.mean(abs(np.fft.rfft(quiet*np.hanning(320),n=1024,axis=1))**2,axis=0)+1e-12
    freq=np.fft.rfftfreq(1024,1/16000)
    band=power[(freq>=200)&(freq<=7000)]
    flat=float(np.exp(np.mean(np.log(band)))/np.mean(band))
    tonal=float(power[(freq>=35)&(freq<=150)].sum()/power[(freq>=35)&(freq<=7000)].sum())
    level=db(np.sqrt(np.mean(x*x)))
    if level<-32: label="quiet"
    elif tonal>.45 and db(floor)>-48: label="tonal"
    elif flat>.18 and db(floor)>-45 and dyn<25: label="broadband"
    else: label="uncertain"
    return Profile(label,level,db(floor),dyn,flat,tonal)

def key_for(engine,ratio):
    return f"{engine}:{int(round(ratio*100))}"

class FeedbackBook:
    """Counts only verified reference-based outcomes; never learn from own guesses."""
    def __init__(self,stats=None):
        self.stats={} if stats is None else {k:dict(v) for k,v in stats.items()}

    def observe(self,decision,reference,*,verified=False):
        if not verified or not tokens(reference):
            raise ValueError("An independently verified reference is required")
        source=next(o for o in decision.options if o.key=="original")
        baseline=analyze(reference,source.hearing.text)
        for o in decision.options:
            if o.key=="original" or not o.hearing.usable: continue
            q=analyze(reference,o.hearing.text)
            k=decision.profile.label+"|"+o.key
            row=self.stats.setdefault(k,{"n":0,"wins":0,"losses":0,"critical":0,"delta":0})
            d=baseline["word_errors"]-q["word_errors"]
            row["n"]+=1
            row["wins"]+=int(d>0); row["losses"]+=int(d<0)
            row["critical"]+=int(bool(set(q["critical_differences"])-set(baseline["critical_differences"])))
            row["delta"]+=d

    def adjustment(self,label,key):
        row=self.stats.get(label+"|"+key)
        if not row or row["n"]<12: return 0.
        # Bounded re-ranking, not automatic proof of quality.
        if row["critical"]: return -2.
        return max(-.35,min(.35,(row["wins"]-row["losses"])/row["n"]*.35+row["delta"]/row["n"]*.05))

    def snapshot(self):
        return {k:dict(v) for k,v in self.stats.items()}

def plan(p,book=None,budget=3,explore=False):
    if not 1<=budget<=8: raise ValueError("Invalid filter budget")
    ordered=list(POLICY[p.label])
    if explore: ordered.extend(EXPLORATION)
    else:
        # Learned options may enter the route after adequate verified evidence.
        if book:
            ordered.extend(q for q in EXPLORATION if book.adjustment(p.label,key_for(*q))>0)
    unique=list(dict.fromkeys(ordered))
    if book:
        unique=sorted(unique,key=lambda q:-book.adjustment(p.label,key_for(*q)))
    result=[]; used=set()
    for q in unique:
        if q[0] in used: continue
        result.append(q); used.add(q[0])
        if len(result)==budget: break
    return result

def guard(source,filtered,ratio,spans):
    """Reject large distortions and blend only time-aligned waveforms."""
    x=samples(source); z,lag,corr=aligned(source,filtered)
    if z is None or corr<.35: return None,"correlation"
    if not 0<ratio<=1: raise ValueError("Invalid blend")
    y=(1-ratio)*x+ratio*z
    # Guard energetic frames within original speech regions. It cannot prove
    # preservation of phonemes, only flag strong attenuation.
    n=len(x)//320
    a=np.sqrt(np.mean(x[:n*320].reshape(n,320)**2,axis=1)+1e-12)
    b=np.sqrt(np.mean(y[:n*320].reshape(n,320)**2,axis=1)+1e-12)
    voice=a>=max(float(np.quantile(a,.65))*.3,1e-4)
    if spans:
        centers=np.arange(n)*320+160
        within=np.zeros(n,dtype=bool)
        for lo,hi in spans: within|=(centers>=lo)&(centers<hi)
        if (voice&within).any(): voice &=within
    if not voice.any(): return None,"no_active_frames"
    relative=b[voice]/a[voice]
    if np.mean(relative<.25)>.08 or np.median(relative)<.35:
        return None,"speech_attenuation"
    if np.max(np.abs(y))>=.999 or not np.all(np.isfinite(y)):
        return None,"peak_or_nonfinite"
    return output(source,y),"ok"

def recognize(model,audio):
    """Whisper evidence. Confidence is heuristic, not a probability of correctness."""
    x=samples(audio)
    segments,_=model.transcribe(x,language="it",beam_size=5,condition_on_previous_text=False,
                                vad_filter=False,temperature=0)
    seg=list(segments)
    transcript=" ".join(s.text.strip() for s in seg).strip()
    words=tokens(transcript)
    if not words or not seg: return Hearing(transcript,-10.,False)
    counts=[max(1,len(s.tokens)) for s in seg]
    score=sum(s.avg_logprob*c for s,c in zip(seg,counts))/sum(counts)
    max_rate=len(words)/max(len(x)/16000,.25)
    repeated=max(Counter(words).values())/len(words) if len(words)>=8 else 0.
    suspicious=any(s.compression_ratio>2.4 or (s.no_speech_prob>.6 and s.avg_logprob<-.8) for s in seg)
    return Hearing(transcript,float(score),bool(math.isfinite(score) and score>=-1.4 and max_rate<=8 and repeated<.6 and not suspicious))

def select(audio,recognizer,*,book=None,budget=3,explore=False,filter_fn=apply_filter,vad_fn=speech_segments):
    p=profile(audio); diag=diagnose(audio); errors={}
    def heard(a):
        try:
            h=recognizer(a)
            if not isinstance(h,Hearing) or not math.isfinite(h.score): raise ValueError("Invalid recognizer evidence")
            return h
        except Exception as exc:
            errors["recognizer"]=type(exc).__name__
            return Hearing("",-10.,False)
    original=Option("original",audio,heard(audio))
    opts=[original]
    def finish(o,reason):
        return Decision(o.audio,o.key,reason,p,audio,tuple(opts),dict(errors))
    if diag.status not in ("ok","quiet"): return finish(original,"source_diagnostic")
    try: spans=vad_fn(audio)
    except Exception as exc:
        errors["vad"]=type(exc).__name__; spans=[]
    if not spans and "vad" not in errors: return finish(original,"no_vad_speech")
    cache={audio.pcm:original.hearing}
    for engine,ratio in plan(p,book,budget,explore):
        k=key_for(engine,ratio)
        try:
            filtered=filter_fn(audio,engine)
            candidate,reason=guard(audio,filtered,ratio,spans)
            if candidate is None: errors[k]=reason; continue
            if candidate.pcm not in cache: cache[candidate.pcm]=heard(candidate)
            opts.append(Option(k,candidate,cache[candidate.pcm]))
        except Exception as exc:
            errors[k]=type(exc).__name__
    # Two differently processed, nonidentical waveforms must corroborate an
    # alternative text. This is still the same recognizer, NOT two ASR families.
    valid=[o for o in opts[1:] if o.hearing.usable and tokens(o.hearing.text) and o.audio.pcm!=audio.pcm]
    groups={}
    for o in valid: groups.setdefault(tuple(tokens(o.hearing.text)),[]).append(o)
    choices=[]
    original_tokens=tuple(tokens(original.hearing.text))
    for words,group in groups.items():
        if words==original_tokens: continue
        if len({o.audio.pcm for o in group})<2: continue
        # Never switch if a critical token differs from a nonempty baseline.
        if original_tokens and critical_differences(original.hearing.text,group[0].hearing.text): continue
        if original_tokens and abs(len(words)-len(original_tokens))>max(1,len(original_tokens)//5): continue
        best=max(group,key=lambda o:o.hearing.score)
        if original.hearing.usable and (best.hearing.score<original.hearing.score+.12 or
                sum(o.hearing.score for o in group)/len(group)<original.hearing.score+.05): continue
        choices.append(best)
    if len(choices)==1: return finish(choices[0],"corroborated_alternative")
    return finish(original,"ambiguous_alternatives" if choices else "original_conservative")
