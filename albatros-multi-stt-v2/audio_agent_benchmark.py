"""Reproducible paired audio-agent benchmark on external Italian examples.

Public samples fetched only at runtime, never redistributed. Check commercial
dataset terms before using beyond evaluation. Noise is simulated, not field audio.
"""
import collections
import io
import json
import os
import time
import urllib.request
import wave
import numpy as np
from faster_whisper import WhisperModel
from audio_agent import prepare
from italian_quality import analyze, score_dataset
from wyoming_server import Audio

BASE="https://raw.githubusercontent.com/Nexdata-AI/347-Hours-Italian-Speech-Data-Collected-by-Mobile-Phone/main/"
NAMES=("T0263G0002S0001","T0263G0003S0001","T0263G0007S0001",
       "T0263G0009S0002","T0263G0015S0002","T0263G0025S0001",
       "T0263G0041S0001","T0263G0060S0001","T0266G0107S0001","T0266G0116S0001")
CASES=("clean","white_10dB","fan_5dB","hum_10dB","quiet_minus18dB")

def fetch(name):
    with urllib.request.urlopen(BASE+name,timeout=30) as response:
        return response.read(1000000)

def load(name):
    with wave.open(io.BytesIO(fetch(name+".wav")),"rb") as f:
        assert (f.getframerate(),f.getsampwidth(),f.getnchannels())==(16000,2,1)
        pcm=f.readframes(f.getnframes())
    reference=fetch(name+".txt").decode("utf-8-sig").strip()
    if not reference: raise ValueError("empty reference")
    return Audio(pcm,16000,2,1),reference

def pcm(x):
    return Audio(np.rint(np.clip(x,-1,32767/32768)*32768).astype("<i2").tobytes(),16000,2,1)

def simulate(audio,case,seed):
    if case=="clean": return audio
    x=np.frombuffer(audio.pcm,dtype="<i2").astype(float)/32768
    if case=="quiet_minus18dB": return pcm(x*10**(-18/20))
    rng=np.random.default_rng(seed)
    t=np.arange(len(x))/16000
    noise=rng.normal(size=len(x))
    if case=="fan_5dB":
        # Low-frequency broadband and mechanical tone approximation.
        freq=np.fft.rfftfreq(len(x),1/16000)
        noise=np.fft.irfft(np.fft.rfft(noise)/(1+(freq/600)**2),n=len(x))
        noise+=.2*np.sin(2*np.pi*120*t)
        snr=5
    elif case=="hum_10dB":
        noise=.8*np.sin(2*np.pi*50*t)+.35*np.sin(2*np.pi*100*t)+.1*noise
        snr=10
    elif case=="white_10dB": snr=10
    else: raise ValueError(case)
    noise-=noise.mean()
    noise*=np.sqrt(np.mean(x*x))/max(np.sqrt(np.mean(noise*noise)),1e-12)*10**(-snr/20)
    y=x+noise
    return pcm(y*min(1,.98/max(np.max(np.abs(y)),1e-12)))

def decode(model,audio):
    x=np.frombuffer(audio.pcm,dtype="<i2").astype(np.float32)/32768
    segments,_=model.transcribe(x,language="it",beam_size=5,
       condition_on_previous_text=False,vad_filter=False,temperature=0)
    return " ".join(s.text.strip() for s in segments).strip()

def main():
    model_name=os.getenv("WHISPER_MODEL","base")
    model=WhisperModel(model_name,device="cpu",compute_type="int8",cpu_threads=2,num_workers=1)
    records=[]
    decisions=collections.Counter()
    paired=collections.Counter()
    for i,name in enumerate(NAMES):
        audio,reference=load(name)
        for case in CASES:
            source=simulate(audio,case,25112026+i)
            start=time.perf_counter()
            out=prepare(source)
            latency_ms=round((time.perf_counter()-start)*1000,2)
            raw=decode(model,source)
            processed=raw if out.selected.pcm==source.pcm else decode(model,out.selected)
            a,b=analyze(reference,raw),analyze(reference,processed)
            decisions[out.reason]+=1
            delta=b["word_errors"]-a["word_errors"]
            paired["improved" if delta<0 else "worsened" if delta>0 else "unchanged"]+=1
            records.append({"reference":reference,"category":case,
                            "hypotheses":{"original":raw,"agent":processed}})
            print(json.dumps({"sample":name,"case":case,"decision":out.reason,
               "original_wer":a["wer"],"agent_wer":b["wer"],
               "original_cer":a["cer"],"agent_cer":b["cer"],
               "original_critical":a["critical_differences"],
               "agent_critical":b["critical_differences"],"dsp_ms":latency_ms},
               ensure_ascii=False),flush=True)
    per_case={c:score_dataset([r for r in records if r["category"]==c]) for c in CASES}
    overall=score_dataset(records)
    summary={"model":model_name,"clips":len(NAMES),"cases":len(records),
             "overall":overall,"per_case":per_case,"paired":dict(paired),
             "decisions":dict(decisions)}
    print("BENCHMARK_SUMMARY="+json.dumps(summary,ensure_ascii=False),flush=True)
    # Only aggregate metrics, no audio/reference/transcript persistence.
    with open("audio-agent-benchmark.json","w",encoding="utf-8") as f:
        json.dump(summary,f,indent=2,ensure_ascii=False)

if __name__=="__main__":
    main()
