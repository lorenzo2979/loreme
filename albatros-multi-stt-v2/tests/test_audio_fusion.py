import unittest
import numpy as np
from wyoming_server import Audio
from audio_filter_bank import samples
from audio_fusion import aligned,mix_candidates

def audio(x):
    return Audio(np.asarray(np.rint(x*32768),dtype="<i2").tobytes(),16000,2,1)

class FusionTests(unittest.TestCase):
    def setUp(self):
        self.x=np.random.default_rng(42).normal(0,.03,16000)
    def test_identity(self):
        a=audio(self.x)
        variants,meta=mix_candidates(a,a)
        self.assertEqual(meta["lag"],0)
        self.assertEqual(len(variants),3)
        self.assertTrue(all(v.pcm==a.pcm for v in variants.values()))
    def test_delay_and_edges(self):
        a=audio(self.x)
        delayed=audio(np.r_[np.zeros(160),self.x[:-160]])
        z,lag,confidence=aligned(a,delayed)
        self.assertEqual(lag,160)
        self.assertGreater(confidence,.99)
        np.testing.assert_array_equal(z,samples(a))
    def test_no_false_alignment(self):
        a=audio(self.x)
        b=audio(np.random.default_rng(19).normal(0,.03,16000))
        variants,meta=mix_candidates(a,b)
        self.assertEqual(variants,{})
        self.assertEqual(meta["status"],"alignment_rejected")
    def test_mixing_weights(self):
        a=audio(self.x)
        b=audio(self.x*.5)
        variants,_=mix_candidates(a,b)
        np.testing.assert_allclose(samples(variants["mix50"]),samples(a)*.75,atol=2/32768)
        self.assertTrue(all(len(v.pcm)==len(a.pcm) for v in variants.values()))
