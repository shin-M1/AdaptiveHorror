# Next Codex Prompt

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
