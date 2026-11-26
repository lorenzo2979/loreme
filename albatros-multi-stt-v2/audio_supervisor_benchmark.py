"""Exploratory supervisor test: old corpus for feedback, separate FLEURS test
recordings mixed with recorded DEMAND noise. No test reference enters selection.
Audio and transcript contents are not uploaded in result artifacts.
"""
import collections
import hashlib
import io
import json
import os
from pathlib import Path
import tempfile
import time
import wave
import zipfile
import subprocess
import numpy as np
import requests
from audio_agent_benchmark import NAMES,CASES,load,simulate,pcm,WhisperModel
from audio_supervisor import select,recognize,FeedbackBook
from audio_filter_bank import samples
from italian_quality import analyze,score_dataset

FLEURS="https://datasets-server.huggingface.co/rows"
DEMAND="https://zenodo.org/records/1227121/files/"
NOISES={"kitchen":"DKITCHEN_16k.zip","street":"SPSQUARE_16k.zip"}

def download(url,path,limit=750_000_000):
    with requests.get(url,stream=True,timeout=(15,120)) as r:
        r.raise_for_status()
        count=0
        with open(path,"wb") as f:
            for chunk in r.iter_content(1024*1024):
                count+=len(chunk)
                if count>limit: raise ValueError("Download exceeded limit")
                f.write(chunk)
    print("downloaded",path.name,count,flush=True)

def decode_bytes(data):
    raw=subprocess.run(["ffmpeg","-nostdin","-v","error","-i","pipe:0","-f","s16le","-ar","16000","-ac","1","pipe:1"],
                       input=data,stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=True,timeout=30).stdout
    return np.frombuffer(raw,dtype="<i2").astype(np.float32)/32768

def viewer_corpus(folder,count):
    # Request only a bounded subset of the Italian held-out test split.
    # The dataset viewer provides signed audio links, avoiding huge parquet.
    clips=[]; seen=set(); session=requests.Session()
    for offset in range(0,128,32):
        response=session.get(FLEURS,params={"dataset":"google/fleurs","config":"it_it","split":"test","offset":offset,"length":32},timeout=(15,120))
        response.raise_for_status()
        rows=response.json().get("rows",[])
        if not rows: break
        for entry in rows:
            row=entry["row"]
            ref=row["transcription"].strip()
            if ref in seen or not ref: continue
            assets=row["audio"]
            if isinstance(assets,dict): assets=[assets]
            url=next((a["src"] for a in assets if isinstance(a,dict) and a.get("src")),None)
            if not url or not url.startswith("https://"): raise ValueError("No HTTPS FLEURS audio URL")
            path=folder/"fleurs-clip.audio"
            download(url,path,12_000_000)
            x=decode_bytes(path.read_bytes())
            if not 1<=len(x)/16000<=25: continue
            seen.add(ref); clips.append((pcm(x),ref))
            if len(clips)==count: return clips
    raise ValueError(f"Only {len(clips)} eligible independent clips")


def independent_corpus(folder,count):
    # Keep the same held-out split even if the viewer service fails.
    try:
        return viewer_corpus(folder,count)
    except (requests.RequestException, ValueError, subprocess.SubprocessError) as exc:
        print("FLEURS viewer unavailable:", type(exc).__name__, flush=True)
    import pyarrow.parquet as pq
    base="https://huggingface.co/api/datasets/google/fleurs/parquet/it_it/test"
    response=requests.get(base,timeout=(15,120))
    response.raise_for_status()
    urls=response.json()
    if not isinstance(urls,list) or not urls:
        raise ValueError("No Italian FLEURS test parquet shards")
    clips=[]; seen=set()
    for index,url in enumerate(urls):
        if not isinstance(url,str) or not url.startswith("https://"):
            raise ValueError("Invalid parquet shard URL")
        path=folder/f"fleurs-test-{index}.parquet"
        download(url,path,2_500_000_000)
        parquet=pq.ParquetFile(path)
        for batch in parquet.iter_batches(batch_size=8,columns=["audio","transcription"]):
            for row in batch.to_pylist():
                ref=row["transcription"].strip()
                if not ref or ref in seen: continue
                audio=row["audio"]
                if not audio or not audio.get("bytes"): continue
                x=decode_bytes(audio["bytes"])
                if not 1<=len(x)/16000<=25: continue
                seen.add(ref); clips.append((pcm(x),ref))
                if len(clips)==count:
                    print("FLEURS held-out source: parquet",flush=True)
                    return clips
        path.unlink()
    raise ValueError(f"Only {len(clips)} eligible independent parquet clips")


def real_noises(folder):
    result={}
    for label,filename in NOISES.items():
        path=folder/filename; download(DEMAND+filename+"?download=1",path,250_000_000)
        with zipfile.ZipFile(path) as z:
            member=next(n for n in z.namelist() if Path(n).name.lower()=="ch01.wav")
            raw=z.read(member)
        result[label]=decode_bytes(raw)
    return result

def mix_real(audio,noise,seed,snr):
    x=samples(audio)
    rng=np.random.default_rng(seed)
    if len(noise)<len(x): raise ValueError("Noise too short")
    start=int(rng.integers(0,len(noise)-len(x)+1))
    n=noise[start:start+len(x)].copy(); n-=n.mean()
    n*=np.sqrt(np.mean(x*x))/max(float(np.sqrt(np.mean(n*n))),1e-9)*10**(-snr/20)
    y=x+n; return pcm(y*min(1.,.98/max(float(np.max(abs(y))),1e-9)))

def evaluate(model,items,book=None,train=False):
    records=[]; rows=[]; counts=collections.Counter()
    for label,case,audio,reference in items:
        start=time.perf_counter()
        decision=select(audio,lambda a:recognize(model,a),book=book if not train else None,budget=8 if train else 3,explore=train)
        elapsed=round((time.perf_counter()-start)*1000,1)
        original=decision.options[0].hearing.text
        chosen=next(o for o in decision.options if o.key==decision.selected).hearing.text
        qa,qb=analyze(reference,original),analyze(reference,chosen)
        # Evaluation-only oracle: theoretical best among usable candidates.
        # Never passed to the selector or used as real deployment performance.
        oracle=min((o for o in decision.options if o.hearing.usable or o.key=="original"),key=lambda o:analyze(reference,o.hearing.text)["word_errors"])
        qo=analyze(reference,oracle.hearing.text)
        counts["improved" if qb["word_errors"]<qa["word_errors"] else "worsened" if qb["word_errors"]>qa["word_errors"] else "unchanged"]+=1
        records.append({"reference":reference,"category":case,"hypotheses":{"original":original,"supervisor":chosen,"oracle_diagnostic":oracle.hearing.text}})
        row={"sample":label,"case":case,"profile":decision.profile.label,"selected":decision.selected,
             "reason":decision.reason,"errors":decision.errors,"original_wer":qa["wer"],"selected_wer":qb["wer"],
             "original_critical":qa["critical_differences"],"selected_critical":qb["critical_differences"],
             "elapsed_ms":elapsed,"candidates":len(decision.options),"oracle_wer":qo["wer"],"oracle_key":oracle.key}
        rows.append(row); print(json.dumps(row),flush=True)
        if train: book.observe(decision,reference,verified=True)
    return {"overall":score_dataset(records),"per_case":{c:score_dataset([r for r in records if r["category"]==c]) for c in sorted(set(r["category"] for r in records))},"paired":dict(counts),"rows":rows}

def main():
    model=WhisperModel("base",device="cpu",compute_type="int8",cpu_threads=2,num_workers=1)
    book=FeedbackBook()
    old=[]
    for i,name in enumerate(NAMES):
        clean,ref=load(name)
        for case in CASES: old.append((name,case,simulate(clean,case,25112026+i),ref))
    dev=evaluate(model,old,book,train=True)
    summary={"development":dev,"feedback":book.snapshot()}
    print("SUPERVISOR_DEV="+json.dumps(summary),flush=True)
    with open("supervisor-dev.json","w") as f: json.dump(summary,f,indent=2)
    # Explicitly freeze feedback before looking at independent references.
    frozen=FeedbackBook(book.snapshot())
    with tempfile.TemporaryDirectory() as tmp:
        folder=Path(tmp); clips=independent_corpus(folder,24); noises=real_noises(folder)
        items=[]
        for i,(clean,ref) in enumerate(clips):
            items.extend([(f"fleurs-{i:02d}","clean",clean,ref),
                          (f"fleurs-{i:02d}","kitchen_5dB",mix_real(clean,noises["kitchen"],100+i,5),ref),
                          (f"fleurs-{i:02d}","street_10dB",mix_real(clean,noises["street"],200+i,10),ref)])
        heldout=evaluate(model,items,frozen)
    summary["heldout"]=heldout
    print("SUPERVISOR_SUMMARY="+json.dumps(summary),flush=True)
    with open("supervisor-result.json","w") as f: json.dump(summary,f,indent=2)

if __name__=="__main__": main()
