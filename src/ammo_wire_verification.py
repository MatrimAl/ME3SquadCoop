"""Bounded native field observations; encoding is not a delivery/sync verdict."""
from collections import Counter

FIELDS = {"AmmoUsedCount": (1, "used"), "CurrentSpareAmmo": (2, "spare")}


def _integer(value, minimum=0):
    return type(value) is int and value >= minimum


def ammo_wire_evidence(rows):
    counts = Counter()
    scopes, decodes, duplicates = {}, {}, set()
    observations = []
    installed = False
    limited = False
    for row in rows:
        event = row.get("ammo_wire")
        if event == "installed":
            installed = row.get("probe_changed_payload") is False
        elif event == "limit":
            limited = True
        elif event == "scope" and installed:
            generation, channel, at = (row.get(k) for k in ("generation", "channel_index", "monotonic_ms"))
            if _integer(generation, 1) and _integer(channel) and channel <= 2047 and _integer(at):
                if generation in scopes:
                    scopes[generation] = None
                else:
                    scopes[generation] = (channel, at)
        elif event in ("field", "receive-complete") and installed:
            generation, channel, at = (row.get(k) for k in ("generation", "channel_index", "monotonic_ms"))
            scope = scopes.get(generation) if _integer(generation, 1) else None
            if not scope or not _integer(channel) or channel != scope[0] or not _integer(at) or at < scope[1]:
                counts["rejected"] += 1
                continue
            receive = row.get("receive_id")
            if event == "field":
                field = row.get("field")
                valid = (isinstance(field, str) and field in FIELDS and row.get("accepted") is True
                    and row.get("native_result") == 1 and type(row.get("native_result")) is int
                    and row.get("archive_error_before") == 0 and type(row.get("archive_error_before")) is int
                    and row.get("archive_error_after") == 0 and type(row.get("archive_error_after")) is int
                    and row.get("original_forwarded") is True and row.get("probe_changed_payload") is False
                    and _integer(row.get("value_after")))
                if not valid:
                    counts["rejected"] += 1
                    continue
                if row.get("direction") == "encode" and receive == 0:
                    counts["encodes"] += 1
                elif row.get("direction") == "decode" and _integer(receive, 1):
                    counts["decodes"] += 1
                    key = generation, receive, field
                    if key in decodes:
                        duplicates.add(key)
                    decodes[key] = (row["value_after"], at)
                else:
                    counts["rejected"] += 1
            elif (_integer(receive, 1) and row.get("original_receive_forwarded") is True
                  and row.get("probe_changed_payload") is False
                  and type(row.get("field_mask")) is int and 1 <= row["field_mask"] <= 3):
                counts["completed_receives"] += 1
                for field, (bit, short) in FIELDS.items():
                    if not row["field_mask"] & bit:
                        continue
                    key = generation, receive, field
                    decoded = decodes.pop(key, None)
                    before, after = row.get(f"{short}_before"), row.get(f"{short}_after")
                    if (key in duplicates or not decoded or decoded[1] > at
                        or not _integer(before) or not _integer(after)
                        or not _integer(row.get(f"decoded_{short}")) or row[f"decoded_{short}"] != decoded[0]):
                        counts["rejected"] += 1
                        continue
                    applied = after == decoded[0]
                    counts["applied_fields"] += applied
                    counts["applied_value_changes"] += applied and after != before
                    counts["used_decreases"] += applied and short == "used" and after < before
                    observations.append({"field": field, "before": before, "decoded": decoded[0],
                                         "after": after, "matches_decoded": applied, "monotonic_ms": at})
            else:
                counts["rejected"] += 1
    return {
        "observer_installed": installed, "record_limit_observed": limited,
        "accepted_encodes": counts["encodes"], "accepted_decodes": counts["decodes"],
        "completed_receives": counts["completed_receives"], "fields_matching_final_storage": counts["applied_fields"],
        "applied_value_changes": counts["applied_value_changes"],
        "used_count_decreases_during_receive": counts["used_decreases"], "rejected_records": counts["rejected"],
        "uncompleted_decode_fields": len(decodes), "observations": observations,
        "received_value_matches_storage_observed": counts["applied_fields"] > 0,
        "received_field_change_observed": counts["applied_value_changes"] > 0,
        "packet_matching_verified": False, "ammo_synchronization_verified": False,
    }
