"""Strict direct-IP configuration and argument formatting; does not open sockets."""
import argparse
from dataclasses import dataclass
import ipaddress
import json
from pathlib import Path
import re

DEFAULT_PORT = 17779
_IPV4 = re.compile(r"(?:0|[1-9][0-9]{0,2})(?:\.(?:0|[1-9][0-9]{0,2})){3}\Z", re.ASCII)
_PORT = re.compile(r"[1-9][0-9]{0,4}\Z", re.ASCII)


def validate_port(value):
    """Accept a JSON integer or canonical decimal CLI string, never a bool/float."""
    if type(value) is str:
        if not _PORT.fullmatch(value):
            raise ValueError("port must be a canonical decimal integer")
        value = int(value)
    if type(value) is not int or not 1 <= value <= 65535:
        raise ValueError("port must be between 1 and 65535")
    return value


def validate_ipv4(value, *, loopback_qa=False, subnet=None):
    """Validate a unicast literal; Radmin's observed 26/8 must remain accepted.

    Public unicast is valid direct-IP input. RFC1918-only filtering would reject
    the actual installed VPN. Directed broadcast/network addresses can only be
    identified when their real subnet is supplied; do not guess from '.255'.
    """
    if type(loopback_qa) is not bool:
        raise ValueError("loopback_qa must be a boolean")
    if type(value) is not str or not _IPV4.fullmatch(value):
        raise ValueError("address must be a canonical dotted-decimal IPv4 literal")
    try:
        address = ipaddress.IPv4Address(value)
    except ipaddress.AddressValueError as error:
        raise ValueError("invalid IPv4 octet") from error
    if int(address) >> 24 == 0 or address.is_multicast or address.is_reserved or address.is_link_local:
        raise ValueError("address must be usable unicast, not wildcard/link-local/multicast/reserved")
    if address.is_loopback and not loopback_qa:
        raise ValueError("loopback requires explicit loopback_qa=true")
    if subnet is not None:
        if type(subnet) is not str:
            raise ValueError("subnet must be canonical IPv4 CIDR text")
        try:
            network = ipaddress.IPv4Network(subnet, strict=True)
        except ValueError as error:
            raise ValueError("subnet must be canonical IPv4 CIDR text") from error
        if str(network) != subnet or address not in network:
            raise ValueError("address must be within the declared canonical subnet")
        # /31 point-to-point and /32 host routes have usable endpoint addresses.
        if network.prefixlen <= 30 and address in (network.network_address, network.broadcast_address):
            raise ValueError("subnet network and directed-broadcast addresses are not endpoints")
    return str(address)


@dataclass(frozen=True)
class DirectIPConfig:
    role: str
    bind_ipv4: str
    port: int = DEFAULT_PORT
    peer_ipv4: str = None
    loopback_qa: bool = False
    bind_subnet: str = None
    peer_subnet: str = None

    def __post_init__(self):
        if self.role not in ("host", "client"):
            raise ValueError("role must be host or client")
        if type(self.loopback_qa) is not bool:
            raise ValueError("loopback_qa must be a boolean")
        object.__setattr__(self, "port", validate_port(self.port))
        object.__setattr__(self, "bind_ipv4", validate_ipv4(
            self.bind_ipv4, loopback_qa=self.loopback_qa, subnet=self.bind_subnet))
        if self.role == "client":
            object.__setattr__(self, "peer_ipv4", validate_ipv4(
                self.peer_ipv4, loopback_qa=self.loopback_qa, subnet=self.peer_subnet))
        elif self.peer_ipv4 is not None or self.peer_subnet is not None:
            raise ValueError("host configuration does not take a peer address")
        if self.loopback_qa and (not ipaddress.IPv4Address(self.bind_ipv4).is_loopback
                                 or (self.peer_ipv4 is not None
                                     and not ipaddress.IPv4Address(self.peer_ipv4).is_loopback)):
            raise ValueError("loopback QA configuration must remain entirely on loopback")

    @property
    def launcher_arguments(self):
        """Append these individual argv elements; never interpolate a shell command."""
        return ["-MULTIHOME=" + self.bind_ipv4, "-PORT=" + str(self.port)]

    @property
    def client_console_command(self):
        """Fixed UE3 command structure whose variables are validated digits/dots."""
        if self.role != "client":
            raise ValueError("only a client has a join command")
        return "open " + self.peer_ipv4 + ":" + str(self.port)

    def to_dict(self):
        return {"schema": 1, "role": self.role, "bind_ipv4": self.bind_ipv4,
                "peer_ipv4": self.peer_ipv4, "port": self.port,
                "loopback_qa": self.loopback_qa, "bind_subnet": self.bind_subnet,
                "peer_subnet": self.peer_subnet}

    def diagnostic(self, *, show_addresses=False):
        """Configuration validity is separate from VPN/UDP/gameplay readiness."""
        result = {"schema": 1, "configuration_valid": True, "role": self.role,
                  "transport": "udp-direct-ip", "port": self.port,
                  "loopback_qa": self.loopback_qa, "bind_address_configured": True,
                  "peer_address_configured": self.peer_ipv4 is not None,
                  "vpn_availability_checked": False, "peer_reachability_tested": False,
                  "udp_transport_tested": False, "campaign_coop_verified": False}
        if show_addresses:
            result.update(bind_ipv4=self.bind_ipv4, peer_ipv4=self.peer_ipv4)
        return result


def config_from_dict(data):
    if type(data) is not dict:
        raise ValueError("configuration must be a JSON object")
    allowed = {"schema", "role", "bind_ipv4", "peer_ipv4", "port", "loopback_qa", "bind_subnet", "peer_subnet"}
    if data.keys() - allowed:
        raise ValueError("unknown configuration keys")
    if type(data.get("schema")) is not int or data["schema"] != 1:
        raise ValueError("configuration schema must be integer 1")
    if not {"role", "bind_ipv4"}.issubset(data):
        raise ValueError("role and bind_ipv4 are required")
    return DirectIPConfig(**{key: value for key, value in data.items() if key != "schema"})


def load_config(path):
    """Reject duplicate JSON keys instead of silently accepting last-key overrides."""
    def unique_keys(pairs):
        result = {}
        for key, value in pairs:
            if key in result:
                raise ValueError("duplicate configuration key")
            result[key] = value
        return result
    with Path(path).open("rb") as stream:
        raw = stream.read(16385)
    if len(raw) > 16384:
        raise ValueError("configuration is unexpectedly large")
    return config_from_dict(json.loads(raw.decode("utf-8-sig"), object_pairs_hook=unique_keys))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("config", type=Path)
    parser.add_argument("--show-addresses", action="store_true",
                        help="include private endpoint addresses in the diagnostic")
    args = parser.parse_args()
    try:
        config = load_config(args.config)
    except (ValueError, OSError) as error:
        # Do not echo the input address/path in a default shareable diagnostic.
        print(json.dumps({"configuration_valid": False, "error_type": type(error).__name__}))
        return 2
    print(json.dumps(config.diagnostic(show_addresses=args.show_addresses), indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
