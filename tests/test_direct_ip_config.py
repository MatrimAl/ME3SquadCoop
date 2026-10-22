import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src"))
from direct_ip_config import DirectIPConfig, config_from_dict, load_config, validate_ipv4, validate_port


class DirectIPTests(unittest.TestCase):
    def test_radmin_26_addresses_and_public_unicast_are_supported(self):
        for value in ("26.123.45.67", "192.168.1.10", "10.0.0.1", "100.64.1.2", "8.8.8.8"):
            with self.subTest(value=value):
                self.assertEqual(validate_ipv4(value), value)

    def test_non_unicast_or_unusable_addresses_are_rejected(self):
        for value in ("0.0.0.0", "0.1.2.3", "169.254.1.2", "224.0.0.1", "239.1.2.3", "240.0.0.1", "255.255.255.255"):
            with self.subTest(value=value), self.assertRaises(ValueError):
                validate_ipv4(value)

    def test_canonical_literals_prevent_shell_and_engine_url_injection(self):
        for value in ("26.1.2.3?listen", "26.1.2.3:17779", "26.1.2.3;quit", "26.1.2.3\nquit", " 26.1.2.3", "26.1.2.3 ",
                      "026.1.2.3", "26.1.2", "26.1.2.256", "0x1a010203", "localhost", "::1", "２６.1.2.3", 123, None):
            with self.subTest(value=value), self.assertRaises(ValueError):
                validate_ipv4(value)

    def test_loopback_is_explicit_and_qa_never_spills_to_remote(self):
        with self.assertRaises(ValueError):
            validate_ipv4("127.0.0.1")
        config = DirectIPConfig("client", "127.0.0.1", peer_ipv4="127.0.0.1", loopback_qa=True)
        self.assertEqual(config.client_console_command, "open 127.0.0.1:17779")
        for bind, peer in (("26.1.2.3", "127.0.0.1"), ("127.0.0.1", "26.1.2.3")):
            with self.subTest(bind=bind, peer=peer), self.assertRaises(ValueError):
                DirectIPConfig("client", bind, peer_ipv4=peer, loopback_qa=True)

    def test_directed_broadcast_is_prefix_aware(self):
        for value in ("26.0.0.0", "26.255.255.255"):
            with self.subTest(value=value), self.assertRaises(ValueError):
                validate_ipv4(value, subnet="26.0.0.0/8")
        self.assertEqual(validate_ipv4("192.168.0.255", subnet="192.168.0.0/23"), "192.168.0.255")
        self.assertEqual(validate_ipv4("10.0.0.0", subnet="10.0.0.0/31"), "10.0.0.0")
        self.assertEqual(validate_ipv4("10.0.0.1", subnet="10.0.0.1/32"), "10.0.0.1")
        for subnet in ("26.1.0.0/8", "10.0.0.0/8", "26.0.0.0/08", 8):
            with self.subTest(subnet=subnet), self.assertRaises(ValueError):
                validate_ipv4("26.1.2.3", subnet=subnet)

    def test_ports_reject_implicit_coercion_and_injection(self):
        for value in (1, 17779, 65535, "17779"):
            self.assertEqual(validate_port(value), int(value))
        for value in (0, -1, 65536, True, 17779.0, "017779", "17779?listen", "17779\n", "+17779", " 17779"):
            with self.subTest(value=value), self.assertRaises(ValueError):
                validate_port(value)

    def test_role_peer_and_boolean_requirements(self):
        for data in (("client", "26.1.2.3", {}), ("host", "26.1.2.3", {"peer_ipv4": "26.2.3.4"}),
                     ("server", "26.1.2.3", {}), ("host", "26.1.2.3", {"loopback_qa": 1})):
            with self.subTest(data=data), self.assertRaises(ValueError):
                DirectIPConfig(data[0], data[1], **data[2])

    def test_config_round_trip_and_argv_are_exact(self):
        config = DirectIPConfig("client", "26.1.2.3", 20000, "26.2.3.4", peer_subnet="26.0.0.0/8")
        self.assertEqual(config_from_dict(config.to_dict()), config)
        self.assertEqual(config.launcher_arguments, ["-MULTIHOME=26.1.2.3", "-PORT=20000"])
        self.assertEqual(config.client_console_command, "open 26.2.3.4:20000")
        with self.assertRaises(ValueError):
            DirectIPConfig("host", "26.1.2.3").client_console_command

    def test_schema_unknown_keys_and_wrong_types_fail_closed(self):
        good = {"schema": 1, "role": "host", "bind_ipv4": "26.1.2.3"}
        for data in ([], None, {**good, "schema": True}, {**good, "schema": "1"},
                     {**good, "extra": "ignored?"}, {"schema": 1, "role": "host"}):
            with self.subTest(data=data), self.assertRaises(ValueError):
                config_from_dict(data)

    def test_diagnostic_is_private_by_default_and_never_claims_transport_pass(self):
        config = DirectIPConfig("client", "26.1.2.3", peer_ipv4="26.2.3.4")
        result = config.diagnostic()
        self.assertNotIn("26.", json.dumps(result))
        self.assertFalse(result["udp_transport_tested"])
        self.assertFalse(result["campaign_coop_verified"])
        self.assertEqual(config.diagnostic(show_addresses=True)["peer_ipv4"], "26.2.3.4")

    def test_json_duplicate_keys_and_oversized_files_are_rejected(self):
        temp_root = Path(__file__).resolve().parents[1] / "local" / "test-tmp"
        temp_root.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=temp_root) as folder:
            path = Path(folder) / "endpoint.json"
            path.write_text('{"schema":1,"role":"host","bind_ipv4":"26.1.2.3","role":"client"}')
            with self.assertRaises(ValueError):
                load_config(path)
            path.write_bytes(b" " * 16385)
            with self.assertRaises(ValueError):
                load_config(path)
            path.write_text(json.dumps(DirectIPConfig("host", "26.1.2.3").to_dict()), encoding="utf-8-sig")
            self.assertEqual(load_config(path).bind_ipv4, "26.1.2.3")


if __name__ == "__main__":
    unittest.main(verbosity=2)
