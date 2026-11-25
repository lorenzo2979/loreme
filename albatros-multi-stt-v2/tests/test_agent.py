import unittest
from agent import Result, judge, conflicts, normalize

class AgentTests(unittest.TestCase):
    def test_normalization(self):
        self.assertEqual(normalize("  ACCENDI, la LUCE! "), "accendi la luce")
    def test_single_engine_abstains(self):
        self.assertIsNone(judge([Result("speech_to_phrase","accendi la luce")]))
    def test_agreement_prefers_primary(self):
        result = judge([
            Result("speech_to_phrase","accendi la luce cucina"),
            Result("whisper_local","Accendi la luce cucina")])
        self.assertEqual(result.engine, "speech_to_phrase")
    def test_opposites_reject(self):
        self.assertTrue(conflicts("accendi luce", "spegni luce"))
        self.assertIsNone(judge([
            Result("speech_to_phrase","accendi luce"),
            Result("whisper_local","spegni luce")]))
    def test_numbers_reject(self):
        self.assertTrue(conflicts("imposta 20 gradi", "imposta 30 gradi"))
    def test_no_agreement(self):
        self.assertIsNone(judge([
            Result("speech_to_phrase","apri garage"),
            Result("whisper_local","metti Vasco Rossi")]))
if __name__ == "__main__":
    unittest.main()
