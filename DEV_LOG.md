# Development Log

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