"""Checks for the baseline reader's version selection and invalid-data handling."""

from pathlib import Path
import tempfile
import unittest

import numpy as np

from plot_baseline import TRACE_FILES, latest_baseline, read_throughput, read_trace


class BaselineReaderTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.addCleanup(self.temp.cleanup)

    def baseline(self, version):
        path = self.root / f"bbr-baseline-v{version}"
        path.mkdir()
        for name in TRACE_FILES:
            (path / name).write_text("0 1\n", encoding="ascii")
        return path

    def test_numeric_version_order_ignores_newer_incomplete_run(self):
        self.baseline(9)
        chosen = self.baseline(10)
        incomplete = self.baseline(11)
        (incomplete / "rtt.dat").write_text("", encoding="ascii")
        self.assertEqual(latest_baseline(self.root), chosen)

    def test_missing_results_report_clear_error(self):
        with self.assertRaisesRegex(FileNotFoundError, "No complete"):
            latest_baseline(self.root / "missing")

    def test_single_sample_trace_is_valid(self):
        path = self.root / "rtt.dat"
        path.write_text("0.14 50\n", encoding="ascii")
        self.assertEqual(read_trace(path).shape, (1, 2))

    def test_invalid_measurements_and_time_order_are_rejected(self):
        path = self.root / "queueSize.dat"
        for data in ("0 nan\n", "0 -1\n", "2 1\n1 2\n", "0 1 2\n"):
            with self.subTest(data=data):
                path.write_text(data, encoding="ascii")
                with self.assertRaises(ValueError):
                    read_trace(path)

    def test_legacy_throughput_units(self):
        path = self.root / "throughput.dat"
        path.write_text("0.3s 9.5 Mbps\n0.5s 10 Mbps\n", encoding="ascii")
        np.testing.assert_allclose(read_throughput(path), [[0.3, 9.5], [0.5, 10]])
        path.write_text("0.3s 9.5 Kbps\n", encoding="ascii")
        with self.assertRaisesRegex(ValueError, "Invalid throughput format"):
            read_throughput(path)


if __name__ == "__main__":
    unittest.main()
