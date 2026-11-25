"""ALBATROS four-provider controller. All configuration is local, never HA API."""
import asyncio
import base64
import io
import json
import logging
import os
import urllib.request
import uuid
import wave
from agent import Result
from supervisor import inspect_audio, decide
from mic_diagnostics import diagnose
from wyoming_server import Audio, read_event, write_event

LOG = logging.getLogger("albatros.engines")
ORDER = ("speech_to_phrase", "whisper_local", "groq", "gemini")
_WHISPER = None

def env_on(name):
    return os.getenv(name,"false").lower() in ("1","true","yes")

def wav_bytes(audio):
    output=io.BytesIO()
    with wave.open(output,"wb") as target:
        target.setnchannels(audio.channels)
        target.setsampwidth(audio.width)
        target.setframerate(audio.rate)
        target.writeframes(audio.pcm)
    return output.getvalue()

async def speech_to_phrase(audio):
    """Connect ONLY to a child service on loopback of the same container."""
    host="127.0.0.1"
    port=int(os.getenv("LOCAL_STP_PORT","10351"))
    reader,writer=await asyncio.open_connection(host,port)
    try:
        await send_audio(writer,audio)
        while True:
            event=await asyncio.wait_for(read_event(reader),timeout=12)
            if event is None: return ""
            header,_=event
            if header.get("type")=="transcript":
                return str((header.get("data") or {}).get("text",""))
            if header.get("type")=="error": return ""
    finally:
        writer.close()
        await writer.wait_closed()

async def send_audio(writer,audio):
    await write_event(writer,"transcribe",{"language":"it"})
    await write_event(writer,"audio-start",{"rate":audio.rate,"width":audio.width,"channels":audio.channels})
    fmt={"rate":audio.rate,"width":audio.width,"channels":audio.channels}
    for i in range(0,len(audio.pcm),32768):
        payload=audio.pcm[i:i+32768]
        header={"type":"audio-chunk","data":fmt,"payload_length":len(payload)}
        writer.write((json.dumps(header)+"\n").encode()+payload)
        await writer.drain()
    await write_event(writer,"audio-stop")

def whisper_options():
    """Conservative Italian recognition settings; hints never replace audio."""
    beam = int(os.getenv("WHISPER_BEAM_SIZE", "5"))
    if not 1 <= beam <= 10:
        raise ValueError("WHISPER_BEAM_SIZE deve essere tra 1 e 10")
    hints = os.getenv("WHISPER_ITALIAN_HINTS", "").strip()
    if len(hints) > 300 or any(ord(c) < 32 for c in hints):
        raise ValueError("WHISPER_ITALIAN_HINTS non valido")
    options = {"language": "it", "beam_size": beam,
               "condition_on_previous_text": False, "vad_filter": True}
    if hints:
        options["hotwords"] = hints
    return options


def whisper_local(audio):
    global _WHISPER
    if not env_on("ENABLE_WHISPER"): return ""
    import numpy as np
    from faster_whisper import WhisperModel
    if _WHISPER is None:
        _WHISPER=WhisperModel(os.getenv("WHISPER_MODEL","base"),device="cpu",
            compute_type="int8",download_root="/models/whisper")
    samples=np.frombuffer(audio.pcm,dtype="<i2").astype(np.float32)/32768.0
    segments,_=_WHISPER.transcribe(samples,**whisper_options())
    return " ".join(x.text.strip() for x in segments)

def groq(audio):
    key=os.getenv("GROQ_API_KEY","")
    if not env_on("ENABLE_GROQ") or not key:return ""
    boundary="albatros"+uuid.uuid4().hex
    def part(name,data,filename=None):
        header=f'--{boundary}\r\nContent-Disposition: form-data; name="{name}"'
        if filename:header+=f'; filename="{filename}"\r\nContent-Type: audio/wav'
        return (header+"\r\n\r\n").encode()+data+b"\r\n"
    body=(part("model",b"whisper-large-v3-turbo")+part("language",b"it")
          +part("file",wav_bytes(audio),"audio.wav")+f"--{boundary}--\r\n".encode())
    req=urllib.request.Request("https://api.groq.com/openai/v1/audio/transcriptions",
        body,headers={"Authorization":"Bearer "+key,
            "Content-Type":"multipart/form-data; boundary="+boundary})
    with urllib.request.urlopen(req,timeout=18) as res:
        return json.load(res).get("text","")

def gemini(audio):
    key=os.getenv("GEMINI_API_KEY","")
    if not env_on("ENABLE_GEMINI") or not key:return ""
    import re
    model=os.getenv("GEMINI_MODEL","gemini-2.5-flash")
    if not re.fullmatch(r"[a-zA-Z0-9_.-]+",model):
        raise ValueError("Invalid Gemini model name")
    payload={"contents":[{"parts":[
        {"text":"Trascrivi esattamente l'audio italiano, senza aggiungere nulla. Solo testo."},
        {"inline_data":{"mime_type":"audio/wav",
             "data":base64.b64encode(wav_bytes(audio)).decode()}}]}],
        "generationConfig":{"temperature":0,"maxOutputTokens":160}}
    req=urllib.request.Request(
       "https://generativelanguage.googleapis.com/v1beta/models/"+model+":generateContent",
       json.dumps(payload).encode(),headers={"x-goog-api-key":key,
       "Content-Type":"application/json"})
    with urllib.request.urlopen(req,timeout=18) as res:
        response=json.load(res)
    parts=(response.get("candidates") or [{}])[0].get("content",{}).get("parts",[])
    return " ".join(p.get("text","") for p in parts if isinstance(p.get("text"),str))

async def run(audio):
    """Compare up to four transcriptions; fail closed with empty text if uncertain."""
    diagnostic = diagnose(audio)
    quality = inspect_audio(audio)
    if diagnostic.status != "ok" or diagnostic.possible_cut_start or diagnostic.possible_cut_end:
        LOG.info("microphone status=%s peak_dbfs=%s rms_dbfs=%s suggestions=%s",
                 diagnostic.status, diagnostic.peak_dbfs, diagnostic.rms_dbfs,
                 "; ".join(diagnostic.suggestions))
    if quality != "ok":
        LOG.info("audio rejected reason=%s", quality)
        return ""
    if env_on("ENABLE_AUDIO_AGENT"):
        from audio_agent import prepare
        preparation = await asyncio.to_thread(prepare, audio)
        audio = preparation.selected
        LOG.info("audio_agent decision=%s", preparation.reason)
    answers=[]
    for name in ORDER:
        if name=="whisper_local" and not env_on("ENABLE_WHISPER"):continue
        if name=="groq" and not env_on("ENABLE_GROQ"):continue
        if name=="gemini" and not env_on("ENABLE_GEMINI"):continue
        try:
            provider={"speech_to_phrase":speech_to_phrase,
                "whisper_local":whisper_local,"groq":groq,"gemini":gemini}[name]
            if name=="speech_to_phrase":
                text=await asyncio.wait_for(provider(audio),15)
            else:
                text=await asyncio.wait_for(asyncio.to_thread(provider,audio),75)
            if text and len(text.strip())>=3:
                answers.append(Result(name,text.strip()))
        except Exception as exc:
            LOG.warning("provider %s failed (%s)",name,type(exc).__name__)
    verdict = decide(answers)
    LOG.info("supervisor status=%s reason=%s engines=%d",
             verdict.status, verdict.reason, len(answers))
    return verdict.text if verdict.status == "accepted" else ""

async def main():
    from wyoming_server import serve
    logging.basicConfig(level=logging.INFO)
    if not env_on("ENABLE_WHISPER"):
        LOG.warning("Whisper disabled")
    LOG.warning("Speech-to-Phrase must be launched locally inside THIS container")
    await serve(run,host=os.getenv("LISTEN_HOST","0.0.0.0"),
                port=int(os.getenv("LISTEN_PORT","10350")))

if __name__=="__main__":
    asyncio.run(main())
