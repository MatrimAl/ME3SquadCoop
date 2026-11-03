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
    panel: bool = False
    power_hotkeys: bool = False
    auto_relisten: bool = False
    auto_rejoin: bool = False
    ready_channel: bool = False
    input_bootstrap: bool = False
    native_power_hotkeys: bool = None
    native_power_icons: bool = None
    local_squad_set: bool = None
    native_mapped_powers: bool = False

    def __post_init__(self):
        if self.role not in ("host", "guest"):
            raise ValueError("ordinary runtime role must be host or guest")
        if type(self.panel) is not bool:
            raise ValueError("panel must be a boolean")
        if type(self.power_hotkeys) is not bool or (self.power_hotkeys and self.role != "guest"):
            raise ValueError("power hotkeys require a guest plan and a boolean option")
        if type(self.auto_relisten) is not bool or (self.auto_relisten and self.role != "host"):
            raise ValueError("automatic relisten requires a host plan and a boolean option")
        if type(self.auto_rejoin) is not bool or (self.auto_rejoin and self.role != "guest"):
            raise ValueError("automatic rejoin requires a guest plan and a boolean option")
        if type(self.ready_channel) is not bool:
            raise ValueError("ready channel must be a boolean")
        if type(self.input_bootstrap) is not bool or (self.input_bootstrap and self.role != "guest"):
            raise ValueError("input bootstrap requires a guest plan and a boolean option")
        if self.local_squad_set is not None and (type(self.local_squad_set) is not bool or (self.local_squad_set and self.role != "guest")):
            raise ValueError("local squad initialization requires a guest plan and a boolean option")
        if self.native_power_hotkeys is not None and (type(self.native_power_hotkeys) is not bool or (self.native_power_hotkeys and self.role != "guest")):
            raise ValueError("native power hotkeys require a guest plan and a boolean option")
        if self.native_power_icons is not None and (type(self.native_power_icons) is not bool or (self.native_power_icons and (self.role != "guest" or self.native_power_hotkeys is False))):
            raise ValueError("native power icons require native power hotkeys and a boolean option")
        if type(self.native_mapped_powers) is not bool or (self.native_mapped_powers and self.role != "guest"):
            raise ValueError("native mapped powers require a guest plan and a boolean option")
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
        if self.ready_channel and endpoint.port == 65535:
            raise ValueError("ready channel needs an adjacent UDP port")
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
        if self.panel:
            values["ME3_SQUAD_COOP_PANEL"] = "1"
        if self.power_hotkeys:
            values["ME3_SQUAD_COOP_POWER_HOTKEYS"] = "1"
        if self.auto_relisten:
            values["ME3_SQUAD_COOP_AUTO_RELISTEN"] = "1"
        if self.auto_rejoin:
            values["ME3_SQUAD_COOP_AUTO_REJOIN"] = "1"
        if self.ready_channel:
            values["ME3_SQUAD_COOP_READY_CHANNEL"] = "1"
        if self.input_bootstrap:
            values["ME3_SQUAD_COOP_INPUT_BOOTSTRAP"] = "1"
        if self.role == "guest":
            values["ME3_SQUAD_COOP_LOCAL_SQUAD_SET"] = "0" if self.local_squad_set is False else "1"
        if self.role == "guest":
            values["ME3_SQUAD_COOP_NATIVE_POWER_HOTKEYS"] = "0" if self.native_power_hotkeys is False else "1"
            values["ME3_SQUAD_COOP_NATIVE_POWER_ICONS"] = "0" if self.native_power_icons is False or self.native_power_hotkeys is False else "1"
        if self.native_mapped_powers:
            values["ME3_SQUAD_COOP_NATIVE_MAPPED_POWERS"] = "1"
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
