"""Independent paired WER benchmark per filter; never selects using references at runtime."""
import argparse
import json
import time
from audio_agent_benchmark import NAMES,CASES,load,simulate,decode,WhisperModel
from audio_filter_bank import apply_filter,speech_segments,FILTERS
from italian_quality import analyze,score_dataset

def main():
    p=argparse.ArgumentParser()
    p.add_argument("engine",choices=(*FILTERS,"silero"))
    args=p.parse_args()
    engine=args.engine
    model=None if engine=="silero" else WhisperModel("base",device="cpu",compute_type="int8",cpu_threads=2,num_workers=1)
    records=[]; rows=[]; improved=worsened=unchanged=0
    for i,name in enumerate(NAMES):
        clean,ref=load(name)
        for case in CASES:
            audio=simulate(clean,case,25112026+i)
            start=time.perf_counter()
            if engine=="silero":
                spans=speech_segments(audio)
                row={"sample":name,"case":case,"spans":spans,"duration_samples":len(audio.pcm)//2}
            else:
                out=apply_filter(audio,engine)
                ms=round((time.perf_counter()-start)*1000,2)
                raw=decode(model,audio)
                filtered=raw if out.pcm==audio.pcm else decode(model,out)
                a,b=analyze(ref,raw),analyze(ref,filtered)
                delta=b["word_errors"]-a["word_errors"]
                improved+=delta<0;worsened+=delta>0;unchanged+=delta==0
                row={"sample":name,"case":case,"original_wer":a["wer"],"filtered_wer":b["wer"],"original_cer":a["cer"],"filtered_cer":b["cer"],"original_critical":a["critical_differences"],"filtered_critical":b["critical_differences"],"dsp_ms":ms,"changed":out.pcm!=audio.pcm}
                records.append({"reference":ref,"category":case,"hypotheses":{"original":raw,engine:filtered}})
            rows.append(row)
            print(json.dumps(row),flush=True)
    summary={"engine":engine,"conditions":len(rows),"rows":rows}
    if records:
        summary.update(overall=score_dataset(records),per_case={c:score_dataset([r for r in records if r["category"]==c]) for c in CASES},improved=improved,worsened=worsened,unchanged=unchanged)
    print("FILTER_BANK_SUMMARY="+json.dumps(summary),flush=True)
    with open("filter-bank-result.json","w") as f: json.dump(summary,f,indent=2)

if __name__=="__main__": main()
