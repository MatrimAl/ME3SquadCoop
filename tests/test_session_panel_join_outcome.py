"""UI guidance for unresolved joins must not claim a native rejection or retry."""
from pathlib import Path
from types import SimpleNamespace
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from session_panel import recovery_hint, STARTUP


class JoinOutcomeHintTests(unittest.TestCase):
    def frame(self, **changes):
        values = dict(control_state=0, enabled=1, netmode=0, startup=19,
                      role=2, readiness_state=0, recovery_flags=0)
        values.update(changes)
        return SimpleNamespace(**values)

    def test_unconfirmed_join_explains_game_message_and_cleanup(self):
        self.assertIn('not confirmed', STARTUP[19])
        hint = recovery_hint(self.frame())
        self.assertIn('Check the game', hint)
        self.assertIn('native loading and cleanup', hint)
        self.assertIn('does not dismiss messages or repair DLC', hint)

    def test_current_native_network_takes_precedence_over_old_join_warning(self):
        self.assertIn('Native networking is active', recovery_hint(self.frame(netmode=3)))

    def test_persistent_control_fault_cannot_be_presented_as_retryable(self):
        self.assertIn('new game session', recovery_hint(self.frame(control_state=4)))


if __name__ == '__main__':
    unittest.main()
