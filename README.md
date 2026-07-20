# Adaptive Horror FPS Demo

## Autonomous development workflow

For future Codex work, start with:

```text
feature/field-pass1としてTASKS/field-pass-1.mdを実装してください。

最初にAGENTS.mdを読み、
REQUIREMENTS.md、TEST_PLAN.md、ROADMAP.mdの関連箇所を確認してください。

自動検証可能な受け入れ条件を満たすまで、
実装、Build、Automation、Runtime Smoke、ログ確認、原因修正を繰り返してください。

条件を満たした場合のみcommitし、
origin/feature/field-pass1へpushしてください。

人間のPIE確認が必要な項目は成功扱いにせず、
最終報告へPIE未確認として残してください。

途中報告や質問は、作業継続不能な場合を除いて不要です。
```

Current source-of-truth document roles:

- `AGENTS.md`: persistent Codex rules, Git rules, autonomous fix loop, final report format.
- `REQUIREMENTS.md`: product requirements and status labels (`Implemented`, `Partial`, `Planned`, `TBD`).
- `ROADMAP.md`: milestone order.
- `TEST_PLAN.md`: real build/automation/runtime smoke/log-scan commands.
- `TASKS/field-pass-1.md`: next executable task specification.
- `DEV_LOG.md`: development history.
- `TODO.md`: open work and manual verification.
- `BUILD_CHECK.md`: recorded verification results and troubleshooting.

Recommended one-command validation:

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\RunCodexValidation.ps1 -MaxParallelActions 4
```

`GAME_DESIGN.md` and `TECH_SPEC.md` remain historical/detail references. New work should avoid duplicating their content and should update the source-of-truth files above when requirements, roadmap, or verification rules change.

## Content Pass 1 progression controls - Cycle 021

Current branch: `feature/content-pass1`.

The Runtime Graybox research facility now has a simple E-key content progression layer on top of the existing combat loop:

1. Restore Facility Power.
2. Find Security Keycard.
3. Unlock Observation Lab.
4. Search Containment Records.
5. Access Data Core.
6. Reach Adam Arena.
7. Defeat Adam.

Interaction:

- `E`: interact with the focused Power Console, Keycard, locked Door, Research Log, or Data Core Console.
- `E`: close the active Research Log overlay.

Normal HUD additions:

- Current Objective.
- Compact progress line for logs, keycard, and power.
- Crosshair-area interaction prompt.
- Research Log overlay while reading.

Debug HUD page 3 now also exposes content state:

- Objective Index.
- Power.
- Keycard.
- Door.
- Logs.
- Data Core.
- Arena.

Content gates:

- Observation Lab requires the door to be opened.
- Containment Ward requires at least one research log.
- Adam Arena requires Data Core access.
- F4 remains the ADAM debug-start path and intentionally bypasses these gates for verification.

## Gameplay Pass 1 Polish debug controls - Cycle 019

Current branch: `feature/gameplay-pass1`.

Debug HUD is now paged to avoid overlap:

- `F9`: toggles Debug HUD ON/OFF and preserves the existing Navigation visualization toggle behavior.
- `N`: advances Debug HUD pages while keeping Navigation visualization unchanged.
- `DEBUG 1/3`: EVA / Gameplay.
- `DEBUG 2/3`: Enemy Adaptation.
- `DEBUG 3/3`: Navigation / Spawn.

Debug HUD is hidden outside active gameplay.

When Debug HUD is ON, enemies can show a short overhead intent line such as:

- `FLANK LEFT`
- `HOLD FRONT`
- `KEEP DISTANCE`
- `ANTI-RANGER`

Normal play still shows enemy name + HP bar only. HUNTER normally shows `HUNTER Tn`; its counter type is shown on the debug-intent line only.

## Gameplay Pass 1 adaptive behavior - Cycle 018

Current branch: `feature/gameplay-pass1`.

The prototype now uses the existing telemetry to drive lightweight enemy adaptation during active gameplay. This is intentionally bounded: it should make the player feel observed and countered without replacing the working zombie chase, Runtime NavMesh, HUNTER, ADAM, Stage Clear, or UI flow.

Normal HUD now shows:

- EVA analysis stage.
- EVA analysis percent.
- Current combat style.

Debug HUD (`F9` / `N`) additionally shows:

- Headshot rate, accuracy, preferred combat distance, close/long range ratios.
- Aggression, stealth, exploration, and sprint-use profile values.
- Current adaptation role such as Flanker, MidRangePressure, Searcher, Ambusher, or CompositeAdaptive.
- Applied enemy tuning multipliers for speed, range, cooldown, damage, and sidestep chance.
- HUNTER counter type.

HUNTER labels can show a counter tag such as `HUNTER T2 [ANTI-RANGER]`.

Evolution label note:

- `COMPOSITE` is the 80% EVA analysis evolved variant.
- In Cycle 018, COMPOSITE was bounded so it does not simply stack every FAST / ARMORED / LONG ARM advantage at full strength; it now chooses an adaptation emphasis from the current player profile.

## Horror immersion pass controls - Cycle 017

Gameplay controls now include:

- `F`: toggle prototype flashlight.
- `Eva.DebugBlackout [seconds]`: console command for a short blackout while gameplay is active.
- `Eva.ReduceFlashing 1`: reduce emergency-light flicker intensity.
- `Eva.ReduceCameraShake 1`: reduce prototype horror camera shake.

Existing debug keys remain unchanged: F1 analysis, F2 HUNTER, F3 wave, F4 ADAM arena, F5 restore, F6 Stage Clear, F7 telemetry, F9/N debug HUD/nav.

## Current Slice — Cycle 003

Runtime生成の研究施設Grayboxが6区画構成になりました。

現在の縦切り:

- Entry Lobby → Security Corridor → Observation Lab → Containment Ward → Data Core Room → Adam Arena → Stage Clear
- `AEvaResearchFacilityDirector` による進行管理
- 区画トリガー、チェックポイント、Cover / HideSpot / EscapeRoute / AmbushPointタグ
- EVAログPickup 5種
- HUNTER出現/撃破/再投入
- 進化個体スポーン
- Boss「ADAM」基盤
- Adam撃破時のStage Clear HUD
- `BUILD_CHECK.md` と `Scripts/RunBuildCheck.ps1` によるUE5実環境検証導線

この環境では `UnrealEditor.exe` / `UnrealBuildTool.exe` / `MSBuild.exe` / `cl.exe` が見つからなかったため、UHT、実ビルド、Automation Test実行、PIEは未実行です。UE5実環境ではまず `BUILD_CHECK.md` の手順を実行してください。

Unreal Engine 5で制作する、敵がプレイヤーの戦い方を観測し、適応・進化するソロ向けFPSサバイバルホラー体験版です。人間向けタイトルは未定、UEプロジェクト／C++モジュールの仮称は `AdaptiveHorror` とします。

## 現在の状態

**C++最小プレイアブル縦切り実装済み／Editor未検証**です。2026-06-23時点で、プロジェクト骨格、FPS入力、HP、ハンドガン、通常ゾンビAI、テレメトリ、C++ HUD、メモリチェックポイント、ゲームオーバー復帰、Pickup、HUNTER／進化接続点があります。

- 起動方式: `/Engine/Maps/Entry` 上で `AEvaPrototypeGameMode` がGrayboxアリーナを実行時生成
- 操作: `WASD` 移動、マウス視点、`Space` ジャンプ、`Left Shift` スプリント、左クリック射撃、`R` リロード
- 戦闘: Handgun（25 damage、HS x2、12/60、1.5秒reload、5000cm hitscan）
- AI: 視覚15m／聴覚10m、NavMesh追跡、10 damage／1.5秒の近接攻撃
- HUD: HP、弾薬、Kill、Accuracy、HS率、Style、EVA解析率
- 検証状態: JSON、構造、必須API、定数の静的検証は完了。UE／MSVC未導入のためUHT、ビルド、Editor、PIEは未実施
- 次のタスク群: HUNTER本格AI、学習倍率連動、進化3種、研究施設進行、アダム基盤

## 起動手順

1. Unreal Engine 5.4以降と、対応するVisual Studio C++ toolchainをインストールする。
2. `AdaptiveHorror.uproject` を対象Engineへ関連付ける。本環境ではEngine未検出のため `EngineAssociation` は意図的に未固定。
3. Project Filesを生成し、`AdaptiveHorrorEditor` Development Editorをビルドする。
4. Editorでプロジェクトを開き、Playする。専用 `.umap` がなくてもRuntime Arenaが生成される。
5. `Tools > Test Automation` で `AdaptiveHorror.EVA` を実行する。

既知の未確認事項は `DEV_LOG.md`、詳細タスクは `TODO.md` を参照してください。

## 体験版の目標

- 想定プレイ時間: 初見30〜60分（基準45分）
- 対象: Windows、キーボード／マウス、ソロプレイ
- 舞台: 封鎖された研究施設
- ゲームの核: プレイヤーの主戦術が敵の行動と個体特性に反映され、同じ戦い方ほど通じにくくなる
- 完了条件: タイトル開始から研究施設探索、通常戦、HUNTER戦、進化個体、ボス「アダム」、ゲームオーバー／チェックポイント復帰、ステージクリアまで通して遊べる

## 設計原則

1. 常にプレイ可能な小さな縦切りを維持する。
2. AI学習の「原因→変化→プレイヤーの気づき」を最優先する。
3. 学習値は再現可能なルールベースで集計し、調整可能なデータにする。
4. 仮アセットを許容し、ゲームループ完成前に美術を作り込まない。
5. 体験版Ver0.1はソロ専用とし、マルチプレイは将来の境界設計だけに留める。

## 文書

- `GAME_DESIGN.md`: 体験設計、ゲームループ、敵、ステージ進行
- `TECH_SPEC.md`: UE5構成、クラス責務、学習データ、保存とテスト方針
- `TODO.md`: 優先順付きの実装チェックリスト
- `DEV_LOG.md`: 各開発サイクルの実施記録
- `NEXT_PROMPT.md`: 次回Codexに渡す実行プロンプト
- `DESIGN_DECISIONS.md`: 仕様変更が発生した時点で作成し、理由と影響を記録する

## 想定ディレクトリ

```text
AdaptiveHorror.uproject
Config/
Content/
  AdaptiveHorror/
    AI/
    Audio/
    Characters/
    Core/
    Maps/
    UI/
    Weapons/
Source/
  AdaptiveHorror/
    AI/
    Characters/
    Core/
    Weapons/
README.md
GAME_DESIGN.md
TECH_SPEC.md
TODO.md
DEV_LOG.md
NEXT_PROMPT.md
```

## 開発サイクル

各サイクルでは、現状確認後に優先順位が最も高い未完了タスクを**1つだけ**選び、実装、可能な範囲のビルド／起動確認、`DEV_LOG.md` 更新、`NEXT_PROMPT.md` 更新まで行います。大きな仕様変更が必要なら実装前に `DESIGN_DECISIONS.md` へ記録します。
## Cycle 008 Runtime Safety Notes

Enemy spawning now uses a shared safe-spawn flow in `AEvaPrototypeGameMode`.

- Initial Entry Lobby zombie, zone encounters, F3 waves, HUNTER, ADAM, and ADAM roar minions all route through collision/NavMesh/floor validation.
- Enemy spawns use `AdjustIfPossibleButDontSpawnIfColliding`; failed spawns are logged instead of forcing overlap.
- The HUD shows active enemy counts, last spawn result/location, NavMesh availability, fallback movement count, and stuck enemy count.
- `LogAdaptiveHorror` contains detailed spawn attempt telemetry for PIE debugging.
- Runtime graybox lighting includes directional, sky, and point lights so the first zombie should be visible without extra map setup.

Build note: if `RunBuildCheck.ps1` fails with `LNK1104` for `UnrealEditor-AdaptiveHorror.dll`, close Unreal Editor and Live Coding Console, then re-run the build check.

## Debug key list - Cycle 009

Current bindings are generated in `AEvaPlayerCharacter::CreateRuntimeInputAssets()` and routed through `AEvaPrototypeGameMode`.

F2:

- Force HUNTER deployment via `DebugForceHunterSpawn()`.

F3:

- Force an infected wave via `DebugForceZombieWave()`.
- Skips if the game mode is not in active combat.

F4:

- Move to the ADAM arena and explicitly start/confirm the ADAM encounter via `DebugWarpPlayerToAdamArena()`.
- F4 also removes nearby non-boss debug enemies so ordinary Adaptive Spawn does not obscure ADAM verification.

F7/F9/N:

- F7 prints a telemetry snapshot.
- F9 and N both toggle navigation debug visualization.
- F9/N are intentionally duplicated.

P:

- No current game-side binding found.

その他:

- F1: EVA analysis +20.
- F5: full player heal/ammo restore.
- F6: force Stage Clear.
- F8: not assigned by game code because it conflicts with PIE Eject.

Evolution label note:

- `COMPOSITE` is the 80% EVA analysis evolved variant.
- FAST / ARMORED / LONG ARM labels still need PIE visual confirmation after the latest ADAM/repath pass.

## Core game UI flow - Cycle 014

The prototype now boots into a basic title/menu flow instead of immediately starting combat.

### Launch and title

- Open `AdaptiveHorror.uproject` in UE5.8 and start PIE.
- The game enters `Title` state and shows `ADAPTIVE HORROR`.
- Buttons:
  - `NEW GAME`: resets the current demo session and starts gameplay.
  - `CONTINUE - Not Available`: intentionally disabled until real save data exists.
  - `SETTINGS`: opens prototype settings.
  - `EXIT`: calls the UE quit path.

### In-game controls

- `Esc`: pause/resume while gameplay is active.
- Pause menu:
  - `RESUME`
  - `RESTART FROM CHECKPOINT`
  - `SETTINGS`
  - `RETURN TO TITLE`
  - `EXIT GAME`
- Player death opens an explicit `GAME OVER` screen with retry/restart/title options.
- Adam defeat opens a `MISSION COMPLETE` Stage Clear screen with retry/title/exit options.

### Settings

Settings are stored in `UEvaSettingsSaveGame` slot `EvaPrototypeSettings`.

Implemented:

- Master Volume.
- BGM Volume.
- SFX Volume.
- Mouse Sensitivity.
- Invert Mouse Y.

Placeholder fields exist for future UI/application logic:

- Fullscreen / Windowed.
- Resolution.
- Graphics Quality.

### HUD and debug keys

Normal HUD now prioritizes:

- HP.
- Ammo.
- Crosshair.
- Current objective.
- EVA analysis/stage.
- HUNTER state.
- Boss HUD during Adam combat.

Detailed debug values are hidden until `F9` or `N` toggles Debug HUD / Navigation visualization.

Current debug keys:

- `F1`: EVA analysis +20.
- `F2`: force HUNTER deployment.
- `F3`: force infected wave.
- `F4`: move to ADAM arena and start/confirm ADAM encounter.
- `F5`: restore player HP/ammo.
- `F6`: force Stage Clear.
- `F7`: print telemetry snapshot.
- `F9` / `N`: toggle Debug HUD and navigation visualization.
- `P`: unbound.
- `F8`: intentionally unbound because it conflicts with PIE Eject.

### Audio status

- Minimal procedural UI tones are generated in C++ for click/menu/game-over/stage-clear feedback.
- Real BGM, gunshot/reload/damage, enemy, HUNTER, and Adam sound assets are still TODO.

### Known limitations

- PIE visual confirmation is still required for the new title/pause/settings/game-over/stage-clear widgets.
- Runtime smoke confirms the game loads into Title state but cannot validate visual layout or audio because it runs with NullRHI/NoSound.
- Title mode still uses the runtime map/pawn as a dark background; gameplay input and combat spawning are blocked until `NEW GAME`.
- Fullscreen/resolution/graphics quality are stored placeholders and are not applied to renderer/window settings yet.
