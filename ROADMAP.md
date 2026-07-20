# ROADMAP.md — Adaptive Horror Development Sequence

This file tracks development order only. Put product requirements in `REQUIREMENTS.md`, verification commands in `TEST_PLAN.md`, and executable task details in `TASKS/*.md`.

## Current milestone

### Field Pass 1

Status: Planned / next implementation candidate.

Goal: turn the runtime research facility from a functional graybox into a readable playable field pass without adding new major systems. See `TASKS/field-pass-1.md`.

## Completed

- Core Game Loop
  - FPS movement, camera, handgun, HP, ammo/reload, damage, zombie combat, death/retry, Stage Clear.
- Runtime Navigation
  - Runtime graybox facility, RuntimeFloor/NavMesh diagnostics, safe spawn foundation, movement fallback safeguards.
- HUNTER / EVA Adaptation Foundation
  - Learning multiplier, HUNTER deployment/defeat/reinsertion, EVA analysis, evolved variants.
- ADAM Boss Foundation
  - Boss actor/controller, chase, melee, charge, roar summon, Phase 2, Boss HUD, Stage Clear on defeat.
- Core UI Flow
  - Title, Pause, Settings, Game Over, Stage Clear, input mode handling.
- Visual Pass 1
  - Placeholder enemy silhouettes and simple visual feedback.
- Horror Immersion Pass
  - Flashlight, blackout/prototype horror feedback, lighting/audio hooks.
- Gameplay Pass 1
  - Bounded adaptive behavior, role/intent debug display, COMPOSITE hybrid.
- Content Pass 1
  - Objective chain, Power Console, Keycard, Locked Door, Research Logs, Data Core, ADAM arena gate.
- Autonomous Development Kit
  - AGENTS, requirements, roadmap, test plan, task template, validation script.

## In progress

- Content Pass 1 PIE confirmation
  - Human validation still required for interactable visibility/feel and full content flow after latest fixes.
- Autonomous workflow hardening
  - First use of `TASKS/field-pass-1.md` should validate that the workflow is sufficient.

## Planned

- Field Pass 1
  - Readable zone dressing, landmarking, navigation cues, encounter-space layout polish using existing systems only.
- Weapon Pass 1
  - Improve handgun feedback and consider one additional weapon only if required by demo pacing.
- Enemy Pass 2
  - Tune enemy role readability and evolved variant presentation.
- Story Pass 1
  - Improve EVA logs/objective messaging and environmental storytelling.
- Optimization Pass 1
  - Reduce runtime-generation overhead, improve memory safety for 16 GB machines, and profile PIE/package performance.
- Shipping Pass
  - Package validation, settings application, final demo start/end flow, known-issue triage.

## Blocked

- Human PIE confirmation for visual/feel criteria. Codex must not mark those as passed from Runtime Smoke.
- Final packaged performance target is TBD.

## Deferred

- Multiplayer.
- External/online learning.
- Cloud services.
- Complex inventory/crafting.
- Multiple levels.
- Final art/audio production.
- Gamepad support.
