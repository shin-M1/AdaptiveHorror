# Next Codex Prompt

## Latest handoff - 2026-07-14 Cycle 020 Enemy Intent Display Consistency

You are continuing the UE5.8 C++ Adaptive Horror prototype.

Current branch: `feature/gameplay-pass1`.

Do not merge this branch into `main` until the user confirms Gameplay Pass 1 Polish in PIE.

### Current verified state

- Development Editor / Win64 build without Live Coding: succeeded.
- `Automation RunTests AdaptiveHorror`: 37 project tests succeeded, 0 project test failures.
- Runtime smoke with `UnrealEditor-Cmd.exe -game -NullRHI -NoSound -ExecCmds="Quit"`: exit code 0.
- `git diff --check`: succeeded, no whitespace errors. CRLF conversion warnings only.
- PIE viewport confirmation was not performed by Codex.

### Most recent fix

- Fixed inconsistent overhead Intent display for existing/spawned enemies while Debug HUD is ON.
- Zombie AI intent now resolves to a safe non-empty state instead of staying blank:
  - `CHASE`
  - `ATTACK`
  - `SEARCH`
  - `IDLE`
- Spawn priming now forces an intent refresh for initial zombies, wave zombies, evolved/adaptive spawns, HUNTER, and HUNTER reinsertion paths that use the common enemy priming flow.
- Debug HUD F9/N now synchronizes existing enemies, so already-spawned enemies update when Debug HUD is toggled or pages are switched.
- Debug Intent labels are still hidden in normal play and remain separate from enemy name / HP bar.
- ADAM was intentionally not given an overhead Intent label.
- Added low-frequency `[EnemyIntent]` logs:
  - Actor
  - EnemyType
  - Intent
  - DebugVisible
  - ControllerValid
- Added automation coverage for:
  - Spawned enemy intent initialization.
  - Controller fallback intent.
  - Debug OFF hiding the intent label while keeping text initialized.
  - HUNTER counter intent preservation.

### Next highest-priority task

Run PIE and verify only the overhead Intent display consistency. Do not add new features, enemies, weapons, maps, or balance changes.

PIE checklist:

1. Press F9 and confirm Debug HUD turns ON.
2. Confirm overhead Intent appears for:
   - Initial Zombie.
   - Wave Zombie.
   - FAST.
   - ARMORED.
   - LONG ARM.
   - COMPOSITE.
   - HUNTER.
   - HUNTER reinserted individual.
3. Confirm no displayed enemy has an empty Intent.
4. Confirm reasonable fallback text appears when state is not yet assigned, e.g. `IDLE`, `SEARCH`, or `CHASE`.
5. Confirm N page switching keeps existing enemy Intent labels synchronized.
6. Confirm F9 Debug OFF hides Intent labels for all existing enemies.
7. Confirm normal enemy name and HP bar remain readable and do not overlap more than before.
8. Confirm ADAM does not receive an overhead Intent label.
9. Confirm gameplay loop, enemy behavior, HUNTER, ADAM, and Stage Clear still behave as before.

If a problem remains:

- Fix only the Intent display inconsistency.
- Do not change AI balance, path following, spawn rules, ADAM, HUNTER behavior, Stage Clear, UI flow, audio, or visual presentation unless the Intent display change directly caused the regression.

### Important files

- `Source/AdaptiveHorror/AI/EvaZombieAIController.h`
- `Source/AdaptiveHorror/AI/EvaZombieAIController.cpp`
- `Source/AdaptiveHorror/AI/EvaZombieCharacter.h`
- `Source/AdaptiveHorror/AI/EvaZombieCharacter.cpp`
- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.h`
- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.cpp`
- `Source/AdaptiveHorror/Tests/EvaLearningTests.cpp`

### Completion condition for next pass

- User confirms in PIE that all target enemies show non-empty Intent while Debug HUD is ON.
- User confirms Intent hides for all enemies when Debug HUD is OFF.
- Development Editor / Win64 build succeeds.
- Automation RunTests `AdaptiveHorror` succeeds.
- Runtime smoke succeeds.
- Docs are updated with the actual PIE result.

## Latest handoff - 2026-07-14 Cycle 019 Gameplay Pass 1 Polish

You are continuing the UE5.8 C++ Adaptive Horror prototype.

Current branch: `feature/gameplay-pass1`.

Do not merge this branch into `main` until the user confirms the Gameplay Pass 1 Polish in PIE.

### Current verified state

- Development Editor / Win64 build without Live Coding: succeeded.
- `Automation RunTests AdaptiveHorror`: 35 project tests succeeded, 0 project test failures.
- Runtime smoke with `UnrealEditor-Cmd.exe -game -NullRHI -NoSound -ExecCmds="Quit"`: exit code 0.
- PIE visual/gameplay confirmation is still required; Codex did not visually inspect the viewport.

### Most recent implementation

- Debug HUD:
  - Split into 3 pages.
  - F9 toggles Debug HUD ON/OFF and preserves existing Navigation visualization toggle behavior.
  - N advances Debug HUD pages without toggling Navigation visualization.
  - Page 1: EVA / Gameplay.
  - Page 2: Enemy Adaptation.
  - Page 3: Navigation / Spawn.
- Enemy role visibility:
  - Added Debug-only overhead Intent labels for enemies/HUNTER.
  - Intent labels update only when the actual AI intent changes.
  - Intent labels hide when Debug HUD is off, enemy dies, overhead display is disabled, or Stage Clear stops combat.
- Role polish:
  - FAST has clearer flank/sidestep behavior.
  - ARMORED is slower and less likely to sidestep/disengage.
  - LONG ARM has clearer mid-range attack pressure with wall-hit protection for long-reach attacks.
  - COMPOSITE remains bounded, exposes a short Hybrid Type, and keeps its selected hybrid for a minimum hold window.
  - HUNTER balance was not broadly changed; only counter display readability was improved.

### Next highest-priority task

Run PIE and verify only the Gameplay Pass 1 Polish. Do not add new enemies, weapons, maps, debug keys, or large UI changes.

PIE checklist:

1. Confirm F9 toggles Debug HUD and Navigation visualization.
2. Confirm N advances DEBUG 1/3 -> 2/3 -> 3/3 without toggling Navigation visualization.
3. Confirm Debug HUD does not overlap normal HUD at 1280x720.
4. Confirm Debug HUD is hidden in Title / Game Over / Stage Clear.
5. Confirm enemy overhead Intent labels appear only while Debug HUD is ON.
6. Confirm Intent labels do not overlap enemy name / HP bar.
7. Confirm FAST visibly flanks and feels faster without getting stuck.
8. Confirm ARMORED feels slower and front-holding without blocking all paths.
9. Confirm LONG ARM attacks from a clearer longer range and does not attack through walls.
10. Confirm COMPOSITE shows a readable Hybrid Type and feels like a bounded counter, not an all-max enemy.
11. Confirm HUNTER still feels unchanged in balance, with improved counter readability.
12. Confirm existing zombie chase, HUNTER reinsertion, ADAM, Stage Clear, death/respawn, UI flow, visual/audio/horror presentation, HP bars, and Boss HUD still work.

If a problem is found:

- Fix only the observed Gameplay Pass 1 Polish regression.
- Preserve Runtime NavMesh, path-following/repathing, spawn safety, HUNTER reinsertion, ADAM, Stage Clear, Player Death, Title, Pause, Settings, Game Over, Visual / Audio, Horror Immersion, Boss HUD, and enemy HP bars.

### Important files

- `Source/AdaptiveHorror/UI/EvaHUD.cpp`
- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.h`
- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.cpp`
- `Source/AdaptiveHorror/AI/EvaTelemetryTypes.h`
- `Source/AdaptiveHorror/AI/EvaLearningSubsystem.cpp`
- `Source/AdaptiveHorror/AI/EvaZombieCharacter.h`
- `Source/AdaptiveHorror/AI/EvaZombieCharacter.cpp`
- `Source/AdaptiveHorror/AI/EvaZombieAIController.h`
- `Source/AdaptiveHorror/AI/EvaZombieAIController.cpp`
- `Source/AdaptiveHorror/AI/EvaHunterCharacter.cpp`
- `Source/AdaptiveHorror/AI/EvaHunterAIController.cpp`
- `Source/AdaptiveHorror/Tests/EvaLearningTests.cpp`

### Completion condition for next pass

- User confirms the Debug HUD pages and Intent labels are readable in PIE.
- User confirms FAST / ARMORED / LONG ARM / COMPOSITE differences are more visible without game-loop regression.
- Development Editor / Win64 build succeeds.
- Automation RunTests `AdaptiveHorror` succeeds.
- Runtime smoke succeeds.
- Docs are updated with the actual PIE result.

## Latest handoff - 2026-07-14 Cycle 018 Gameplay Pass 1

You are continuing the UE5.8 C++ Adaptive Horror prototype.

Current branch: `feature/gameplay-pass1`.

Do not merge this branch into `main` until the user confirms the adaptive gameplay pass in PIE.

### Current verified state

- Branch was created from latest `main`.
- Development Editor / Win64 build without Live Coding: succeeded.
- `Automation RunTests AdaptiveHorror`: 32 project tests succeeded, 0 project test failures.
- Runtime smoke with `UnrealEditor-Cmd.exe -game -NullRHI -NoSound -ExecCmds="Quit"`: exit code 0.
- PIE visual/gameplay confirmation is still required; Codex did not visually inspect the viewport.

### Most recent implementation

- EVA adaptation profile:
  - Aggregates weapon/combat telemetry into a bounded profile.
  - Tracks headshot rate, accuracy, preferred distance, close/long range ratios, damage-taken tendency, sprint usage, stealth, exploration, dominant weapon, combat style, EVA analysis percent, and analysis stage.
  - Updates only while gameplay is active; pauses/stops during pause, death, title, and Stage Clear.
- Enemy behavior:
  - Zombies apply safe adaptation tuning from the profile.
  - Roles include Standard, Flanker, Frontliner, MidRangePressure, Searcher, Ambusher, and CompositeAdaptive.
  - Existing path-following/repathing behavior was preserved.
- Evolution:
  - FAST / ARMORED / LONG ARM remain specific.
  - COMPOSITE remains the 80% EVA analysis variant, but no longer stacks every advantage at full strength.
- HUNTER:
  - HUNTER locks a counter profile per deployment/reinsertion.
  - HUNTER defeat stores the profile for later tiers.
  - Labels can show counter tags such as `HUNTER T2 [ANTI-RANGER]`.
- HUD:
  - Normal HUD shows EVA stage, analysis, and combat style.
  - Debug HUD (`F9` / `N`) shows adaptation profile, enemy tuning multipliers, and HUNTER counter type.
- Tests:
  - Added Gameplay Pass 1 tests for profile clamping, combat style selection, enemy tuning, COMPOSITE bounded behavior, HUNTER counter profile, and reset behavior.

### Next highest-priority task

Run PIE and verify the adaptive gameplay feel only. Do not add new enemies, weapons, maps, debug keys, or large UI changes. Do not rebalance until a specific observed problem is found.

PIE checklist:

1. Start New Game and confirm baseline title/menu/pause/game-over/stage-clear flow still works.
2. Confirm normal zombies still chase, repath around obstacles, attack, take damage, and die.
3. Use F1/F3/F7 and ordinary combat to raise EVA analysis and observe adaptation debug values.
4. Confirm close-range/Berserker play causes enemies/HUNTER to counter by spacing, flanking, or pressure without breaking chase.
5. Confirm long-range/Ranger play causes cover/side-pressure behavior without making enemies passive.
6. Confirm Ghost/Searcher/Ambusher behavior is understandable when hide spots or escape routes are logged.
7. Confirm COMPOSITE at 80% analysis is distinct but not unfair.
8. Confirm HUNTER labels/counter behavior after spawn, defeat, and reinsertion.
9. Confirm ADAM still starts, fights, dies, and Stage Clear still fires once.
10. Confirm logs during active gameplay include `[EVAProfile]`, `[EnemyAdapt]`, and `[HunterAdapt]`.

If a problem is found:

- Fix only the observed Gameplay Pass 1 regression.
- Preserve Runtime NavMesh, zombie path following, HUNTER reinsertion, ADAM, Stage Clear, player death, UI flow, visual/audio/horror pass behavior, HP bars, and Boss HUD.

### Important files

- `Source/AdaptiveHorror/AI/EvaTelemetryTypes.h`
- `Source/AdaptiveHorror/AI/EvaLearningSubsystem.h`
- `Source/AdaptiveHorror/AI/EvaLearningSubsystem.cpp`
- `Source/AdaptiveHorror/AI/EvaZombieAIController.h`
- `Source/AdaptiveHorror/AI/EvaZombieAIController.cpp`
- `Source/AdaptiveHorror/AI/EvaZombieCharacter.cpp`
- `Source/AdaptiveHorror/AI/EvaHunterAIController.h`
- `Source/AdaptiveHorror/AI/EvaHunterAIController.cpp`
- `Source/AdaptiveHorror/AI/EvaHunterCharacter.h`
- `Source/AdaptiveHorror/AI/EvaHunterCharacter.cpp`
- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.h`
- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.cpp`
- `Source/AdaptiveHorror/UI/EvaHUD.cpp`
- `Source/AdaptiveHorror/Tests/EvaLearningTests.cpp`

### Completion condition for next pass

- User confirms in PIE that enemies feel adaptive without new progression blockers.
- Development Editor / Win64 build succeeds.
- Automation RunTests `AdaptiveHorror` succeeds.
- Runtime smoke succeeds.
- Docs are updated with the actual PIE result.

## Latest handoff - 2026-07-14 Cycle 017 Horror Immersion Pass 1

You are continuing the UE5.8 C++ Adaptive Horror prototype.

Current branch: `feature/horror-immersion-pass1`.

Do not merge this branch into `main` until the user visually confirms the horror presentation pass in PIE.

### Current verified state

- `feature/visual-audio-pass1` was merged into `main` before this branch was created.
- Development Editor / Win64 build without Live Coding: succeeded.
- `Automation RunTests AdaptiveHorror`: 25 project tests succeeded, 0 project test failures.
- Runtime smoke with `UnrealEditor-Cmd.exe -game -NullRHI -NoSound -ExecCmds="Quit"`: exit code 0.
- PIE visual/audio verification is still required; Codex did not visually inspect the viewport or hear audio.

### Most recent implementation

- Horror effects:
  - Gameplay-only blackout with light restore.
  - Emergency light flicker with `Eva.ReduceFlashing`.
  - Runtime fog for the graybox facility.
  - Door/zone cue and short blackout on selected first-time zone entries.
  - HUNTER arrival cue.
  - ADAM entrance, charge, roar, and Phase 2 cues.
- Player presentation:
  - `F` toggles a prototype flashlight.
  - Damage flash, low-health vignette, breathing pulse, and optional camera shake.
  - `Eva.ReduceCameraShake` reduces prototype shake.
- HUD:
  - Gameplay-only blackout/pulse/damage overlay and centered horror warning text.
  - Title / Pause / Game Over / Stage Clear are kept clear of horror overlays.
- Debug / comfort:
  - `Eva.DebugBlackout [seconds]`
  - `Eva.ReduceFlashing 1`
  - `Eva.ReduceCameraShake 1`
- Tests:
  - Blackout flow guard.
  - Player feedback clamp test.

### Next highest-priority task

Run PIE and verify the horror-immersion pass only. Do not add new AI, enemies, weapons, maps, or rules. Do not rebalance gameplay unless a regression is directly caused by this branch.

PIE checklist:

1. Confirm Title / New Game / Pause / Game Over / Stage Clear still work.
2. Confirm blackout triggers only during gameplay, restores lights, and does not remain stuck.
3. Confirm `Eva.DebugBlackout 2` works during gameplay and is ignored outside gameplay.
4. Confirm emergency flicker reads as alarm lighting and `Eva.ReduceFlashing 1` makes it calmer.
5. Confirm flashlight toggle with `F` works and does not break input.
6. Confirm fog improves mood without hiding objectives/enemies too much.
7. Confirm damage flash/vignette/camera shake are readable but not excessive.
8. Confirm HUNTER arrival warning/effect appears.
9. Confirm ADAM entrance / charge / roar / Phase 2 cues are distinguishable.
10. Confirm existing zombie chase, HUNTER, ADAM, Stage Clear, Game Over, and debug keys still work.

If a problem is found:

- Fix only the presentation regression.
- Do not change NavMesh, path following, spawn rules, game flow, boss logic, or balance unless the horror presentation change directly caused the problem.

### Important files

- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.h`
- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.cpp`
- `Source/AdaptiveHorror/Characters/EvaPlayerCharacter.h`
- `Source/AdaptiveHorror/Characters/EvaPlayerCharacter.cpp`
- `Source/AdaptiveHorror/UI/EvaHUD.h`
- `Source/AdaptiveHorror/UI/EvaHUD.cpp`
- `Source/AdaptiveHorror/World/EvaResearchFacilityDirector.cpp`
- `Source/AdaptiveHorror/AI/EvaAdamBossCharacter.cpp`
- `Source/AdaptiveHorror/Tests/EvaLearningTests.cpp`

### Completion condition for next pass

- User confirms in PIE that the horror pass improves atmosphere without AI/gameflow regression.
- Development Editor / Win64 build succeeds.
- Automation RunTests `AdaptiveHorror` succeeds.
- Runtime smoke succeeds.
- Docs are updated with the actual PIE result.

## Latest handoff - 2026-07-14 Cycle 016 Visual / Audio Pass 1

You are continuing the UE5.8 C++ Adaptive Horror prototype.

Current branch: `feature/visual-audio-pass1`.

Do not merge this branch into `main` until the user visually confirms the pass in PIE.

### Current verified state

- Development Editor / Win64 build without Live Coding: succeeded.
- `Automation RunTests AdaptiveHorror`: 23 tests succeeded, 0 failures.
- Runtime smoke with `UnrealEditor-Cmd.exe -game -NullRHI -NoSound -ExecCmds="Quit"`: exit code 0.
- Runtime log confirms `EvaPrototypeGameMode` loaded and Title state/UI initialized.
- PIE visual/audio verification is still required; Codex did not visually inspect the viewport or hear audio.

### Most recent implementation

- Enemy visual pass:
  - Added leg and shoulder primitive mesh parts.
  - Zombie / FAST / ARMORED / LONG ARM / COMPOSITE / HUNTER / ADAM now have clearer silhouettes.
  - Material tint is attempted through dynamic material parameters, but shape differences do not depend on tint.
- Simple animation pass:
  - Idle/walk component rotation.
  - Attack feedback for regular enemies.
  - ADAM Attack / Charge / Roar have distinct temporary poses.
- Audio pass:
  - Added `UEvaAudioFunctionLibrary` for procedural placeholder tones.
  - UI tones now share that helper.
  - Added temporary tones for gun, reload, player damage/death, enemy attack/death, HUNTER spawn, ADAM cues, and facility ambience.
- Lighting pass:
  - Darker movable base lighting.
  - Red emergency point lights per facility zone.
  - Runtime graybox geometry mobility was not changed to avoid NavMesh/gameplay risk.

### Next highest-priority task

Run PIE and verify visuals/audio only. Do not rebalance AI or gameplay unless a regression is directly caused by this visual/audio branch.

PIE checklist:

1. Confirm title screen still appears and NEW GAME starts gameplay.
2. Confirm Zombie / FAST / ARMORED / LONG ARM / COMPOSITE / HUNTER / ADAM are visually distinguishable.
3. Confirm enemies visibly walk and attack.
4. Confirm ADAM Attack / Charge / Roar can be recognized visually.
5. Confirm gun, reload, damage, enemy attack, HUNTER spawn, ADAM cues, Stage Clear, Game Over, and UI tones are audible.
6. Confirm lighting is darker with emergency/alarm tone.
7. Confirm no unwanted `Lighting needs to be rebuilt` warning appears.
8. Confirm existing zombie chase, HUNTER, ADAM, Stage Clear, Game Over, and debug keys still work.

If a problem is found:

- Fix only the visual/audio regression.
- Do not change NavMesh, path following, game flow, boss logic, or balance unless the visual/audio change directly caused the issue.

### Important files

- `Source/AdaptiveHorror/Audio/EvaAudioFunctionLibrary.h`
- `Source/AdaptiveHorror/Audio/EvaAudioFunctionLibrary.cpp`
- `Source/AdaptiveHorror/AI/EvaZombieCharacter.h`
- `Source/AdaptiveHorror/AI/EvaZombieCharacter.cpp`
- `Source/AdaptiveHorror/AI/EvaHunterCharacter.cpp`
- `Source/AdaptiveHorror/AI/EvaAdamBossCharacter.h`
- `Source/AdaptiveHorror/AI/EvaAdamBossCharacter.cpp`
- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.cpp`
- `Source/AdaptiveHorror/Weapons/EvaWeaponBase.cpp`
- `Source/AdaptiveHorror/Weapons/EvaHitscanWeapon.cpp`
- `Source/AdaptiveHorror/Characters/EvaPlayerCharacter.cpp`
- `Source/AdaptiveHorror/Characters/EvaPlayerController.cpp`

### Completion condition for next pass

- User confirms in PIE that the game looks/sounds more game-like without AI/gameflow regression.
- Development Editor / Win64 build succeeds.
- Automation RunTests `AdaptiveHorror` succeeds.
- Runtime smoke succeeds.
- Docs are updated with the actual PIE result.

## Latest handoff - 2026-07-14 Cycle 015 Title widget display / PIE input flow

You are continuing the UE5.8 C++ Adaptive Horror prototype.

### Current verified state

- Development Editor / Win64 build without Live Coding: succeeded.
- `Automation RunTests AdaptiveHorror`: 21 tests succeeded, 0 failures.
- Runtime smoke with `UnrealEditor-Cmd.exe -game -NullRHI -ExecCmds="Quit"`: exit code 0.
- Runtime smoke log confirms Title UI is now actually created and added to the viewport:
  - `CurrentState=EEvaGameFlowState::Title`
  - `TitleWidgetClassValid=true`
  - `CreateWidgetResult=true`
  - `RootWidgetValid=true`
  - `AddToViewportAttempted=true`
  - `IsInViewport=true`
  - `Visibility=ESlateVisibility::Visible`
  - `RenderOpacity=1.00`
  - `NativeConstructCalled=true`
  - `FocusAssigned=true`
  - `InputMode=GameAndUI`
  - `ShowMouseCursor=true`
  - `IgnoreMoveInput=true`
  - `IgnoreLookInput=true`
- PIE visual verification is still required; Codex did not visually inspect the viewport.

### Most recent fix

Fixed the bug where PIE entered Title state with cursor visible but no title text/buttons appeared, leaving the player unable to click NEW GAME or control the pawn.

Root cause:

- C++ menu widgets were creating `WidgetTree->RootWidget` in `NativeConstruct()`.
- For native C++ `UUserWidget` classes, this can be too late because Slate rebuild has already happened.
- Result: flow/input entered Title, but the actual displayed widget could be empty.

Fix:

- `UEvaMenuWidgetBase::RebuildWidget()` now builds the native WidgetTree before Slate rebuild completes.
- Title/Pause/GameOver/StageClear/Settings menus now build content through `BuildMenuContent()`.
- Primary menu buttons get initial focus.
- Title/Settings input explicitly blocks gameplay input.
- Added rich diagnostics:
  - `[GameFlow]`
  - `[TitleUI]`
  - `[InputState]`
  - `[Player]`
- Added a Development fallback: if a local PlayerController exists and Title UI still fails to become visible, the game logs an error and falls back to `StartNewGameFlow()` so PIE is not trapped in an unusable state.

### Next highest-priority task

Run PIE and verify the title screen visually.

PIE checklist:

1. Press Play.
2. Confirm title text/buttons are visible:
   - `ADAPTIVE HORROR`
   - `NEW GAME`
   - `CONTINUE - Not Available`
   - `SETTINGS`
   - `EXIT`
3. Confirm the log contains `[TitleUI] ... IsInViewport=true ... FailureReason=None`.
4. Click `NEW GAME`.
5. Confirm log contains `[GameFlow] ... CurrentState=EEvaGameFlowState::Playing`.
6. Confirm log contains `[InputState] ... InputMode=GameOnly ShowMouseCursor=false IgnoreMoveInput=false IgnoreLookInput=false`.
7. Confirm log contains `[Player] Context=StartNewGameFlow PossessedPawn=EvaPlayerCharacter`.
8. Confirm WASD, mouse look, shooting, and Esc Pause all work after NEW GAME.

If Title is still invisible:

- Search for `[TitleUI]`.
- Check:
  - `TitleWidgetClassValid`
  - `CreateWidgetResult`
  - `RootWidgetValid`
  - `AddToViewportAttempted`
  - `IsInViewport`
  - `Visibility`
  - `RenderOpacity`
  - `NativeConstructCalled`
  - `FocusAssigned`
  - `LocalPlayerValid`
  - `GameViewportClientValid`
  - `FailureReason`
- Fix only the failing field.
- Do not add new UI features until Title -> NEW GAME is confirmed.

### Important files

- `Source/AdaptiveHorror/UI/EvaMenuWidgets.h`
- `Source/AdaptiveHorror/UI/EvaMenuWidgets.cpp`
- `Source/AdaptiveHorror/Characters/EvaPlayerController.h`
- `Source/AdaptiveHorror/Characters/EvaPlayerController.cpp`
- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.cpp`
- `DEV_LOG.md`
- `TODO.md`
- `BUILD_CHECK.md`

### Completion condition for next pass

- User confirms in PIE that Title UI is visible and NEW GAME restores gameplay input.
- Development Editor / Win64 build succeeds.
- Automation RunTests `AdaptiveHorror` succeeds.
- Runtime smoke succeeds.
- Logs and docs are updated with the actual PIE result.

## Latest handoff - 2026-07-14 Cycle 014 Core game UI flow

You are continuing the UE5.8 C++ Adaptive Horror prototype.

### Current verified state

- Branch created for the work: `feature/core-game-ui-flow`.
- Development Editor / Win64 build without Live Coding: succeeded.
- `Automation RunTests AdaptiveHorror`: 21 tests succeeded, 0 failures.
- Runtime smoke with `UnrealEditor-Cmd.exe -game -NullRHI -ExecCmds="Quit"`: exit code 0.
- Runtime smoke log confirms `EvaPrototypeGameMode` loads and transitions `Loading -> Title`.
- PIE visual verification is still required; Codex did not visually confirm the new menus in the viewport.

### Most recent implementation

Implemented the first complete prototype UI/screen-transition foundation:

- Title screen:
  - `NEW GAME`
  - disabled `CONTINUE - Not Available`
  - `SETTINGS`
  - `EXIT`
- New Game flow:
  - resets player HP/ammo via checkpoint reset,
  - resets telemetry,
  - resets EVA learning/analysis,
  - clears HUNTER/Adam/stage/game-over state,
  - clears combat actors/timers,
  - returns input to Game Only.
- Pause flow:
  - `Esc` toggles pause/resume while playing,
  - pause menu supports Resume, Restart From Checkpoint, Settings, Return to Title, Exit Game.
- Game Over flow:
  - player death enters `PlayerDead`,
  - stops combat actors,
  - opens explicit Game Over menu,
  - Retry uses checkpoint flow.
- Stage Clear flow:
  - Adam defeat opens `MISSION COMPLETE`,
  - old canvas `STAGE CLEAR TODO` overlay removed,
  - existing Stage Clear safety behavior preserved.
- Settings:
  - `UEvaSettingsSaveGame` stores Master/BGM/SFX volume, mouse sensitivity, invert Y, and placeholder window/resolution/quality fields.
  - Settings widget applies mouse sensitivity and invert Y immediately.
- Audio:
  - minimal procedural UI tones for clicks and major menu events.
  - Gameplay/BGM/enemy/boss audio remains TODO.
- HUD:
  - normal HUD reduced to gameplay essentials,
  - detailed debug HUD moved behind F9/N toggle,
  - gameplay HUD hidden in Title/Loading.
- Tests:
  - Added title blocking combat,
  - new-game terminal-state reset,
  - retry clears game over,
  - settings default range tests.

### Important files

- `Source/AdaptiveHorror/Core/EvaGameFlowTypes.h`
- `Source/AdaptiveHorror/Core/EvaSettingsSaveGame.h`
- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.h`
- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.cpp`
- `Source/AdaptiveHorror/Characters/EvaPlayerController.h`
- `Source/AdaptiveHorror/Characters/EvaPlayerController.cpp`
- `Source/AdaptiveHorror/Characters/EvaPlayerCharacter.h`
- `Source/AdaptiveHorror/Characters/EvaPlayerCharacter.cpp`
- `Source/AdaptiveHorror/UI/EvaMenuWidgets.h`
- `Source/AdaptiveHorror/UI/EvaMenuWidgets.cpp`
- `Source/AdaptiveHorror/UI/EvaHUD.cpp`
- `Source/AdaptiveHorror/Tests/EvaLearningTests.cpp`
- `DEV_LOG.md`
- `TODO.md`
- `BUILD_CHECK.md`

### Next highest-priority task

Run a real PIE manual verification pass for the new UI flow. Do not add new AI/enemy content until this is confirmed.

Recommended PIE script:

1. Launch PIE.
2. Confirm title screen appears and cursor is visible.
3. Click `SETTINGS`, adjust mouse sensitivity / invert Y, Back.
4. Click `NEW GAME`.
5. Confirm cursor hides, FPS movement and shooting work, HUD appears, initial zombie spawns.
6. Press `Esc`.
7. Confirm Pause menu appears once; Resume works; Esc again does not duplicate widgets.
8. Open Settings from Pause, return to Pause, Resume.
9. Force or receive player death.
10. Confirm Game Over menu appears and Retry restores checkpoint safely.
11. Press F4, defeat Adam, confirm `MISSION COMPLETE` menu appears.
12. Return to Title.
13. Start a second New Game and confirm no stale Adam / HUNTER / Stage Clear / Game Over / telemetry state remains.
14. Confirm UI tones are audible where expected.

### If issues are found

- If input remains blocked after New Game/Resume, inspect `AEvaPlayerController::ApplyGameplayInputMode()` and `AEvaPrototypeGameMode::SetGameFlowState()`.
- If widgets duplicate, inspect `CloseAllMenus()`, `RemoveWidget()`, and the relevant `Show*Menu()` method.
- If combat spawns during Title, inspect `StartCombatSpawningAfterNavigationReady()` and `IsGameplayActive()`.
- If Stage Clear and Game Over overlap, preserve the existing Stage Clear / Player Death priority rules; fix only the path that violates them.
- If UI audio is silent, inspect `AEvaPlayerController::PlayTone()` and platform audio initialization; do not mark gameplay audio complete yet.

### Completion condition for next pass

- User confirms the complete Title -> New Game -> Pause -> Game Over/Retry -> Adam defeat -> Stage Clear -> Return to Title -> second New Game loop in PIE.
- Development Editor / Win64 build succeeds.
- Automation RunTests `AdaptiveHorror` succeeds.
- Runtime smoke succeeds.
- `DEV_LOG.md`, `TODO.md`, `BUILD_CHECK.md`, and `NEXT_PROMPT.md` are updated with actual PIE results.

## Latest handoff - 2026-07-12 Cycle 013

You are continuing the UE5.8 C++ Adaptive Horror prototype.

### Current verified state

- Development Editor / Win64 build without Live Coding: succeeded.
- `Automation RunTests AdaptiveHorror`: 19 tests succeeded, 0 failures.
- Runtime smoke with absolute `.uproject` path and `-game -NullRHI -ExecCmds="Quit"`: exit code 0.
- PIE visual verification is still required; Codex did not visually confirm the Stage Clear regression in viewport.

### Most recent fix

Fixed the Adam defeat / Stage Clear conflict where remaining enemies continued damaging the player after Stage Clear and could trigger Player Death / GAME OVER afterward.

Important implementation notes:

- `AEvaPrototypeGameMode::HandleStageClear()` is now the terminal combat transition.
- Stage Clear clears respawn/spawn/HUNTER timers.
- Stage Clear stops all existing enemy AI combat, movement, focus, Tick, and hides overhead labels/HP bars.
- Post-clear spawn requests are skipped.
- Post-clear player damage returns `0`.
- Post-clear `HandleDeath()` / `HandlePlayerDeath()` are rejected.
- Movement/fire/reload/jump are disabled after Stage Clear; look input is intentionally left available.
- Director rejects `CompleteStage()` if Player Death was already active first, so Player Death wins if it genuinely happened first.
- Added logs:
  - `[StageClear] ...`
  - `[PlayerDeath] ...`
- Added automation tests:
  - `AdaptiveHorror.StageClear.RejectsPlayerDeath`
  - `AdaptiveHorror.StageClear.SkipsSpawns`
  - `AdaptiveHorror.StageClear.StopsEnemyCombat`
  - `AdaptiveHorror.StageClear.Idempotent`

### Next highest-priority task

Run a real PIE manual verification pass for the Stage Clear transition.

Use this procedure:

1. Open `AdaptiveHorror.uproject` in Unreal Editor 5.8.
2. Start PIE.
3. Press F4 to start ADAM encounter.
4. Defeat ADAM with `DebugBoss=true` active.
5. Confirm Stage Clear appears.
6. Do not move for 60 seconds after Stage Clear.
7. Confirm:
   - player HP no longer decreases
   - residual enemies stop chasing/attacking
   - GAME OVER does not appear
   - checkpoint respawn does not start
   - camera look remains available
   - movement/shooting remain disabled
   - Boss HUD hides
   - enemy overhead labels/HP bars hide

### If the bug still reproduces

Collect the latest PIE log and search for:

- `[StageClear]`
- `[PlayerDeath]`
- `RespawnTimerCreated`
- `Spawn skipped after clear`
- enemy AI movement/attack logs after `Context=Begin`

Then fix only the specific path that still runs after Stage Clear.

Likely suspects:

- an Adam-specific timer/delegate still firing after Stage Clear
- a non-`AEvaZombieAIController` controller path not stopped by `StopAllEnemyCombatForStageClear()`
- debug restore/F5 re-enabling input after Stage Clear
- a damage path not going through `AEvaPlayerCharacter::TakeDamage()`

### Files most relevant for next work

- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.h`
- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.cpp`
- `Source/AdaptiveHorror/Characters/EvaPlayerCharacter.h`
- `Source/AdaptiveHorror/Characters/EvaPlayerCharacter.cpp`
- `Source/AdaptiveHorror/AI/EvaZombieAIController.h`
- `Source/AdaptiveHorror/AI/EvaZombieAIController.cpp`
- `Source/AdaptiveHorror/AI/EvaZombieCharacter.h`
- `Source/AdaptiveHorror/AI/EvaZombieCharacter.cpp`
- `Source/AdaptiveHorror/World/EvaResearchFacilityDirector.cpp`
- `Source/AdaptiveHorror/UI/EvaHUD.cpp`
- `Source/AdaptiveHorror/Tests/EvaLearningTests.cpp`
- `DEV_LOG.md`
- `TODO.md`
- `BUILD_CHECK.md`

### Completion condition for next pass

- User confirms in PIE that Adam defeat leads to a stable Stage Clear state and the player can no longer die afterward.
- Development Editor / Win64 build succeeds.
- Automation RunTests `AdaptiveHorror` succeeds.
- `DEV_LOG.md`, `TODO.md`, `BUILD_CHECK.md`, and `NEXT_PROMPT.md` are updated with actual PIE results.

## 最新引き継ぎ — 2026-07-12 Cycle 009

あなたはこのプロジェクトのゲームディレクター兼リードエンジニアです。既存設計と現在の `main` ブランチを正として、UE5.8 C++体験版の安定化を続けてください。

### 現在の検証済み状態

- Development Editor / Win64 Build: Succeeded
- `Scripts/RunBuildCheck.ps1 -MaxParallelActions 1`: Succeeded
- Automation RunTests `AdaptiveHorror`: 15件すべてSuccess
- Runtime smoke:
  - `/Engine/Maps/Entry` が `EvaPrototypeGameMode` で起動
  - Runtime Navigation Build開始を確認
  - 初期ゾンビBeginPlayログを確認
  - Fatal / crashなし
- ユーザー報告によるPIE確認:
  - Runtime NavMesh Ready
  - 通常ゾンビの出現、追跡、接近攻撃
  - MoveToActor accepted / PathValid=true
  - 壁上スポーン再発なし

### 今回入った主な修正

- 通常ゾンビの障害物挟み停止対策:
  - 詰まり検知時に `TrySidestepAroundObstacle()` で左右迂回を試す。
  - Direct fallback前方ブロック時に、押し付けではなく側方候補へ逃がす。
- 頭上ラベル:
  - 固定Yaw 180度を廃止。
  - プレイヤーカメラ方向へのYaw-only Billboardに変更。
  - 死亡時と極端な遠距離では非表示。
- 敵タイプの見た目差:
  - `LeftArmVisual` / `RightArmVisual` を追加。
  - LongArmはActorScaleではなく腕パーツを伸ばす。
  - Fast / Armored / LongArm / Composite / HUNTER / ADAM / ADAM Phase2のBody/Head/Arm相対サイズを変更。
  - Capsule Collision / Navigation Agentを壊さないため通常進化ではActorScaleを固定。
- Automation:
  - `RunBuildCheck.ps1` は `Automation RunTests AdaptiveHorror` を実行する。
  - Visual系テスト2件を追加し、合計15件。

### 次回の最優先タスク

新規機能追加より、必ずUE5.8 PIEで実際の見た目と挙動を確認してください。

1. 通常ゾンビ障害物回避
   - 障害物を挟んでも停止し続けず、左右迂回してプレイヤーへ再接近すること。
   - EnemyStuckが連続発生しないこと。
2. 頭上ラベル
   - 左右反転しないこと。
   - プレイヤーカメラ方向を向くこと。
   - 遠距離/死亡時の表示が邪魔にならないこと。
3. 敵タイプ識別
   - Fast / Armored / LongArm / Composite / HUNTER / ADAMがプレイ中に見分けられること。
4. HUNTER
   - F2強制出現、追跡、攻撃、撃破、解析コアDrop、学習倍率0.3、30秒後Tier+1再投入。
5. ADAM
   - F4でAdam Arenaへ移動、追跡、近接、突進、攻撃後再追跡、Phase2、撃破Stage Clear。
6. HUD / Debug
   - NAV DEBUG、Active Enemy、Spawn結果、Fallback/Stuck count、Hunter Tier、Adam Phaseが読めること。
   - F9 Navigation Debug切替がPIEで機能すること。
7. ライト
   - Runtime debug lightが暗すぎず明るすぎず、敵とNavMesh確認を妨げないこと。

### 注意点

- この環境ではEditor viewportの目視PIE確認は未実施。Automation成功だけで「見た目」「実際に歩いた」は完了扱いにしない。
- 複雑な障害物回避はBehavior Tree全面移行ではなく、現AIController構造内の軽量リカバリで対応している。
- 正式 `.umap` 化後は保存済みNavMeshBoundsVolumeとNavMesh品質を優先して再調整する。

## 最新引き継ぎ — 2026-07-11 Cycle 007

あなたはこのプロジェクトのゲームディレクター兼リードエンジニアです。既存設計に従い、UE5.8 C++体験版の開発を継続してください。

今回の直前作業では、体験版ゲームループ安定化としてRuntime GrayboxのNavMesh問題とAI追跡停止リスクを修正しました。

### 現在の検証済み状態

- Development Editor / Win64 Build: Succeeded
- `AdaptiveHorror.EVA` Automation Test: 8件すべてSuccess
- Standalone Game相当の短時間起動:
  - `/Engine/Maps/Entry` が `EvaPrototypeGameMode` で起動することを確認
  - Runtime Navigation Build開始を確認
  - Runtime Spawnした空 `ANavMeshBoundsVolume` 由来のempty bounds警告は解消済み
  - 起動時Fatal / crashなし
- ユーザー報告により、PIEでプレイヤー移動・視点・射撃、マウス上下反転修正は確認済み

### 今回入った主な修正

- `AEvaPrototypeGameMode::BuildPrototypeArena()`
  - Runtime Spawnした `ANavMeshBoundsVolume` を使うのをやめた。
  - UE5.8の `UNavigationSystemV1::bWholeWorldNavigable = true` を設定し、Runtime Graybox向けに `Build()` を呼ぶようにした。
  - Runtime StaticMesh床/壁/遮蔽物をNavigation relevant化し、最終Transform後はStatic mobilityに戻す。
- `AEvaZombieAIController`
  - `MoveToActorOrDirect()`
  - `MoveToLocationOrDirect()`
  - NavMesh未生成やMoveTo失敗時でも、Pawnが目標方向へ直接移動するfallbackを追加。
- `AEvaHunterAIController`
  - HUNTERの対抗移動をDirect fallback対応。
- `AEvaAdamBossAIController`
  - Adamの追跡・突進をDirect fallback対応。
- `AEvaPrototypeGameMode`
  - GameOver / StageClear / Debug Restore時のTimer重複と敵ターゲット残りを軽減。

### 次回の最優先タスク

新規コンテンツ追加より、必ずUE5.8 PIEで体験版ゲームループを実際に通して確認してください。静的確認だけで完了扱いにしないでください。

1. 通常ゾンビ検証
   - スポーン、感知、追跡、接近、攻撃、プレイヤーHP減少
   - 射撃ダメージ、ヘッドショット倍率、死亡、Kill/Telemetry更新
2. プレイヤー死亡と復帰
   - GAME OVER、入力停止、3秒Checkpoint復帰、HP/弾薬復帰、複数回死亡時のTimer重複なし
3. HUNTER
   - F2強制出現、3Kill/45秒出現、重複なし、撃破、解析コアDrop、倍率0.3、30秒再投入、Tier+1
4. EVA解析・進化
   - F1/F3/F7を使い、解析率0〜100、Learning/Adapting/Evolving、20/40/60/80%進化個体を確認
5. 研究施設6区画
   - Entry Lobby → Security Corridor → Observation Lab → Containment Ward → Data Core Room → Adam Arena
   - Zone Trigger、Objective、Checkpoint、EVAログ5件、進行不能なし
6. ADAM戦
   - F4または通常進行でAdam Arenaへ移動
   - 追跡、近接、突進、咆哮召喚、Phase 2、撃破、Stage Clear一度だけ
7. Debugキー
   - F1〜F7すべてをPIEで確認

### 主要ゲームループがPIEで正常に動いた場合の次タスク

- タイトル画面
- 難易度選択
- 設定画面
- マウス感度と上下反転設定
- 一時停止画面
- 体験版開始・終了フロー
- 仮サウンドとBGM
- UI改善
- 研究施設のホラー演出
- 30〜60分のバランス調整

### 注意点

- Runtime Grayboxは正式 `.umap` ではない。将来的にはEditor上で研究施設マップを保存し、NavMeshBoundsVolumeを配置する。
- 現状は `bWholeWorldNavigable` とDirect movement fallbackで、Runtime Grayboxの敵追跡が止まらないようにしている。
- Direct fallbackは体験版を止めないための保険。正式ステージ化後はNavMesh上の移動品質を優先して調整する。
- git操作は行わない。

## 最新引き継ぎ — 2026-07-11 Cycle 006

UE5.8 PIEで発生していた「マウス上下だけ逆向き」問題を修正済みです。

### 今回の修正内容

- `AEvaPlayerCharacter::Look` のPitch入力符号を修正した。
- 左右方向は既存どおり `AddControllerYawInput(LookValue.X)` のまま維持した。
- MouseYの反転管理はInput Mapping Contextではなく、C++側の `bInvertMouseY` に一本化した。
- `bInvertMouseY` を追加した。
  - default: `false`
  - `false`: 通常FPS操作。マウス上移動で視点が上、マウス下移動で視点が下。
  - `true`: 上下反転。
  - `IsMouseYInverted()` / `SetInvertMouseY()` からBlueprintまたは設定画面で変更可能。
- Runtime Enhanced Input Mapping側のMouseYには `Negate` を追加していない。
  - MouseY Mappingは `Swizzle(YXZ)` のみ。
  - 二重反転を避けるため、今後もMouseYのNegateは追加しないこと。

### 変更ファイル

- `Source/AdaptiveHorror/Characters/EvaPlayerCharacter.h`
- `Source/AdaptiveHorror/Characters/EvaPlayerCharacter.cpp`
- `Scripts/RunBuildCheck.ps1`
- `DEV_LOG.md`
- `TODO.md`
- `NEXT_PROMPT.md`

### 検証状況

- 静的確認: PASS
  - `.generated.h` include順序
  - `.uproject` JSON
  - `bInvertMouseY` / `SetInvertMouseY()` / `IsMouseYInverted()` の存在
  - Pitch入力が `bInvertMouseY ? LookValue.Y : -LookValue.Y` になっていること
  - MouseY Mapping側に `UInputModifierNegate` がないこと
- `RunBuildCheck.ps1 -MaxParallelActions 4` は実行したが、現在起動中のUE Editorが `UnrealEditor-AdaptiveHorror.dll` を掴んでいたため、最終リンクで停止した。
  - 主なエラー: `LNK1104: UnrealEditor-AdaptiveHorror.dll を開けない`
  - C++ compile段階では今回変更した `EvaPlayerCharacter.cpp` まで進んでいる。

### 次に必ずやること

1. Unreal Editor と LiveCodingConsole を閉じる。
2. 以下を実行する。

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\RunBuildCheck.ps1 -MaxParallelActions 4
```

3. Development Editor / Win64 Buildの完走を確認する。
4. `AdaptiveHorror.EVA` Automation Testを確認する。
5. PIEで以下を手動確認する。
   - マウスを上へ動かす → 視点が上を向く。
   - マウスを下へ動かす → 視点が下を向く。
   - 左右方向は従来どおり。
   - `bInvertMouseY=true` にすると上下反転する。

### 注意点

- MouseYの上下反転はC++側の `bInvertMouseY` だけで管理する。
- Input Mapping Context側へMouseY Negateを追加すると二重反転のリスクがある。
- Editor起動中の通常ビルドはDLLロックで失敗することがある。実ビルド検証はEditorを閉じて行う。

## 最新引き継ぎ — Cycle 004後

あなたはこのプロジェクトのゲームディレクター兼リードエンジニアです。既存設計に従い、UE5 C++体験版「敵がプレイヤーを学習し、適応・進化するFPSサバイバルホラー」を継続してください。

### Cycle 005追記

UE5.8 Development Editor / Win64の初回ビルドエラー修正は完了しています。

- `DefaultBuildSettings = BuildSettingsVersion.V7` へ更新済み。
- `AdaptiveHorror.Build.cs` に `ModuleDirectory` include pathを追加済み。
- UE5.8 API差分 `APointLight::GetPointLightComponent()` 修正済み。
- C4458 shadowing修正済み。
- `Scripts/RunBuildCheck.ps1` に `-MaxParallelActions` 追加済み。
- `powershell -ExecutionPolicy Bypass -File .\Scripts\RunBuildCheck.ps1 -MaxParallelActions 4` で以下を確認済み。
  - Generate Project Files: Succeeded。
  - Development Editor Build / Win64: Succeeded。
  - `AdaptiveHorror.EVA` Automation Test 8件: Success。

次回はビルド修正ではなく、UE5.8 EditorでのPIE実プレイ確認から始めてください。

まず `BUILD_CHECK.md` と `DEV_LOG.md` を読み、次に必ず以下を実行してください。

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\RunBuildCheck.ps1 -MaxParallelActions 4
```

### 現在の進捗

Cycle 004で以下を実装済みです。

- `RunBuildCheck.ps1` を再実行した。
  - `.uproject` 検出成功。
  - Static source sanity PASS。
  - `UnrealEditor.exe` / `UnrealBuildTool.exe` / `MSBuild.exe` / `cl.exe` はこの環境では未検出。
  - Generate Project Files / Development Editor Build / Automation Test / PIEは未実行。
- `RunBuildCheck.ps1` を強化した。
  - UE未検出時でも `.generated.h` 順序と `.uproject` JSONを静的確認する。
  - UE実環境ではBuild後に `AdaptiveHorror.EVA` Automation Testを実行する。
- 非Shipping用Debugキーを追加した。
  - F1: EVA解析率+20。
  - F2: HUNTER強制出現。
  - F3: ゾンビWave強制スポーン。
  - F4: ADAM戦へワープ。
  - F5: プレイヤー全回復・弾薬補充。
  - F6: Stage Clear強制。
  - F7: Telemetry Snapshot表示。
- HUD/画面Debug表示を追加した。
  - 射撃ヒット。
  - ゾンビ死亡。
  - HUNTER出現/撃破。
  - EVA解析率上昇。
  - ADAM Phase 2。
  - Stage Objective更新。
- クラッシュ予防を強化した。
  - HUD/Player/Weapon/Zombie/HUNTER/ADAM/Directorのnull保護。
  - Weapon Reload Timer解除。
  - 死亡後射撃防止。
  - Zombie死亡処理二重実行防止。
  - Checkpoint復帰時の敵AIターゲット解除。

### 次回タスク

1. UE5 PIEでの実プレイ確認。
2. UE5実ビルドで出たUHT/Compile/Automation/PIEエラー修正。
3. 研究施設6区画の導線調整。
4. HUNTER出現・再投入の体感調整。
5. ADAM戦の難易度調整。
6. 最低限のサウンドと画面演出追加。
7. 体験版としての開始・終了フロー整備。

### 注意点

- この環境ではUE/MSVCが未検出だったため、実ビルドは未検証です。
- UE5実環境では最初に `Scripts/RunBuildCheck.ps1` を実行し、出たエラーを `DEV_LOG.md` に記録してから修正してください。
- Debugキーは非Shipping限定です。ShippingではDebug入力を使わない設計にしています。
- gitはPATH上にない前提なので、git操作は行わないでください。
- 大規模改造より、UE5実ビルド/PIEで落ちた箇所を小さく確実に直してください。

### 主な実装対象ファイル

- `Scripts/RunBuildCheck.ps1`
- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.*`
- `Source/AdaptiveHorror/Characters/EvaPlayerCharacter.*`
- `Source/AdaptiveHorror/AI/EvaLearningSubsystem.*`
- `Source/AdaptiveHorror/AI/EvaZombieAIController.*`
- `Source/AdaptiveHorror/AI/EvaZombieCharacter.*`
- `Source/AdaptiveHorror/AI/EvaHunterCharacter.*`
- `Source/AdaptiveHorror/AI/EvaAdamBossCharacter.*`
- `Source/AdaptiveHorror/Weapons/EvaWeaponBase.*`
- `Source/AdaptiveHorror/Weapons/EvaHitscanWeapon.*`
- `Source/AdaptiveHorror/UI/EvaHUD.*`
- `Source/AdaptiveHorror/World/EvaResearchFacilityDirector.*`
- `Source/AdaptiveHorror/Tests/EvaLearningTests.cpp`
- `DEV_LOG.md`
- `TODO.md`
- `BUILD_CHECK.md`
- `TECH_SPEC.md`

### 完了条件

- `RunBuildCheck.ps1` がUE実環境で完走する、または失敗ログと修正方針が `DEV_LOG.md` に記録されている。
- PIEで移動、射撃、ゾンビ戦闘、HUNTER、ADAM戦、Stage Clearが確認できる。
- F1〜F7 DebugキーがPIEで確認できる。
- 研究施設6区画の導線が迷いにくくなっている。
- `DEV_LOG.md` / `TODO.md` / `NEXT_PROMPT.md` が更新されている。

---

あなたはこのプロジェクトのゲームディレクター兼リードエンジニアです。既存設計に従い、UE5 C++体験版「敵がプレイヤーを学習し、適応・進化するFPSサバイバルホラー」を継続してください。

## 現在の進捗

Cycle 003で以下を実装済みです。

- `BUILD_CHECK.md`
- `Scripts/RunBuildCheck.ps1`
- 研究施設6区画Runtime Graybox
  - Entry Lobby
  - Security Corridor
  - Observation Lab
  - Containment Ward
  - Data Core Room
  - Adam Arena
- `AEvaResearchFacilityDirector`
- `AEvaFacilityZoneTrigger`
- `AEvaStoryLogPickup`
- EVAログ5種
- `AEvaAdamBossCharacter`
- `AEvaAdamBossAIController`
- Adam Phase 2基盤
- Adam撃破Stage Clear
- HUDのZone / Objective / EVA Logs / Story Log / Stage Clear Result表示
- Automation Testコード
  - Director進行
  - EVAログ取得
  - Adamフェーズ移行
  - Adam撃破Stage Clear

## 重要な現状

この環境では以下が見つからなかった。

- `UnrealEditor.exe`
- `UnrealBuildTool.exe`
- `MSBuild.exe`
- `cl.exe`

そのため、UHT / Development Editor Build / Automation Test実行 / PIEは未実行。

次回、UE5実環境がある場合は必ず最初に `BUILD_CHECK.md` と `Scripts/RunBuildCheck.ps1` を使って実ビルド検証を行うこと。

## 次回タスク

### 1. UE5実ビルドで出たエラー修正

最優先。

実行:

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\RunBuildCheck.ps1
```

確認:

- Generate Project Files
- Development Editor Build
- `AdaptiveHorror.EVA` Automation Test
- PIE Smoke Test

エラーが出た場合は、機能追加より先に修正する。

### 2. 研究施設Grayboxのプレイ感調整

調整対象:

- 6区画の距離感
- 通路幅
- ゲート視認性
- Checkpoint位置
- Cover / HideSpot / EscapeRoute / AmbushPoint位置
- ゾンビスポーン数
- 弾薬/回復配置
- HUNTER出現位置
- Adam Arenaサイズ

### 3. アダム戦のバランス調整

調整対象:

- HP 2500が長すぎないか
- 近接25 damage
- 突進35 damage / 8秒Cooldown
- 咆哮召喚頻度
- Phase 2移動速度+20%
- Phase 2攻撃間隔-20%
- 進化個体召喚タイミング
- EVA解析率に応じた攻撃パターン

### 4. HUNTER演出追加

追加候補:

- 出現警告HUD
- 再投入警告
- Tier表示
- 解析音
- 解析コアDrop演出
- HUNTER用Material色

### 5. 体験版UI改善

追加候補:

- Objective表示の整理
- Story Log表示の読みやすさ改善
- Stage Clear Result画面の整形
- HUNTER状態通知
- EVA解析率上昇通知
- 進化個体出現通知

### 6. 簡易サウンド追加

追加候補:

- 銃声
- リロード
- ゾンビ攻撃
- HUNTER出現警告
- EVAログ取得
- Adam咆哮
- Stage Clear

## 主な対象ファイル

- `BUILD_CHECK.md`
- `Scripts/RunBuildCheck.ps1`
- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.*`
- `Source/AdaptiveHorror/World/EvaResearchFacilityDirector.*`
- `Source/AdaptiveHorror/World/EvaFacilityZoneTrigger.*`
- `Source/AdaptiveHorror/Pickups/EvaStoryLogPickup.*`
- `Source/AdaptiveHorror/AI/EvaAdamBossCharacter.*`
- `Source/AdaptiveHorror/AI/EvaAdamBossAIController.*`
- `Source/AdaptiveHorror/UI/EvaHUD.*`
- `Source/AdaptiveHorror/Tests/EvaLearningTests.cpp`
- `DEV_LOG.md`
- `TODO.md`
- `NEXT_PROMPT.md`
- `TECH_SPEC.md`
- `DESIGN_DECISIONS.md`
- `README.md`

## 完了条件

- UE5実環境でビルドできる、または最初のビルドエラーと修正方針が `DEV_LOG.md` に明記されている。
- PIEで6区画を進行できる。
- EVAログを取得できる。
- Data Core RoomでHUNTERと交戦できる。
- Adam ArenaでAdamと戦える。
- Adam撃破でStage Clear表示が出る。
- HUDで進行状態が分かる。
- 次回作業内容が `NEXT_PROMPT.md` に更新されている。

---

以下に古い引き継ぎ内容が残っている場合があるが、次回は上記のクリーンな引き継ぎを優先する。

あなたはこのプロジェクトのゲームディレクター兼リードエンジニアです。既存設計に従い、UE5 C++体験版「敵がプレイヤーを学習し、適応・進化するFPSサバイバルホラー」を継続してください。

## 現在の進捗

Cycle 002で「ただのゾンビFPS」から「学習されるゲーム」へ進めた。

実装済み:

- UE5 C++プロジェクト骨格。
- Runtime Graybox Arena。
- FPSプレイヤー、HP、ハンドガン、射撃、リロード、弾薬。
- 通常ゾンビAI、追跡、攻撃、死亡。
- Player Telemetry + EVA Learning Subsystem。
- HUNTER本格AI。
  - ゾンビ撃破数3体または45秒で出現。
  - HUNTER撃破で解析コアDrop。
  - 学習倍率1.0 → 0.3。
  - 30秒後にTier+1再投入。
  - 再投入で学習倍率0.3 → 1.0。
- EVA解析率0〜100%。
  - 命中、HS、キル、被ダメージ、HUNTER観測で上昇。
  - HUNTER観測はTelemetry Snapshotを高精度同期。
- 敵適応。
  - 近距離型: 距離を取る。
  - 遠距離型: 遮蔽物利用。
  - 隠密型: 索敵範囲増加。
  - 探索型: 待ち伏せ増加。
- 進化個体。
  - 20% 高速型: 移動速度+25%。
  - 40% 装甲型: 頭部被ダメージ-50%、最大HP+30%。
  - 60% 長腕型: 攻撃距離+2.5m、攻撃力+15%。
  - 80% 複合型: 高速/装甲/長腕を同時適用。
- HUD。
  - HP、弾薬、Kill、命中率、HS率、戦闘スタイル、EVA解析率、解析段階、適応方針、次進化型、HUNTER状態、学習倍率。
- Automation Testコード。
  - Telemetry分類。
  - HUNTER撃破倍率。
  - 解析率増加。
  - 進化閾値。

## 重要な現状制約

このCodex環境では `UnrealEditor.exe`、`MSBuild.exe`、`cl.exe` が見つからず、UHT / C++ Build / Automation Test実行 / PIEは未実行。次回、UE5 + Visual Studio C++ toolchainが使える場合は、まずビルド検証から始めること。

gitはPATH上にないため、git操作は行わない。

## 次回の最優先タスク

### 1. 実ビルド検証

最初に以下を実行する。

1. Generate Project Files。
2. `AdaptiveHorrorEditor` Development Editor Build。
3. `AdaptiveHorror.EVA` Automation Test。
4. `/Engine/Maps/Entry` のPIE Smoke Test。

確認項目:

- Play開始できる。
- HUNTERが3Killまたは45秒で出る。
- HUNTER撃破で解析コアが出る。
- 学習倍率が1.0から0.3へ落ちる。
- 30秒後にHUNTERがTier+1で再投入される。
- 再投入後に倍率が1.0へ戻る。
- EVA解析率が増える。
- 20/40/60/80%で進化型スポーンが変わる。
- HUDにHUNTER状態、学習倍率、次進化型が表示される。

ビルドエラーが出たら、機能追加より先に修正する。

### 2. 研究施設ステージの簡易進行

Runtime Arenaを6区画の研究施設Grayboxへ拡張する。

区画案:

1. 搬入口。
2. 研究棟廊下。
3. 観測区画。
4. 隔離区画。
5. 中央コア。
6. アダム収容室。

各区画に以下を置く。

- Cover。
- HideSpot。
- EscapeRoute。
- AmbushPoint。
- Checkpoint。
- Encounter ID。

### 3. アダムボス基盤

`AEvaAdamCharacter` と最低限のBoss AIControllerを作る。

最初は仮でよい。

- HP。
- Phase 1 / Phase 2。
- 弱点露出。
- プレイヤーTelemetryに応じた簡易攻撃選択。
- 撃破時Stage Clear。

### 4. 体感演出

学習されていることがプレイヤーに伝わるように、仮演出を追加する。

- HUNTER出現警告。
- EVA解析率上昇時のHUD通知。
- 進化個体出現時のHUDログ。
- 進化型のMaterial色分け。
- HUNTER再投入時のTier表示。

## 主な対象ファイル

- `Source/AdaptiveHorror/AI/EvaHunterAIController.*`
- `Source/AdaptiveHorror/AI/EvaHunterCharacter.*`
- `Source/AdaptiveHorror/AI/EvaLearningSubsystem.*`
- `Source/AdaptiveHorror/AI/EvaZombieAIController.*`
- `Source/AdaptiveHorror/AI/EvaZombieCharacter.*`
- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.*`
- `Source/AdaptiveHorror/UI/EvaHUD.*`
- `Source/AdaptiveHorror/Tests/EvaLearningTests.cpp`
- 新規: `Source/AdaptiveHorror/AI/EvaAdamCharacter.*`
- 新規: `Source/AdaptiveHorror/AI/EvaAdamAIController.*`
- ドキュメント: `DEV_LOG.md`、`TODO.md`、`NEXT_PROMPT.md`、必要なら `TECH_SPEC.md`、`DESIGN_DECISIONS.md`

## 完了条件

- UE5実環境でビルドできる。
- Automation Testが通る、または失敗理由と修正方針が `DEV_LOG.md` に明記されている。
- PIEでHUNTER出現/撃破/再投入が確認できる。
- PIEでEVA解析率と進化個体スポーンが確認できる。
- プレイヤーが「敵に学習されている」と体感できる。
- `DEV_LOG.md`、`TODO.md`、`NEXT_PROMPT.md` が更新されている。

---

以下に古い引き継ぎ内容が残っている場合があるが、次回は上記のクリーンな引き継ぎを優先する。

あなたはこのプロジェクトのゲームディレクター兼リードエンジニアです。最初に `README.md`、`GAME_DESIGN.md`、`TECH_SPEC.md`、`DESIGN_DECISIONS.md`、`TODO.md`、`DEV_LOG.md` を読み、既存のC++縦切りを壊さず開発を継続してください。

## 現在の進捗

- UE5 C++プロジェクト骨格、Runtime Graybox Arena、FPS入力、HP、Handgun、通常Zombie AI、Telemetry、Learning Subsystem、C++ HUD、Pickup、Checkpoint、GAME OVER復帰を実装済み。
- `AEvaHunterCharacter` 仮クラスはあり、撃破時にLearning Speedを1.0から0.3へ変更できる。
- `None/Fast/Armored/LongArm` の進化型とZombieの適用接続点はあるが、Modifierは未実装。
- Telemetry分類と倍率のAutomation Testコードを追加済み。
- 現開発環境にはUnreal Engine、Visual Studio C++ toolchain、Gitがないため、UHT／build／PIEは未実行。

## 最初に必ず行う検証

利用可能なUE5とC++ toolchainを再確認する。利用可能なら、機能追加前に次を実施し、Cycle 001由来のcompile error／runtime errorを修正する。

1. Project Files生成
2. `AdaptiveHorrorEditor` Development Editor build
3. `AdaptiveHorror.EVA` Automation Test
4. `/Engine/Maps/Entry` のPIE
5. 移動、視点、Jump、Sprint、Fire、Reload、Zombie追跡／攻撃、Zombie／Player死亡、HUD、Checkpoint復帰のsmoke test

環境が引き続き存在しない場合も止まらず、安全な静的検証を続け、未実行範囲を `DEV_LOG.md` に記録する。

## 次回タスク

### 1. HUNTERの本格AI実装

- `AEvaHunterCharacter`／AIControllerを通常Zombieから分化する。
- 高精度観測、追跡、退路選択、主戦術への対抗行動を実装する。
- 撃破時に解析コアをDropし、世代、撃破時刻、再投入待ちをSubsystemへ保存する。
- 時間＋研究施設進行TriggerでTier 2を再投入し、倍率を1.0へ戻す。

### 2. 学習倍率による敵適応速度の変化

- 通常ZombieのObserver Accuracy 0.20、HUNTER 1.00を導入する。
- `EffectiveWeight = BaseWeight × ObserverAccuracy × LearningSpeedMultiplier` を集計へ適用する。
- Learning／Adapting／Evolving段階、最低Sample、区画Gateを実装する。
- Debug HUDで観測量、倍率、段階、主傾向、選択Counterを表示する。

### 3. 進化個体3種

- 高速型: 移動速度+25%
- 装甲型: 頭部被damage-50%、胴体HP+30%
- 長腕型: 攻撃range+2.5m、攻撃力+15%
- `ConfigureEvolution()` をData-driven Modifierへ置き換え、1個体1種に制限する。
- 仮Material／scale／HUD labelで型を識別可能にする。

### 4. 研究施設ステージの簡易進行

- Runtime ArenaまたはEditor利用可能なら `L_DevGym.umap` を、搬入口→研究棟→観測区画→隔離区画→中央コア→アダム収容室の6区画へ拡張する。
- Door、Key／Power、Encounter ID、Route ID、Hide Spot ID、Checkpointを接続する。
- 最初のHUNTER、進化個体、Tier 2 HUNTERの投入Triggerを配置する。

### 5. アダムボスの基盤実装

- `AEvaAdamCharacter` とBoss AIControllerを作る。
- HP、Phase 1／2、弱点露出、近接薙ぎ払い、突進の仮動作を作る。
- Telemetry最上位傾向から選ぶ適応枠を1つ接続する。
- 撃破イベントをStage Clear基盤へ接続する。

## 注意点

- 既存の最小ゲームループを常に起動可能に保つ。
- C++を状態の正とし、Blueprint／Data Assetから調整可能にする。
- 適応は遭遇中に突然変えず、遭遇終了またはSpawn時に適用する。
- 一度に有効な通常適応は最大2、進化は1個体1種。
- HUNTER撃破中は倍率0.3、再投入時は1.0をAutomation Testで保証する。
- Runtime Arenaは暫定策。専用Mapへ移行できたら `DESIGN_DECISIONS.md` のDD-002を更新する。
- 製品版美術、マルチプレイ、外部ML、クラウド送信は実装しない。
- GitはPATH上にない限り操作しない。

## 主な対象ファイル

- `Source/AdaptiveHorror/AI/EvaHunterCharacter.*`
- `Source/AdaptiveHorror/AI/EvaLearningSubsystem.*`
- `Source/AdaptiveHorror/AI/EvaTelemetryTypes.h`
- `Source/AdaptiveHorror/AI/EvaZombieCharacter.*`
- `Source/AdaptiveHorror/AI/EvaZombieAIController.*`
- 新規HUNTER AIController／Adaptation Profile／Adamクラス
- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.*`
- `Source/AdaptiveHorror/UI/EvaHUD.*`
- `Source/AdaptiveHorror/Tests/EvaLearningTests.cpp`
- `Config/Default*.ini`
- `DEV_LOG.md`、`TODO.md`、`NEXT_PROMPT.md`、必要なら `TECH_SPEC.md`／`DESIGN_DECISIONS.md`

## 完了条件

- 既存縦切りがcompileし、PIE smoke testを通る。環境がない場合は未達理由を正確に記録する。
- HUNTERが観測、追跡、撃破、Core Drop、Tier 2再投入まで動く。
- 倍率1.0／0.3がObservation Mass増加量へ反映される。
- 高速／装甲／長腕の数値と視認差を検証できる。
- 6区画を簡易進行でき、HUNTER／進化の投入順が成立する。
- アダムの2 Phase基盤を撃破し、Stage Clearへ到達できる。
- Automation Testを追加／更新する。
- `DEV_LOG.md`、`TODO.md`、`NEXT_PROMPT.md` を更新する。
## Latest Handoff - 2026-07-12 Cycle 008

Continue the UE5.8 AdaptiveHorror prototype. The latest work replaced unsafe enemy spawn paths with a shared safe-spawn flow and added spawn telemetry.

Current important state:

- `AEvaPrototypeGameMode::FindSafeEnemySpawnLocation(...)` now checks NavMesh projection, floor trace, capsule overlap, player distance, and enemy separation.
- `SpawnEnemyNearLocation(...)` now uses `AdjustIfPossibleButDontSpawnIfColliding` and logs `LogAdaptiveHorror` spawn attempts/results.
- Initial zombie, zone zombies, F3 wave, HUNTER, ADAM, and ADAM roar minions use safe spawn routing.
- HUD displays active zombie/hunter/ADAM counts, last spawn result/location, NavMesh availability, fallback movement count, and stuck enemy count.
- AI direct movement fallback now uses separation and a short obstacle trace.
- Runtime graybox now has directional light and sky light.
- Added automation tests under `AdaptiveHorror.Spawn.*`.

Immediate next steps:

1. Close Unreal Editor and Live Coding Console. The previous build compiled the modified files but failed to link because `UnrealEditor-AdaptiveHorror.dll` was locked by `UnrealEditor.exe`.
2. Run:

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\RunBuildCheck.ps1 -MaxParallelActions 2
```

3. Run automation tests and confirm the new `AdaptiveHorror.Spawn.*` tests pass.
4. PIE manual checks:
   - Entry Lobby initial zombie is visible, safe, and chases/attacks.
   - F3 repeated waves spawn dispersed.
   - P key shows NavMesh around spawn areas.
   - F2 HUNTER and F4 ADAM spawn safely.
   - ADAM roar minions do not stack.
   - No repeated stuck spam during ordinary pursuit.

Unresolved risk:

- Runtime graybox still depends on `bWholeWorldNavigable`; a saved `.umap` with authored NavMeshBoundsVolume remains the long-term fix.

## Latest Handoff - 2026-07-12 Cycle 009

Continue from the obstacle-pursuit and enemy-label stabilization pass.

Current important state:

- Development Editor / Win64 build succeeds after closing Unreal Editor/Live Coding.
- `Automation RunTests AdaptiveHorror` succeeds; latest run confirmed 15 successful tests.
- Runtime smoke using `UnrealEditor-Cmd.exe -game -NullRHI -ExecCmds="Quit"` exits with code 0.
- `AEvaZombieAIController` now logs detailed pursuit/path diagnostics:
  - MoveRequest result.
  - PathFollowing state.
  - PathValid / IsPartial / PathPoints.
  - CurrentPathPointIndex.
  - player/enemy NavProjection results.
  - left/right detour NavProjection results.
  - enemy-to-player trace result.
  - MoveCompleted ResultCode.
  - capsule radius versus Nav Agent radius.
- Direct fallback is now restricted to no-valid-nav-path + clear line-of-sight + clear forward trace.
- Sidestep recovery now uses only NavProjection-successful destinations and returns to MoveToActor(Player).
- Debug labels now use a common initialization path and log final state for normal zombies, evolved zombies, HUNTER, and ADAM.
- README/DEV_LOG contain the current debug key list:
  - F2 HUNTER
  - F3 zombie wave
  - F4 ADAM arena warp
  - F7 telemetry
  - F9/N navigation visualization
  - P unbound
  - F8 intentionally unbound because of PIE Eject.

Immediate next PIE checks:

1. Open the editor after the successful build.
2. PIE in the runtime graybox.
3. Place or use an obstacle that still has a real NavMesh route around it.
4. Confirm a normal zombie follows the NavPath around the obstacle rather than stopping.
5. Confirm that when no NavMesh detour exists, the AI does not force through the obstacle.
6. Confirm labels are visible for:
   - InitialVisibleZombie.
   - F3 wave spawn.
   - Adaptive/evolved spawn.
   - HUNTER.
   - ADAM.
   - ADAM roar minions.
   - HUNTER reinsertion.
7. Test F2/F3/F4/F7/F9/N in PIE and confirm P is unbound.

If issues remain:

- Use `[AIPath]` logs first; do not add another blind sidestep loop.
- Check whether `DiagnosticPathValid=true` and `DiagnosticIsPartial=false`.
- If `DiagnosticPathValid=false`, fix map/NavMesh authoring before forcing AI code.
- If label component exists but is hidden, inspect `HiddenInGame`, `OwnerNoSee`, `OnlyOwnerSee`, `DistanceHideCondition`, and `CameraAcquired` in `[EnemyVisual]` logs.

Recommended next task:

- PIE visual verification and targeted fixes only. Avoid new content until obstacle pursuit and all-enemy labels are confirmed in the viewport.

## Latest Handoff - 2026-07-12 Cycle 010

Continue from the stationary-target repath and F4 ADAM debug-start fix.

Current important state:

- Live Codingなし Development Editor / Win64 build succeeds.
- `Automation RunTests AdaptiveHorror` succeeds; latest log shows 15 successful tests.
- Runtime smoke with `UnrealEditor-Cmd.exe -game -NullRHI -ExecCmds="Quit"` exits with code 0.
- Zombies now monitor movement progress even when the player is completely stationary.
- `[AIRepath]` logs include:
  - Repath reason: TargetMoved / NoProgress / MoveCompleted / PathInvalid / SidestepFinished / PeriodicRefresh.
  - Time since last MoveTo.
  - Time since last meaningful progress.
  - Recent movement distance.
  - DirectFallback active.
  - Sidestep active.
  - PathFollowing status.
  - CurrentMoveRequestID.
  - MoveTo reissue result.
- NoProgress now aborts the current move and reissues `MoveToActor(Player)` at low frequency.
- Direct fallback is cleared whenever Path Following is accepted.
- Sidestep completion/timeout returns to normal player pursuit.
- F4 now:
  - Logs `[AdamDebug] F4 pressed`.
  - Teleports player to ADAM arena.
  - Removes non-boss/non-HUNTER enemies near ADAM arena.
  - Explicitly starts the ADAM encounter through the Director.
  - Avoids ADAM duplicate spawn.
  - Re-primes ADAM target when available.
- Director ADAM start now:
  - Does not leave `bAdamEncounterActive=true` after failed spawn.
  - Reuses existing living ADAM if present.
  - Logs AdamClass, existing count, spawn result, final location, NavProjection, AIController, possession, PlayerTarget, Health, Phase, and failure reason.
- Generic safe spawn no longer calls `ConfigureEvolution()` on actors tagged `Adam` or `Boss`, preventing ADAM from being reset into normal zombie visuals/label.
- Ordinary AdaptiveSpawn is skipped while ADAM encounter is active.
- README notes that COMPOSITE is the 80% EVA analysis evolved variant.

Immediate next PIE checks:

1. Start PIE and place/stand behind the same obstacle as the previous repro.
2. Keep the player completely still. Do not jump.
3. Confirm a zombie reroutes around the obstacle via `[AIRepath] Reason=NoProgress` or `PeriodicRefresh`.
4. Confirm it does not keep pressing into the wall for several seconds.
5. Confirm it reaches melee range and attacks.
6. Repeat with multiple zombies.
7. Press F4:
   - Confirm ADAM is visible in Adam Arena.
   - Confirm normal/COMPOSITE debug enemies were cleared if they were blocking verification.
   - Confirm repeated F4 does not duplicate ADAM.
   - Confirm ADAM AIController is possessed and targeting the player.
8. Confirm labels still visible:
   - ADAM.
   - ADAM summoned enemies.
   - FAST.
   - ARMORED.
   - LONG ARM.

If obstacle pursuit still fails:

- Inspect `[AIRepath]` and `[AIPath]` together.
- If `NoProgress` fires and MoveTo result is successful but the pawn still presses into the wall, inspect path corridor/collision mismatch around that obstacle.
- If `PathValid=true` and NavMesh is green/connected, verify the obstacle collision is represented consistently for both navigation and character movement.
- Do not add another direct sidestep loop until the path/movement mismatch is identified.

If F4 still does not show ADAM:

- Search logs for `[AdamDebug]`.
- Check `ExistingAdamCount`, `SpawnAttempted`, `SpawnResult`, `AdamClass`, `NavProjected`, `AIController`, `Possessed`, `PlayerTarget`, and `DestroyReason`.
- If `SpawnResult` is valid but the visual looks like a zombie, check for any remaining non-ADAM `ConfigureEvolution()` call path.

## Latest Handoff - 2026-07-12 Cycle 011

Continue from the Adam chase crash fix.

Current important state:

- User reported Unreal Editor exit during Adam chase after several seconds.
- Windows Event Viewer showed `ucrtbase.dll` and `0xC0000005`, but UE CrashContext identified the crash as `Unhandled Exception: EXCEPTION_STACK_OVERFLOW`.
- Crash artifacts analyzed:
  - `Saved/Crashes/UECC-Windows-3B04608E41723DFD16C5B88742084507_0000/CrashContext.runtime-xml`
  - `Saved/Crashes/UECC-Windows-3B04608E41723DFD16C5B88742084507_0000/AdaptiveHorror.log`
- WinDbg/cdb were not available in this environment; Visual Studio was installed but not usable for non-interactive dump analysis here.
- First AdaptiveHorror frame:
  - `AEvaZombieAIController::OnMoveCompleted()`
  - `Source/AdaptiveHorror/AI/EvaZombieAIController.cpp:133`
- Repeating stack:
  - `AEvaZombieAIController::ReissueMoveToTarget()`
  - `AEvaZombieAIController::OnMoveCompleted()`
  - repeated through AIModule until stack overflow.
- Root cause:
  - Adam inherits `AEvaZombieAIController` path completion handling.
  - `ReissueMoveToTarget()` could call `MoveToActor()` and receive synchronous/immediate `MoveCompleted` before `LastMoveRequestTime` was updated.
  - `OnMoveCompleted()` then reissued another MoveTo on the same call stack.
- Fix applied:
  - Added `bIssuingRepathMove` to `AEvaZombieAIController`.
  - `OnMoveCompleted()` now refuses to reissue while a reissued MoveTo is in flight.
  - `LastMoveRequestTime` is now stamped before `MoveToActor()` inside `ReissueMoveToTarget()`.
- No Adam attack/charge/roar/summon/phase logic was changed.
- Development Editor / Win64 build without Live Coding succeeds.
- `Automation RunTests AdaptiveHorror` succeeds: 15 tests, 0 failures.
- Runtime smoke with `UnrealEditor-Cmd.exe -game -NullRHI -ExecCmds="Quit"` exits with code 0.

Immediate next PIE checks:

1. Open the editor after the successful build.
2. Start PIE and press F4 to begin Adam encounter.
3. Let Adam chase the player for at least 60 seconds.
4. Confirm Unreal Editor does not freeze or exit.
5. Confirm the log does not show same-frame flooding of:
   - `[AIPath] MoveCompleted`
   - `[AIRepath] Reason=MoveCompleted`
6. Confirm Adam still:
   - tracks the player,
   - uses melee,
   - uses charge,
   - uses roar summon,
   - enters Phase 2,
   - can be defeated to Stage Clear.
7. If a crash still occurs, analyze the new CrashContext first before changing gameplay code.

Important caution:

- Do not add new Adam features until the Adam chase crash is visually confirmed fixed in PIE.
- If a new stack points to Adam-specific `TryChargeAttack`, `TryRoarSummon`, `SpawnRoarMinions`, `ActiveSummons`, `Destroy`, or `EndPlay`, fix only that exact root cause.
