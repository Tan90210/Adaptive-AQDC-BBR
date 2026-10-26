"""Stage 1 protects sample semantics, measurement windows and episode validation."""
import tempfile
import unittest
from pathlib import Path
import numpy as np
from stage1_fixed_coefficients import read_stage1_rtt, queue_time_mean, validate_events

class Stage1AnalysisTest(unittest.TestCase):
    def test_raw_reader_preserves_repeated_equal_samples(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "rtt.dat"
            path.write_text("# stage1-rtt-v1 time_s raw_estimator_rtt_ms smoothed_rtt_ms\n"
                            "# controller=ns3::TcpBbr coefficient=1 stop_s=120 start_s=0.1\n"
                            "0.2 50 50\n0.3 50 50\n0.4 100 56.25\n")
            data, meta = read_stage1_rtt(path)
            self.assertEqual(len(data), 3)
            self.assertEqual(meta["controller"], "ns3::TcpBbr")
            self.assertAlmostEqual(np.mean(data[:, 1]), 200/3)

    def test_legacy_smoothed_rtt_cannot_be_used_as_raw(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "rtt.dat"
            path.write_text("0.2 50\n0.3 60\n")
            with self.assertRaisesRegex(ValueError, "schema"):
                read_stage1_rtt(path)

    def test_invalid_raw_sample_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "rtt.dat"
            path.write_text("# stage1-rtt-v1 time_s raw_estimator_rtt_ms smoothed_rtt_ms\n"
                            "# controller=ns3::TcpBbr\n0.2 0 10\n")
            with self.assertRaises(ValueError):
                read_stage1_rtt(path)

    def test_queue_mean_clips_boundaries_and_weights_time(self):
        data = np.array([[0, 2], [0.2, 10], [0.5, 0], [1, 0]])
        self.assertAlmostEqual(queue_time_mean(data, 0.1, 0.8), 3.2/0.7)

    def test_queue_incomplete_window_rejected(self):
        with self.assertRaises(ValueError):
            queue_time_mean(np.array([[0.2, 0], [1, 1]]), 0.1, 0.8)

    def test_event_pair_duration_and_inflight(self):
        log = ("AQDC entry time_s=10 debt_bytes=20 coefficient=1.5 target_bytes=50 inflight_bytes=80\n"
               "AQDC complete time_s=10.02 debt_bytes=20 coefficient=1.5 target_bytes=50 inflight_bytes=49\n")
        episodes, pending = validate_events(log)
        self.assertIsNone(pending)
        self.assertAlmostEqual(episodes[0]["duration_ms"], 20)
        with self.assertRaisesRegex(ValueError, "above target"):
            validate_events(log.replace("inflight_bytes=49", "inflight_bytes=51"))
        with self.assertRaisesRegex(ValueError, "snapshot"):
            validate_events(log.replace("time_s=10.02 debt_bytes=20", "time_s=10.02 debt_bytes=21"))

    def test_missing_event_lines_cannot_hide_summary_episodes(self):
        with self.assertRaisesRegex(ValueError, "counts"):
            validate_events("AQDC summary debt_increases=2 entries=1 completions=1")

    def test_unfinished_episode_is_reported(self):
        episodes, pending = validate_events("AQDC entry time_s=10 debt_bytes=20 coefficient=1 target_bytes=50 inflight_bytes=80")
        self.assertEqual(episodes, [])
        self.assertIsNotNone(pending)

if __name__ == "__main__":
    unittest.main()
