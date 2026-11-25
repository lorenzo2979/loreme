import math
import unittest
import numpy as np
from wyoming_server import Audio
from audio_agent import prepare

def make(x):
    return Audio(np.rint(np.clip(x,-1,.9999)*32768).astype("<i2").tobytes(),16000,2,1)

class TestAudioAgent(unittest.TestCase):
    def test_silence_unchanged(self):
        audio=make(np.zeros(16000))
        self.assertIs(prepare(audio).selected,audio)

    def test_saturation_unchanged(self):
        audio=make(np.ones(16000))
        self.assertIs(prepare(audio).selected,audio)

    def test_clean_signal_unchanged(self):
        t=np.arange(16000)/16000
        audio=make(.15*np.sin(2*np.pi*220*t))
        self.assertIs(prepare(audio).selected,audio)

    def test_quiet_signal_gain(self):
        t=np.arange(16000)/16000
        audio=make(.015*np.sin(2*np.pi*220*t))
        out=prepare(audio)
        self.assertIn("calibrated",out.reason)
        self.assertEqual(len(out.selected.pcm),len(audio.pcm))
        self.assertIs(out.original,audio)

    def test_noise_candidate_preserves_length_and_source(self):
        rng=np.random.default_rng(25)
        t=np.arange(32000)/16000
        envelope=np.where((t>.4)&(t<1.6),.16,0.)
        audio=make(envelope*np.sin(2*np.pi*220*t)+rng.normal(0,.012,len(t)))
        out=prepare(audio)
        self.assertIs(out.original,audio)
        self.assertEqual(len(out.selected.pcm),len(audio.pcm))
        self.assertEqual(out.reason,"denoised")
        self.assertEqual((out.selected.rate,out.selected.width,out.selected.channels),(16000,2,1))
