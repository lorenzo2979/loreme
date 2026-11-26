import torch
import numpy as np
from transformers import AutoModel, AutoFeatureExtractor, WhisperModel, WhisperFeatureExtractor
from datasets import load_dataset, Audio
from smoke import Distillation

torch.manual_seed(42)
torch.set_num_threads(2)
# Italian FLEURS validation: engineering smoke only, not independent evaluation.
ds = load_dataset("google/fleurs", "it_it", split="validation", streaming=True, trust_remote_code=True).cast_column("audio", Audio(sampling_rate=16000))
w = WhisperModel.from_pretrained("openai/whisper-tiny").eval()
wf = WhisperFeatureExtractor.from_pretrained("openai/whisper-tiny")
name = "jonatasgrosman/wav2vec2-large-xlsr-53-italian"
t = AutoModel.from_pretrained(name).eval()
tf = AutoFeatureExtractor.from_pretrained(name)
for p in t.parameters(): p.requires_grad_(False)
for p in w.parameters(): p.requires_grad_(False)
# Train only a residual adapter on frozen student features.
adapter = torch.nn.Linear(w.config.d_model, w.config.d_model)
torch.nn.init.zeros_(adapter.weight)
torch.nn.init.zeros_(adapter.bias)
kd = Distillation(w.config.d_model, [t.config.hidden_size, w.config.d_model])
opt = torch.optim.AdamW(list(adapter.parameters()) + list(kd.parameters()), lr=0.001)
pairs = []
for row in ds:
    audio = np.asarray(row["audio"]["array"], dtype=np.float32)
    if not 16000 <= len(audio) <= 160000: continue
    with torch.no_grad():
        features = wf(audio, sampling_rate=16000, return_tensors="pt").input_features
        # Whisper encoder expects 30-second input; crop output to actual duration.
        s = w.encoder(features).last_hidden_state[:, :len(audio)//320]
        h = t(tf(audio, sampling_rate=16000, return_tensors="pt").input_values).last_hidden_state
    pairs.append((s,h))
    if len(pairs)==4: break
assert len(pairs)==4
def loss(pair):
    s,h = pair
    return kd(s+adapter(s), [h,s], torch.tensor([s.shape[1]]))
initial = sum(loss(p).item() for p in pairs[:3])/3
for epoch in range(10):
    for p in pairs[:3]:
        opt.zero_grad()
        l=loss(p)
        assert torch.isfinite(l)
        l.backward()
        opt.step()
final=sum(loss(p).item() for p in pairs[:3])/3
print(dict(train_initial=initial,train_final=final,heldout_loss=loss(pairs[3]).item(),validated_asr=False))
assert final < initial
