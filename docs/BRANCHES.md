# Branch layout

| Branch | Purpose |
| --- | --- |
| `dev/campaign-coop` | Default branch and current development source. Base new contributions here. |
| `main` | Reviewed snapshots promoted by maintainers. It may lag development; it is not a claim of a playable release. |
| `fix/*`, `feat/*`, `chore/*`, `research/*` | Focused work proposed through a PR to `dev/campaign-coop`. Remove the remote branch after integration. |

The README and validation notes on `dev/campaign-coop` describe the current
prototype. Source availability, passing fixtures and a native build do not
establish a complete playable campaign.

## Consolidation on 2026-11-03

The long-lived `fix/guest-equipment-readiness` branch had accumulated the
runtime work beyond its original equipment scope. Development now includes
that source through `25bb4fd`, together with the previous public development
history. Subsequent work should use `dev/campaign-coop` as its baseline.

The consolidation preserves the current runtime and packaging implementation.
It does not apply the older packaging snapshot over the current activation/TOC
tooling. PR #2's transactional reinstall/recovery implementation remains
available in the archived development history for a focused future integration;
recording that history does not claim that those functions are in the active
implementation.

Historical tips are retained as annotated tags:

| Tag | Preserved work |
| --- | --- |
| `archive/development-before-consolidation-2026-11-03` | Previous public development, including PR #2. |
| `archive/reversible-packaging-2026-11-03` | Former reversible-packaging feature branch. |
| `archive/equipment-readiness-2026-11-03` | Runtime development before consolidation. |

These tags are historical source references, not release downloads. Existing
local research checkouts may remain on their historical branches; keep their
uncommitted files and evidence when moving new work to the shared baseline.
