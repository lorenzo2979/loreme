"""Independent paired WER benchmark per filter; never selects using references at runtime."""
import argparse
import json
import time
from audio_agent_benchmark import NAMES,CASES,load,simulate,decode,WhisperModel
from audio_filter_bank import apply_filter,speech_segments,FILTERS
from italian_quality import analyze,score_dataset
from audio_fusion import mix_candidates,RATIOS

def main():
    p=argparse.ArgumentParser()
    p.add_argument("engine",choices=(*FILTERS,"silero"))
    args=p.parse_args()
    engine=args.engine
    model=None if engine=="silero" else WhisperModel("base",device="cpu",compute_type="int8",cpu_threads=2,num_workers=1)
    records=[]; rows=[]; fusion_records=[]; fusion_counts={f"mix{int(r*100)}":{"improved":0,"worsened":0,"unchanged":0} for r in RATIOS}; rejected=0; improved=worsened=unchanged=0
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
                variants,meta=mix_candidates(audio,out)
                row["fusion_alignment"]=meta
                hypotheses={"original":raw,engine:filtered}
                cache={audio.pcm:raw,out.pcm:filtered}
                if not variants:
                    rejected+=1
                    # The safe policy uses original when alignment is uncertain.
                    variants={f"mix{int(r*100)}":audio for r in RATIOS}
                for key,variant in variants.items():
                    if variant.pcm not in cache:
                        cache[variant.pcm]=decode(model,variant)
                    hyp=cache[variant.pcm]; hypotheses[key]=hyp
                    q=analyze(ref,hyp)
                    d=q["word_errors"]-a["word_errors"]
                    fusion_counts[key]["improved" if d<0 else "worsened" if d>0 else "unchanged"]+=1
                    row[key+"_wer"]=q["wer"]
                fusion_records.append({"reference":ref,"category":case,"hypotheses":hypotheses})
            rows.append(row)
            print(json.dumps(row),flush=True)
    summary={"engine":engine,"conditions":len(rows),"rows":rows}
    if records:
        summary.update(overall=score_dataset(records),per_case={c:score_dataset([r for r in records if r["category"]==c]) for c in CASES},improved=improved,worsened=worsened,unchanged=unchanged)
    if fusion_records:
        summary["fusion"]={"overall":score_dataset(fusion_records),"per_case":{c:score_dataset([r for r in fusion_records if r["category"]==c]) for c in CASES},"paired":fusion_counts,"alignment_rejected":rejected}
    print("FILTER_BANK_SUMMARY="+json.dumps(summary),flush=True)
    with open("filter-bank-result.json","w") as f: json.dump(summary,f,indent=2)

if __name__=="__main__": main()
