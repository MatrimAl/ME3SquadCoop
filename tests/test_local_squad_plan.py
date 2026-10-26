import json
from pathlib import Path
import subprocess
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'src'))
from runtime_session_plan import RuntimeSessionPlan


class LocalSquadPlanTest(unittest.TestCase):
    def test_guest_option_is_explicit_and_does_not_inherit_stale_setting(self):
        guest = RuntimeSessionPlan('guest', '192.168.1.2', peer_ipv4='192.168.1.3')
        self.assertNotIn('ME3_SQUAD_COOP_LOCAL_SQUAD_SET', guest.environment)
        enabled = RuntimeSessionPlan('guest', '192.168.1.2', peer_ipv4='192.168.1.3', local_squad_set=True)
        self.assertEqual(enabled.environment['ME3_SQUAD_COOP_LOCAL_SQUAD_SET'], '1')
        self.assertNotIn('me3_squad_coop_local_squad_set', guest.child_environment({'me3_squad_coop_local_squad_set': '1'}))

    def test_host_and_non_boolean_options_are_refused(self):
        with self.assertRaises(ValueError):
            RuntimeSessionPlan('host', '192.168.1.2', local_squad_set=True)
        with self.assertRaises(ValueError):
            RuntimeSessionPlan('guest', '192.168.1.2', peer_ipv4='192.168.1.3', local_squad_set=1)

    def test_cli_emits_guest_setting(self):
        result = subprocess.run([sys.executable, str(ROOT/'tools/prepare_session.py'), '--role', 'guest',
                                 '--bind', '192.168.1.2', '--peer', '192.168.1.3', '--local-squad-set'],
                                capture_output=True, text=True, check=True)
        self.assertEqual(json.loads(result.stdout)['environment']['ME3_SQUAD_COOP_LOCAL_SQUAD_SET'], '1')


if __name__ == '__main__':
    unittest.main()
