# Runtime identities of localized scenes

The paired campaign trial exposed a mismatch between a containing PCC filename
and an object's actual Unreal Package outer. The observed airlock line was
`promar_airlock_m_D.Node_Data_Sequence.SeqAct_Interp_15`; it did not start with
`BioD_ProMar_125Elevator_LOC_INT`. The selected Liara localized files likewise
contain explicit Package exports named `promar_intro_liara_c_d`,
`promar_intro_liara_m_d`, and `promar_level02_b_d`. Their outer chains are
independent of the containing file's name. FName spelling is case insensitive.

The previous filename-only filter refused these actual localized identities.
That filter is shared by native transport admission, resident guest story
guards, Player binding, speech-key capture and conversation lookup. Extending
it at the shared boundary supplies these existing paths consistently. It does
not replay a conversation, create story controllers or bypass native channels.
The selected Mars airlock and its actual localized Package are now included
alongside Liara. Other conversations remain outside this selected scope.

The Owner fallback still applies only to numeric line actions in
`promar_intro_liara_m_d.Node_Data_Sequence`. It accepts the actual short Package
outer and the retained filename-qualified forms, with case-insensitive name
comparison. It does not select Liara for an airlock or unrelated Owner query.

Capability version 5 requires both peers to have the expanded resident guards.
A version 4 guest cannot authorize a new host to replicate these added actions.
Only the capability/bootstrap version changes; native scene creation, actor
resolution, event authority, timing, audio and cleanup keep their prior checks.
Fixtures include actual short paths, case variants, foreign prefix extensions,
wrong Owner families and version 4 rejection. Rendered cinematic parity still
requires another paired scene. Camera publication alone does not prove it.
