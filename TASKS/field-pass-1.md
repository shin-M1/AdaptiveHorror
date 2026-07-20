# TASKS/field-pass-1.md — Field Pass 1

## Branch

- Start point: latest clean `main`.
- Branch: `feature/field-pass1`.
- Push target: `origin/feature/field-pass1`.
- Do not merge to `main`.

## Goal

Make the existing runtime research facility easier to read and play as a field, without adding new major game systems. The result should be a completion candidate that Codex can build, test, runtime-smoke, log-scan, commit, and push.

## Background

Current `main` contains the core game loop, Runtime Graybox, Content Pass 1 progression, Gameplay Pass 1 adaptation, Visual Pass 1, Horror Immersion Pass, HUNTER, ADAM, UI flow, and 43 Automation tests. Content Pass interactables have visible bodies and interaction collision, but human PIE confirmation remains required.

## Scope

### In scope

- Improve readability of the six existing runtime zones:
  - Entry Lobby.
  - Security Corridor.
  - Observation Lab.
  - Containment Ward.
  - Data Core Room.
  - Adam Arena.
- Add or adjust lightweight graybox dressing using engine standard primitives/components only.
- Add simple landmarks, signs, floor markings, lighting color accents, or non-blocking guide geometry.
- Keep required interactables visible and reachable:
  - Power Console.
  - Security Keycard.
  - Locked Observation Lab Door.
  - `EVA LEARNING NOTES`.
  - `HUNTER CONTAINMENT REPORT`.
  - `ADAM EXPERIMENT RECORD`.
  - Data Core Console.
- Keep the route from New Game to Adam Arena readable.
- Improve debug/log coverage for generated field structure if needed.
- Add or update Automation for structural checks where practical.
- Update required documents with verified and unverified items.

### Out of scope

- New weapons.
- New enemy classes.
- New maps or saved `.umap` conversion.
- New combat rules.
- New AI behavior or NavMesh algorithm changes.
- HUNTER or ADAM behavior changes.
- Stage Clear, Player Death, Title, Pause, Settings, or Game Over flow changes.
- Major UI redesign.
- Real art/audio asset production.
- Main branch merge.

## Dependencies

- `AGENTS.md`.
- `REQUIREMENTS.md`.
- `ROADMAP.md`.
- `TEST_PLAN.md`.
- Existing runtime facility generation in `AEvaPrototypeGameMode`.
- Existing content progression in `AEvaResearchFacilityDirector` and `AEvaFacilityInteractable`.

## File scope

### Primary files

- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.h`.
- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.cpp`.
- `Source/AdaptiveHorror/World/EvaResearchFacilityDirector.h`.
- `Source/AdaptiveHorror/World/EvaResearchFacilityDirector.cpp`.

### Secondary files

Modify only if directly required by the chosen implementation:

- `Source/AdaptiveHorror/World/EvaFacilityInteractable.h`.
- `Source/AdaptiveHorror/World/EvaFacilityInteractable.cpp`.
- `Source/AdaptiveHorror/UI/EvaHUD.cpp`.
- `Source/AdaptiveHorror/Tests/EvaLearningTests.cpp`.
- `Config/DefaultEngine.ini`.
- `DEV_LOG.md`.
- `TODO.md`.
- `BUILD_CHECK.md`.
- `NEXT_PROMPT.md`.

### Protected systems

Do not modify unless required to fix a demonstrated regression:

- Player combat and weapons.
- EVA Learning/adaptation math.
- Zombie AI behavior.
- HUNTER behavior.
- ADAM behavior.
- Save/settings.
- Title, Pause, Game Over, and Stage Clear flow.
- `Scripts/RunBuildCheck.ps1`.
- `Scripts/RunCodexValidation.ps1`.

## Architectural constraints

- Keep the runtime-generated facility model.
- Keep Runtime NavMesh behavior intact.
- Keep the current objective order from `REQUIREMENTS.md`.
- Preserve ADAM defeat as the Stage Clear trigger.
- Preserve debug-only controls as non-Shipping features.
- Do not rely on Live Coding for verification.

## Expected change scope

Expected scale:

- Primary implementation files: 2-6.
- Test files: 0-2.
- Documentation files: 2-4.
- New production classes: 0.
- New major systems: 0.

Stop and report before continuing if the implementation would exceed any of these limits:

- More than 12 source/config files changed.
- More than 2 new production files.
- More than 800 net new production lines.
- A protected system must be modified.
- A public interface must be changed across multiple systems.

## Acceptance criteria

### Automated

Codex must verify:

- Development Editor / Win64 build succeeds without Live Coding.
- `Automation RunTests AdaptiveHorror` succeeds.
- Runtime Smoke exits 0.
- Log scan finds no current-run Fatal / Ensure / Assertion / EXCEPTION / Stack overflow / Access violation.
- `git diff --check` succeeds.
- Runtime facility still creates six zones.
- Required interactables still exist and are not duplicated.
- Required interactables remain visible/enabled by component-state logs or Automation:
  - Power Console.
  - Security Keycard.
  - Locked Observation Lab Door.
  - Three Research Logs.
  - Data Core Console.
- Runtime NavMesh readiness is not regressed.
- Spawn safety tests remain green.
- Stage Clear tests remain green.

### Human PIE/manual

These are not automatically confirmable by Codex:

- Each zone is visually distinguishable.
- Navigation from one zone to the next is understandable without reading source code.
- Lighting is readable and not too dark.
- Keycard and Research Logs are easy to notice in normal play.
- Combat spaces are not too narrow or confusing.
- Enemies do not visibly pop in front of the player.
- New Game to Stage Clear can be completed in PIE.

Codex may commit and push a completion candidate if all automated criteria pass. Human PIE criteria must be listed as `Not verified by Codex`. Merge to `main` should wait for human PIE acceptance.

## Verification

### Commands

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

### Evidence required

For every automated acceptance criterion, the final report must include evidence.

Minimum evidence:

- Build command, exit code, result summary, and relevant log path.
- Automation command, total tests, passed tests, failed tests, and report/log path.
- Runtime Smoke command, exit code, runtime duration if available, and log path.
- Log scan file, blocking patterns checked, and match count.
- Structural requirement evidence from an Automation test name, runtime log line, or code assertion.

Do not use statements such as `looks correct`, `should work`, or `appears fixed` as verification evidence.

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

## Documentation updates

Update only documents needed to record the work:

- `DEV_LOG.md`.
- `TODO.md`.
- `BUILD_CHECK.md` if verification results or troubleshooting notes change.
- `NEXT_PROMPT.md` if the next task changes.

## Commit and push conditions

Commit only when:

- Automated acceptance criteria pass, or any impossible verification is explicitly documented as `Not verified`.
- No game-code changes outside the task scope remain unexplained.
- `git diff --check` succeeds.
- The final `git status --short --branch` is clean after commit.

Push `feature/field-pass1` to origin. Do not merge to `main`.

## Stop conditions

Stop and report instead of guessing when:

- A protected system must change without demonstrated regression evidence.
- Human PIE confirmation is required to choose the next correction.
- Toolchain or environment issues prevent required automated verification.
- Requirements conflict.
- The autonomous fix loop reaches the limits in `AGENTS.md`.

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
