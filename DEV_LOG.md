# Development Log

## 2026-07-14 - Cycle 016: Visual / Audio Pass 1

Branch: `feature/visual-audio-pass1`

### Scope

- Stable `main` preservation pass. Created feature branch and did not merge to `main`.
- No AI decision, NavMesh, Stage Clear, Game Flow, Player Death, or boss progression logic was intentionally changed.
- Two AI controller files were touched only to fire cosmetic visual/audio feedback after already-existing attack / charge / roar events.

### Implemented

- Enemy prototype silhouettes:
  - Added leg and shoulder mesh parts to the shared enemy visual body.
  - Zombie / FAST / ARMORED / LONG ARM / COMPOSITE now differ by body width, limb length, shoulder mass, leg proportions, label, and attempted material tint.
  - HUNTER uses a darker, taller silhouette.
  - ADAM uses larger body, shoulder, limb proportions and Phase 2 visual scaling.
- Simple procedural animation:
  - Idle / walk are represented by subtle body/head sway and limb swing.
  - Attack feedback makes enemies lean and swing arms.
  - ADAM Attack / Charge / Roar now use distinct temporary poses and tones.
- Prototype audio foundation:
  - Added `UEvaAudioFunctionLibrary` for replaceable procedural tone playback.
  - Existing UI tones now use the shared audio helper.
  - Added temporary tones for gun fire, reload start/end, player damage/death, enemy attack/death, HUNTER spawn, ADAM spawn cue, ADAM attack/charge/roar/death, and facility boot ambience.
  - Runtime smoke was executed with `-NoSound`; audible confirmation remains PIE/manual.
- Lighting pass:
  - Reduced global brightness.
  - Kept runtime graybox floor/cover mobility untouched to avoid destabilizing runtime NavMesh.
  - Movable lights now provide darker blue base lighting plus red emergency lights per facility zone.

### Changed files

- `Source/AdaptiveHorror/Audio/EvaAudioFunctionLibrary.h`
- `Source/AdaptiveHorror/Audio/EvaAudioFunctionLibrary.cpp`
- `Source/AdaptiveHorror/AI/EvaZombieCharacter.h`
- `Source/AdaptiveHorror/AI/EvaZombieCharacter.cpp`
- `Source/AdaptiveHorror/AI/EvaZombieAIController.cpp`
- `Source/AdaptiveHorror/AI/EvaHunterCharacter.cpp`
- `Source/AdaptiveHorror/AI/EvaAdamBossCharacter.h`
- `Source/AdaptiveHorror/AI/EvaAdamBossCharacter.cpp`
- `Source/AdaptiveHorror/AI/EvaAdamBossAIController.cpp`
- `Source/AdaptiveHorror/Characters/EvaPlayerCharacter.cpp`
- `Source/AdaptiveHorror/Characters/EvaPlayerController.cpp`
- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.cpp`
- `Source/AdaptiveHorror/Weapons/EvaWeaponBase.cpp`
- `Source/AdaptiveHorror/Weapons/EvaHitscanWeapon.cpp`
- `DEV_LOG.md`
- `TODO.md`
- `NEXT_PROMPT.md`
- `BUILD_CHECK.md`

### Verification

- `powershell -ExecutionPolicy Bypass -File .\Scripts\RunBuildCheck.ps1`
  - Static source sanity: PASS.
  - Generate Project Files: Succeeded.
  - Development Editor / Win64 build without Live Coding: Succeeded.
  - First pass failed on UHT because child override `AEvaAdamBossCharacter::PlayPrototypeAttackFeedback()` incorrectly repeated `UFUNCTION`; removed child macro and rebuilt.
  - Automation RunTests `AdaptiveHorror`: Succeeded.
  - Latest log confirmed 23 successful tests and `**** TEST COMPLETE. EXIT CODE: 0 ****`.
- Runtime smoke:
  - `UnrealEditor-Cmd.exe -game -Unattended -NullRHI -NoSound -NoSplash -ExecCmds="Quit" -log`
  - Exit code 0.
  - Runtime log confirms `EvaPrototypeGameMode` loaded and Title state/UI initialized.
- PIE visual/audio confirmation: not performed by Codex; remains unconfirmed.

### Known risks / follow-up

- Procedural material tint depends on Engine basic shape material parameters; silhouettes remain differentiated even if tint is not visible.
- Lighting warning must be rechecked in actual PIE viewport. Geometry mobility was not changed because previous runtime NavMesh stability depends on static runtime graybox pieces.
- Audible balance is unverified because smoke test intentionally used `-NoSound`.

### Next recommended task

- Open PIE on branch `feature/visual-audio-pass1` and visually/audibly verify enemy silhouettes, simple walk/attack animation, ADAM charge/roar readability, emergency lighting, and temporary tones.

## 2026-07-12 - Cycle 009: 障害物回避リカバリ / 敵識別表示 / 全Automation導線

### 実装・修正内容

- 通常ゾンビの障害物挟み停止対策を強化した。
  - `MoveTo` がacceptedでも実移動が止まった場合、停止ログだけで終わらず、短い側方迂回 `TrySidestepAroundObstacle()` を実行する。
  - Direct fallbackで前方が壁に塞がれている場合、左右候補を交互に試し、押し付けではなく迂回方向へ移動する。
  - fallback使用時のログとDebug lineを継続し、EnemyStuckが連続発生した際の追跡原因を見やすくした。
- 頭上ラベルの左右反転を修正した。
  - 固定Yaw 180度を廃止し、プレイヤーカメラ方向へYaw-only Billboardする方式に変更。
  - 死亡時は非表示、極端な遠距離では非表示にして、Runtime Grayboxで読みやすさを優先した。
- 敵タイプごとの見た目差を強化した。
  - `LeftArmVisual` / `RightArmVisual` を追加し、LongArmはActorScaleではなく腕パーツを長くする。
  - Fast / Armored / LongArm / Composite / HUNTER / ADAM / ADAM Phase2でBody/Head/Armの相対サイズを変更。
  - Capsule CollisionやNavigation Agentを見た目Scaleで壊さないよう、通常進化ではActorScaleを `OneVector` に固定した。
- HUNTER / ADAMの見た目識別を最低限強化した。
  - HUNTERは高身長・細長い腕・赤ラベル。
  - ADAMは大型Body/Head/Arm、Phase2でさらに大型化。
- `Scripts/RunBuildCheck.ps1` のAutomation対象を `AdaptiveHorror.EVA` から `AdaptiveHorror` 全体へ変更した。
  - EVA / Spawn / Visualの全テストが標準ビルド確認導線で走る。
- Automation Testを2件追加した。
  - `AdaptiveHorror.Visual.Enemy.DebugLabelReadableDefault`
  - `AdaptiveHorror.Visual.Enemy.LongArmUsesPartScale`

### 変更したファイル

- `Scripts/RunBuildCheck.ps1`
- `Source/AdaptiveHorror/AI/EvaZombieAIController.h`
- `Source/AdaptiveHorror/AI/EvaZombieAIController.cpp`
- `Source/AdaptiveHorror/AI/EvaZombieCharacter.h`
- `Source/AdaptiveHorror/AI/EvaZombieCharacter.cpp`
- `Source/AdaptiveHorror/AI/EvaHunterCharacter.cpp`
- `Source/AdaptiveHorror/AI/EvaAdamBossCharacter.cpp`
- `Source/AdaptiveHorror/Tests/EvaLearningTests.cpp`
- `DEV_LOG.md`
- `TODO.md`
- `NEXT_PROMPT.md`
- `BUILD_CHECK.md`

### Build / Automation / Runtime Smoke結果

- 初回 `RunBuildCheck.ps1 -MaxParallelActions 2`
  - Development Editor Build中に `GameFramework/PlayerCameraManager.h` includeパス不備で失敗。
  - UE5.8では `Camera/PlayerCameraManager.h` が正しいため修正。
  - 同時にUBAがLow memoryで一部compile processをkill/retryしたため、以降は `-MaxParallelActions 1` を使用。
- 修正後 `powershell -ExecutionPolicy Bypass -File .\Scripts\RunBuildCheck.ps1 -MaxParallelActions 1`
  - Static source sanity: PASS
  - Generate Project Files: Succeeded
  - Development Editor / Win64 Build: Succeeded
  - Automation RunTests `AdaptiveHorror`: 15件すべてSuccess
  - `TEST COMPLETE. EXIT CODE: 0`
- Runtime Smoke:
  - `/Engine/Maps/Entry` が `EvaPrototypeGameMode` で起動。
  - `UNavigationSystemV1::Build started` を確認。
  - 初期ゾンビ `EnemyVisual` ログを確認。
  - Fatal / crashなし。
  - プロセスは15秒後に手動停止したため終了コードは `-1`。

### PIE確認結果

- ユーザー報告により確認済み:
  - Runtime NavMesh表示/Ready。
  - 通常ゾンビの出現、追跡、接近攻撃。
  - MoveToActor accepted / PathValid=true。
  - 壁上スポーン再発なし。
- このCodex環境ではEditor viewportの目視操作ができないため未確認:
  - 障害物を挟んだ際にゾンビが実際に迂回してプレイヤーへ再接近すること。
  - 頭上ラベルがPIE画面上で左右反転せず読めること。
  - 敵タイプ差がプレイ中に十分見分けられること。
  - HUNTER / ADAMの実PIE追跡、攻撃後の再追跡、撃破/再出現。

### 既知の未解決・リスク

- Runtime Graybox環境のため、正式 `.umap` と保存済みNavMeshBoundsVolumeではない。NavMesh品質はPIEで継続確認が必要。
- 障害物回避はBehavior Tree全面移行ではなく、既存AIController内の軽量リカバリ。複雑な迷路では限界がある。
- TextRenderラベルのBillboardはYaw-only。通常FPS角度での読みやすさ優先で、極端な上下角では未確認。

### 次にやるべきこと

- UE5.8 PIEで、障害物を挟んだゾンビが停止せず迂回・再接近できるか確認する。
- F2でHUNTERの追跡・攻撃・撃破・30秒Tier+1再投入を確認する。
- F4でADAMの追跡・攻撃後の再追跡・Phase2・撃破Stage Clearを確認する。
- 敵タイプ差、頭上ラベル、HUD NAV DEBUG、ライトの見え方を実画面で調整する。

## 2026-07-12 - Cycle 008: Safe Enemy Spawn / Spawn Telemetry / Marker Lighting

### Implemented

- Added a shared safe enemy spawn path in `AEvaPrototypeGameMode`.
  - `FindSafeEnemySpawnLocation(...)` checks distance from player, distance from existing enemies, NavMesh projection, floor trace, and capsule overlap against pawns/world static.
  - `SpawnEnemyNearLocation(...)` uses `AdjustIfPossibleButDontSpawnIfColliding`, spawns the AI controller, logs spawn telemetry, and relocates/destroys if a post-spawn overlap is detected.
- Routed initial zombie, zone encounters, adaptive spawns, F3 waves, HUNTER, ADAM, and ADAM roar minions through the safe spawn flow.
- Added `LogAdaptiveHorror` telemetry for spawn reason, requested/final location, attempt index, NavMesh/floor/overlap state, player distance, nearest enemy distance, collision handling, actor result, and controller possession.
- Added HUD debug telemetry for active enemy counts, last spawn result/location, NavMesh availability, fallback movement count, and stuck enemy count.
- Improved direct movement fallback with enemy separation and a short obstacle trace so fallback does not keep pushing enemies into walls or each other.
- Added movable directional and sky lights to make the runtime graybox visibly playable.
- Added automation coverage for safe spawn failure, floor candidate search, enemy separation, and debug counters.

### Verification

- Ran `powershell -ExecutionPolicy Bypass -File .\Scripts\RunBuildCheck.ps1 -MaxParallelActions 2`.
- Static source sanity: PASS.
- Generate Project Files: Succeeded.
- Modified C++ files compiled after fixing the directional light component cast.
- Full Development Editor link is currently blocked because a running `UnrealEditor.exe` process is holding `Binaries/Win64/UnrealEditor-AdaptiveHorror.dll`.

### Required Next Verification

- Close Unreal Editor and Live Coding Console.
- Re-run `Scripts\RunBuildCheck.ps1 -MaxParallelActions 2`.
- Run automation tests.
- PIE manual test Entry Lobby initial zombie, repeated F3 waves, F2 HUNTER, F4 ADAM, NavMesh visibility, and absence of repeated stuck logs.

### Remaining Risks

- Runtime graybox still relies on `bWholeWorldNavigable`; a saved `.umap` with authored NavMesh bounds remains the long-term fix.
- Safe spawn behavior still needs PIE manual validation across all facility zones.

## 2026-07-11 — Cycle 007: ゲームループ安定化 / NavMesh・AI追跡フォールバック修正

### 実装・修正内容

- 体験版ゲームループの安定化を優先し、Runtime Graybox環境で敵AIが棒立ちになり得る問題を修正した。
- UE5.8 Standalone起動ログで、Runtime生成した `ANavMeshBoundsVolume` が空Boundsとして扱われる警告を確認した。
  - `LogNavigationDirtyArea: Skipped some dirty area creation ... 1 empty bounds`
  - 原因: C++でRuntime Spawnした `ANavMeshBoundsVolume` は、Editorで配置・保存されたブラシ形状を持たないため、Navigation Boundsとして有効な範囲を提供できない場合がある。
- `AEvaPrototypeGameMode::BuildPrototypeArena()` で空のNavMeshBoundsVolumeをSpawnする処理をやめ、UE5.8の `UNavigationSystemV1::bWholeWorldNavigable` を有効化してから `Build()` する構成に変更した。
- Runtime Grayboxの床・壁・遮蔽物StaticMeshをNavigation relevantにし、最終Transform適用後はStatic mobilityに戻すよう調整した。
- `AEvaZombieAIController` にNavMesh失敗時の直接移動フォールバックを追加した。
  - `MoveToActorOrDirect()`
  - `MoveToLocationOrDirect()`
  - `MoveToActor` / `MoveToLocation` が `EPathFollowingRequestResult::Failed` を返した場合でも、Pawnが目標方向へ直接移動する。
- 通常ゾンビ、HUNTER、ADAMの追跡・適応移動・突進移動を上記フォールバック経由に変更した。
- GameOver / StageClear周辺のTimerと状態の重複を予防した。
  - Player死亡時、RespawnTimerをClearしてから再登録。
  - StageClear時、RespawnTimer / Spawn系Timerを解除し、Enemy Targetをリセット。
  - Debug F5復帰時、RespawnTimerと `PlayerAwaitingRespawn` をクリア。
  - GameOver / StageClear中はAdaptive Spawn / Debug Waveを抑制。

### 変更したファイル

- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.cpp`
- `Source/AdaptiveHorror/AI/EvaZombieAIController.h`
- `Source/AdaptiveHorror/AI/EvaZombieAIController.cpp`
- `Source/AdaptiveHorror/AI/EvaHunterAIController.cpp`
- `Source/AdaptiveHorror/AI/EvaAdamBossAIController.cpp`
- `DEV_LOG.md`
- `TODO.md`
- `NEXT_PROMPT.md`
- `BUILD_CHECK.md`
- `TECH_SPEC.md`
- `DESIGN_DECISIONS.md`

### 実行した検証

- `powershell -ExecutionPolicy Bypass -File .\Scripts\RunBuildCheck.ps1 -MaxParallelActions 2`
  - Static source sanity: PASS
  - Generate Project Files: Succeeded
  - Development Editor / Win64 Build: Succeeded
  - `AdaptiveHorror.EVA` Automation Test: 8件すべてSuccess
    - `AdaptiveHorror.EVA.AdamDefeatStageClear`
    - `AdaptiveHorror.EVA.AdamPhaseTransition`
    - `AdaptiveHorror.EVA.AnalysisRateIncrease`
    - `AdaptiveHorror.EVA.DirectorProgression`
    - `AdaptiveHorror.EVA.EvolutionThresholds`
    - `AdaptiveHorror.EVA.HunterLearningMultiplier`
    - `AdaptiveHorror.EVA.StoryLogPickup`
    - `AdaptiveHorror.EVA.TelemetryClassification`
- Standalone Game相当の短時間起動:
  - `/Engine/Maps/Entry` が `EvaPrototypeGameMode` で起動することをログ確認。
  - Runtime Navigation Buildが開始されることをログ確認。
  - 修正前に出ていたRuntime NavMeshBoundsVolumeのempty bounds警告が、空Bounds更新削除後に出なくなったことを確認。
  - 起動時Fatal / crashは確認されなかった。

### PIEで実際に確認済みの項目

- ユーザー報告により確認済み:
  - Development Editor / Win64ビルド成功。
  - Automation Test 8件成功。
  - PIEでプレイヤー移動・視点・射撃が動作。
  - マウス上下反転修正済み。

### このターンでPIE手動確認できなかった項目

こちらの実行環境ではUnreal Editorのビューポート内でWASD、射撃、F1〜F7、敵戦闘を手動操作・目視確認できないため、以下は未確認として残す。

- 通常ゾンビがPIE上でNavMeshまたはDirect fallbackにより追跡・攻撃すること。
- プレイヤー死亡、3秒後のCheckpoint復帰、複数回死亡時のTimer重複なし。
- F2によるHUNTER強制出現、撃破、解析コアDrop、30秒再投入。
- F1/F3/F7によるEVA解析・進化・Telemetry表示。
- 研究施設6区画の通し移動、EVAログ5件取得、Adam Arena到達。
- F4/通常進行によるADAM戦、Phase 2、撃破Stage Clear。
- F1〜F7 DebugキーのPIE目視確認。

### 残っているリスク

- Runtime Grayboxは `.umap` に保存された正式ステージではないため、NavMeshは暫定的に `bWholeWorldNavigable` とDirect movement fallbackで成立させている。
- 正式な研究施設マップへ移行する際は、Editor上でNavMeshBoundsVolumeを配置・保存し、Direct fallbackへの依存を減らす必要がある。
- Standalone短時間起動では入力・戦闘までは検証できていない。

### 次にやるべきこと

- UE5.8 Editor上のPIEで、通常ゾンビ戦闘からStage Clearまでを手動通し確認する。
- F1〜F7 Debugキーを実際に押して、HUNTER、EVA解析、Adam、Stage Clearの状態遷移を確認する。
- 研究施設6区画の導線、敵スポーン量、Adam戦の難易度を調整する。

## 2026-07-11 — Cycle 006: PIEマウスY反転修正

### 実装内容

- UE5.8 PIEでマウス視点操作の上下だけが逆になる問題を修正した。
- 原因:
  - `AEvaPlayerCharacter::Look` が `LookValue.Y` をそのまま `AddControllerPitchInput()` に渡していた。
  - Runtime Enhanced Input Mapping側のMouseYは `Swizzle(YXZ)` のみで、`Negate` は入っていなかった。
  - そのため、反転はInput Mapping側ではなくC++側のPitch入力符号で管理するのが最小修正。
- 修正:
  - `bInvertMouseY` をプレイヤー設定として追加した。
  - デフォルトは `false`。
  - `false` のとき通常FPS操作として `-LookValue.Y` を `AddControllerPitchInput()` へ渡す。
  - `true` のとき上下反転として `LookValue.Y` を渡す。
  - `IsMouseYInverted()` / `SetInvertMouseY()` を追加し、Blueprint/設定画面から変更可能にした。

### 変更ファイル

- `Source/AdaptiveHorror/Characters/EvaPlayerCharacter.h`
- `Source/AdaptiveHorror/Characters/EvaPlayerCharacter.cpp`
- `Scripts/RunBuildCheck.ps1`
- `DEV_LOG.md`
- `TODO.md`
- `NEXT_PROMPT.md`

### 検証結果

- 静的検証:
  - `.generated.h` include順序: PASS。
  - `.uproject` JSON parse: PASS。
  - `bInvertMouseY` / `SetInvertMouseY()` / Pitch符号修正の存在確認: PASS。
  - `Scripts/RunBuildCheck.ps1` PowerShell Parser確認: PASS。
- `RunBuildCheck.ps1 -MaxParallelActions 4` 実行:
  - UHT: 成功。
  - C++ Compile: `EvaPlayerCharacter.cpp` を含めて成功。
  - Link: 失敗。
    - 理由: UE Editor / Live Codingが起動中で `Binaries/Win64/UnrealEditor-AdaptiveHorror.dll` を掴んでいたため。
    - `LNK1104: UnrealEditor-AdaptiveHorror.dll を開けない`
  - Automation Test: Link未完了のため未実行。
- `RunBuildCheck.ps1` に `-NoHotReload` / `-NoHotReloadFromIDE` を追加したが、既にEditorがDLLをロードしている場合はリンク差し替え自体が不可能。

### 残っている確認

- UE Editorを閉じてから `powershell -ExecutionPolicy Bypass -File .\Scripts\RunBuildCheck.ps1 -MaxParallelActions 4` を再実行する。
- Automation Testを再実行する。
- PIEで以下を手動確認する。
  - マウス上移動で視点が上を向く。
  - マウス下移動で視点が下を向く。
  - 左右方向が壊れていない。
  - `bInvertMouseY=true` で上下反転する。

## 2026-07-11 — Cycle 005: UE5.8 Development Editor Build修正

### 実行した検証

- `powershell -ExecutionPolicy Bypass -File .\Scripts\RunBuildCheck.ps1 -SkipAutomation` を実行し、UE5.8 / MSVC toolchainを検出。
- 初回失敗:
  - `AdaptiveHorrorEditor` が `UndefinedIdentifierWarningLevel` / `UnreachableCodeWarningLevel` / `ReturnTypeWarningLevel` / `DanglingWarningLevel` を旧設定へ変更している扱いになり、UE5.8共有Editorビルド環境で拒否された。
- 修正後、`powershell -ExecutionPolicy Bypass -File .\Scripts\RunBuildCheck.ps1 -MaxParallelActions 4` を実行。
- 結果:
  - Generate Project Files: Succeeded。
  - Development Editor Build / Win64: Succeeded。
  - `AdaptiveHorror.EVA` Automation Test: 8件すべてSuccess。
  - PIE Smoke Test: 未実行。次回Editor上で手動確認が必要。

### 修正内容

- `AdaptiveHorrorEditor.Target.cs` と `AdaptiveHorror.Target.cs` を UE5.8向けに `DefaultBuildSettings = BuildSettingsVersion.V7` へ更新。
- UE5.8/V7設定で `#include "AI/..."` などのモジュール内includeが解決できなかったため、`AdaptiveHorror.Build.cs` に `ModuleDirectory` を `PublicIncludePaths` として追加。
- UE5.8 API差分として `APointLight::GetPointLightComponent()` を `GetLightComponent()` + `Cast<UPointLightComponent>()` へ置換。
- V7でエラー化されたC4458 shadowingを修正。
  - `AEvaAdamBossCharacter::SpawnRoarMinions` のローカル `EvolutionType` を `MinionEvolutionType` に変更。
  - `AEvaZombieCharacter::OnDefeated` のローカル `Controller` を `ZombieController` に変更。
- `Scripts/RunBuildCheck.ps1` に `-MaxParallelActions` を追加し、メモリ逼迫時も安定して再ビルドできるようにした。

### 作成・変更ファイル

- `Source/AdaptiveHorrorEditor.Target.cs`
- `Source/AdaptiveHorror.Target.cs`
- `Source/AdaptiveHorror/AdaptiveHorror.Build.cs`
- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.cpp`
- `Source/AdaptiveHorror/AI/EvaAdamBossCharacter.cpp`
- `Source/AdaptiveHorror/AI/EvaZombieCharacter.cpp`
- `Scripts/RunBuildCheck.ps1`
- `DEV_LOG.md`
- `TODO.md`
- `NEXT_PROMPT.md`
- `BUILD_CHECK.md`
- `TECH_SPEC.md`

### 残っているリスク

- Visual Studio compiler 14.51.36248 はUE5.8のpreferred versionより新しいため、UBT警告が出る。
- PIEは未確認。Editor上でPlayし、6区画導線、F1〜F7 Debugキー、HUNTER、ADAM、Stage Clearを確認する必要がある。

### 次にやるべきこと

- UE5.8 EditorでPIE実プレイ確認。
- 研究施設6区画の導線調整。
- HUNTER出現・再投入の体感調整。
- ADAM戦の難易度調整。
- 最低限のサウンドと画面演出追加。

## 2026-07-10 — Cycle 004: 実ビルド導線再検証 / Debugキー / クラッシュ予防

### 実行した検証

- `powershell -ExecutionPolicy Bypass -File .\Scripts\RunBuildCheck.ps1` を実行した。
- 結果:
  - `.uproject` 検出成功。
  - Static source sanity: PASS。
  - `UnrealEditor.exe`: NOT FOUND。
  - `UnrealBuildTool.exe`: NOT FOUND。
  - `MSBuild.exe`: NOT FOUND。
  - `cl.exe`: NOT FOUND。
  - UE toolchain未検出のため、Generate Project Files / Development Editor Build / Automation Test / PIE確認は未実行。
- 追加の静的検証:
  - `.generated.h` include順序: PASS。
  - `.uproject` JSON parse: PASS。
  - Debugキー実装シンボル確認: PASS。
  - GameMode主要関数の重複定義確認: PASS。
  - `Scripts/RunBuildCheck.ps1` PowerShell Parser確認: PASS。

### 実装内容

- `Scripts/RunBuildCheck.ps1` を強化した。
  - UE未検出時でも `.generated.h` 順序と `.uproject` JSONの静的sanity checkを実行する。
  - UE実環境ではDevelopment Editor Build後に `AdaptiveHorror.EVA` Automation Testを実行する導線へ更新した。
  - PIE smoke testはEditor上での手動確認項目として明示した。
- 非Shipping用Debugキーを追加した。
  - F1: EVA解析率 +20。
  - F2: HUNTER強制出現。
  - F3: ゾンビWave強制スポーン。
  - F4: ADAM ArenaへワープしADAM戦を開始。
  - F5: プレイヤー全回復・弾薬補充・入力復帰。
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
  - HUDのHealth/Telemetry/GameMode/Director参照null保護。
  - PlayerのBeginPlay/EndPlay/Input/Respawn/Death処理null保護。
  - WeaponのReload Timer解除と死亡後射撃防止。
  - Zombie死亡処理の二重実行防止。
  - Zombie/Adam/HunterのHealth・Visual・Movement参照保護。
  - Checkpoint復帰時に敵AIのTargetActor/Focus/Moveを解除。
  - HUNTER Spawn失敗/既存HUNTER pending状態の保護。

### 作成・変更ファイル

- `Scripts/RunBuildCheck.ps1`
- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.h`
- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.cpp`
- `Source/AdaptiveHorror/Characters/EvaPlayerCharacter.h`
- `Source/AdaptiveHorror/Characters/EvaPlayerCharacter.cpp`
- `Source/AdaptiveHorror/AI/EvaLearningSubsystem.h`
- `Source/AdaptiveHorror/AI/EvaLearningSubsystem.cpp`
- `Source/AdaptiveHorror/AI/EvaZombieAIController.h`
- `Source/AdaptiveHorror/AI/EvaZombieAIController.cpp`
- `Source/AdaptiveHorror/AI/EvaZombieCharacter.h`
- `Source/AdaptiveHorror/AI/EvaZombieCharacter.cpp`
- `Source/AdaptiveHorror/AI/EvaHunterCharacter.cpp`
- `Source/AdaptiveHorror/AI/EvaAdamBossCharacter.cpp`
- `Source/AdaptiveHorror/Weapons/EvaWeaponBase.h`
- `Source/AdaptiveHorror/Weapons/EvaWeaponBase.cpp`
- `Source/AdaptiveHorror/Weapons/EvaHitscanWeapon.cpp`
- `Source/AdaptiveHorror/UI/EvaHUD.cpp`
- `Source/AdaptiveHorror/World/EvaResearchFacilityDirector.cpp`
- `BUILD_CHECK.md`
- `TODO.md`
- `NEXT_PROMPT.md`
- `TECH_SPEC.md`

### 発見したエラー / 修正したエラー

- 実ビルド環境未検出により、UHT/Compileエラーは実測できなかった。
- 静的確認で、既存の日本語EVAログタイトルはUTF-8としては正常だったが、PowerShell表示では文字化けするため、実ビルド時のソース文字コード差分リスクを下げる目的でRuntime生成ログタイトルをASCIIへ変更した。
- GameModeへのDebug実装追加時に `SpawnAdaptiveEnemy` / `SpawnHunter` が一時的に二重定義になったため、古い定義を削除し安全版へ一本化した。
- HUDがTelemetry/Healthを無条件参照していたため、初期化途中クラッシュを防ぐnull保護を追加した。

### 残っているリスク

- この環境では `UnrealEditor.exe` / `UnrealBuildTool.exe` / `MSBuild.exe` / `cl.exe` がないため、UHT / Development Editor Build / Automation Test / PIEは未検証。
- UE5 minor version差分により、Enhanced InputやAutomation Test周辺で追加include/API調整が必要になる可能性がある。
- Research Facilityの導線、HUNTER再投入、ADAM戦の難易度はPIE実測が必要。

### 次にやるべきこと

- UE5実環境で `Scripts/RunBuildCheck.ps1` を再実行する。
- 出たUHT/Compile/Automation/PIEエラーを最優先で修正する。
- PIEでF1〜F7 Debugキーを確認する。
- 研究施設6区画、HUNTER出現・再投入、ADAM戦のプレイ感を調整する。

## 2026-06-23 — Cycle 003: Build Check導線 / 研究施設Graybox / ADAM基盤

### 実装内容

- `BUILD_CHECK.md` を追加し、Windows UE5実環境での検証手順を整理した。
- `Scripts/RunBuildCheck.ps1` を追加し、UE / UBT / MSBuild / cl.exe 探索、Generate Project Files、Development Editor Buildの導線を作った。
- Runtime Grayboxを6区画の研究施設ステージへ拡張した。
  - Entry Lobby
  - Security Corridor
  - Observation Lab
  - Containment Ward
  - Data Core Room
  - Adam Arena
- `AEvaResearchFacilityDirector` を追加し、現在区画、区画スポーン、EVAログ取得、HUNTERイベント、進化解禁、Adam戦開始、Stage Clearを管理するようにした。
- `AEvaFacilityZoneTrigger` を追加し、プレイヤーOverlapでDirectorへ区画進行を通知するようにした。
- `AEvaStoryLogPickup` を追加し、EVAログ5種の取得状態とHUD表示をDirectorで管理するようにした。
- `AEvaAdamBossCharacter` と `AEvaAdamBossAIController` を追加した。
  - HP 2500
  - 近接25 damage / 2.0秒
  - 突進35 damage / 8秒Cooldown
  - 咆哮召喚
  - HP50%以下でPhase 2
  - Phase 2で移動速度+20%、攻撃間隔-20%、進化個体召喚
  - 撃破でStage Clear
- HUDを拡張した。
  - Zone
  - Objective
  - EVA Logs
  - Story Log表示
  - Stage Clear Result
- Automation Testコードを追加した。
  - Director進行
  - EVAログ取得
  - Adamフェーズ移行
  - Adam撃破Stage Clear

### 作成・変更ファイル

- `BUILD_CHECK.md`
- `Scripts/RunBuildCheck.ps1`
- `Source/AdaptiveHorror/AI/EvaTelemetryTypes.h`
- `Source/AdaptiveHorror/AI/EvaAdamBossCharacter.h/.cpp`
- `Source/AdaptiveHorror/AI/EvaAdamBossAIController.h/.cpp`
- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.h/.cpp`
- `Source/AdaptiveHorror/Pickups/EvaStoryLogPickup.h/.cpp`
- `Source/AdaptiveHorror/UI/EvaHUD.cpp`
- `Source/AdaptiveHorror/World/EvaFacilityZoneTrigger.h/.cpp`
- `Source/AdaptiveHorror/World/EvaResearchFacilityDirector.h/.cpp`
- `Source/AdaptiveHorror/Tests/EvaLearningTests.cpp`
- `README.md`
- `TECH_SPEC.md`
- `DESIGN_DECISIONS.md`
- `TODO.md`
- `DEV_LOG.md`
- `NEXT_PROMPT.md`

### 動作確認結果

- `Scripts/RunBuildCheck.ps1` 実行: Project検出は成功。
- `UnrealEditor.exe`: NOT FOUND。
- `UnrealBuildTool.exe`: NOT FOUND。
- `MSBuild.exe`: NOT FOUND。
- `cl.exe`: NOT FOUND。
- UE toolchain未検出のため、Generate Project Files / Development Editor Build / Automation Test / PIEは未実行。
- `.uproject` JSON parse: PASS。
- UTF-8読み込み: PASS。
- `generated.h` 最終include順序チェック: PASS。

### 未解決の問題

- UE5実環境でのUHT/Compile/PIE検証が未完了。
- Runtime生成した6区画のNavMeshとAI経路はPIEで要確認。
- Adamの突進/咆哮/Phase 2は最小実装。見た目、警告、弱点露出、専用音は未実装。
- 研究施設は仮スケール/仮導線。30〜60分体験版としては敵密度、回復、弾薬、ゲート演出の調整が必要。

### 次にやるべきこと

1. UE5実ビルドで出たエラーを修正する。
2. 研究施設Grayboxのプレイ感を調整する。
3. アダム戦のバランスを調整する。
4. HUNTER演出を追加する。
5. 体験版UIを改善する。
6. 簡易サウンドを追加する。

## 2026-06-23 — Cycle 002: HUNTER本格AI / EVA適応 / 進化個体

### 実装内容

- HUNTERを仮クラスから専用AI付きの観測ユニットへ拡張。
  - `AEvaHunterAIController` を追加し、プレイヤースタイル別に対抗行動を切り替えるようにした。
  - Berserker: 距離を取る。
  - Ranger: `EvaCover` タグ付き遮蔽物へ移動。
  - Ghost: `EvaHideSpot` を巡回。
  - Explorer: `EvaAmbushPoint` で待ち伏せ。
- HUNTER出現条件を追加。
  - 通常ゾンビ撃破数3体以上、または45秒経過でHUNTERを投入。
  - HUNTER撃破後30秒でTier+1のHUNTERを再投入。
- HUNTER撃破フローを実装。
  - 撃破時に `AEvaAnalysisCorePickup` をドロップ。
  - `UEvaLearningSubsystem` の学習倍率を1.0から0.3へ低下。
  - 再投入時に学習倍率を1.0へ復帰。
- EVA解析率を拡張。
  - 命中、HS、キル、被ダメージ、HUNTER観測で解析率が上昇。
  - HUNTER観測は高精度サンプルとして、プレイヤーTelemetry SnapshotをEVAへ同期。
  - `Learning / Adapting / Evolving` の解析段階を追加。
- 敵適応システムを実装。
  - 解析率20%以上でプレイヤー傾向に応じた `EEvaAdaptationDirective` を返す。
  - 近距離型対策: 距離を取る。
  - 遠距離型対策: 遮蔽物を使う。
  - 隠密型対策: 視覚/聴覚範囲を拡張。
  - 探索型対策: 待ち伏せポイントへ移動。
- 進化個体を実体化。
  - 20%: 高速型。移動速度+25%。
  - 40%: 装甲型。頭部被ダメージ-50%、最大HP+30%。
  - 60%: 長腕型。攻撃距離+2.5m、攻撃力+15%。
  - 80%: 複合型。高速/装甲/長腕を同時適用。
  - `AEvaPrototypeGameMode` のAdaptive Spawnで解析率に応じた進化ゾンビを実際にスポーン。
- HUDを拡張。
  - EVA解析率、解析段階、適応方針、次スポーン進化型、HUNTER状態、HUNTER Tier、学習倍率を表示。
- Automation Testを追加/更新。
  - HUNTER撃破中の倍率0.3。
  - 解析率増加。
  - 20/40/60/80%の進化閾値。

### 作成・変更ファイル

- `Source/AdaptiveHorror/AI/EvaTelemetryTypes.h`
- `Source/AdaptiveHorror/AI/EvaLearningSubsystem.h/.cpp`
- `Source/AdaptiveHorror/AI/EvaZombieAIController.h/.cpp`
- `Source/AdaptiveHorror/AI/EvaZombieCharacter.h/.cpp`
- `Source/AdaptiveHorror/AI/EvaHunterCharacter.h/.cpp`
- `Source/AdaptiveHorror/AI/EvaHunterAIController.h/.cpp`
- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.h/.cpp`
- `Source/AdaptiveHorror/Characters/EvaPlayerCharacter.cpp`
- `Source/AdaptiveHorror/Components/EvaPlayerTelemetryComponent.h/.cpp`
- `Source/AdaptiveHorror/Pickups/EvaAnalysisCorePickup.h/.cpp`
- `Source/AdaptiveHorror/UI/EvaHUD.cpp`
- `Source/AdaptiveHorror/Tests/EvaLearningTests.cpp`
- `DEV_LOG.md`
- `TODO.md`
- `NEXT_PROMPT.md`

### 動作確認結果

- `.uproject` JSON parse: PASS。
- 全 `.h/.cpp/.cs/.ini/.md/.uproject` のUTF-8読み込み: PASS。
- UHT向け `generated.h` 最終include順序チェック: PASS。
- HUNTER、適応、進化、HUD、テスト用の必須API/定数の静的確認: PASS。
- `UnrealEditor.exe`、`MSBuild.exe`、`cl.exe` はこの環境のPATHおよび標準インストール先から検出できず。
- そのため、UHT、C++実ビルド、Automation Test実行、Editor起動、PIE Smoke Testは未実行。

### 未解決の問題

- UE5実環境でのコンパイル検証が未完了。
- Runtime生成NavMeshとAI移動はPIE上で要確認。
- HUNTERのカバー/隠れ場所/待ち伏せ移動はタグ付き仮オブジェクト依存。研究施設ステージ化時に専用Actor/Volumeへ移行する。
- 進化個体の視覚差分は仮スケールのみ。Material、VFX、アニメーションは未実装。
- 学習ルールはまだVer0.1の決定論的ルール。将来はサンプル信頼度、時間減衰、連続死亡補正を入れる。

### 次にやるべきこと

1. UE5 + Visual Studio C++ toolchain上でGenerate Project Files、Development Editor Build、Automation Test、PIEを実行する。
2. Compile Error / UHT Error があれば最優先で修正する。
3. 研究施設ステージの6区画Graybox進行を作り、Route/Hide/Cover/Ambushタグを実ステージへ移す。
4. アダムボス基盤を実装する。
5. HUNTER/進化個体の見た目・音・警告演出を追加し、学習されている感を強める。

## 2026-06-23 — Cycle 000: 現状確認と開発基準の作成

### 今回実装した内容

- ワークスペースを確認し、既存のUnrealプロジェクトやゲーム実装がないことを確認した。
- 30〜60分の研究施設体験版について、ゲーム進行、敵種、AI学習の見せ方、スコープ外を定義した。
- UE5 C++を中心とする技術構成、学習データモデル、HUNTERライフサイクル、保存、テスト方針を定義した。
- 指定優先順を、検証可能な小タスクのロードマップへ分解した。
- 次サイクルを「プロジェクト構成」だけに限定する引き継ぎプロンプトを作成した。

### 変更したファイル

- `README.md` — プロジェクト概要、現状、目標、文書構成
- `GAME_DESIGN.md` — 体験設計、ゲームループ、敵、進行、AI学習のフェアネス
- `TECH_SPEC.md` — アーキテクチャ、データ、適応規則、保存、テスト、ロードマップ
- `TODO.md` — 優先順付きチェックリスト
- `DEV_LOG.md` — 本記録
- `NEXT_PROMPT.md` — 次サイクルの実行指示

### 動作確認結果

- ファイル構成確認: 完了。確認時点でトップレベルには `outputs/` と `work/` の空ディレクトリだけが存在した。
- Unreal Editor起動: 未実施。`.uproject` が存在しないため。
- C++ビルド: 未実施。`Source/` とBuild Targetが存在しないため。
- ゲーム機能テスト: 未実施。実装が存在しないため。
- 文書整合性: 指定された機能、優先順、HUNTERの100%／30%／再投入、3進化の数値を各文書へ反映した。
- 文書検証: 指定6ファイルの存在、各ファイルの単一H1、必須仕様キーワード12項目をUTF-8で機械照合し、すべて通過した。
- Git差分確認: 未実施。この環境では `git` コマンドがPATH上に存在しなかったため。

### 残タスク

ゲーム機能はすべて未実装。詳細は `TODO.md` を参照。直近では、Git利用可否、UEバージョン／toolchain確認、C++プロジェクト骨格、GameMode、PlayerController、空のPlayerCharacter、開発Map、Editorビルド／PIE確認が必要。

### 次に実装すべきタスク

**P0: 最小のUE5 C++プロジェクト構成を作り、空の開発MapをEditorで起動可能にする。**

このサイクルではプレイヤー移動や武器へ進まない。プロジェクトが生成・ビルド・起動できることを先に確定する。

## 2026-06-23 — Cycle 001: C++最小プレイアブル縦切り

### 実装内容

- Engine Associationを推測で固定しないUE5 C++プロジェクトとRuntimeモジュールを作成した。
- `/Engine/Maps/Entry` 上に床、壁、遮蔽物、照明、NavMesh Bounds、PlayerStart、Zombie、Checkpoint、Pickupを生成するRuntime Arenaを実装した。
- Enhanced InputのAction／Mapping ContextをC++で生成し、WASD、マウス、Jump、Sprint、Fire、Reloadを実装した。
- 共有Health Component、被ダメージ、死亡通知、復帰を実装した。
- Handgunを実装した。25 damage、HS x2、12発magazine、60発reserve、1.5秒reload、5000cm hitscan。
- 仮Head／Torso Hitbox、命中距離、射撃／命中／HS／Kill記録を接続した。
- 通常Zombieを実装した。100 HP、AI Perception視覚15m／聴覚10m、NavMesh追跡、10 damage／1.5秒近接攻撃、被弾時Alert、死亡ログ。
- Player TelemetryとGameInstance Learning Subsystemを実装し、指定9項目、各種率、主武器、仮Style分類を公開した。
- HUNTER仮クラスと、撃破時に学習倍率を1.0から0.3へ変更する接続を実装した。再投入はTODO。
- 進化型EnumとZombieの `ConfigureEvolution()` 接続点を実装した。数値Modifierは次サイクル。
- C++ HUD、Ammo／Health Pickup、Checkpoint接触、GAME OVER、3秒後のメモリ復帰を実装した。
- Telemetry分類と学習倍率を対象にAutomation Testを2件追加した。

### 作成・変更ファイル

- Project／Build: `AdaptiveHorror.uproject`、`.gitignore`、`Config/Default*.ini`、`Source/*.Target.cs`、`AdaptiveHorror.Build.cs`
- Module: `AdaptiveHorror.h/.cpp`
- Core: `Core/EvaPrototypeGameMode.h/.cpp`
- Player／Components: `Characters/EvaPlayerCharacter.*`、`EvaPlayerController.*`、`Components/EvaHealthComponent.*`、`EvaPlayerTelemetryComponent.*`
- Weapons: `Weapons/EvaWeaponBase.*`、`EvaHitscanWeapon.*`
- AI: `AI/EvaTelemetryTypes.h`、`EvaLearningSubsystem.*`、`EvaZombieCharacter.*`、`EvaZombieAIController.*`、`EvaHunterCharacter.*`
- World／UI／Pickup: `World/EvaCheckpoint.*`、`UI/EvaHUD.*`、`Pickups/EvaPickupBase.*`、`EvaAmmoPickup.*`、`EvaHealthPickup.*`
- Tests: `Tests/EvaLearningTests.cpp`
- Content note: `Content/AdaptiveHorror/Maps/README.md`
- Documents: `README.md`、`TECH_SPEC.md`、`DESIGN_DECISIONS.md`、`TODO.md`、`DEV_LOG.md`、`NEXT_PROMPT.md`

### 動作確認結果

- `.uproject` JSON parse: 成功。
- Source／Build 39ファイル: 波括弧数、反射ヘッダーの `generated.h` 個数／最終include配置を静的確認し、問題なし。
- 必須クラス／API 22項目: 定義存在を機械照合し、すべて検出。
- Handgun、Zombie、HUNTERの必須数値12項目: ソース値を機械照合し、すべて検出。
- Unreal Engine／Epic Launcher manifest: 未検出。
- Visual Studio、MSVC、MSBuild: 未検出。
- UHT／Development Editor build／Automation Test／Editor起動／PIE: 実行環境がないため未実施。
- Git: ユーザー指定どおり操作していない。PATH上にも存在しない。

### 未解決の問題

- 実コンパイルを通していないため、採用するUE minor version固有のAPI差は未確定。
- `L_DevGym.umap` は未作成。Runtime Arenaで代替している。
- Runtime NavMesh Boundsの生成とZombie追跡はPIE実測が必要。
- Runtime Enhanced Input Mappingのキー方向、マウス感度、focus復帰はPIE調整が必要。
- Graybox、C++ HUD、Cube／Sphere敵は仮。アニメーション、音、専用Materialはない。
- Reload中断、Shot ID、Gameplay Team、Behavior Tree、SaveGame永続化は未実装。
- 逃走経路／隠れ場所は記録APIのみで、Stage側のTrigger接続は未実装。

### 次にやるべきこと

最初にUE5＋C++ toolchain上でUHT、Development Editor build、Automation Test、PIE smoke testを実行し、Cycle 001の縦切りを実測する。その後、次の機能群を一つの適応デモ縦切りとして実装する。

1. HUNTERの本格AI
2. 学習倍率による敵適応速度の変化
3. 高速型、装甲型、長腕型
4. 研究施設ステージの簡易進行
5. アダムボスの基盤

## 2026-07-12 - Runtime Navigation recovery pass

### Scope

- Reworked runtime navigation startup order around the generated graybox arena.
- Kept enemy spawning blocked until runtime navigation readiness is confirmed.
- Did not add new gameplay features.

### Changes

- `AEvaPrototypeGameMode::BuildRuntimeNavigation()` now creates runtime floors first, creates a runtime `ANavMeshBoundsVolume` after geometry exists, gives the volume a registered `UBoxComponent` so bounds are non-zero in PIE, notifies navigation bounds, updates runtime floor components/owners in the nav octree, processes queued nav work before `Build()`, and waits for projection success before spawning enemies.
- Spawn candidates now require `NavProjected=true`; floor-trace-only fallback is rejected.
- Floor validation uses floor component tags and expected floor height tolerance instead of actor-level floor tags.
- F8 is no longer used for nav debug because it conflicts with PIE eject; F9/N are used.
- Runtime class binding and enemy visual/debug label data are logged at BeginPlay.
- `AEvaZombieAIController` logs accepted `MoveToActor` requests with synchronous path diagnostics.

### Verification

- Development Editor build without Live Coding: Success.
- Runtime smoke:
  - Recast: `RecastNavMesh-Default`
  - Runtime generation: `Dynamic`
  - Nav bounds location: `V(X=-300.00, Z=360.00)`
  - Nav bounds extent: `V(X=6800.00, Y=1600.00, Z=1100.00)`
  - Player projection: `Projected=true`, `V(X=-5624.00, Z=30.00)`
  - Representative floor projection: `Projected=true`, `V(X=-4800.00, Z=30.00)`
  - Navigation readiness attempt 1: `Ready=true`
  - Initial zombie spawned after navigation ready.
  - Initial zombie path: `MoveToActor accepted`, `PathValid=true`, `PathPoints=3`
- Automation:
  - `Automation RunTests AdaptiveHorror`
  - Found 13 tests.
  - Result: `**** TEST COMPLETE. EXIT CODE: 0 ****`

### Remaining manual PIE checks

- Visual green NavMesh display via Show Navigation in the editor viewport.
- Visual confirmation that one normal zombie walks all the way toward the player over several seconds.
- Visual confirmation of HUD NAV DEBUG display, overhead labels, and enemy size differences.

## 2026-07-12 - Cycle 009: Obstacle pursuit diagnostics and enemy label unification

### Scope

- No new gameplay features were added.
- Focused only on:
  - Obstacle-side pursuit diagnosis/fix.
  - Consistent debug label generation for every enemy type/spawn path.

### Changes

- `AEvaZombieAIController`
  - Added detailed path diagnostics for blocked/stuck pursuit:
    - MoveRequest result.
    - PathFollowing state.
    - active PathValid / IsPartial / PathPoints.
    - CurrentPathPointIndex.
    - diagnostic path valid/partial/point count.
    - player and enemy NavProjection result.
    - left/right detour candidate NavProjection result.
    - enemy-to-player WorldStatic trace result.
    - capsule radius and Nav Agent radius.
  - Added `OnMoveCompleted` logging with ResultCode split into Success / Blocked / OffPath / Aborted / Invalid.
  - Direct fallback is now allowed only when:
    - no valid nav path exists,
    - line trace to the player is clear,
    - the forward movement trace is clear.
  - Direct fallback no longer overwrites valid Path Following detours.
  - Sidestep recovery now accepts only `ProjectPointToNavigation`-successful destinations and moves via `MoveToLocation`.
  - Partial-path recovery now searches candidate detours from the partial path end when available.
  - After a projected sidestep completes, the AI returns to `MoveToActor(Player)`.
  - Repeated `MoveToActor` calls are throttled so pursuit is not overwritten every tick.

- `AEvaZombieCharacter` / `AEvaHunterCharacter` / `AEvaAdamBossCharacter`
  - Added common `EnsurePrototypeDebugLabelInitialized()` path.
  - Label component is always registered, attached to the capsule, visible, not HiddenInGame, and not OwnerNoSee/OnlyOwnerSee.
  - Camera acquisition failure no longer prevents label component creation or leaves labels hidden.
  - Debug label visibility range is now exposed as `DebugLabelMaxVisibleDistance` and defaults to `12000`.
  - Enemy label state logs now include:
    - Actor name.
    - class.
    - EnemyType.
    - DebugLabelComponent presence.
    - LabelText.
    - Visible.
    - HiddenInGame.
    - OwnerNoSee / OnlyOwnerSee.
    - distance hide condition.
    - camera acquisition result.
  - HUNTER and ADAM now emit final label logs after their subclass-specific label text is applied.
  - `ConfigureEvolution()` emits final label logs for evolved/AdaptiveSpawn/Adam-minion paths.

### Build / test verification

- First build attempt:
  - GenerateProjectFiles: Success.
  - Development Editor Build: failed at compile due UE5.8 API const mismatch in `ProjectPointToNavigation` and `FindPathToLocationSynchronously`.
  - Fixed without deleting functionality by aligning local pointers to UE5.8 non-const overload requirements.
- Second build attempt:
  - Compile succeeded.
  - Link failed with unresolved `FPathFollowingResultFlags::ToString`; UE5.8 header symbol was visible but not link-safe for this module usage.
  - Fixed by logging path-following result flags as numeric `0x%04x`.
- Third build attempt:
  - Link failed because `UnrealEditor-AdaptiveHorror.dll` was locked by a running `UnrealEditor.exe`.
  - Closed the editor normally and reran.
- Final build check:
  - `powershell -ExecutionPolicy Bypass -File .\Scripts\RunBuildCheck.ps1 -MaxParallelActions 2`
  - Static source sanity: PASS.
  - GenerateProjectFiles: Success.
  - Development Editor / Win64 build without Live Coding: Success.
  - Automation RunTests AdaptiveHorror: exit code 0.
  - Automation log confirms 15 successful tests.
- Runtime smoke:
  - `UnrealEditor-Cmd.exe AdaptiveHorror.uproject -game -NullRHI -unattended -nop4 -nosplash -ExecCmds="Quit"`
  - Result: exit code 0.

### Runtime smoke observations from logs

- Initial zombie spawn path produced label logs with:
  - `DebugLabelComponent=true`
  - `LabelText=ZOMBIE`
  - `Visible=true`
  - `HiddenInGame=false`
  - `OwnerNoSee=false`
  - `OnlyOwnerSee=false`
  - `DistanceHideCondition=false`
  - `CameraAcquired=true`
- Initial zombie pursuit produced:
  - `MoveToActor accepted`
  - `PathValid=true`
  - `IsPartial=false`
  - `PathPoints=3`
  - `PlayerNavProjection=true`
  - `EnemyNavProjection=true`
  - left/right detour NavProjection both true.
  - enemy-to-player trace blocked false in the smoke setup.
  - `CapsuleRadius=42.0`
  - `NavAgentRadius=42.0`

### Debug key list from code

F2:

- `DebugForceHunterSpawn()` / `AEvaPrototypeGameMode::DebugForceHunterSpawn()`
- Forces HUNTER deployment through the game mode debug path.

F3:

- `DebugForceZombieWave()` / `AEvaPrototypeGameMode::DebugForceZombieWave()`
- Spawns a forced infected wave when the game is in active combat.

F4:

- `DebugWarpPlayerToAdamArena()` / `AEvaPrototypeGameMode::DebugWarpPlayerToAdamArena()`
- Warps player to ADAM arena and starts/sets up ADAM arena flow.

F7/F9/N:

- F7: `DebugPrintTelemetrySnapshot()` prints telemetry snapshot.
- F9 and N: `DebugToggleNavigationVisualization()` toggle navigation debug visualization.
- F9 and N are intentionally duplicated for the same nav debug function.

P:

- No current game-side binding found in `AEvaPlayerCharacter` runtime Enhanced Input setup.

その他:

- F1: EVA analysis +20.
- F5: restore player health/ammo.
- F6: force Stage Clear.
- F8: not bound by game code; intentionally reserved because it conflicts with PIE Eject.

### PIE/manual verification status

- Not visually confirmed in this environment:
  - Obstacle-between-player-and-zombie pursuit behavior in PIE.
  - Mixed-label issue across every live spawn route in a human-visible PIE session.
  - F2/F3/F4/F7/F9/N runtime keypress behavior in the viewport.
- The next PIE pass should specifically place an obstacle with a real NavMesh route around it and confirm that the AI follows Path Following rather than direct fallback.

### Remaining risk

- If there is no actual NavMesh route around a placed obstacle, the AI now intentionally refuses to force through by code. This is correct behavior; map/NavMesh authoring must provide a valid route.
- Runtime graybox still benefits from converting to a saved `.umap` with authored NavMeshBoundsVolume for long-term stability.

## 2026-07-12 - Cycle 010: Stationary target repathing and F4 ADAM debug start

### PIE feedback addressed

- Player stationary behind an obstacle:
  - Zombies could stop completely or keep pressing into a wall.
  - If the player jumped, zombies immediately resumed pathing around the obstacle.
  - NavMesh was visually connected and previous `[AIPath]` logs did not show PathValid/Partial/Blocked errors.
- F4:
  - Player teleported to Adam Arena.
  - ADAM was not visible/confirmed.
  - Arena contained normal or COMPOSITE infected instead.

### Changes

- `AEvaZombieAIController`
  - Added low-frequency stationary-target repath monitoring.
  - Repath reasons now include:
    - `TargetMoved`
    - `NoProgress`
    - `MoveCompleted`
    - `PathInvalid`
    - `SidestepFinished`
    - `PeriodicRefresh`
  - Added `[AIRepath]` log fields:
    - last MoveTo age.
    - last meaningful movement age.
    - recent movement distance.
    - DirectFallback active.
    - Sidestep active.
    - PathFollowing status.
    - PathValid / IsPartial.
    - CurrentMoveRequestID.
    - MoveTo reissue result.
  - If Path Following is accepted but the pawn makes no progress for roughly 1.25 seconds, the current move is aborted and `MoveToActor(Player)` is reissued.
  - Repath monitoring runs around every 0.65 seconds, not every Tick.
  - Direct fallback is explicitly marked inactive when any MoveTo path request is accepted.
  - Sidestep timeout/completion now forces a normal `MoveToActor(Player)` return.

- `AEvaPrototypeGameMode`
  - F4 now acts as a reliable ADAM debug start:
    - Logs `[AdamDebug] F4 pressed`.
    - Moves the player to the ADAM arena safe offset.
    - Removes non-boss/non-HUNTER enemies near the arena to avoid debug-spawn interference.
    - Calls the Director Adam encounter start explicitly.
    - Re-primes the active ADAM target when available.
    - Logs final ADAM count and active ADAM name.
  - Adaptive spawn now skips while ADAM encounter is active, so ordinary adaptive enemies do not obscure ADAM verification.
  - Generic enemy safe spawn no longer applies `ConfigureEvolution()` to actors tagged `Adam` or `Boss`; this prevents ADAM from being visually/label-wise reset into a normal zombie after spawn.

- `AEvaResearchFacilityDirector`
  - `StartAdamEncounter()` now:
    - Searches for an existing living ADAM and avoids duplicate spawn.
    - Does not leave `bAdamEncounterActive=true` if spawning fails.
    - Spawns ADAM only when no living ADAM exists.
    - Ensures controller possession and player target assignment.
    - Emits `[AdamDebug]` logs with:
      - Arena location.
      - Arena state.
      - AdamClass.
      - Existing Adam count.
      - Spawn attempted/result.
      - Final location.
      - NavProjection result.
      - AIController.
      - Possessed.
      - PlayerTarget.
      - Health.
      - Phase.
      - Destroy/failure reason.

### Build / test verification

- First build attempt:
  - Failed due missing `AdaptiveHorror.h` include in `EvaResearchFacilityDirector.cpp` for `LogAdaptiveHorror`.
  - Fixed by adding the include.
- Final build:
  - `powershell -ExecutionPolicy Bypass -File .\Scripts\RunBuildCheck.ps1 -MaxParallelActions 2`
  - Static source sanity: PASS.
  - GenerateProjectFiles: Success.
  - Development Editor / Win64 build without Live Coding: Success.
  - Automation RunTests AdaptiveHorror: exit code 0.
  - Automation log confirms 15 successful tests.
- Runtime smoke:
  - `UnrealEditor-Cmd.exe AdaptiveHorror.uproject -game -NullRHI -unattended -nop4 -nosplash -ExecCmds="Quit"`
  - Result: exit code 0.

### Automation/runtime observations

- `[AdamDebug] Context=StartAdamEncounterSpawned` appeared during automation, confirming that Director Adam encounter start calls the ADAM spawn path.
- Automation world has no real player/NavMesh, so `PlayerTarget=false` and `NavProjected=false` in that specific automation log are expected and not a PIE result.

### PIE/manual verification status

- Not visually confirmed in this environment:
  - Player remains completely still and does not jump while zombie routes around an obstacle.
  - Zombie stops pressing into the wall and reaches melee range after the repath.
  - Multiple zombies preserve pursuit around the obstacle.
  - F4 spawns/keeps a visible ADAM in the arena in PIE.
  - ADAM label visibility.
  - ADAM summoned enemy label visibility.
  - FAST label visibility.
  - ARMORED label visibility.
  - LONG ARM label visibility.

### Notes

- COMPOSITE is the 80% EVA analysis evolved variant.
- If `[AIRepath] Reason=NoProgress` repeatedly fires while NavMesh is visibly connected, inspect whether the obstacle collision is affecting movement but not cutting path data. If no real NavPath detour exists, fix map/NavMesh authoring rather than forcing AI through code.

## 2026-07-12 - Cycle 011: Adam chase crash dump analysis and access-violation fix

### User-reported crash

- Adam body scale/display: confirmed by user.
- Adam pursuit: confirmed by user.
- Crash symptom: after several seconds of Adam chase, PIE freezes and Unreal Editor exits.
- Windows Event Viewer: `UnrealEditor.exe`, fault module `ucrtbase.dll`, exception code `0xC0000005`.
- Full dumps present in `C:\CrashDumps`:
  - `UnrealEditor.exe.29144.dmp`
  - `UnrealEditor.exe_260712_181459.dmp`

### Crash dump / crash artifact analysis

- `cdb.exe` / `windbg.exe` were not installed or discoverable on PATH or under Windows Kits Debuggers in this environment.
- Visual Studio 18 Community was present, but no non-interactive WinDbg command-line analysis path was available.
- Used the UE-generated crash artifacts next to the dump:
  - `Saved/Crashes/UECC-Windows-3B04608E41723DFD16C5B88742084507_0000/CrashContext.runtime-xml`
  - `Saved/Crashes/UECC-Windows-3B04608E41723DFD16C5B88742084507_0000/AdaptiveHorror.log`
- UE crash context reports:
  - Exception: `Unhandled Exception: EXCEPTION_STACK_OVERFLOW`
  - Seconds since start: `68`
  - Game: `UE-AdaptiveHorror`
  - Build configuration: `Development`
- First AdaptiveHorror frame:
  - `AEvaZombieAIController::OnMoveCompleted()`
  - `Source/AdaptiveHorror/AI/EvaZombieAIController.cpp:133`
- Repeating call stack pattern:
  - `AEvaZombieAIController::ReissueMoveToTarget()`
  - `Source/AdaptiveHorror/AI/EvaZombieAIController.cpp:598`
  - `AEvaZombieAIController::OnMoveCompleted()`
  - `Source/AdaptiveHorror/AI/EvaZombieAIController.cpp:170`
  - AIModule frames
  - repeated until stack overflow.
- Crash log corroboration:
  - `MoveCompleted` with `ResultCode=Invalid` was emitted thousands of times in the same frame for the same controller/pawn.
  - This matches synchronous or immediate MoveTo completion re-entering `OnMoveCompleted` and reissuing another MoveTo without unwinding the stack.
- Pointer / access information:
  - No null game pointer was identified in the symbolicated UE call stack.
  - The controller, pawn, and target were valid enough for `MoveCompleted` and `ReissueMoveToTarget` to execute repeatedly.
  - UE crash context classified the failure as stack overflow rather than a direct null read/write/execute access.
  - The Windows `0xC0000005` report appears to be the external process failure classification after the stack overflow.

### Root cause

- `AEvaAdamBossAIController` inherits `AEvaZombieAIController`, so Adam uses the inherited `OnMoveCompleted` path.
- `OnMoveCompleted` reissued `MoveToActor(Player)` for non-success/non-aborted results.
- `ReissueMoveToTarget()` called `MoveToActor()` before updating `LastMoveRequestTime` and without an in-flight request guard.
- When `MoveToActor()` completed synchronously/immediately with `Invalid` or `AlreadyAtGoal`-adjacent path following state, `OnMoveCompleted()` ran again while the previous reissue was still on the stack.
- The cooldown check still saw the old timestamp, so it did not stop the nested reissue.
- Result: `OnMoveCompleted -> ReissueMoveToTarget -> MoveToActor -> OnMoveCompleted` recursion until `EXCEPTION_STACK_OVERFLOW`.

### Fix

- Changed only `AEvaZombieAIController` pursuit reissue safety:
  - Added `bIssuingRepathMove`.
  - `OnMoveCompleted()` now returns without reissuing while a reissued MoveTo is currently being submitted.
  - `ReissueMoveToTarget()` now stamps `LastMoveRequestTime` before calling `MoveToActor()`, so synchronous completion cannot bypass the cooldown.
- No Adam attack, charge, roar, summon, phase, label, HUD, Zombie, or HUNTER gameplay logic was changed.

### Changed files

- `Source/AdaptiveHorror/AI/EvaZombieAIController.h`
- `Source/AdaptiveHorror/AI/EvaZombieAIController.cpp`
- `DEV_LOG.md`
- `TODO.md`
- `BUILD_CHECK.md`
- `NEXT_PROMPT.md`

### Build / test verification

- Command:
  - `powershell -ExecutionPolicy Bypass -File .\Scripts\RunBuildCheck.ps1 -MaxParallelActions 2`
- Results:
  - Static source sanity: PASS.
  - Generate Project Files: Succeeded.
  - Development Editor / Win64 build without Live Coding: Succeeded.
  - Automation RunTests AdaptiveHorror: exit code 0.
  - Automation log confirms 15 successful tests and 0 failures.
- Runtime smoke:
  - `UnrealEditor-Cmd.exe AdaptiveHorror.uproject -game -NullRHI -unattended -nop4 -nosplash -ExecCmds="Quit"`
  - Result: exit code 0.

### PIE/manual verification status

- Not visually confirmed in this environment:
  - Adam chase for several seconds/minutes without editor crash.
  - Adam charge/roar/phase behavior after the stack-overflow fix.
  - Repeated F4 Adam debug start after this fix.
- Next PIE pass should reproduce the previous Adam chase scenario and confirm no rapid `MoveCompleted -> ReissueMoveToTarget` recursion appears in logs.

## 2026-07-12 - Cycle 012: Adam debug readability and HP display split

### Goal

- No new gameplay feature work.
- Improve Adam encounter debugging so the current boss state is visible at a glance.
- Split ordinary enemy overhead HP display from ADAM boss HP display.

### Changes

- Added `UEvaBossHUDWidget`
  - Independent C++ UMG widget intended as a future replacement point for formal UI.
  - Draws a top-center Boss HUD only while the ADAM encounter is active.
  - Normal display:
    - `ADAM`
    - Boss HP Bar
    - Phase
  - Debug display, enabled by `DebugBoss=true` on ADAM:
    - CurrentHP / MaxHP
    - Remaining HP
    - Current State
    - Target Distance
    - Last Event
    - Summon Count

- Updated `AEvaHUD`
  - Creates and owns the Boss HUD widget separately from the existing Canvas debug HUD.
  - Updates the Boss HUD from `AEvaResearchFacilityDirector::GetActiveAdam()`.
  - Hides the Boss HUD when ADAM is defeated, Stage Clear is reached, or the encounter is inactive.

- Updated `AEvaAdamBossCharacter`
  - Added `DebugBoss` (`bDebugBoss`) and `DebugBossHP`.
  - With `DebugBoss=true`, ADAM max HP is now `500` for short defeat-to-StageClear verification.
  - Normal ADAM HP remains `2500` when `DebugBoss=false`.
  - ADAM overhead label remains `ADAM` only.
  - ADAM overhead HP bar and numeric overhead HP are disabled.
  - Phase 2 no longer changes the overhead label to `ADAM P2`; Phase is shown in Boss HUD instead.
  - Tracks last summon count and total summon count for Boss HUD debug output.

- Updated `AEvaAdamBossAIController`
  - Tracks ADAM debug state:
    - `Acquiring Target`
    - `Chasing`
    - `Attack`
    - `Charge`
    - `Roar / Summon`
    - `No Valid Target`
  - Records recent boss events:
    - `Attack started`
    - `Charge started`
    - `Roar started / Summon started`
  - Tracks current target distance.
  - Tracks last summon count.

- Updated `AEvaZombieCharacter`
  - Ordinary enemies now show overhead:
    - enemy name
    - HP bar
  - Applies to:
    - Zombie
    - FAST
    - ARMORED
    - LONG ARM
    - COMPOSITE
    - HUNTER
  - Ordinary enemy overhead display does not show numeric HP in normal play.
  - Debug numeric HP can be enabled per actor with `bDebugHealthNumbersVisible` or globally with:
    - `Eva.DebugEnemyHealthNumbers 1`
  - When enabled, it shows `CurrentHP / MaxHP` under the overhead HP bar.

### Changed files

- `Source/AdaptiveHorror/UI/EvaBossHUDWidget.h`
- `Source/AdaptiveHorror/UI/EvaBossHUDWidget.cpp`
- `Source/AdaptiveHorror/UI/EvaHUD.h`
- `Source/AdaptiveHorror/UI/EvaHUD.cpp`
- `Source/AdaptiveHorror/AI/EvaAdamBossAIController.h`
- `Source/AdaptiveHorror/AI/EvaAdamBossAIController.cpp`
- `Source/AdaptiveHorror/AI/EvaAdamBossCharacter.h`
- `Source/AdaptiveHorror/AI/EvaAdamBossCharacter.cpp`
- `Source/AdaptiveHorror/AI/EvaZombieCharacter.h`
- `Source/AdaptiveHorror/AI/EvaZombieCharacter.cpp`
- `DEV_LOG.md`

### Build / test verification

- Command:
  - `powershell -ExecutionPolicy Bypass -File .\Scripts\RunBuildCheck.ps1 -MaxParallelActions 2`
- Results:
  - Static source sanity: PASS.
  - Generate Project Files: Succeeded.
  - Development Editor / Win64 build without Live Coding: Succeeded.
  - Automation RunTests AdaptiveHorror: exit code 0.
  - Automation log confirms 15 successful tests and 0 failures.
- Runtime smoke:
  - `UnrealEditor-Cmd.exe AdaptiveHorror.uproject -game -NullRHI -unattended -nop4 -nosplash -ExecCmds="Quit"`
  - Result: exit code 0.
- `git diff --check`:
  - No whitespace errors. CRLF conversion warnings only.

### PIE/manual verification status

- Not visually confirmed in this environment:
  - Boss HUD appears at top center only during ADAM encounter.
  - ADAM overhead label shows only `ADAM`.
  - ADAM has no overhead HP bar.
  - Boss HP bar, Phase, Current State, Target Distance, Summon Count, and HP debug values update during live ADAM combat.
  - Ordinary enemies show name + HP bar, and no numeric HP unless debug numeric HP is enabled.

## 2026-07-12 - Cycle 013: Stage Clear / Player Death conflict fix

### Goal

- No new content or feature expansion.
- Fix the post-Adam Stage Clear regression where remaining enemies kept damaging the player, eventually triggering Player Death and disabling movement/look/input after the clear result.
- Make Stage Clear a stable terminal combat state while preserving Player Death priority if the player dies first.

### Cause

- `AEvaPrototypeGameMode::HandleStageClear()` marked `bStageClear` and cleared a few timers, but existing enemies were still alive, ticking, moving, and able to run attack damage.
- `AEvaPlayerCharacter::TakeDamage()` and `HandleDeath()` did not reject damage/death transitions after Stage Clear.
- Director/GameMode state could diverge if Adam defeat and Player Death happened in a tight race.

### Changes

- Updated `AEvaPrototypeGameMode`
  - Stage Clear now clears combat timers:
    - respawn
    - initial/wave spawn
    - adaptive spawn
    - HUNTER timed spawn
    - HUNTER reinsertion
  - Stage Clear now stops all existing enemy combat for:
    - normal zombies
    - evolved zombies
    - HUNTER
    - ADAM / boss-tagged enemies
    - ADAM roar minions
  - Enemy stop behavior:
    - clears target/focus
    - stops MoveTo/path following
    - disables AI tick
    - stops/disable character movement
    - hides overhead labels/HP bars
  - Stage Clear disables player movement input but resets look ignore state so camera look can remain available.
  - Stage Clear does not pause the game and does not set global time dilation to zero.
  - Post-clear spawn requests are skipped and logged.
  - Post-clear enemy kill notifications are ignored.
  - Respawn timer is cleared and respawn is rejected if Stage Clear became active.
  - Added `[StageClear]` logs for:
    - Begin / AlreadyCleared / player-death-first rejection
    - PlayerHP / PlayerDead
    - ActiveZombies / ActiveHunters / ActiveAdam
    - ClearedEnemyAI / ClearedTimers
    - PlayerDamageDisabled
    - PlayerMoveInputDisabled / PlayerLookInputDisabled
    - IsPaused / GlobalTimeDilation
    - IgnoreMoveInput / IgnoreLookInput
    - combat timer activity
  - Added `[PlayerDeath]` logs for:
    - DeathRequest
    - RejectedBecauseStageClear
    - HP / StageClearState / GameOverState
    - RespawnTimerCreated / RespawnTimerActive
    - input ignore state

- Updated `AEvaPlayerCharacter`
  - Player damage is ignored after Stage Clear.
  - Player death handling is rejected after Stage Clear.
  - Movement, sprint, jump, fire, and reload are blocked after Stage Clear.
  - Look input remains allowed after Stage Clear unless the player is already dead.

- Updated `AEvaZombieAIController`
  - Added `StopCombatForStageClear()`.
  - Adds a combat-enabled guard to target acquisition, perception, attack, MoveTo, sidestep/direct fallback, Tick, and MoveCompleted recovery.

- Updated `AEvaZombieCharacter`
  - Added `SetOverheadDisplayEnabled()` for safe Stage Clear hiding of labels and HP bars.
  - Overhead labels/HP bars now stay hidden after being disabled instead of being recreated by camera-facing updates.

- Updated `AEvaResearchFacilityDirector`
  - If Player Death is already active, Adam defeat/CompleteStage is rejected so Director and GameMode do not diverge.

- Updated Automation tests
  - Added Stage Clear regression coverage:
    - `AdaptiveHorror.StageClear.RejectsPlayerDeath`
    - `AdaptiveHorror.StageClear.SkipsSpawns`
    - `AdaptiveHorror.StageClear.StopsEnemyCombat`
    - `AdaptiveHorror.StageClear.Idempotent`

### Changed files

- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.h`
- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.cpp`
- `Source/AdaptiveHorror/Characters/EvaPlayerCharacter.h`
- `Source/AdaptiveHorror/Characters/EvaPlayerCharacter.cpp`
- `Source/AdaptiveHorror/AI/EvaZombieAIController.h`
- `Source/AdaptiveHorror/AI/EvaZombieAIController.cpp`
- `Source/AdaptiveHorror/AI/EvaZombieCharacter.h`
- `Source/AdaptiveHorror/AI/EvaZombieCharacter.cpp`
- `Source/AdaptiveHorror/World/EvaResearchFacilityDirector.cpp`
- `Source/AdaptiveHorror/Tests/EvaLearningTests.cpp`
- `DEV_LOG.md`
- `TODO.md`
- `BUILD_CHECK.md`
- `NEXT_PROMPT.md`

### Build / test verification

- Command:
  - `powershell -ExecutionPolicy Bypass -File .\Scripts\RunBuildCheck.ps1 -MaxParallelActions 2`
- Results:
  - Static source sanity: PASS.
  - Generate Project Files: Succeeded.
  - First build attempt found one test include issue:
    - `AEvaPlayerCharacter` was spawned in `EvaLearningTests.cpp` with only a forward declaration available.
    - Fixed by including `Characters/EvaPlayerCharacter.h`.
  - Development Editor / Win64 build without Live Coding: Succeeded.
  - Automation RunTests `AdaptiveHorror`: exit code 0.
  - Latest automation log confirmed 19 successful tests and 0 failures.
- Runtime smoke:
  - First attempt using relative `.\\AdaptiveHorror.uproject` failed to locate the descriptor.
  - Re-ran with absolute project path:
    - `UnrealEditor-Cmd.exe C:\Users\shinn\Documents\Codex\2026-06-23\unreal-engine-5-fps-30-60\AdaptiveHorror.uproject -game -Unattended -NullRHI -NoSound -NoSplash -ExecCmds="Quit" -log`
  - Result: exit code 0.
  - Runtime log confirms `/Engine/Maps/Entry` started, runtime navigation built, initial zombie spawned, then exited by `Quit`.
- `git diff --check`:
  - No whitespace errors. CRLF conversion warnings only.

### PIE/manual verification status

- Not visually confirmed by Codex:
  - Adam defeat followed by Stage Clear no longer allows remaining enemies to reduce player HP.
  - Residual enemies stop chasing/attacking after Stage Clear.
  - Player Death no longer starts after Stage Clear.
  - Boss HUD hides after Stage Clear in live PIE.
  - Enemy overhead labels/HP bars hide after Stage Clear in live PIE.

### Next manual PIE pass

1. Press F4, defeat ADAM with `DebugBoss=true`.
2. Confirm Stage Clear appears.
3. Stand still for 60 seconds after clear.
4. Confirm player HP no longer decreases.
5. Confirm residual enemies no longer chase/attack.
6. Confirm camera look still works while movement/shooting remain disabled.
7. Confirm no GAME OVER / checkpoint respawn starts after Stage Clear.

## 2026-07-14 - Cycle 014 Core game UI flow

### Implementation summary

- Added the first playable front-end flow for the prototype:
  - Launch now enters `Title` flow instead of immediately starting combat.
  - `NEW GAME` resets combat/session state and enters `Playing`.
  - `Esc` toggles `Paused` / `Playing`.
  - Player death now opens an explicit `GAME OVER` menu instead of auto-respawning after a timer.
  - Adam defeat / Stage Clear opens a `MISSION COMPLETE` menu.
  - Menus support retry/restart, return to title, and exit.
- Added `EEvaGameFlowState` / `EEvaSettingsReturnTarget` for minimal state management.
- Added C++ UMG menu widgets:
  - `UEvaTitleMenuWidget`
  - `UEvaPauseMenuWidget`
  - `UEvaGameOverWidget`
  - `UEvaStageClearWidget`
  - `UEvaSettingsWidget`
- Added local settings save object:
  - Master/BGM/SFX volume values.
  - Mouse sensitivity.
  - Invert Mouse Y.
  - Placeholder Fullscreen / Resolution / Graphics Quality fields for the future settings UI.
- Added simple procedural UI tones for click, pause/menu open, game over, and stage clear events.
- Split normal HUD and debug HUD:
  - Normal HUD keeps HP, ammo, EVA analysis/stage, HUNTER state, objective, and crosshair.
  - Debug stats are shown only when Debug HUD is toggled with `F9/N`.
  - Title/loading states hide the gameplay HUD.
  - Old canvas `GAME OVER` / `STAGE CLEAR TODO` overlays were removed in favor of widgets.
- Preserved the existing Stage Clear safety fixes:
  - Stage Clear still blocks player damage/death, stops enemy combat, clears spawn timers, and hides boss/enemy combat UI.
- Added flow reset helpers:
  - `UEvaLearningSubsystem::ResetLearning()`
  - `UEvaPlayerTelemetryComponent::ResetTelemetry()`
  - `AEvaResearchFacilityDirector::ResetForNewGame()`
- Set the runtime-generated point light to Movable to reduce runtime graybox lighting warnings.
- Added automation coverage for title blocking combat, new-game reset, retry flow, and settings defaults.

### Changed files

- `Source/AdaptiveHorror/Core/EvaGameFlowTypes.h`
- `Source/AdaptiveHorror/Core/EvaSettingsSaveGame.h`
- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.h`
- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.cpp`
- `Source/AdaptiveHorror/Characters/EvaPlayerController.h`
- `Source/AdaptiveHorror/Characters/EvaPlayerController.cpp`
- `Source/AdaptiveHorror/Characters/EvaPlayerCharacter.h`
- `Source/AdaptiveHorror/Characters/EvaPlayerCharacter.cpp`
- `Source/AdaptiveHorror/Components/EvaPlayerTelemetryComponent.h`
- `Source/AdaptiveHorror/Components/EvaPlayerTelemetryComponent.cpp`
- `Source/AdaptiveHorror/AI/EvaLearningSubsystem.h`
- `Source/AdaptiveHorror/AI/EvaLearningSubsystem.cpp`
- `Source/AdaptiveHorror/UI/EvaMenuWidgets.h`
- `Source/AdaptiveHorror/UI/EvaMenuWidgets.cpp`
- `Source/AdaptiveHorror/UI/EvaHUD.cpp`
- `Source/AdaptiveHorror/World/EvaResearchFacilityDirector.h`
- `Source/AdaptiveHorror/World/EvaResearchFacilityDirector.cpp`
- `Source/AdaptiveHorror/Tests/EvaLearningTests.cpp`
- `DEV_LOG.md`
- `TODO.md`
- `NEXT_PROMPT.md`
- `BUILD_CHECK.md`
- `README.md`
- `TECH_SPEC.md`

### Build / test verification

- Initial git state:
  - Started from `main`, clean working tree.
  - Created branch `feature/core-game-ui-flow`.
- `Scripts/RunBuildCheck.ps1`
  - Static source sanity: PASS.
  - Generate Project Files: Succeeded.
  - Development Editor / Win64 build without Live Coding: Succeeded.
  - Automation RunTests `AdaptiveHorror`: Succeeded.
  - Latest automation log confirms 21 successful tests and exit code 0.
- First build pass found UE5.8/MSVC issues in the new UI code and they were fixed:
  - `Slot` local variable shadowing `UWidget::Slot`.
  - Ambiguous `TSubclassOf` ternary expressions.
  - `TObjectPtr<USlider>` reference mismatch.
  - `UGameplayStatics::QuitGame` replaced with `UKismetSystemLibrary::QuitGame`.
  - `bHunterEncounterTriggered` typo corrected to existing `bHunterEventTriggered`.
  - Retry flow no longer overwrites the pending respawn pawn with `nullptr` in controller-less automation worlds.
- Runtime smoke:
  - Command:
    - `UnrealEditor-Cmd.exe AdaptiveHorror.uproject -game -Unattended -NullRHI -NoSound -NoSplash -ExecCmds="Quit" -log`
  - Result: exit code 0.
  - Log confirms `EvaPrototypeGameMode` loaded and transitioned `Loading -> Title`.
- `git diff --check`:
  - No whitespace errors. CRLF conversion warnings only.

### PIE/manual verification status

- Not visually confirmed by Codex in PIE:
  - Title screen appears and buttons are clickable.
  - New Game starts gameplay from title.
  - Esc pause/resume works without duplicate widgets.
  - Settings sliders/check box are visible and apply correctly.
  - Player death opens Game Over and Retry works.
  - Adam defeat opens the new Stage Clear widget.
  - Return to Title works after Pause / Game Over / Stage Clear.
  - UI tones are audible in PIE.
  - Lighting rebuild warning is fully gone in viewport.

### Known limitations

- Settings UI stores Fullscreen / Resolution / Graphics Quality fields but does not yet apply renderer/window changes.
- Audio is procedural placeholder UI sound only; gameplay/enemy/BGM audio remains TODO.
- Title mode still uses the runtime map/pawn as a dark background, but gameplay input and combat spawning are locked until `NEW GAME`.
- Runtime smoke uses NullRHI, so it cannot prove visual menu layout or audible UI tones.

### Next recommended work

Run a real PIE pass focused only on UI flow:

1. Launch PIE and confirm title screen.
2. `NEW GAME` -> verify movement, shooting, HUD, and initial zombie.
3. Press `Esc` -> Resume / Settings / Return to Title.
4. Kill the player -> Game Over -> Retry.
5. Press F4, defeat Adam -> Stage Clear -> Return to Title.
6. Start a second New Game and confirm no old Stage Clear / Adam / HUNTER state remains.

## 2026-07-14 - Cycle 015 Title widget display / PIE input flow fix

### Problem

User PIE verification showed:

- Gameplay field appeared.
- Mouse cursor appeared.
- No title text/buttons were visible.
- Esc, mouse look, WASD, and shooting did not work.
- Log only showed `Loading -> Title`; there were no widget/input diagnostics.

Most likely state was:

`Loading -> Title -> menu input lock -> title widget not rendered -> NEW GAME unreachable`.

### Root cause

The C++ menu widgets built their `WidgetTree->RootWidget` inside `NativeConstruct()`. For C++-only `UUserWidget` classes this can happen after the Slate widget has already been rebuilt, causing the viewport to contain an empty/null widget even though the flow state and mouse cursor look correct.

### Fix

- Moved native menu tree creation into `UEvaMenuWidgetBase::RebuildWidget()`.
- Added `UEvaMenuWidgetBase` debug helpers:
  - `HasNativeMenuRoot()`
  - `WasNativeConstructCalled()`
  - `AssignInitialFocus()`
- Assigned initial focus to the primary button on Title / Pause / Game Over / Stage Clear menus.
- Changed Title and Settings input mode to block gameplay input explicitly.
- Added detailed logs:
  - `[GameFlow]` now includes previous/current state, world name, net mode, GameMode class, PlayerController validity/class, LocalController, LocalPlayer, Pawn, PossessedPawn, and unchanged state.
  - `[TitleUI]` logs widget class, class validity, CreateWidget attempt/result, widget name, root validity, AddToViewport attempt, IsInViewport, visibility, render opacity, z-order, NativeConstruct, focus, LocalPlayer, viewport client, and failure reason.
  - `[InputState]` logs mode, cursor, ignore move/look, pause, time dilation, controller, pawn, local controller, and LocalPlayer.
  - `[Player]` logs possessed pawn and movement status after New Game.
- Added Development fallback:
  - if a local PlayerController exists and Title UI still fails to become visible, log an error and fall back to `StartNewGameFlow()` instead of trapping PIE in an unusable Title state.
  - Controller-less automation worlds remain in Title and do not trigger the fallback.

### Changed files

- `Source/AdaptiveHorror/UI/EvaMenuWidgets.h`
- `Source/AdaptiveHorror/UI/EvaMenuWidgets.cpp`
- `Source/AdaptiveHorror/Characters/EvaPlayerController.h`
- `Source/AdaptiveHorror/Characters/EvaPlayerController.cpp`
- `Source/AdaptiveHorror/Core/EvaPrototypeGameMode.cpp`
- `DEV_LOG.md`
- `TODO.md`
- `NEXT_PROMPT.md`
- `BUILD_CHECK.md`

### Build / test verification

- Command:
  - `powershell -ExecutionPolicy Bypass -File .\Scripts\RunBuildCheck.ps1`
- Results:
  - Static source sanity: PASS.
  - Generate Project Files: Succeeded.
  - Development Editor / Win64 build without Live Coding: Succeeded.
  - Automation RunTests `AdaptiveHorror`: 21 tests succeeded, 0 failures.
- Runtime smoke:
  - `UnrealEditor-Cmd.exe AdaptiveHorror.uproject -game -Unattended -NullRHI -NoSound -NoSplash -ExecCmds="Quit" -log`
  - Result: exit code 0.
  - Log confirms:
    - `CurrentState=EEvaGameFlowState::Title`
    - `TitleWidgetClassValid=true`
    - `CreateWidgetResult=true`
    - `RootWidgetValid=true`
    - `AddToViewportAttempted=true`
    - `IsInViewport=true`
    - `Visibility=ESlateVisibility::Visible`
    - `NativeConstructCalled=true`
    - `FocusAssigned=true`
    - `InputMode=GameAndUI`
    - `ShowMouseCursor=true`
    - `IgnoreMoveInput=true`
    - `IgnoreLookInput=true`
- `git diff --check`:
  - passed with CRLF conversion warnings only.

### PIE/manual verification status

- Not visually confirmed by Codex:
  - Title text and buttons are visible in PIE viewport.
  - NEW GAME can be clicked and transitions to Playing.
  - After NEW GAME, cursor hides and movement/look/shooting return.
  - Esc works after Playing starts.

### Next manual PIE pass

1. Start PIE.
2. Confirm `[TitleUI] ... IsInViewport=true ... FocusAssigned=true ... FailureReason=None`.
3. Confirm title text/buttons are visible.
4. Click `NEW GAME`.
5. Confirm `[GameFlow] ... Title ... Playing`.
6. Confirm `[InputState] ... InputMode=GameOnly ShowMouseCursor=false IgnoreMoveInput=false IgnoreLookInput=false`.
7. Confirm `[Player] ... PossessedPawn=EvaPlayerCharacter`.
8. Verify movement, look, shooting, and Esc Pause.
