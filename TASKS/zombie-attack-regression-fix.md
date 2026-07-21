# TASKS/zombie-attack-regression-fix.md

## Branch and Dependencies

- Base branch: `main`
- Working branch: `fix/zombie-attack-regression`
- Depends on:
  - Zone Identity Pass 1
  - Zone Identity Hotfix 1
  - PR #3 merged to `main`
- Required merge order: merge this branch only after PR #3 is already on `main`.

## Pull Request

- Expected PR base: `main`
- Expected PR compare: `fix/zombie-attack-regression`
- Review required: yes
- Required GitHub checks: use configured GitHub checks when present; absent checks are reported, not treated as failure.
- Merge strategy: Merge Commit
- Squash: Not allowed
- Rebase: Not allowed

## Purpose

- Goal: identify and minimally fix the regression where normal Zombies chase the player but do not enter a reliable attack/damage cycle.
- Current symptom: Zombies approach the player, but attack animation/state and player damage are not reliably observed.
- Expected behavior: normal Zombies acquire the player, chase, enter attack range, start attack, apply player damage once per cooldown, complete cooldown, repeat attack while still in range, and return to chase if the player moves away.
- Non-goals: new AI systems, new enemy types, new combat rules, new assets, tuning unrelated balance, or changes to HUNTER/ADAM/Zone/Boundary/Stage Clear.
- User-visible outcome: a normal Zombie can repeatedly attack and reduce player HP after reaching attack range.

## Scope

### In scope

- Normal Zombie attack-state diagnosis.
- Minimal Zombie attack gate/cooldown/damage fix.
- Development-only diagnostic logging with prefix `[ZombieAttack]`.
- Runtime summary logging with prefix `[ZombieAttackSummary]`.
- Narrow automation or runtime-smoke validation support if it fits existing test architecture.

### Out of scope / protected

- HUNTER AI.
- ADAM AI.
- Evolved enemy logic unless a shared base change is proven safe for normal Zombie only.
- Player movement.
- Weapon, shooting, and reload.
- Stage progression and required interactables.
- Stage Clear.
- Zone Identity geometry.
- Boundary geometry.
- Zone HUD tracking.
- Runtime NavMesh generation algorithm.
- Save.
- Title, pause, game-over UI.
- Lighting and audio.

## Primary files

- `Source/AdaptiveHorror/AI/EvaZombieAIController.cpp`
- `Source/AdaptiveHorror/AI/EvaZombieAIController.h`
- `Source/AdaptiveHorror/AI/EvaZombieCharacter.cpp`
- `Source/AdaptiveHorror/AI/EvaZombieCharacter.h`
- Related automation tests only if needed.

## Secondary files

- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.cpp`
- `Source/AdaptiveHorror/Player/EvaPlayerCharacter.cpp`
- `Source/AdaptiveHorror/Player/EvaHealthComponent.cpp`
- `Source/AdaptiveHorror/Tests/*.cpp`

## Investigation order

1. Zombie actor spawn.
2. AI controller possession.
3. Player target acquisition.
4. Chase start.
5. MoveTo result.
6. Player distance.
7. Attack range check.
8. Vertical distance check.
9. Line of sight / trace.
10. Attack condition gate.
11. State transition.
12. Attack cooldown gate.
13. Attack function invocation.
14. Animation state / feedback trigger.
15. Damage event.
16. Player HP change.
17. Attack completion.
18. Cooldown reset.
19. Repeated attack.

## Required logs

Use low-frequency or event-based logs with prefix `[ZombieAttack]`.

Required fields where applicable:

- Zombie identifier.
- Target validity.
- AI state / intent.
- Distance to player.
- Attack range.
- Vertical distance.
- Line of sight result.
- Attack condition result.
- Cooldown remaining.
- Attack started.
- Animation trigger requested.
- Damage attempted.
- Damage applied.
- Player HP before.
- Player HP after.
- Attack completed.
- Cooldown started.
- Cooldown completed.
- Returned to chase.
- Repeated attack count.

Runtime summary prefix: `[ZombieAttackSummary]`.

Summary fields:

- ZombieSpawned.
- TargetAcquired.
- EnteredAttackRange.
- AttackStarted.
- DamageApplied.
- PlayerHpChanged.
- CooldownStarted.
- CooldownCompleted.
- RepeatedAttackObserved.
- FatalErrors.
- EnsureFailures.

## Acceptance criteria

- Development Editor / Win64 build succeeds without Live Coding.
- `Automation RunTests AdaptiveHorror` succeeds.
- Runtime Smoke succeeds.
- Current-run Log Scan succeeds.
- `git diff --check` succeeds.
- Runtime Smoke or automation provides evidence of:
  - target acquisition,
  - entering attack range,
  - attack started,
  - damage applied,
  - player HP decreased,
  - cooldown started,
  - cooldown completed,
  - repeated attack when feasible.
- No protected-system regression is introduced.
- Human PIE items remain labeled as not verified by Codex.

## Pre-Merge Verification

- Build: `powershell -ExecutionPolicy Bypass -File .\Scripts\RunCodexValidation.ps1 -MaxParallelActions 4`
- Automation: included in validation script.
- Runtime Smoke: included in validation script.
- Log Scan: included in validation script plus `[ZombieAttackSummary]` review.
- `git diff --check`: included in validation script.
- Scope review: verify production changes stay focused on Zombie attack.
- Documentation review: update status docs after validation.
- Working tree clean: required before PR review/merge.

## Post-Merge Verification

- Update main: `git checkout main; git pull --ff-only origin main`
- Confirm expected commits are ancestors of main.
- Build on main: standard validation script.
- Automation on main: standard validation script.
- Runtime Smoke on main: standard validation script.
- Log Scan on main: standard validation script and `[ZombieAttackSummary]`.
- `git diff --check`: required.
- Working tree clean: required.
- Stop after reporting.

## Human Verification

- Required PIE checks:
  - A normal Zombie reaches the player and visibly attacks.
  - Player HP decreases on attack.
  - Damage is not applied every frame.
  - A second attack occurs after cooldown.
  - Zombie returns to chase if the player moves away.
  - Player death/respawn remains normal.
  - HUNTER, ADAM, and Stage Clear do not visibly regress.
- Previously human-verified: PR #3 branch was reported to preserve Zone behavior before this task.
- Not verified by Codex: all PIE visual/control/game-feel checks.

## Expected change scope

- Production files: target 1-4.
- Test files: target 0-2.
- Task/documentation files: target 1-5.
- New production classes: 0.
- New assets: 0.
- Config changes: 0.
- Plugin changes: 0.
- Stop if broad AI redesign, Player Damage redesign, Zone/Boundary edits, or protected-system changes become necessary.

## Commit / Push

- Commit message: `Fix zombie attack regression`
- Push target: `origin/fix/zombie-attack-regression`

## Final report

- Summary.
- Branch.
- Starting main commit.
- Symptom reproduced.
- Root cause.
- Investigation evidence.
- First failing pipeline stage.
- Minimal fix.
- Production files changed.
- Tests added or updated.
- `[ZombieAttack]` evidence.
- `[ZombieAttackSummary]`.
- Player HP before / after.
- Cooldown evidence.
- Repeated attack evidence.
- HUNTER status.
- ADAM status.
- Player Damage semantics.
- Build.
- Automation.
- Runtime Smoke.
- Log Scan.
- `git diff --check`.
- Expected change scope.
- Commit hash.
- Push result.
- PR number and URL.
- PR review result.
- PR merge status.
- Merge commit hash.
- Final main validation.
- Working tree.
- Remaining blockers.
- Human verification required.

## Implementation status - 2026-07-22

- Root cause identified:
  - Player-target `MoveToActor` used stop-on-overlap, allowing path following to stop before explicit Zombie `AttackRange`.
  - Attack LoS trace did not ignore the target actor, so the target could block the attack condition.
- Minimal fix:
  - Use `bStopOnOverlap=false` only when chasing the current player target.
  - Ignore target actor in the attack LoS trace.
  - Preserve non-target actor MoveTo overlap behavior.
- Automated validation:
  - Development Editor / Win64 build: PASS.
  - Automation: 45 success, 0 failures.
  - Runtime Smoke: PASS with `[ZombieAttackSummary] RuntimeSmokeResult=PASS`.
  - Log Scan: PASS.
  - `git diff --check`: PASS.
- Human PIE:
  - Not verified by Codex.
