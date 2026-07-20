# Adaptive Horror FPS Demo

Adaptive Horror is a solo Unreal Engine 5 FPS survival-horror prototype where enemies observe the player's habits, adapt their tactics, and evolve into readable threat variants. The current project is a runtime-generated research facility vertical slice built around the route from Title/New Game to ADAM defeat and Stage Clear.

## Current state

- Engine target used by recent verification: UE5.8 / Win64 / Development Editor.
- Entry map: `/Engine/Maps/Entry`.
- Runtime world owner: `AEvaPrototypeGameMode`.
- Facility progression owner: `AEvaResearchFacilityDirector`.
- Current Automation baseline: 43 `AdaptiveHorror` tests.
- Human PIE confirmation is still required for visual feel, audio feel, route readability, and full playthrough acceptance.

## Source-of-truth documents

- `AGENTS.md`: persistent Codex rules only.
- `REQUIREMENTS.md`: product requirements, status labels, and architectural invariants.
- `ROADMAP.md`: milestone order and current/next work.
- `TEST_PLAN.md`: real verification commands, success conditions, and manual PIE boundary.
- `TASKS/*.md`: executable task specs for the current work only.
- `DEV_LOG.md`: chronological implementation and verification history.
- `TODO.md`: current open work and manual checks.
- `BUILD_CHECK.md`: build/automation/runtime verification results and troubleshooting notes.
- `GAME_DESIGN.md` / `TECH_SPEC.md`: historical and detailed design references. Do not duplicate their contents into new source-of-truth sections unless the information is still current.

## Standard autonomous launch prompt

Use a specific task file rather than putting task scope in `AGENTS.md`.

```text
最新mainから指定されたタスク用ブランチを作成し、
指定されたTASKS/*.mdを実装してください。

AGENTS.mdに従い、
REQUIREMENTS.md、TEST_PLAN.md、ROADMAP.mdの関連箇所だけを確認してください。

自動検証可能な受け入れ条件をすべて満たすまで、
実装、Build、Automation、Runtime Smoke、ログ確認、原因修正を繰り返してください。

すべて合格した時点で作業を停止し、
必要な文書のみ更新してcommit・pushしてください。

人間のPIE確認項目は成功扱いにせず、
Not verifiedとして最終報告に残してください。
```

## Quick verification

From the repository root:

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\RunCodexValidation.ps1 -MaxParallelActions 4
```

This wrapper runs the build/automation path, runtime smoke, log scan, `git diff --check`, and final `git status`. See `TEST_PLAN.md` for exact success conditions, fallback commands, and limits of Runtime Smoke.

## Manual PIE boundary

Codex must not claim these as passed without a human PIE result:

- Title/New Game/Pause/Game Over/Stage Clear visual layout.
- Mouse/keyboard feel.
- Combat and enemy readability.
- Horror lighting/audio feel.
- Field route readability.
- Full New Game to Stage Clear playthrough.

## Debug keys

Debug controls are Development/Editor-only and must remain unavailable in Shipping builds.

| Key | Current behavior |
|---|---|
| F1 | EVA analysis +20. |
| F2 | Force HUNTER deployment. |
| F3 | Force zombie wave. |
| F4 | Warp/start ADAM arena encounter. |
| F5 | Restore player HP/ammo. |
| F6 | Force Stage Clear. |
| F7 | Print telemetry snapshot. |
| F9 / N | Toggle/page Debug HUD and navigation visualization behavior. |
| F8 | Intentionally unbound because PIE uses it for Eject. |
| P | No current game-side binding found. |

## Current playable loop

1. Start at Title.
2. New Game enters the runtime research facility.
3. Restore Facility Power.
4. Find Security Keycard.
5. Unlock Observation Lab.
6. Search Containment Records.
7. Access Data Core.
8. Reach Adam Arena.
9. Defeat ADAM.
10. Stage Clear.

## Design guardrails

- Keep Version 0.1 solo.
- Keep Runtime NavMesh compatible with the generated facility.
- Keep ADAM defeat as the Stage Clear trigger.
- Keep Stage Clear idempotent.
- Keep debug-only controls out of Shipping builds.
- Record unknowns as `TBD`; do not invent unverified requirements or results.
