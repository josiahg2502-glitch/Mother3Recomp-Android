# Reverse-engineering notes

One file per topic. Agents (`.claude/agents/`) read these before starting and update
them after. Each note records addresses (and whether JP == EN), what the code does,
the evidence, and open questions. Unverified claims are marked as hypotheses.

| Note | Topic | Status |
|---|---|---|
| [combo.md](combo.md) | Battle rhythm combos, hit-window check, latency offset | Hit check located; units unknown |
| widescreen.md | Overworld camera, map/tilemap, object culling | Not started |

## Workflow
re-scout (find, write note) → decomp-matcher (optional: readable source in the fork)
→ hook-writer (implement) → verifier (test, report).
