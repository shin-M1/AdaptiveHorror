# REQUIREMENTS.md — Adaptive Horror Product Requirements

This file describes what the Adaptive Horror demo should be when correct. It is not an implementation plan. See `ROADMAP.md` for sequence, `TEST_PLAN.md` for commands, and `TASKS/*.md` for current work.

Status labels:

- `Implemented`: Present in code and covered by recent build/test/runtime evidence.
- `Partial`: Present but still needs tuning, content, PIE confirmation, or broader coverage.
- `Planned`: Desired but not implemented.
- `TBD`: Unknown; do not invent values.

## Product overview

- Status: Partial.
- Adaptive Horror is a solo FPS survival-horror vertical slice built in Unreal Engine 5.
- Core fantasy: enemies observe player behavior, adapt their tactics, and evolve into variants that punish repeated habits without removing counterplay.
- Target demo length: 30-60 minutes, baseline 45 minutes.
- Current runtime map flow is generated from `/Engine/Maps/Entry` by `AEvaPrototypeGameMode`.

## Platform and performance

- Status: Partial.
- Primary platform: Windows 64-bit, Development Editor.
- Engine target currently verified in logs: UE5.8 / Win64.
- Input: keyboard and mouse.
- Multiplayer: Planned/deferred; Version 0.1 is solo.
- Recommended development memory: 16 GB minimum; if Unreal Build Accelerator or compiler memory pressure appears, use `Scripts/RunBuildCheck.ps1 -MaxParallelActions 1` or `2`.
- Performance target: TBD. Do not claim frame-rate success without PIE/profile data.

## Player

- Status: Implemented.
- Player class: `AEvaPlayerCharacter`.
- Controller class: `AEvaPlayerController`.
- Supports WASD movement, mouse look, jump, sprint, fire, reload, interact, flashlight, and debug keys in non-Shipping builds.
- Mouse sensitivity and invert-Y are stored in `UEvaSettingsSaveGame`.
- Interaction uses camera-origin Visibility tracing/sweeping against actor-owned interactable collision. Wall-through interaction must remain blocked.

## Combat, weapons, HP, death, checkpoints

- Status: Implemented/Partial.
- Health component: `UEvaHealthComponent`.
- Weapon base: `AEvaWeaponBase`; handgun hitscan implementation: `AEvaHitscanWeapon`.
- Current handgun baseline from existing docs/code history: damage 25, headshot x2, magazine 12, reserve 60, reload 1.5s, range 5000 cm.
- Player death enters Game Over flow, blocks gameplay input, and supports retry/checkpoint flow.
- Checkpoint actor: `AEvaCheckpoint`.
- Death and retry must not continue enemy attacks against stale player state.
- Full save-game checkpoint persistence is Planned/TBD; current runtime checkpoint behavior is memory/session based.

## EVA Learning

- Status: Implemented/Partial.
- Player telemetry component: `UEvaPlayerTelemetryComponent`.
- Learning subsystem: `UEvaLearningSubsystem`.
- Tracked concepts include shot count, hit count, headshot count, weapon use, average combat distance, kills, death cause, escape route, and hide spot.
- Combat style classification includes Berserker, Tactician, Ranger, Ghost, and Explorer.
- EVA analysis rate is clamped from 0-100%.
- Analysis stages: Learning, Adapting, Evolving.
- HUNTER alive/reinserted learning multiplier is 1.0; defeated cooldown multiplier is 0.3.
- Do not present the system as external/online machine learning. It is currently deterministic telemetry plus rules.

## Enemies and adaptation

- Status: Implemented/Partial.
- Base zombie: `AEvaZombieCharacter` with `AEvaZombieAIController`.
- Enemy variants:
  - Zombie: Implemented.
  - FAST: Implemented/Partial; visual and feel require PIE confirmation.
  - ARMORED: Implemented/Partial; visual and feel require PIE confirmation.
  - LONG ARM: Implemented/Partial; visual and feel require PIE confirmation.
  - COMPOSITE: Implemented/Partial; 80% EVA analysis variant, bounded hybrid behavior.
- HUNTER: `AEvaHunterCharacter` plus `AEvaHunterAIController`.
  - Status: Implemented/Partial.
  - Spawn triggers include kill threshold/time.
  - Defeat drops analysis/core concept and reduces learning multiplier.
  - Reinsertion increases tier and restores multiplier.
- ADAM boss: `AEvaAdamBossCharacter` plus `AEvaAdamBossAIController`.
  - Status: Implemented/Partial.
  - Has chase, melee attack, charge, roar summon, Phase 2, Boss HUD, and Stage Clear on defeat.
  - Visual, balance, and full fight feel remain PIE-driven.

## AI tracking, navigation, and spawn safety

- Status: Implemented/Partial.
- Runtime Graybox uses dynamic Runtime NavMesh behavior configured in `Config/DefaultEngine.ini`.
- `AEvaPrototypeGameMode` owns runtime arena generation, RuntimeFloor tracking, safe enemy spawn search, spawn logs, and game flow gating.
- Path following is primary. Direct fallback is only a recovery path and must not override valid NavMesh following.
- Spawn safety must reject unsafe floor, missing navigation, overlap, too-close-to-player, crowding, and presentation-unsafe view candidates where requested.
- Spawn/Navigation logs are part of the debugging contract.

## Research facility content

- Status: Implemented/Partial.
- Runtime facility has six zones:
  1. Entry Lobby.
  2. Security Corridor.
  3. Observation Lab.
  4. Containment Ward.
  5. Data Core Room.
  6. Adam Arena.
- Director: `AEvaResearchFacilityDirector`.
- Interactable: `AEvaFacilityInteractable`.
- Required content objects:
  - Power Console.
  - Security Keycard.
  - Locked Observation Lab door.
  - Research Logs:
    - `EVA LEARNING NOTES`
    - `HUNTER CONTAINMENT REPORT`
    - `ADAM EXPERIMENT RECORD`
  - Data Core Console.
- Objective order:
  1. Restore Facility Power.
  2. Find Security Keycard.
  3. Unlock Observation Lab.
  4. Search Containment Records.
  5. Access Data Core.
  6. Reach Adam Arena.
  7. Defeat Adam.
- Current limitation: PIE visibility/usability of content interactables must be confirmed by a human before merging field/content changes to `main`.

## UI and HUD

- Status: Implemented/Partial.
- Title, Pause, Settings, Game Over, and Stage Clear widgets are implemented in C++ menu widgets.
- Normal HUD includes HP, ammo, crosshair, EVA analysis/stage, HUNTER state, objective/progress, and interaction prompt.
- Debug HUD has three pages and is toggled with F9/N in non-Shipping builds.
- Boss HUD shows ADAM name, HP bar, phase, and debug details during Adam encounter.
- Enemy overhead display normally shows enemy name and HP bar; numeric HP is debug-only.
- UI layout and readability remain human PIE verification items.

## Presentation

- Status: Partial.
- Enemy placeholder visuals use engine standard meshes/parts.
- Visual Pass 1, Horror Immersion Pass, and Content Pass 1 have been merged into `main` as of current history.
- Flashlight is implemented.
- Blackout/horror warnings are implemented as prototype effects.
- Lighting uses runtime movable setup; exact darkness/readability is PIE-driven.
- Audio uses procedural/prototype tones and partial audio hooks. Final sound assets and BGM are Planned.

## Debug controls

- Status: Implemented for Development/Editor, disabled for Shipping by code guards.
- F1: EVA analysis +20.
- F2: force HUNTER deployment.
- F3: force zombie wave.
- F4: warp/start ADAM arena encounter.
- F5: restore player HP/ammo.
- F6: force Stage Clear.
- F7: telemetry snapshot.
- F9/N: Debug HUD/navigation visualization paging/toggle.
- F8: intentionally unbound because PIE uses it for Eject.
- P: no current game-side binding found.

## Regression rules

Do not regress:

- Development Editor / Win64 build.
- `Automation RunTests AdaptiveHorror`.
- Runtime Smoke launch/exit.
- Title -> New Game -> gameplay input flow.
- Player death and retry/checkpoint.
- Stage Clear idempotence and no post-clear death transition.
- Runtime NavMesh readiness for generated facility.
- Zombie chase/attack.
- HUNTER spawn/defeat/reinsert lifecycle.
- EVA analysis/adaptation/evolution thresholds.
- ADAM spawn/fight/defeat/Stage Clear.
- Content Pass keycard/door/log/data-core progression.
- Debug HUD availability in non-Shipping builds.

## TBD

- Final title/name/branding.
- Final packaged Windows performance target.
- Final art direction and asset list.
- Full save data format.
- Gamepad support.
- Real sound asset map and BGM rules.
- Final 30-60 minute balance values.
