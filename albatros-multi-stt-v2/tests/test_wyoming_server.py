import asyncio
import json
import unittest
from wyoming_server import session, INFO

def msg(typ,data=None,payload=b""):
    header={"type":typ}
    if data is not None: header["data"]=data
    if payload: header["payload_length"]=len(payload)
    return (json.dumps(header)+"\n").encode()+payload

class ProtocolTests(unittest.IsolatedAsyncioTestCase):
    def test_info_protocol_attribution(self):
        model = INFO["asr"][0]["models"][0]
        self.assertEqual(model["languages"], ["it"])
        self.assertTrue(model["attribution"]["name"])
        self.assertTrue(model["attribution"]["url"].startswith("https://"))

    async def test_stream_once_and_describe(self):
        called=[]
        async def recognize(audio):
            called.append(audio)
            return "accendi luce cucina"
        async def client(reader,writer):
            await session(reader,writer,recognize)
        server=await asyncio.start_server(client,"127.0.0.1",0)
        try:
            host,port=server.sockets[0].getsockname()
            reader,writer=await asyncio.open_connection(host,port)
            fmt={"rate":16000,"width":2,"channels":1}
            writer.write(msg("describe")+msg("transcribe",{"language":"it"})+
                msg("audio-start",fmt)+msg("audio-chunk",fmt,b"\x00\x00"*160)+
                msg("audio-stop"))
            await writer.drain()
            info=json.loads(await asyncio.wait_for(reader.readline(),3))
            out=json.loads(await asyncio.wait_for(reader.readline(),3))
            self.assertEqual(info["type"],"info")
            self.assertEqual(out["data"]["text"],"accendi luce cucina")
            self.assertEqual(len(called),1)
            self.assertEqual(len(called[0].pcm),320)
            writer.close()
            await writer.wait_closed()
        finally:
            server.close()
            await server.wait_closed()
    async def test_empty_audio_never_invokes_engine(self):
        count=0
        async def recognize(audio):
            nonlocal count
            count+=1
            return "not expected"
        server=await asyncio.start_server(lambda r,w:session(r,w,recognize),"127.0.0.1",0)
        try:
            host,port=server.sockets[0].getsockname()
            reader,writer=await asyncio.open_connection(host,port)
            fmt={"rate":16000,"width":2,"channels":1}
            writer.write(msg("audio-start",fmt)+msg("audio-stop"))
            await writer.drain()
            out=json.loads(await asyncio.wait_for(reader.readline(),3))
            self.assertEqual(out["data"]["text"],"")
            self.assertEqual(count,0)
            writer.close()
            await writer.wait_closed()
        finally:
            server.close()
            await server.wait_closed()
