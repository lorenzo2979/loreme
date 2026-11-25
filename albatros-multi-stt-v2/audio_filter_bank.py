"""Experimental multi-filter bank. Each filter sees original PCM.
Dependencies/models are explicit; no silent substitute for a missing engine.
No networking at inference, no physical device writes. Never deploy blindly.
"""
from dataclasses import dataclass
from functools import lru_cache
import ctypes
import ctypes.util
import os
from pathlib import Path
import subprocess
import tempfile
import wave
import numpy as np
from scipy.signal import resample_poly
from wyoming_server import Audio
from audio_agent import prepare
from audio_calibration import calibrate
from mic_diagnostics import diagnose

FILTERS=("albatros_v1","gtcrn","dpdfnet","deepfilternet","rnnoise","webrtc","ffmpeg","agc")
MODEL_DIR=Path(os.getenv("ALBATROS_AUDIO_MODELS","audio-models"))

@dataclass(frozen=True)
class Candidate:
    engine: str
    audio: Audio

def samples(audio):
    if (audio.rate,audio.width,audio.channels)!=(16000,2,1) or not 8000<=len(audio.pcm)<=960000 or len(audio.pcm)%2:
        raise ValueError("Expected 0.25–30 s PCM16 mono 16 kHz")
    return np.frombuffer(audio.pcm,dtype="<i2").astype(np.float32)/32768

def output(audio,y):
    x=samples(audio)
    y=np.asarray(y,dtype=np.float32)
    if y.ndim!=1 or len(y)!=len(x) or not np.all(np.isfinite(y)):
        raise ValueError("Invalid output length, shape or samples")
    if np.max(np.abs(y))>1.0:
        y=y/np.max(np.abs(y))*.98
    if np.max(np.abs(x))>.01 and np.max(np.abs(y))<1e-5:
        raise ValueError("Filter erased non-silent input")
    return Audio(np.rint(np.clip(y,-1,32767/32768)*32768).astype("<i2").tobytes(),16000,2,1)

def model_path(name):
    path=MODEL_DIR/name
    if not path.is_file():
        raise FileNotFoundError(path)
    return str(path)

@lru_cache(maxsize=2)
def sherpa_model(engine):
    import sherpa_onnx as s
    if engine=="gtcrn":
        kw={"gtcrn":s.OfflineSpeechDenoiserGtcrnModelConfig(model=model_path("gtcrn_simple.onnx"))}
    else:
        kw={"dpdfnet":s.OfflineSpeechDenoiserDpdfNetModelConfig(model=model_path("dpdfnet_baseline.onnx"),attenuation_limit_db=12.0)}
    config=s.OfflineSpeechDenoiserConfig(model=s.OfflineSpeechDenoiserModelConfig(**kw,num_threads=1,provider="cpu",debug=False))
    if not config.validate(): raise ValueError("Invalid denoiser config")
    return s.OfflineSpeechDenoiser(config)

def neural(audio,engine):
    result=sherpa_model(engine)(samples(audio),16000)
    if result.sample_rate!=16000: raise ValueError("Unexpected output sample rate")
    return output(audio,result.samples)

def write_wav(path,x,rate):
    with wave.open(str(path),"wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(rate)
        w.writeframes(np.rint(np.clip(x,-1,32767/32768)*32768).astype("<i2").tobytes())

def decode_wav(path):
    # ffmpeg handles PCM and floating point WAV from external tools.
    raw=subprocess.run(["ffmpeg","-nostdin","-v","error","-i",str(path),"-f","f32le","-ac","1","-ar","16000","pipe:1"],check=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=60).stdout
    return np.frombuffer(raw,dtype="<f4")

def deepfilter(audio):
    with tempfile.TemporaryDirectory() as tmp:
        base=Path(tmp); src=base/"input.wav"; dest=base/"out"; dest.mkdir()
        write_wav(src,resample_poly(samples(audio),3,1),48000)
        subprocess.run([os.getenv("DEEP_FILTER_BIN","deep-filter"),"--compensate-delay","--out-dir",str(dest),str(src)],check=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=90)
        files=list(dest.glob("*.wav"))
        if len(files)!=1: raise ValueError("Expected one DeepFilterNet output")
        return output(audio,decode_wav(files[0]))

def rnnoise(audio):
    lib=ctypes.CDLL(os.getenv("RNNOISE_LIB") or ctypes.util.find_library("rnnoise") or "librnnoise.so")
    lib.rnnoise_create.argtypes=[ctypes.c_void_p];lib.rnnoise_create.restype=ctypes.c_void_p
    lib.rnnoise_destroy.argtypes=[ctypes.c_void_p];lib.rnnoise_destroy.restype=None
    lib.rnnoise_get_frame_size.restype=ctypes.c_int
    ptr=ctypes.POINTER(ctypes.c_float)
    lib.rnnoise_process_frame.argtypes=[ctypes.c_void_p,ptr,ptr];lib.rnnoise_process_frame.restype=ctypes.c_float
    frame=lib.rnnoise_get_frame_size()
    if frame!=480: raise ValueError("Unexpected RNNoise frame size")
    x=resample_poly(samples(audio),3,1)*32768
    # Flush the one-frame overlap/add delay; preserve original duration.
    padded=np.pad(x,(0,(-len(x))%frame+frame)).astype(np.float32)
    y=np.empty_like(padded)
    st=lib.rnnoise_create(None)
    if not st: raise RuntimeError("RNNoise init failed")
    try:
        for i in range(0,len(padded),frame):
            inp=padded[i:i+frame];out=y[i:i+frame]
            lib.rnnoise_process_frame(st,out.ctypes.data_as(ptr),inp.ctypes.data_as(ptr))
    finally:
        lib.rnnoise_destroy(st)
    return output(audio,resample_poly(y[frame:frame+len(x)]/32768,1,3))

def webrtc(audio):
    # AEC explicitly disabled: no synchronized playback reference in Wyoming.
    with tempfile.TemporaryDirectory() as tmp:
        src=Path(tmp)/"in.wav"; dest=Path(tmp)/"out.wav"
        write_wav(src,samples(audio),16000)
        cmd=["gst-launch-1.0","-q","filesrc","location="+str(src),"!","wavparse","!","audioconvert","!","audio/x-raw,format=S16LE,rate=16000,channels=1,layout=interleaved","!","webrtcdsp","echo-cancel=false","noise-suppression=true","gain-control=true","!","wavenc","!","filesink","location="+str(dest)]
        subprocess.run(cmd,check=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=60)
        return output(audio,decode_wav(dest))

def ffmpeg_filter(audio):
    # Conservative broadband filter plus high-pass, not automatic dereverb.
    raw=subprocess.run(["ffmpeg","-nostdin","-v","error","-f","s16le","-ar","16000","-ac","1","-i","pipe:0","-af","highpass=f=70,afftdn=nr=8:nf=-40","-f","s16le","pipe:1"],input=audio.pcm,stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=True,timeout=60).stdout
    return output(audio,np.frombuffer(raw,dtype="<i2").astype(np.float32)/32768)

def apply_filter(audio,engine):
    samples(audio)
    if engine=="original": return audio
    if diagnose(audio).status not in ("ok","quiet"): return audio
    if engine=="albatros_v1": return prepare(audio).selected
    if engine=="agc": return calibrate(audio,max_gain_db=6).processed
    if engine in ("gtcrn","dpdfnet"): return neural(audio,engine)
    if engine=="rnnoise": return rnnoise(audio)
    if engine=="deepfilternet": return deepfilter(audio)
    if engine=="webrtc": return webrtc(audio)
    if engine=="ffmpeg": return ffmpeg_filter(audio)
    raise ValueError("Unknown engine")

def speech_segments(audio):
    """Silero reports regions only, never cuts or reconstructs words."""
    import sherpa_onnx as s
    x=samples(audio)
    config=s.VadModelConfig()
    config.silero_vad.model=model_path("silero_vad.onnx")
    config.silero_vad.threshold=.5
    config.silero_vad.min_silence_duration=.25
    config.silero_vad.min_speech_duration=.15
    config.sample_rate=16000
    vad=s.VoiceActivityDetector(config,buffer_size_in_seconds=35)
    size=config.silero_vad.window_size
    padded=np.pad(x,(0,(-len(x))%size))
    spans=[]
    for i in range(0,len(padded),size):
        vad.accept_waveform(padded[i:i+size])
        while not vad.empty():
            seg=vad.front
            spans.append((int(seg.start),min(len(x),int(seg.start)+len(seg.samples))))
            vad.pop()
    vad.flush()
    while not vad.empty():
        seg=vad.front
        spans.append((int(seg.start),min(len(x),int(seg.start)+len(seg.samples))))
        vad.pop()
    return [(a,b) for a,b in spans if 0<=a<b<=len(x)]

def candidates(audio,engines=FILTERS):
    """Returns source plus successful variants and explicit errors."""
    variants=[Candidate("original",audio)]; errors={}
    for engine in engines:
        try:
            variants.append(Candidate(engine,apply_filter(audio,engine)))
        except Exception as exc:
            errors[engine]=type(exc).__name__+": "+str(exc)[:200]
    return variants,errors
