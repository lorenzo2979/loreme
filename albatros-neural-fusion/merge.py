import torch
from transformers import WhisperForConditionalGeneration, AutoProcessor
from huggingface_hub import HfApi
import json
from pathlib import Path

base = "openai/whisper-large-v3"
experts = ["LocalAI-io/whisper-large-v3-it", "LocalAI-io/whisper-large-v3-it-yodas-only", "LocalAI-io/whisper-large-v3-it-multi-yodas-asr_only-nocv"]
api = HfApi()
revs = {m: api.model_info(m).sha for m in [base, *experts]}
def load(m):
    return WhisperForConditionalGeneration.from_pretrained(m, revision=revs[m], torch_dtype=torch.float16, use_safetensors=True, low_cpu_mem_usage=True)
with torch.no_grad():
    model = load(base)
    original = model.state_dict()
    # Accumulate one expert at a time, keeping only FP32 deltas.
    delta = {k: torch.zeros_like(v, dtype=torch.float32) for k,v in original.items() if v.is_floating_point()}
    for expert in experts:
        other = load(expert)
        state = other.state_dict()
        assert state.keys() == original.keys()
        for k,v in original.items():
            assert state[k].shape == v.shape, k
            if k in delta:
                d = state[k].float() - v.float()
                assert torch.isfinite(d).all(), k
                delta[k].add_(d, alpha=1/len(experts))
            else:
                assert torch.equal(state[k],v), k
        del state, other
        print("MERGED", expert, flush=True)
    # Build independent state dict to avoid updating tied weights twice.
    merged = {k: (v.float()+delta[k]).to(v.dtype) if k in delta else v for k,v in original.items()}
    model.load_state_dict(merged, strict=True)
    del merged, delta
    model.generation_config.language = "it"
    model.generation_config.task = "transcribe"
    model.generation_config.forced_decoder_ids = None
    out = Path("fusion-output")
    model.save_pretrained(out, safe_serialization=True)
    AutoProcessor.from_pretrained(base, revision=revs[base]).save_pretrained(out)
    (out/"fusion_manifest.json").write_text(json.dumps({"method":"equal task arithmetic","revisions":revs,"language":"it","validated":False},indent=2))
    print("FUSION_SAVED", flush=True)
