import json
from pathlib import Path
import subprocess
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'src'))
from runtime_session_plan import RuntimeSessionPlan


class LocalSquadPlanTest(unittest.TestCase):
    def test_guest_context_is_default_and_does_not_inherit_stale_setting(self):
        guest = RuntimeSessionPlan('guest', '192.168.1.2', peer_ipv4='192.168.1.3')
        self.assertEqual(guest.environment['ME3_SQUAD_COOP_LOCAL_SQUAD_SET'], '1')
        disabled = RuntimeSessionPlan('guest', '192.168.1.2', peer_ipv4='192.168.1.3', local_squad_set=False)
        self.assertEqual(disabled.environment['ME3_SQUAD_COOP_LOCAL_SQUAD_SET'], '0')
        self.assertNotIn('ME3_SQUAD_COOP_LOCAL_SQUAD_SET', RuntimeSessionPlan('host', '192.168.1.2').environment)
        enabled = RuntimeSessionPlan('guest', '192.168.1.2', peer_ipv4='192.168.1.3', local_squad_set=True)
        self.assertEqual(enabled.environment['ME3_SQUAD_COOP_LOCAL_SQUAD_SET'], '1')
        self.assertNotIn('me3_squad_coop_local_squad_set', guest.child_environment({'me3_squad_coop_local_squad_set': '1'}))

    def test_host_and_non_boolean_options_are_refused(self):
        with self.assertRaises(ValueError):
            RuntimeSessionPlan('host', '192.168.1.2', local_squad_set=True)
        with self.assertRaises(ValueError):
            RuntimeSessionPlan('guest', '192.168.1.2', peer_ipv4='192.168.1.3', local_squad_set=1)

    def test_native_mapped_power_candidate_is_guest_only_and_explicit(self):
        key = 'ME3_SQUAD_COOP_NATIVE_MAPPED_POWERS'
        guest = RuntimeSessionPlan('guest', '192.168.1.2', peer_ipv4='192.168.1.3')
        self.assertNotIn(key, guest.environment)
        self.assertNotIn(key, guest.child_environment({key:'1'}))
        opted = RuntimeSessionPlan('guest', '192.168.1.2', peer_ipv4='192.168.1.3', native_mapped_powers=True)
        self.assertEqual(opted.environment[key], '1')
        with self.assertRaises(ValueError):
            RuntimeSessionPlan('host', '192.168.1.2', native_mapped_powers=True)
        with self.assertRaises(ValueError):
            RuntimeSessionPlan('guest', '192.168.1.2', peer_ipv4='192.168.1.3', native_mapped_powers=1)
        result = subprocess.run([sys.executable,str(ROOT/'tools/prepare_session.py'),'--role','guest',
            '--bind','192.168.1.2','--peer','192.168.1.3','--native-mapped-powers'],capture_output=True,text=True,check=True)
        self.assertEqual(json.loads(result.stdout)['environment'][key], '1')

    def test_normal_guest_power_slots_are_automatic_and_reversible(self):
        guest = RuntimeSessionPlan('guest', '192.168.1.2', peer_ipv4='192.168.1.3')
        self.assertEqual(guest.environment['ME3_SQUAD_COOP_NATIVE_POWER_HOTKEYS'], '1')
        self.assertEqual(guest.environment['ME3_SQUAD_COOP_NATIVE_POWER_ICONS'], '1')
        disabled = RuntimeSessionPlan('guest', '192.168.1.2', peer_ipv4='192.168.1.3', native_power_hotkeys=False)
        self.assertEqual(disabled.environment['ME3_SQUAD_COOP_NATIVE_POWER_HOTKEYS'], '0')
        self.assertEqual(disabled.environment['ME3_SQUAD_COOP_NATIVE_POWER_ICONS'], '0')
        host = RuntimeSessionPlan('host', '192.168.1.2')
        self.assertNotIn('ME3_SQUAD_COOP_NATIVE_POWER_HOTKEYS', host.environment)
        self.assertNotIn('ME3_SQUAD_COOP_NATIVE_POWER_ICONS', host.environment)

    def test_cli_emits_guest_setting(self):
        result = subprocess.run([sys.executable, str(ROOT/'tools/prepare_session.py'), '--role', 'guest',
                                 '--bind', '192.168.1.2', '--peer', '192.168.1.3', '--local-squad-set'],
                                capture_output=True, text=True, check=True)
        self.assertEqual(json.loads(result.stdout)['environment']['ME3_SQUAD_COOP_LOCAL_SQUAD_SET'], '1')
        disabled = subprocess.run([sys.executable, str(ROOT/'tools/prepare_session.py'), '--role', 'guest',
                                   '--bind', '192.168.1.2', '--peer', '192.168.1.3', '--no-local-squad-set'],
                                  capture_output=True, text=True, check=True)
        self.assertEqual(json.loads(disabled.stdout)['environment']['ME3_SQUAD_COOP_LOCAL_SQUAD_SET'], '0')


if __name__ == '__main__':
    unittest.main()
