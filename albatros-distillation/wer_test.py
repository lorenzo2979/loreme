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
opt = torch.optim.AdamW(list(adapter.parameters()) + list(kd.parameters()), lr=0.0003)

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

def adapted(s,n):
    return torch.cat([s[:,:n]+adapter(s[:,:n]),s[:,n:]],dim=1)

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

train=collect("train",12,True)
test=collect("test",24,False)
train_refs={tuple(normalize(r[3])) for r in train}
test=[r for r in test if tuple(normalize(r[3])) not in train_refs]
assert len(test)>=16
for epoch in range(5):
    for s,n,h,ref in train:
        opt.zero_grad()
        p=s[:,:n]
        z=p+adapter(p)
        loss=kd(z,[h,p],torch.tensor([n]))+0.5*(z-p).square().mean()
        assert torch.isfinite(loss)
        loss.backward()
        opt.step()
rows=[]
for s,n,_,ref in test:
    a=decode(s)
    b=decode(adapted(s,n).detach())
    r=normalize(ref)
    rows.append(dict(reference=ref,baseline=a,adapted=b,words=len(r),before=distance(r,normalize(a)),after=distance(r,normalize(b))))
words=sum(r["words"] for r in rows)
before=sum(r["before"] for r in rows)
after=sum(r["after"] for r in rows)
report=dict(samples=len(rows),words=words,baseline_errors=before,adapted_errors=after,baseline_wer=before/words,adapted_wer=after/words,improved=sum(r["after"]<r["before"] for r in rows),worse=sum(r["after"]>r["before"] for r in rows),rows=rows)
print(json.dumps({k:v for k,v in report.items() if k!="rows"}))
with open("wer-report.json","w") as f: json.dump(report,f,ensure_ascii=False,indent=2)
torch.save(adapter.state_dict(),"adapter.pt")
