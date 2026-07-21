# TASKS/zone-identity-pass-1.md — Zone Identity Pass 1

## Branch

- Start point: latest clean `main`.
- Branch: `feature/zone-identity-pass1`.
- Push target: `origin/feature/zone-identity-pass1`.
- Do not merge to `main`.

## Goal

Improve the spatial identity of the six runtime-generated research facility zones so the player can identify the current area by room structure, not color alone.

## Background

PIE feedback after the Field Pass direction pass:

- Rooms still feel too similar.
- The overall layout feels too linear.
- Zone personality is weak.

Current `main` does not contain `TASKS/TEMPLATE.md` or the local `feature/field-pass1` implementation, so this task is created from the current user instruction while staying on latest `main`.

## Scope

### In scope

- Modify runtime-generated geometry only inside existing research facility generation.
- Keep six zones and progression order:
  1. Entry Lobby.
  2. Security Corridor.
  3. Observation Lab.
  4. Containment Ward.
  5. Data Core Room.
  6. Adam Arena.
- Make zones structurally distinct using existing engine primitives/components:
  - room shape,
  - width,
  - height,
  - cover placement,
  - central structure,
  - sightline shape,
  - walking pattern.
- Add `ZoneIdentity` runtime logs for each zone:
  - `FloorArea`,
  - `ObstacleCount`,
  - `LandmarkCount`,
  - `AverageWidth`,
  - `AverageHeight`,
  - `ZoneShape`.
- Update required documentation with verification and PIE-not-verified notes.

### Out of scope

- New gameplay systems.
- New enemies, weapons, maps, rules, AI behavior, or NavMesh algorithms.
- HUNTER, ADAM, Player, Combat, Stage Clear, UI, Save/settings behavior changes.
- Audio, horror effects, or art assets.
- Maze-like layout changes.
- Main branch merge.

## File scope

### Primary files

- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.h`
- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.cpp`

### Secondary files

Modify only if needed for documentation or verification:

- `DEV_LOG.md`
- `TODO.md`
- `BUILD_CHECK.md`
- `NEXT_PROMPT.md`
- `ROADMAP.md`

### Protected systems

Do not change:

- AI behavior and pathfinding algorithms.
- HUNTER.
- ADAM.
- Player combat/weapons.
- Player Death / Stage Clear / Title / Pause / Settings / Game Over.
- Save/settings.
- Content objective rules and interactable behavior.
- Runtime NavMesh algorithm/configuration.

## Expected change scope

Stop and report before continuing if any of these become necessary:

- More than 12 source/config files changed.
- More than 2 new production files.
- More than 800 net new production lines.
- Protected system behavior change.
- Public interface change across multiple systems.

## Acceptance criteria

### Automated

- Development Editor / Win64 build succeeds without Live Coding.
- `Automation RunTests AdaptiveHorror` succeeds and keeps the existing 43-test baseline.
- Runtime Smoke exits 0.
- Log scan finds no current-run Fatal / Ensure / Assertion / EXCEPTION / Stack overflow / Access violation.
- `git diff --check` succeeds.
- Runtime log confirms six zones were generated.
- Runtime log contains one `ZoneIdentity` line per zone.
- Runtime log confirms Runtime NavMesh readiness with player and representative projection.
- Existing spawn safety tests remain green.
- Existing Stage Clear regression tests remain green.

### Human PIE/manual

Not verified by Codex:

- Entry Lobby reads as an open reception space.
- Security Corridor reads as a light zigzag/L-shape route without becoming a maze.
- Observation Lab forces a readable loop around central equipment.
- Containment Ward reads as side cells plus denser cover.
- Data Core reads as a half-loop around the central core.
- Adam Arena remains large enough for the boss fight.
- Full New Game to Stage Clear playthrough remains comfortable.

## Verification

Preferred:

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\RunCodexValidation.ps1 -MaxParallelActions 4
```

If memory pressure occurs, retry only with `-MaxParallelActions 2` or `1`.

## Documentation updates

- `DEV_LOG.md`
- `TODO.md`
- `BUILD_CHECK.md`
- `NEXT_PROMPT.md`
- `ROADMAP.md` if milestone state changes.

## Commit and push conditions

Commit only after automated acceptance criteria pass.

Use:

```powershell
git add .
git commit -m "Implement Zone Identity Pass 1"
git push -u origin feature/zone-identity-pass1
```

If push is blocked by environment policy, report the exact blocker and leave the local commit clean.

## Completion report

- Summary.
- Changed files.
- Build.
- Automation.
- Runtime Smoke.
- `ZoneIdentity` logs.
- Log Scan.
- Expected change scope.
- Known issues.
- Human PIE items.
- Commit hash.
- Push result.
- Working tree.
