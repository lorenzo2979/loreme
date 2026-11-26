import json
from huggingface_hub import HfApi, hf_hub_download
BASE = "openai/whisper-large-v3"
MODELS = [BASE, "LocalAI-io/whisper-large-v3-it", "LocalAI-io/whisper-large-v3-it-yodas-only", "LocalAI-io/whisper-large-v3-it-multi-yodas-asr_only-nocv"]
FIELDS = ["model_type","d_model","encoder_layers","decoder_layers","encoder_attention_heads","decoder_attention_heads","encoder_ffn_dim","decoder_ffn_dim","vocab_size","num_mel_bins","max_source_positions","max_target_positions"]
api = HfApi()
def inspect(name):
    info = api.model_info(name, files_metadata=True)
    config = json.load(open(hf_hub_download(name,"config.json",revision=info.sha)))
    files = [f for f in info.siblings if f.rfilename.endswith(".safetensors")]
    if not files: raise RuntimeError("No safetensors: "+name)
    card = info.card_data.to_dict() if info.card_data else {}
    return dict(model=name,revision=info.sha,license=card.get("license"),base_model=card.get("base_model"),shape={k:config.get(k) for k in FIELDS},bytes=sum(f.size or 0 for f in files))
rows = [inspect(n) for n in MODELS]
for row in rows:
    row["config_compatible"] = row["shape"] == rows[0]["shape"]
print(json.dumps(rows,indent=2))
with open("preflight.json","w") as f: json.dump(rows,f,indent=2)
if not all(r["config_compatible"] and r["license"] in ("mit","apache-2.0") for r in rows): raise SystemExit("Compatibility or license review required")
# Config equality is necessary, not sufficient: tensor keys/shapes and tokenizer still need verification.
