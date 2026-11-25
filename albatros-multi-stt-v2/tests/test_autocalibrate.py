import unittest
from dataclasses import replace
from autocalibrate import AutoCalibrator, MicSettings
from mic_diagnostics import MicDiagnostic


def observation(status="ok", start=False, end=False):
    return MicDiagnostic(status, 1.0, -8, -22, 0.0, start, end, ())


class AutoCalibrationTests(unittest.TestCase):
    def test_requires_repeated_evidence(self):
        self.assertIsNone(AutoCalibrator().propose([observation("quiet")]))

    def test_gain_up_after_multiple_quiet_clips(self):
        agent = AutoCalibrator()
        adjustment = agent.propose([observation("quiet")]*3)
        self.assertEqual(adjustment.name, "gain_db")
        self.assertEqual(agent.candidate().gain_db, 2.0)
        self.assertEqual(agent.settings.gain_db, 0.0)
        self.assertEqual(agent.evaluate(7, 4, 0, 0), "accepted")
        self.assertEqual(agent.settings.gain_db, 2.0)

    def test_no_change_without_improvement(self):
        agent = AutoCalibrator()
        agent.propose([observation("clipping")]*3)
        self.assertEqual(agent.evaluate(4, 5, 0, 0), "rolled_back")
        self.assertEqual(agent.settings.gain_db, 0.0)

    def test_pre_roll_only_on_repeated_cut(self):
        agent = AutoCalibrator()
        trial = agent.propose([observation(start=True)]*3)
        self.assertEqual(trial.name, "pre_roll_ms")
        self.assertEqual(trial.new_value, 500)

    def test_end_silence(self):
        agent = AutoCalibrator()
        trial = agent.propose([observation(end=True)]*3)
        self.assertEqual(trial.name, "end_silence_ms")
        self.assertEqual(trial.new_value, 850)

    def test_never_tunes_bad_audio(self):
        agent = AutoCalibrator()
        self.assertIsNone(agent.propose([observation("invalid_audio")]*3))

    def test_boundaries(self):
        agent = AutoCalibrator(MicSettings(gain_db=12.0))
        self.assertIsNone(agent.propose([observation("quiet")]*3))

    def test_critical_regression_blocks_change(self):
        agent = AutoCalibrator()
        agent.propose([observation("quiet")]*3)
        self.assertEqual(agent.evaluate(10, 2, 0, 1), "rolled_back")

    def test_cannot_stack_trials(self):
        agent = AutoCalibrator()
        agent.propose([observation("quiet")]*3)
        with self.assertRaises(RuntimeError):
            agent.propose([observation("quiet")]*3)


if __name__ == "__main__":
    unittest.main()
