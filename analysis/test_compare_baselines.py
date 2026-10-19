"""Check that comparison statistics honor their actual measurement intervals."""

import unittest

import numpy as np

from compare_baselines import mean_goodput


class GoodputTest(unittest.TestCase):
    def test_short_final_interval_is_not_weighted_like_a_full_interval(self):
        data = np.array([[0.3, 10.0], [0.4, 4.0]])
        self.assertAlmostEqual(mean_goodput(data, start=0.1, stop=0.4), 8.0)

    def test_incomplete_run_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "complete run"):
            mean_goodput(np.array([[0.3, 10.0], [100.0, 9.0]]))


if __name__ == "__main__":
    unittest.main()
