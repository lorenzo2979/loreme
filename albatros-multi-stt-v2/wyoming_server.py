"""Wyoming front-end: one audio stream in, exactly one transcript out.
Backends are supplied by an injected async transcriber; no Home Assistant API access.
"""
import asyncio
import json
import logging
import os
from dataclasses import dataclass

MAX_BYTES = 8 * 1024 * 1024
MAX_HEADER = 65536
LOG = logging.getLogger("albatros.wyoming")

@dataclass(frozen=True)
class Audio:
    pcm: bytes
    rate: int
    width: int
    channels: int

async def read_event(reader):
    line = await reader.readline()
    if not line:
        return None
    if len(line) > MAX_HEADER or not line.endswith(b"\n"):
        raise ValueError("Invalid/oversized Wyoming header")
    header = json.loads(line)
    n = int(header.get("payload_length", 0))
    m = int(header.get("data_length", 0))
    if n < 0 or n > MAX_BYTES or m < 0 or m > MAX_HEADER:
        raise ValueError("Wyoming payload too large")
    if m:
        header["data"] = {**header.get("data", {}), **json.loads(await reader.readexactly(m))}
    return header, (await reader.readexactly(n) if n else b"")

async def write_event(writer, typ, data=None):
    hdr = {"type": typ}
    if data is not None:
        hdr["data"] = data
    writer.write((json.dumps(hdr, ensure_ascii=False) + "\n").encode())
    await writer.drain()

INFO = {"asr":[{"models":[{"name":"ALBATROS Multi-STT","languages":["it"],
    "description":"Multi-engine transcription with conservative validation",
    "attribution":{"name":"ALBATROS","url":"https://github.com/lorenzo2979/loreme"},
    "installed":True}],"supports_transcript_streaming":False,
    "requires_external_vad":True}]}

async def session(reader, writer, transcribe):
    """transcribe(Audio) -> str. This function never executes smart-home commands."""
    pcm = bytearray()
    rate = width = channels = None
    active = False
    try:
        while True:
            event = await asyncio.wait_for(read_event(reader), timeout=90)
            if event is None:
                return
            head, payload = event
            typ = head.get("type")
            data = head.get("data") or {}
            if typ == "describe":
                await write_event(writer, "info", INFO)
            elif typ == "audio-start":
                rate, width, channels = (int(data[k]) for k in ("rate","width","channels"))
                if rate != 16000 or width != 2 or channels != 1:
                    raise ValueError("This prototype accepts PCM16/16kHz mono")
                pcm.clear()
                active = True
            elif typ == "audio-chunk":
                if not active:
                    raise ValueError("audio-start required")
                if any(int(data.get(k,v)) != v for k,v in
                       (("rate",rate),("width",width),("channels",channels))):
                    raise ValueError("Audio format changed")
                if len(pcm) + len(payload) > MAX_BYTES:
                    raise ValueError("Audio limit exceeded")
                pcm.extend(payload)
            elif typ == "audio-stop":
                result = ""
                if active and pcm:
                    result = await transcribe(Audio(bytes(pcm),rate,width,channels))
                await write_event(writer,"transcript",{"text":result or "", "language":"it"})
                pcm.clear()
                active = False
            elif typ in ("transcribe","select-program"):
                pass
    except (ConnectionError, asyncio.TimeoutError, asyncio.IncompleteReadError):
        LOG.info("Client left")
    except Exception as exc:
        LOG.warning("session rejected: %s", type(exc).__name__)
    finally:
        writer.close()
        await writer.wait_closed()

async def serve(transcribe, host="127.0.0.1", port=10350):
    server = await asyncio.start_server(lambda r,w:session(r,w,transcribe),host,port)
    async with server:
        await server.serve_forever()
