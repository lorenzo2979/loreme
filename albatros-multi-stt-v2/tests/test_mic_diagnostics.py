import math
import struct
import unittest
from mic_diagnostics import diagnose
from wyoming_server import Audio


def tone(seconds=1.0, amplitude=3000):
    return Audio(b"".join(struct.pack("<h",int(amplitude*math.sin(2*math.pi*200*i/16000)))
                          for i in range(int(seconds*16000))), 16000, 2, 1)


class MicDiagnosticsTests(unittest.TestCase):
    def test_valid_audio(self):
        result = diagnose(tone())
        self.assertEqual(result.status, "ok")
        self.assertGreater(result.peak_dbfs, -30)
        self.assertEqual(result.duration_s, 1.0)

    def test_silence(self):
        result = diagnose(Audio(b"\x00\x00" * 16000, 16000, 2, 1))
        self.assertEqual(result.status, "near_silence")

    def test_clipping(self):
        result = diagnose(Audio(struct.pack("<h", 32767) * 16000, 16000, 2, 1))
        self.assertEqual(result.status, "clipping")

    def test_quiet(self):
        self.assertEqual(diagnose(tone(amplitude=600)).status, "quiet")

    def test_bad_format(self):
        self.assertEqual(diagnose(Audio(b"1234", 8000, 2, 1)).status, "unsupported_format")

    def test_boundary_risk_not_certainty(self):
        # Loud ending may indicate truncation but is not definitive.
        silence = b"\x00\x00" * 14400
        loud = struct.pack("<h", 10000) * 1600
        result = diagnose(Audio(silence + loud, 16000, 2, 1))
        self.assertTrue(result.possible_cut_end)
        self.assertTrue(any("possibile taglio" in x for x in result.suggestions))


if __name__ == "__main__":
    unittest.main()
