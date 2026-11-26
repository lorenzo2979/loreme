"""Compare pretrained ASR models without selecting or tuning on references."""
import json, os, time, tempfile
from pathlib import Path
import numpy as np
from faster_whisper import WhisperModel
from audio_supervisor_benchmark import independent_corpus, real_noises, mix_real
from audio_filter_bank import samples
from italian_quality import analyze, tokens, edit_distance

def main():
    name=os.environ["ASR_MODEL"]
    # Skip all previously examined 24 unique test references.
    with tempfile.TemporaryDirectory() as tmp:
        folder=Path(tmp)
        clips=independent_corpus(folder,48)[24:]
        noises=real_noises(folder)
    if name=="wav2vec2":
        import torch
        from transformers import pipeline
        torch.set_num_threads(2)
        pipe=pipeline("automatic-speech-recognition",model="jonatasgrosman/wav2vec2-large-xlsr-53-italian",device=-1)
        def transcribe(x):
            return pipe({"raw":x,"sampling_rate":16000})["text"]
    else:
        model=WhisperModel(name,device="cpu",compute_type="int8",cpu_threads=2,num_workers=1)
        def transcribe(x):
            seg,_=model.transcribe(x,language="it",beam_size=5,temperature=0,condition_on_previous_text=False,vad_filter=False)
            return " ".join(s.text.strip() for s in seg)
    rows=[]
    for i,(clean,ref) in enumerate(clips):
        for case,audio in [("clean",clean),("kitchen",mix_real(clean,noises["kitchen"],500+i,5)),("street",mix_real(clean,noises["street"],600+i,10))]:
            x=samples(audio)
            start=time.perf_counter()
            hyp=transcribe(x)
            elapsed=time.perf_counter()-start
            q=analyze(ref,hyp)
            a,b=list(" ".join(tokens(ref))),list(" ".join(tokens(hyp)))
            rows.append(dict(sample=i,case=case,words=q["reference_words"],errors=q["word_errors"],chars=len(a),char_errors=edit_distance(a,b)["errors"],critical=q["critical_differences"],exact=q["exact_match"],edits=q["word_edits"],seconds=elapsed,audio_seconds=len(x)/16000))
    def aggregate(rr):
        return dict(n=len(rr),wer=sum(x["errors"] for x in rr)/sum(x["words"] for x in rr),cer=sum(x["char_errors"] for x in rr)/sum(x["chars"] for x in rr),critical=sum(bool(x["critical"]) for x in rr),exact=sum(x["exact"] for x in rr),rtf=sum(x["seconds"] for x in rr)/sum(x["audio_seconds"] for x in rr))
    result=dict(model=name,overall=aggregate(rows),cases={c:aggregate([x for x in rows if x["case"]==c]) for c in ("clean","kitchen","street")},rows=rows)
    Path("asr-diagnostic.json").write_text(json.dumps(result,indent=2))
    print("ASR_DIAGNOSTIC="+json.dumps(result),flush=True)
if __name__=="__main__": main()
