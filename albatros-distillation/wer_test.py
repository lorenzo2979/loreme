import json
import re
import numpy as np
import torch
from torch import nn
from datasets import load_dataset, Audio
from transformers import WhisperForConditionalGeneration, WhisperProcessor, AutoModel, AutoFeatureExtractor
from transformers.modeling_outputs import BaseModelOutput
from smoke import Distillation

torch.manual_seed(42)
torch.set_num_threads(2)
student_id = "openai/whisper-tiny"
teacher_id = "jonatasgrosman/wav2vec2-large-xlsr-53-italian"
student = WhisperForConditionalGeneration.from_pretrained(student_id).eval()
processor = WhisperProcessor.from_pretrained(student_id)
teacher = AutoModel.from_pretrained(teacher_id).eval()
extractor = AutoFeatureExtractor.from_pretrained(teacher_id)
for model in (student, teacher):
    model.requires_grad_(False)
dim = student.config.d_model
adapter = nn.Linear(dim, dim)
nn.init.zeros_(adapter.weight)
nn.init.zeros_(adapter.bias)
kd = Distillation(dim, [teacher.config.hidden_size, dim])
opt = torch.optim.AdamW(list(adapter.parameters()) + list(kd.parameters()), lr=0.0001)

def collect(split, count, with_teacher):
    ds = load_dataset("google/fleurs", "it_it", split=split, streaming=True, trust_remote_code=True).cast_column("audio", Audio(sampling_rate=16000))
    rows = []
    for row in ds:
        a = np.asarray(row["audio"]["array"], dtype=np.float32)
        if not 16000 <= len(a) <= 160000: continue
        with torch.no_grad():
            x = processor.feature_extractor(a, sampling_rate=16000, return_tensors="pt").input_features
            s = student.model.encoder(x).last_hidden_state
            n = min(s.shape[1], len(a)//320)
            h = teacher(extractor(a, sampling_rate=16000, return_tensors="pt").input_values).last_hidden_state if with_teacher else None
        rows.append((s,n,h,row["transcription"]))
        if len(rows)==count: break
    assert len(rows)==count
    return rows

def adapted(s,n,scale=1.0):
    return torch.cat([s[:,:n]+scale*adapter(s[:,:n]),s[:,n:]],dim=1)

def normalize(s):
    return re.findall(r"[^\W_]+",s.lower(),flags=re.UNICODE)

def distance(a,b):
    d=list(range(len(b)+1))
    for i,x in enumerate(a,1):
        e=[i]
        for j,y in enumerate(b,1):
            e.append(min(e[-1]+1,d[j]+1,d[j-1]+(x!=y)))
        d=e
    return d[-1]

@torch.no_grad()
def decode(s):
    ids=student.generate(encoder_outputs=BaseModelOutput(last_hidden_state=s), language="it", task="transcribe", do_sample=False, num_beams=1, max_new_tokens=128)
    return processor.batch_decode(ids,skip_special_tokens=True)[0]

train=collect("train",48,True)
dev=collect("validation",24,False)
test=collect("test",24,False)
train_refs={tuple(normalize(r[3])) for r in train}
dev=[r for r in dev if tuple(normalize(r[3])) not in train_refs]
dev_refs={tuple(normalize(r[3])) for r in dev}
test=[r for r in test if tuple(normalize(r[3])) not in train_refs|dev_refs]
assert len(dev)>=16 and len(test)>=16
# Preserve the decoder's Italian transcription task while transferring features.
for epoch in range(3):
    for s,n,h,ref in train:
        opt.zero_grad()
        p=s[:,:n]
        z=p+adapter(p)
        enc=BaseModelOutput(last_hidden_state=adapted(s,n))
        processor.tokenizer.set_prefix_tokens(language="italian",task="transcribe",predict_timestamps=False)
        labels=processor.tokenizer(ref,return_tensors="pt").input_ids
        if labels[0,0].item()==student.config.decoder_start_token_id:
            labels=labels[:,1:]
        ce=student(encoder_outputs=enc,labels=labels,use_cache=False).loss
        loss=ce+0.1*kd(z,[h,p],torch.tensor([n]))+2.0*(z-p).square().mean()
        assert torch.isfinite(loss)
        loss.backward()
        torch.nn.utils.clip_grad_norm_(list(adapter.parameters())+list(kd.parameters()),1.0)
        opt.step()
def evaluate(data,scale):
    rows=[]
    for s,n,_,ref in data:
        a=decode(s if scale==0 else adapted(s,n).detach() if scale==1 else adapted(s,n,scale).detach())
        r=normalize(ref)
        rows.append(dict(reference=ref,hypothesis=a,words=len(r),errors=distance(r,normalize(a))))
    return rows
scales=[0.0,0.1,0.25,0.5,1.0]
dev_scores={scale:sum(r["errors"] for r in evaluate(dev,scale)) for scale in scales}
chosen=min(scales,key=lambda scale:(dev_scores[scale],scale))
baseline=evaluate(test,0)
candidate=evaluate(test,chosen)
full=evaluate(test,1)
words=sum(r["words"] for r in baseline)
before=sum(r["errors"] for r in baseline)
after=sum(r["errors"] for r in candidate)
report=dict(samples=len(test),words=words,dev_errors=dev_scores,chosen_scale=chosen,baseline_errors=before,adapted_errors=after,baseline_wer=before/words,adapted_wer=after/words,full_scale_errors=sum(r["errors"] for r in full),improved=sum(b["errors"]<a["errors"] for a,b in zip(baseline,candidate)),worse=sum(b["errors"]>a["errors"] for a,b in zip(baseline,candidate)),baseline=baseline,candidate=candidate)
print(json.dumps({k:v for k,v in report.items() if k not in ("baseline","candidate")}))
with open("wer-report.json","w") as f: json.dump(report,f,ensure_ascii=False,indent=2)
torch.save({"adapter":adapter.state_dict(),"scale":chosen},"adapter.pt")
