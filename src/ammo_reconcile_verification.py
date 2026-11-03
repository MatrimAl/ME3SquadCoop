"""Match native owner-PC reconciliation requests/replies without assuming clock sync."""
from collections import defaultdict


def _integer(value, low=0, high=1000000):
    return type(value) is int and low <= value <= high


def _valid(row, event, side):
    return (row.get("ammo_reconcile") == event and row.get("side") == side
        and _integer(row.get("request"), 1, (1 << 64) - 1)
        and _integer(row.get("generation"), 1, (1 << 32) - 1)
        and _integer(row.get("channel_index"), 0, 2047)
        and _integer(row.get("used")) and _integer(row.get("spare"))
        and _integer(row.get("monotonic_ms"), 0, (1 << 32) - 1))


def ammo_reconcile_evidence(host_rows, guest_rows):
    requests, incoming, replies, applications = (defaultdict(list) for _ in range(4))
    for row in guest_rows:
        if _valid(row, "requested", "guest") and row.get("reason") == "settled-guest-weapon":
            requests[row["request"]].append(row)
        elif (row.get("ammo_reconcile") in ("applied", "already-matched")
              and _valid(row, row["ammo_reconcile"], "guest") and row.get("reason") == "native-owner-pc-response"):
            applications[row["request"]].append(row)
    for row in host_rows:
        if _valid(row, "received-request", "host") and row.get("reason") == "native-owner-pc-rpc":
            incoming[row["request"]].append(row)
        elif _valid(row, "replied", "host") and row.get("reason") == "settled-host-snapshot":
            replies[row["request"]].append(row)
    results = []
    for request_id, values in applications.items():
        groups = [requests[request_id], incoming[request_id], replies[request_id], values]
        if any(len(group) != 1 for group in groups):
            continue
        request, received, reply, applied = (group[0] for group in groups)
        if (len({r["channel_index"] for r in (request, received, reply, applied)}) != 1
            or request["generation"] != applied["generation"]
            or received["generation"] != reply["generation"]
            or request_id >> 32 != request["generation"]
            or (applied["used"], applied["spare"]) != (reply["used"], reply["spare"])
            or ((applied["monotonic_ms"] - request["monotonic_ms"]) & 0xffffffff) > 5000
            or ((reply["monotonic_ms"] - received["monotonic_ms"]) & 0xffffffff) > 5000):
            continue
        results.append({"request": request_id, "channel_index": request["channel_index"],
            "guest_before_request": [request["used"], request["spare"]],
            "host_snapshot": [reply["used"], reply["spare"]],
            "guest_after_response": [applied["used"], applied["spare"]],
            "changed_by_adapter": applied["ammo_reconcile"] == "applied",
            "guest_roundtrip_ms": (applied["monotonic_ms"] - request["monotonic_ms"]) & 0xffffffff})
    exceptions = sum("exception" in r for r in host_rows + guest_rows)
    return {"roundtrips": results, "native_exception_count": exceptions,
        "reconciliation_roundtrip_verified": bool(results) and exceptions == 0,
        "difference_corrected_observed": exceptions == 0 and any(r["changed_by_adapter"] and
            r["guest_before_request"] != r["host_snapshot"] for r in results),
        "firing_verified": False, "full_combat_verified": False, "two_computer_verified": False}
