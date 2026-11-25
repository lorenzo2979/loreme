import asyncio
import os
import unittest
from unittest.mock import patch, AsyncMock
from wyoming_server import Audio
import engine_runner

AUDIO=Audio(pcm=b"\x00\x00"*320,rate=16000,width=2,channels=1)

class TestEngineRunner(unittest.IsolatedAsyncioTestCase):
    async def test_two_matching_texts_win_once(self):
        with patch.dict(os.environ,{"ENABLE_WHISPER":"true","ENABLE_GROQ":"false","ENABLE_GEMINI":"false"}):
            with patch.object(engine_runner,"speech_to_phrase",new_callable=AsyncMock,return_value="accendi luce cucina") as stp:
                with patch.object(engine_runner,"whisper_local",return_value="accendi luce cucina") as whisper:
                    out=await engine_runner.run(AUDIO)
                    self.assertEqual(out,"accendi luce cucina")
                    stp.assert_awaited_once()
                    whisper.assert_called_once()
    async def test_opposite_commands_fail_closed(self):
        with patch.dict(os.environ,{"ENABLE_WHISPER":"true","ENABLE_GROQ":"false","ENABLE_GEMINI":"false"}):
            with patch.object(engine_runner,"speech_to_phrase",new_callable=AsyncMock,return_value="accendi luce cucina"):
                with patch.object(engine_runner,"whisper_local",return_value="spegni luce cucina"):
                    self.assertEqual(await engine_runner.run(AUDIO),"")
    async def test_single_stt_cannot_execute(self):
        with patch.dict(os.environ,{"ENABLE_WHISPER":"false","ENABLE_GROQ":"false","ENABLE_GEMINI":"false"}):
            with patch.object(engine_runner,"speech_to_phrase",new_callable=AsyncMock,return_value="spegni cucina"):
                self.assertEqual(await engine_runner.run(AUDIO),"")
    async def test_cloud_off_does_not_call_it(self):
        with patch.dict(os.environ,{"ENABLE_WHISPER":"false","ENABLE_GROQ":"false","ENABLE_GEMINI":"false"}):
            with patch.object(engine_runner,"groq") as groq:
                with patch.object(engine_runner,"gemini") as gemini:
                    await engine_runner.run(AUDIO)
                    groq.assert_not_called()
                    gemini.assert_not_called()
    async def test_bad_audio_fails_closed(self):
        malformed=Audio(pcm=b"",rate=8000,width=2,channels=1)
        self.assertEqual(await engine_runner.run(malformed),"")
