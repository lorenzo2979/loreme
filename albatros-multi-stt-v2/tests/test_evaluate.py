import unittest
from evaluate import score

class EvaluationTests(unittest.TestCase):
    def test_consensus_and_abstention(self):
        result=score([
            {"expected":"accendi luce cucina","outputs":[
                {"engine":"speech_to_phrase","text":"Accendi luce cucina"},
                {"engine":"whisper_local","text":"accendi luce cucina"}]},
            {"expected":"spegni luce cucina","outputs":[
                {"engine":"speech_to_phrase","text":"accendi luce cucina"},
                {"engine":"whisper_local","text":"spegni luce cucina"}]}
        ])
        self.assertEqual(result["total"],2)
        self.assertEqual(result["correct"],1)
        self.assertEqual(result["abstentions"],1)
        self.assertEqual(result["wrong_commands"],0)
    def test_single_model_abstains(self):
        result=score([{"expected":"metti Vasco Rossi","outputs":[
            {"engine":"speech_to_phrase","text":"metti vasco rossi"}]}])
        self.assertEqual(result["accepted"],0)
