import unittest
from agent import Result
from fusion import fuse
from supervisor import decide


class FusionTests(unittest.TestCase):
    def test_identical_across_independent_families(self):
        outputs = [
            Result("speech_to_phrase", "metti brano vasco"),
            Result("whisper_local", "metti brano vasco"),
            Result("gemini", "metti brano vasco"),
        ]
        result = fuse(outputs)
        self.assertEqual(result.status, "accepted")
        self.assertEqual(result.text, "metti brano vasco")

    def test_two_whisper_deployments_are_not_independent(self):
        result = fuse([Result("whisper_local", "ciao mondo"),
                       Result("groq", "ciao mondo")])
        self.assertEqual(result.reason, "insufficient_independent_families")

    def test_mixed_word_fusion(self):
        values = [Result("speech_to_phrase", "riproduci marco rossi adesso"),
                  Result("whisper_local", "riproduci marco rossi subito"),
                  Result("gemini", "riproduci marco luca adesso")]
        result = fuse(values)
        self.assertEqual(result.status, "accepted")
        self.assertEqual(result.text, "riproduci marco rossi adesso")

    def test_single_word_uncertain(self):
        outputs = [Result("speech_to_phrase", "metti bosco"),
                   Result("whisper_local", "metti vasco"),
                   Result("gemini", "metti basco")]
        result = fuse(outputs)
        self.assertEqual(result.status, "retry")
        self.assertEqual(result.reason, "uncertain_words")
        self.assertEqual(result.disagreements, (1,))

    def test_inserted_or_missing_words_not_guessed(self):
        outputs = [Result("speech_to_phrase", "metti musica"),
                   Result("gemini", "metti la musica")]
        self.assertEqual(fuse(outputs).reason, "word_alignment_ambiguous")

    def test_critical_disagreement(self):
        outputs = [Result("speech_to_phrase", "accendi la luce"),
                   Result("whisper_local", "spegni la luce"),
                   Result("gemini", "accendi la luce")]
        self.assertEqual(fuse(outputs).reason, "critical_disagreement")
        self.assertEqual(decide(outputs).reason, "critical_disagreement")

    def test_supervisor_uses_fusion(self):
        values = [Result("speech_to_phrase", "riproduci marco rossi adesso"),
                  Result("whisper_local", "riproduci marco rossi subito"),
                  Result("gemini", "riproduci marco luca adesso")]
        result = decide(values)
        self.assertEqual(result.status, "accepted")
        self.assertEqual(result.reason, "word_level_fusion")
        self.assertEqual(result.text, "riproduci marco rossi adesso")


if __name__ == "__main__":
    unittest.main()
