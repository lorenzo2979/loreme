import unittest
from italian_quality import analyze, edit_distance, score_dataset, tokens
from engine_runner import whisper_options
from unittest.mock import patch


class ItalianQualityTests(unittest.TestCase):
    def test_apostrophes_and_accents(self):
        self.assertEqual(tokens("L’acqua è CALDA!"), ["l'acqua", "è", "calda"])

    def test_word_edits(self):
        self.assertEqual(edit_distance(["accendi", "luce"], ["spegni", "luce"])["errors"], 1)
        self.assertEqual(edit_distance([], ["ciao"])["insertions"], 1)
        self.assertEqual(edit_distance(["ciao"], [])["deletions"], 1)

    def test_exact_match(self):
        m = analyze("Accendi la luce.", "accendi la luce")
        self.assertEqual(m["wer"], 0)
        self.assertEqual(m["cer"], 0)
        self.assertTrue(m["exact_match"])

    def test_opposite_command_is_critical(self):
        m = analyze("Accendi la luce", "Spegni la luce")
        self.assertGreater(m["wer"], 0)
        self.assertIn("opposti:accendi/spegni", m["critical_differences"])

    def test_negation_and_number(self):
        m = analyze("Non impostare 20 gradi", "impostare 30 gradi")
        self.assertIn("negazione", m["critical_differences"])
        self.assertIn("cifre", m["critical_differences"])

    def test_dataset_and_missing_hypotheses(self):
        rows = [
            {"reference": "Apri la porta", "hypotheses": {"whisper": "apri la porta"}},
            {"reference": "Chiudi la porta", "hypotheses": {"whisper": "apri la porta"}},
        ]
        score = score_dataset(rows)["whisper"]
        self.assertEqual(score["samples"], 2)
        self.assertEqual(score["words"], 6)
        self.assertEqual(score["word_errors"], 1)
        self.assertEqual(score["critical_cases"], 1)

    def test_no_fake_result_from_empty_dataset(self):
        with self.assertRaises(ValueError):
            score_dataset([])

    def test_italian_whisper_options(self):
        with patch.dict("os.environ", {}, clear=True):
            values = whisper_options()
            self.assertEqual(values["language"], "it")
            self.assertEqual(values["beam_size"], 5)
            self.assertNotIn("hotwords", values)
        with patch.dict("os.environ", {"WHISPER_ITALIAN_HINTS": "Vasco Rossi, Messina",
                                      "WHISPER_BEAM_SIZE": "6"}, clear=True):
            options = whisper_options()
            self.assertEqual(options["beam_size"], 6)
            self.assertEqual(options["hotwords"], "Vasco Rossi, Messina")
        with patch.dict("os.environ", {"WHISPER_BEAM_SIZE": "0"}, clear=True):
            with self.assertRaises(ValueError):
                whisper_options()


if __name__ == "__main__":
    unittest.main()
