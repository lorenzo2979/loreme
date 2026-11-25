import unittest
import numpy as np
from wyoming_server import Audio
from audio_supervisor import Hearing,FeedbackBook,select,guard,profile,plan

def audio(x):
    return Audio(np.rint(np.clip(x,-1,32767/32768)*32768).astype("<i2").tobytes(),16000,2,1)

class SupervisorTests(unittest.TestCase):
    def setUp(self):
        t=np.arange(16000)/16000
        self.x=np.random.default_rng(18).normal(0,.08,len(t))*(.6+.4*np.sin(2*np.pi*3*t)**2)
        self.a=audio(self.x)
        self.k=0

    def filt(self,a,e):
        self.k+=1
        return audio(self.x*(.95-self.k*.05))

    def test_original_preserved_on_same_transcript(self):
        d=select(self.a,lambda a:Hearing("accendi luce",-.3,True),filter_fn=self.filt,vad_fn=lambda a:[(0,16000)])
        self.assertEqual(d.selected,"original")
        self.assertIs(d.original,self.a)

    def test_corroborated_alternative(self):
        def rec(a): return Hearing("questa prova" if a.pcm==self.a.pcm else "questa frase",-.9 if a.pcm==self.a.pcm else -.3,True)
        d=select(self.a,rec,filter_fn=self.filt,vad_fn=lambda a:[(0,16000)])
        self.assertNotEqual(d.selected,"original")
        self.assertEqual(d.reason,"corroborated_alternative")

    def test_critical_guard(self):
        def rec(a): return Hearing("accendi luce" if a.pcm==self.a.pcm else "spegni luce",-.9 if a.pcm==self.a.pcm else -.3,True)
        d=select(self.a,rec,filter_fn=self.filt,vad_fn=lambda a:[(0,16000)])
        self.assertEqual(d.selected,"original")

    def test_failed_filter_and_learning(self):
        def fail(a,e): raise RuntimeError("missing")
        d=select(self.a,lambda a:Hearing("ciao",-.4,True),filter_fn=fail,vad_fn=lambda a:[(0,16000)])
        self.assertEqual(d.selected,"original")
        self.assertTrue(d.errors)
        with self.assertRaises(ValueError): FeedbackBook().observe(d,"ciao")

    def test_single_supported_alternative(self):
        p=profile(self.a)
        first=plan(p)[0]
        key=f"{p.label}|{first[0]}:{int(first[1]*100)}"
        book=FeedbackBook({key:{"n":12,"wins":4,"losses":0,"critical":0,"delta":4}})
        def rec(a): return Hearing("questa prova" if a.pcm==self.a.pcm else "questa frase",-.7 if a.pcm==self.a.pcm else -.4,True)
        d=select(self.a,rec,book=book,budget=1,filter_fn=self.filt,vad_fn=lambda a:[(0,16000)])
        self.assertEqual(d.reason,"verified_policy_alternative")

    def test_weak_single_stays_original(self):
        def rec(a): return Hearing("questa prova" if a.pcm==self.a.pcm else "questa frase",-.5 if a.pcm==self.a.pcm else -.48,True)
        d=select(self.a,rec,budget=1,filter_fn=self.filt,vad_fn=lambda a:[(0,16000)])
        self.assertEqual(d.selected,"original")

    def test_guard_rejects_erasure(self):
        self.assertIsNone(guard(self.a,audio(np.zeros_like(self.x)),1,[(0,16000)])[0])
