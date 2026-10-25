"""Prepare ordinary co-op session configuration; never start or modify a game."""
from dataclasses import dataclass

from direct_ip_config import DEFAULT_PORT, DirectIPConfig


@dataclass(frozen=True)
class RuntimeSessionPlan:
    role: str
    bind_ipv4: str
    port: int = DEFAULT_PORT
    peer_ipv4: str = None
    start: str = None

    def __post_init__(self):
        if self.role not in ("host", "guest"):
            raise ValueError("ordinary runtime role must be host or guest")
        endpoint = DirectIPConfig(
            "host" if self.role == "host" else "client",
            self.bind_ipv4, self.port, self.peer_ipv4, loopback_qa=False)
        action = self.start
        if action is None:
            action = "listen" if self.role == "host" else "join"
        if action not in ("attach", "listen" if self.role == "host" else "join"):
            raise ValueError("startup action does not match the session role")
        object.__setattr__(self, "start", action)
        object.__setattr__(self, "port", endpoint.port)
        object.__setattr__(self, "bind_ipv4", endpoint.bind_ipv4)
        object.__setattr__(self, "peer_ipv4", endpoint.peer_ipv4)

    @property
    def arguments(self):
        """Validated argv elements, never a shell command or executable path."""
        return ["-MULTIHOME=" + self.bind_ipv4, "-PORT=" + str(self.port)]

    @property
    def environment(self):
        values = {"ME3_SQUAD_COOP_ENABLED": "1", "ME3_SQUAD_COOP_ROLE": self.role,
                  "ME3_SQUAD_COOP_BIND_IPV4": self.bind_ipv4,
                  "ME3_SQUAD_COOP_PORT": str(self.port)}
        if self.peer_ipv4 is not None:
            values["ME3_SQUAD_COOP_PEER_IPV4"] = self.peer_ipv4
        if self.start != "attach":
            values["ME3_SQUAD_COOP_START"] = self.start
        return values

    def child_environment(self, inherited):
        """Return a new mapping; never mutate the caller or the OS environment.

        Remove inherited probe switches, stale startup requests and opposite
        role settings before applying ordinary configuration. Windows keys
        are case-insensitive, so lowercase aliases are discarded too.
        """
        values = {key: value for key, value in inherited.items()
                  if not key.upper().startswith("ME3_SQUAD_COOP_")}
        values.update(self.environment)
        return values

    def to_dict(self):
        return {"schema": 1, "kind": "ordinary-runtime-session-plan",
                "role": self.role, "start": self.start,
                "arguments": self.arguments, "environment": self.environment,
                "inherited_environment_policy": "replace-ME3_SQUAD_COOP-prefix-case-insensitively",
                "configuration_valid": True,
                "installation_verified": False, "local_interface_verified": False,
                "connection_verified": False, "campaign_coop_verified": False}
