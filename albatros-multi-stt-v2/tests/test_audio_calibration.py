import math
import struct
import unittest

from audio_calibration import calibrate
from wyoming_server import Audio


def audio_from_signal(values):
    return Audio(b"".join(struct.pack("<h", int(v)) for v in values), 16000, 2, 1)


def sine(amp=2000, seconds=1, offset=0):
    return audio_from_signal([offset + amp*math.sin(2*math.pi*220*i/16000)
                              for i in range(int(16000*seconds))])


class AudioCalibrationTests(unittest.TestCase):
    def test_keeps_original_and_separate_processed(self):
        a = sine(2000)
        out = calibrate(a)
        self.assertEqual(out.original.pcm, a.pcm)
        self.assertEqual(len(out.processed.pcm), len(a.pcm))
        self.assertNotEqual(out.processed.pcm, a.pcm)
        self.assertLess(out.after.clipped_pct, 1)

    def test_reduces_dc_offset(self):
        result = calibrate(sine(2500, offset=4000))
        self.assertLess(abs(result.after.dc_offset), abs(result.before.dc_offset))

    def test_clipping_not_repaired(self):
        a = audio_from_signal([32767]*16000)
        result = calibrate(a)
        self.assertEqual(result.processed.pcm, a.pcm)
        self.assertEqual(result.after.action, "clipped_unmodified")

    def test_silence_not_boosted(self):
        a = audio_from_signal([0]*16000)
        result = calibrate(a)
        self.assertEqual(result.processed.pcm, a.pcm)
        self.assertEqual(result.after.action, "near_silence_unmodified")

    def test_does_not_create_clipping(self):
        a = sine(22000)
        result = calibrate(a)
        self.assertLess(result.after.peak_dbfs, -0.9)

    def test_invalid_format(self):
        a = Audio(b"1234",8000,2,1)
        with self.assertRaises(ValueError):
            calibrate(a)

    def test_invalid_config(self):
        with self.assertRaises(ValueError):
            calibrate(sine(),target_rms_dbfs=0)


if __name__ == "__main__":
    unittest.main()
