import math
import struct
import unittest
from unittest.mock import patch

from agent import Result
from supervisor import decide, inspect_audio
from wyoming_server import Audio
from engine_runner import run


def sample_pcm(seconds=1, amp=2000):
    count = int(16000 * seconds)
    return b"".join(struct.pack("<h", int(amp*math.sin(2*math.pi*440*i/16000)))
                    for i in range(count))


class SupervisorTests(unittest.TestCase):
    def test_clean_audio(self):
        self.assertEqual(inspect_audio(Audio(sample_pcm(), 16000, 2, 1)), "ok")

    def test_short_audio(self):
        self.assertEqual(inspect_audio(Audio(sample_pcm(.1), 16000, 2, 1)), "too_short")

    def test_silent_audio(self):
        self.assertEqual(inspect_audio(Audio(b"\x00\x00"*16000, 16000, 2, 1)), "near_silence")

    def test_clipping(self):
        self.assertEqual(inspect_audio(Audio(struct.pack("<h",32767)*16000,16000,2,1)),
                         "clipping")

    def test_bad_format(self):
        self.assertEqual(inspect_audio(Audio(b"\x00"*100, 8000, 2, 1)),
                         "unsupported_format")

    def test_agree_reuse_original(self):
        original = "Accendi la luce in cucina."
        v = decide([Result("speech_to_phrase", original),
                    Result("whisper_local", "accendi la luce in cucina")])
        self.assertEqual(v.status, "accepted")
        self.assertEqual(v.text, original)

    def test_opposites_never_accepted(self):
        v = decide([Result("speech_to_phrase", "accendi luce"),
                    Result("whisper_local", "spegni luce"),
                    Result("gemini", "accendi luce")])
        self.assertEqual(v.status, "retry")
        self.assertEqual(v.reason, "critical_disagreement")

    def test_only_one_engine_fails_closed(self):
        self.assertEqual(decide([Result("whisper_local", "ciao")]).reason,
                         "insufficient_independent_engines")

    def test_no_outputs(self):
        self.assertEqual(decide([]).reason, "no_transcript")

    def test_uncertain_phrase(self):
        v = decide([Result("whisper_local", "metti Vasco Rossi"),
                    Result("gemini", "metti basco rossi")])
        self.assertEqual(v.status, "retry")

    def test_pipeline_routes_through_supervisor(self):
        async def stp(audio):
            return "Accendi la luce in cucina"
        def wh(audio):
            return "accendi la luce in cucina"
        async def execute():
            with patch("engine_runner.speech_to_phrase", stp), \
                 patch("engine_runner.whisper_local", wh), \
                 patch.dict("os.environ", {"ENABLE_WHISPER": "true",
                                          "ENABLE_GROQ": "false", "ENABLE_GEMINI": "false"},
                            clear=True):
                return await run(Audio(sample_pcm(), 16000, 2, 1))
        import asyncio
        self.assertEqual(asyncio.run(execute()), "Accendi la luce in cucina")


if __name__ == "__main__":
    unittest.main()
