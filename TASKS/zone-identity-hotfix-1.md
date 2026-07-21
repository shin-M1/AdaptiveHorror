# TASKS/zone-identity-hotfix-1.md — Zone Identity Hotfix 1

## Branch

- Start point: latest clean `main`.
- Branch: `feature/zone-identity-hotfix1`.
- Push target: `origin/feature/zone-identity-hotfix1`.
- Do not merge to `main`.

## Context

- Human PIE feedback from Zone Identity Pass 1 found player-fall gaps at early zone connections.
- Latest `main` in this workspace does not contain `TASKS/zone-identity-pass-1.md` or the Zone Identity Pass 1 production changes, while `origin/feature/zone-identity-pass1` does.
- This hotfix stays on the user-requested latest-main branch and applies only the runtime facility geometry/connectivity work needed for the hotfix candidate.

## Goal

Fix only the Zone Identity Pass 1 PIE issues:

1. Remove floor/wall/collision gaps at zone connections, especially:
   - Entry Lobby -> Security Corridor.
   - Security Corridor -> Observation Lab.
2. Slightly widen readable zone spaces without changing progression or gameplay systems.

## Scope

### In scope

- Runtime-generated facility geometry inside `AEvaPrototypeGameMode`.
- Existing primitive mesh layout only.
- Existing `ZoneIdentity` logs.
- New `ConnectionIntegrity` runtime logs for five zone links:
  - Entry -> Security.
  - Security -> Observation.
  - Observation -> Containment.
  - Containment -> DataCore.
  - DataCore -> Arena.

### Out of scope / protected

- New systems, rooms, enemies, weapons, encounters, rules, art, audio, or lighting.
- AI behavior.
- Combat.
- HUNTER.
- ADAM.
- Stage Clear.
- UI.
- Save/settings.
- Runtime NavMesh algorithm/configuration.

## Expected change scope

- Production changed files: 2-4 maximum.
- Production net-new lines: target 250 or fewer.
- Stop if protected systems or public interface changes become necessary.

## Automated acceptance criteria

- Development Editor / Win64 build succeeds without Live Coding.
- `Automation RunTests AdaptiveHorror` keeps the 43-test baseline and succeeds.
- Runtime Smoke exits 0.
- Current runtime log contains:
  - six `ZoneIdentity` lines,
  - five `ConnectionIntegrity` lines,
  - `Connected=true`,
  - `GapDetected=false`,
  - Runtime NavMesh readiness with player and representative projection.
- Existing Stage Clear regression tests remain green.
- Log scan finds no current-run Fatal / Ensure failure / Assertion failure / EXCEPTION / Stack overflow / Access violation / `NavReady=false` / automation failure.
- `git diff --check` succeeds.

## Human PIE / manual

Not verified by Codex:

- Player no longer falls between Entry Lobby and Security Corridor.
- Player no longer falls between Security Corridor and Observation Lab.
- Expanded zones feel less cramped.
- Full New Game to Adam Arena traversal remains comfortable.

## Verification command

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\RunCodexValidation.ps1 -MaxParallelActions 4
```

Lower `-MaxParallelActions` only for memory/toolchain pressure.

## Commit

```powershell
git add .
git commit -m "Implement Zone Identity Hotfix 1"
git push -u origin feature/zone-identity-hotfix1
```
