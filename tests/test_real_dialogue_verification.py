"""Synthetic traces distinguish actual dialogue handoffs from labelled scenes."""
import copy
from pathlib import Path
import sys
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"src"))
from real_dialogue_verification import real_dialogue_evidence


def trace():
    host=[
        dict(session="acquired",epoch=1,remote_owns_ashley=True,host_story_preserved=True,monotonic_ms=1000),
        dict(narrative_probe="conversation",current_mode=7,mode_read_valid=True,
             active_array_valid=True,active_count=1,full_getter_dispatched=True,
             full_controller={"status":"live"},controller_fields_valid=True,
             full_active_memberships=1,monotonic_ms=2900),
        dict(session="gate-observation",source="engine",blocked=True,mode=7,monotonic_ms=3000),
        dict(session="released",reason="scene",epoch=1,ai_restored=True,
             host_story_preserved=True,monotonic_ms=3000),
        dict(session="acquired",epoch=2,remote_owns_ashley=True,host_story_preserved=True,monotonic_ms=6000)]
    guest=[
        dict(session="guest-observation",reciprocal_controller=True,camera_targets_ashley=True,monotonic_ms=2000),
        dict(session="guest-idle",has_pawn=False,monotonic_ms=4000),
        dict(session="guest-observation",reciprocal_controller=True,camera_targets_ashley=True,monotonic_ms=6500)]
    return host,guest


class RealDialogueEvidence(unittest.TestCase):
    def test_complete_real_handoff_proves_control_but_not_camera_playability(self):
        result=real_dialogue_evidence(*trace())
        self.assertTrue(result['real_dialogue_control_return_verified'])
        self.assertFalse(result['dialogue_camera_playability_verified'])

    def test_empty_evidence_cannot_pass(self):
        self.assertFalse(real_dialogue_evidence([],[])['real_dialogue_control_return_verified'])

    def test_synthetic_scene_cannot_pass_even_with_control_return(self):
        host,guest=trace();host[2]['source']='synthetic'
        self.assertFalse(real_dialogue_evidence(host,guest)['real_dialogue_control_return_verified'])

    def test_no_prior_host_lease_cannot_pass(self):
        host,guest=trace();host.pop(0)
        self.assertFalse(real_dialogue_evidence(host,guest)['real_dialogue_control_return_verified'])

    def test_no_observed_guest_ownership_before_scene_cannot_pass(self):
        host,guest=trace();guest.pop(0)
        self.assertFalse(real_dialogue_evidence(host,guest)['real_dialogue_control_return_verified'])

    def test_detach_and_return_are_both_required(self):
        host,guest=trace()
        for index in [1,2]:
            modified=copy.deepcopy(guest);modified.pop(index)
            self.assertFalse(real_dialogue_evidence(host,modified)['real_dialogue_control_return_verified'])

    def test_unresolved_or_nonunique_conversation_cannot_pass(self):
        host,guest=trace();host[1]['full_active_memberships']=2
        self.assertFalse(real_dialogue_evidence(host,guest)['real_dialogue_control_return_verified'])

    def test_conversation_outside_lease_cannot_pass(self):
        host,guest=trace();host[0]['monotonic_ms']=2950
        self.assertFalse(real_dialogue_evidence(host,guest)['real_dialogue_control_return_verified'])


if __name__=='__main__':
    unittest.main()
