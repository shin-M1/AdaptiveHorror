# TASKS/field-pass-1.md — Field Pass 1

## Goal

Make the existing runtime research facility easier to read and play as a field, without adding new major game systems. The result should be a completion candidate that Codex can build, test, runtime-smoke, log-scan, commit, and push to `feature/field-pass1`.

## Background

The current `main` contains the core game loop, Runtime Graybox, Content Pass 1 progression, Gameplay Pass 1 adaptation, Visual Pass 1, Horror Immersion Pass, HUNTER, ADAM, UI flow, and 43 Automation tests. Content Pass interactables were recently fixed so Keycard and Research Logs have visible bodies and interaction collision, but human PIE confirmation remains required.

## In scope

- Improve readability of the six existing runtime zones:
  - Entry Lobby
  - Security Corridor
  - Observation Lab
  - Containment Ward
  - Data Core Room
  - Adam Arena
- Add or adjust lightweight graybox dressing using engine standard primitives/components only.
- Add simple landmarks, signs, floor markings, lighting color accents, or non-blocking guide geometry.
- Keep interactables visible and reachable:
  - Power Console
  - Security Keycard
  - Locked Door
  - Research Logs
  - Data Core Console
- Keep route from New Game to Adam Arena readable.
- Improve debug/log coverage for generated field structure if needed.
- Add or update Automation for structural checks where practical.
- Update docs with verified and unverified items.

## Out of scope

- New weapons.
- New enemy classes.
- New maps.
- New combat rules.
- New AI behavior or NavMesh algorithm changes.
- HUNTER or ADAM behavior changes.
- Stage Clear, Player Death, Title, Pause, Settings, or Game Over flow changes.
- Major UI redesign.
- Real art/audio asset production.
- Main branch merge.

## Dependencies

- `AGENTS.md`
- `REQUIREMENTS.md`
- `TEST_PLAN.md`
- `ROADMAP.md`
- Existing runtime facility generation in `AEvaPrototypeGameMode`.
- Existing content progression in `AEvaResearchFacilityDirector` and `AEvaFacilityInteractable`.

## Relevant files

- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.h`
- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.cpp`
- `Source/AdaptiveHorror/World/EvaResearchFacilityDirector.h`
- `Source/AdaptiveHorror/World/EvaResearchFacilityDirector.cpp`
- `Source/AdaptiveHorror/World/EvaFacilityInteractable.h`
- `Source/AdaptiveHorror/World/EvaFacilityInteractable.cpp`
- `Source/AdaptiveHorror/UI/EvaHUD.cpp`
- `Source/AdaptiveHorror/Tests/EvaLearningTests.cpp`
- `Config/DefaultEngine.ini`
- `Scripts/RunBuildCheck.ps1`
- `Scripts/RunCodexValidation.ps1`
- `DEV_LOG.md`
- `TODO.md`
- `BUILD_CHECK.md`
- `NEXT_PROMPT.md`

## Constraints

- Start from latest `main`.
- Create `feature/field-pass1`.
- Do not edit `main` directly.
- Do not rebase/cherry-pick/rewrite history.
- Keep changes minimal and field-readability focused.
- Do not break current 43 Automation tests.
- Do not claim PIE visual success unless the user provides PIE results.
- Runtime Smoke is not a substitute for human visual confirmation.

## Acceptance criteria — automated

Codex must verify:

- Development Editor / Win64 build succeeds without Live Coding.
- `Automation RunTests AdaptiveHorror` succeeds.
- Runtime Smoke exits 0.
- Log scan finds no current-run Fatal / Ensure / Assertion / EXCEPTION / Stack overflow / Access violation.
- `git diff --check` succeeds.
- Runtime facility still creates six zones.
- Required interactables still exist and are not duplicated.
- Required interactables remain visible/enabled by component-state logs or Automation:
  - Power Console
  - Security Keycard
  - Locked Door
  - three Research Logs
  - Data Core Console
- Runtime NavMesh readiness is not regressed.
- Spawn safety tests remain green.
- Stage Clear tests remain green.

## Acceptance criteria — human PIE

These are not automatically confirmable by Codex:

- Each zone is visually distinguishable.
- Navigation from one zone to the next is understandable without reading source code.
- Lighting is readable and not too dark.
- Keycard and Research Logs are easy to notice in normal play.
- Combat spaces are not too narrow or confusing.
- Enemies do not visibly pop in front of the player.
- New Game to Stage Clear can be completed in PIE.

Codex may commit and push a completion candidate if all automated criteria pass. Human PIE criteria must be listed as `Not verified by Codex`. Merge to `main` should wait for human PIE acceptance.

## Automated verification

Preferred:

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\RunCodexValidation.ps1 -MaxParallelActions 4
```

Fallback:

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\RunBuildCheck.ps1 -MaxParallelActions 4
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Users\shinn\Documents\Codex\2026-06-23\unreal-engine-5-fps-30-60\AdaptiveHorror.uproject" -game -Unattended -NullRHI -NoSound -NoSplash -ExecCmds="Quit" -log
git diff --check
git status --short --branch
```

## Regression requirements

Preserve:

- Title -> New Game input flow.
- Player movement/fire/reload/interact.
- Death/Game Over/retry.
- Stage Clear idempotence.
- Runtime NavMesh.
- Zombie chase/attack.
- HUNTER lifecycle.
- EVA adaptation/evolution.
- ADAM encounter and Boss HUD.
- Content Pass progression and interactables.
- Debug HUD and debug keys.

## Completion report

Final report must include:

- Summary.
- Changed files.
- Build result.
- Automation result.
- Runtime Smoke result.
- Log scan result.
- Automated acceptance status.
- Human PIE items not verified.
- Known issues/TBD.
- Commit hash.
- Push result.
- Working tree state.
