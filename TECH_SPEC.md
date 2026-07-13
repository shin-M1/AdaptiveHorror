# Technical Specification — Adaptive Horror FPS Demo

## Cycle 009 実装仕様メモ

### AI障害物回避リカバリ

- 既存のAIController Tick制御を維持し、Behavior Tree全面移行は行わない。
- `MoveTo` accepted後でも実移動が止まる場合があるため、一定時間進捗がない場合は `TrySidestepAroundObstacle()` を実行する。
- Direct fallbackの前方traceが壁を検出した場合、前進を押し付けず左右のsidestep候補を交互に試す。
- fallback移動が発生した場合は `AEvaPrototypeGameMode::NotifyFallbackMovementUsed()` でHUD/Debug counterに反映する。

### 敵Visual識別

- Runtime Graybox用の仮モデルは正式アセットではなく、StaticMeshのBody/Head/Armパーツで敵タイプを識別する。
- `AEvaZombieCharacter` は `BodyVisual` / `HeadVisual` / `LeftArmVisual` / `RightArmVisual` / `TypeLabel` を持つ。
- LongArmやCompositeでもActorScaleは変更しない。Capsule CollisionとNavigation Agentを壊さないため、見た目差は相対パーツのみで表現する。
- HUNTER / ADAM / ADAM Phase2は派生クラス側でBody/Head/Armサイズとラベルを上書きする。

### 頭上ラベル

- `UTextRenderComponent` の固定Yaw 180度は使わない。
- Tickでプレイヤーカメラ方向へのYaw-only Billboardを行い、左右反転を避ける。
- 死亡時は非表示。遠距離では邪魔にならないよう非表示。

### Automation

- `Scripts/RunBuildCheck.ps1` は `Automation RunTests AdaptiveHorror` を実行する。
- 2026-07-12時点のテスト数は15件。

## Cycle 007 実装仕様メモ

### Runtime Graybox Navigation

- Runtime生成の研究施設Grayboxでは、UE5.8で `ANavMeshBoundsVolume` をSpawnしてスケールする方式を使わない。
- 理由は、Runtime Spawnされた `ANavMeshBoundsVolume` がBrush形状を持たず、Navigation更新時にempty boundsとして扱われる場合があるため。
- `AEvaPrototypeGameMode::BuildPrototypeArena()` は床・壁・遮蔽物を生成した後、`UNavigationSystemV1::bWholeWorldNavigable = true` を有効化し、`Build()` を呼び出す。
- Runtime生成したStaticMeshComponentは、位置/スケール/Collision/Navigation関連設定を終えてからStatic Mobilityへ戻す。
- 正式な `.umap` 化後は、Editor上で保存された `NavMeshBoundsVolume` を使う方針に戻してよい。

### AI移動フォールバック

- `AEvaZombieAIController` は `MoveToActorOrDirect()` / `MoveToLocationOrDirect()` を持つ。
- `MoveToActor()` / `MoveToLocation()` が成功または進行中なら通常のPathFollowingを使う。
- `EPathFollowingRequestResult::Failed` の場合のみ、Controlled Pawnへ2D方向の `AddMovementInput()` を行い、NavMesh未構築時でも最低限の追跡を継続する。
- `AEvaHunterAIController` と `AEvaAdamBossAIController` はこのフォールバックを継承利用する。
- UE5.8では `EPathFollowingRequestResult::Failed` の完全定義参照に `Navigation/PathFollowingComponent.h` が必要。

### GameOver / StageClear / Debug復帰のTimer安全性

- Player死亡時は `RespawnTimer` をClearしてから再登録する。
- Stage Clear時は `RespawnTimer`、敵Spawn系Timer、HUNTER再投入Timerを解除し、Enemy Targetをリセットする。
- Debug F5復帰時は `PlayerAwaitingRespawn` と `RespawnTimer` をクリアし、死亡復帰Timerとの競合を避ける。
- `bGameOver` または `bStageClear` 中は、Debug Zombie WaveやAdaptive Spawnが新規Spawnを行わない。

### Cycle 007 Verification

- `Scripts/RunBuildCheck.ps1 -MaxParallelActions 2`
  - Generate Project Files: Succeeded
  - Development Editor / Win64 Build: Succeeded
  - `AdaptiveHorror.EVA` Automation Test 8件: Success
- 短時間Standalone起動:
  - `/Engine/Maps/Entry` が `AEvaPrototypeGameMode` で起動。
  - Fatal/Crashなし。
  - Runtime Navigation Build開始ログを確認。
- PIEのフル手動検証はEditor viewport操作が必要なため、未確認項目として `DEV_LOG.md` と `TODO.md` に残す。

## Cycle 005 実装仕様メモ

### UE5.8 Build Settings

- `AdaptiveHorrorEditor.Target.cs` と `AdaptiveHorror.Target.cs` は `DefaultBuildSettings = BuildSettingsVersion.V7` を使用する。
- `IncludeOrderVersion = EngineIncludeOrderVersion.Latest` を維持する。
- UE5.8/V7では旧warning level差分をEditor共有ビルド環境に持ち込むと拒否されるため、旧BuildSettingsへ戻さない。

### Module Include Path

- 現在のソース構成は `Source/AdaptiveHorror/AI`、`Characters`、`Core` などをモジュール直下に持ち、`#include "AI/..."` 形式を使う。
- UE5.8でこの構成を維持するため、`AdaptiveHorror.Build.cs` で `ModuleDirectory` を `PublicIncludePaths` に追加している。
- 将来的に `Public/Private` 構成へ整理する場合は、このinclude path依存を段階的に解消する。

### UE5.8 API / Warning Fix

- `APointLight::GetPointLightComponent()` は使わず、`GetLightComponent()` を `UPointLightComponent` へCastして使う。
- V7ではshadowingがエラー化されるため、ローカル変数名は親クラス/メンバー名と衝突しない名前にする。
- `RunBuildCheck.ps1` は `-MaxParallelActions` を受け取り、UBTへ渡す。現在の推奨値は4。

### Verification

- `powershell -ExecutionPolicy Bypass -File .\Scripts\RunBuildCheck.ps1 -MaxParallelActions 4`
  - Generate Project Files: Succeeded。
  - Development Editor Build / Win64: Succeeded。
  - `AdaptiveHorror.EVA` Automation Test 8件: Success。
- PIEはEditor上で手動確認が必要。

## Cycle 004 実装仕様メモ

### Build Check更新

- `Scripts/RunBuildCheck.ps1` はUE未検出時でも静的sanity checkを実行する。
  - `.generated.h` はUCLASS/USTRUCT/UENUMを持つヘッダで1つだけ存在し、最後のincludeであること。
  - `AdaptiveHorror.uproject` はJSONとしてparseできること。
- UE実環境が検出された場合は以下を順に実行する。
  - Generate Project Files。
  - `AdaptiveHorrorEditor Win64 Development` build。
  - `AdaptiveHorror.EVA` Automation Test。
  - PIE Smoke TestはEditor上の手動確認として提示。

### Debug Hotkeys

Debug入力は `AEvaPlayerCharacter` のRuntime Enhanced Input Mappingで作成し、実処理は `AEvaPrototypeGameMode` に集約する。

Shippingでは `#if !UE_BUILD_SHIPPING` により入力マッピングと実処理を無効化する。

| キー | GameMode処理 |
|---|---|
| F1 | `DebugIncreaseEvaAnalysis(20.0f)` |
| F2 | `DebugForceHunterSpawn()` |
| F3 | `DebugForceZombieWave()` |
| F4 | `DebugWarpPlayerToAdamArena()` |
| F5 | `DebugRestorePlayer()` |
| F6 | `DebugForceStageClear()` |
| F7 | `DebugPrintTelemetrySnapshot()` |

### HUD / Debug Messaging

- `AEvaPrototypeGameMode::ShowDebugStatusMessage()` が短時間表示用の最新Debugメッセージを保持する。
- `AEvaHUD` はGameModeのDebugメッセージをHUD上部に表示する。
- 非Shippingでは `GEngine->AddOnScreenDebugMessage` も併用する。
- 通知対象:
  - 射撃ヒット。
  - ゾンビ死亡。
  - EVA解析率10%刻み上昇。
  - HUNTER出現/撃破。
  - Stage Objective更新。
  - ADAM Phase 2。
  - Stage Clear。

### Crash Prevention

- Player:
  - Health / Movement / StimuliSource / InputAction / MappingContextのnull保護。
  - 死亡後の被ダメージ・射撃入力を抑止。
  - Checkpoint復帰時に移動、Collision、Health、Weaponを復帰。
- Weapon:
  - EndPlay / Respawn時にReload Timerを解除。
  - Owner Player死亡時は発射しない。
- AI:
  - `AEvaZombieAIController::ClearPlayerTarget()` でTargetActor / Focus / Moveを解除。
  - Player死亡時やCheckpoint復帰時に攻撃継続を止める。
  - Zombie死亡処理は `bDefeatHandled` で二重実行を防止。
- HUD:
  - Player / Health / Telemetry / GameMode / Directorがないフレームでもreturnする。
- Boss/HUNTER:
  - Health / Visual / Movement / SpawnActor失敗を保護。
  - ADAM撃破時、Directorが見つからない場合はGameModeへStage Clearをフォールバックする。

## Cycle 003 実装仕様メモ

### Build Check

- `BUILD_CHECK.md` にWindows UE5実環境での検証手順を追加。
- `Scripts/RunBuildCheck.ps1` で以下を探索/実行する。
  - `.uproject`
  - `UnrealEditor.exe`
  - `UnrealBuildTool.exe`
  - `MSBuild.exe`
  - `cl.exe`
  - Generate Project Files
  - Development Editor Build
  - Automation Test実行コマンド提示

### Research Facility Graybox

Runtime Grayboxは6区画構成。

1. `EntryLobby`
2. `SecurityCorridor`
3. `ObservationLab`
4. `ContainmentWard`
5. `DataCoreRoom`
6. `AdamArena`
7. `Clear`

進行管理は `AEvaResearchFacilityDirector`。

管理対象:

- 現在区画
- 区画ごとのスポーン
- EVAログ取得状態
- HUNTER出現イベント
- 進化個体解禁
- Adam戦開始
- Stage Clear

区画トリガーは `AEvaFacilityZoneTrigger`。Player overlapでDirectorへ `NotifyZoneEntered()` を送る。

### EVA Story Logs

`AEvaStoryLogPickup` を追加。

実装ログ:

1. EVA幸福最大化プロトコル
2. 人類最大リスク判定
3. 生体適応実験記録
4. HUNTER観測ユニット説明
5. ADAM起動記録

取得状態はDirectorが保持し、HUDに数秒表示する。

### ADAM Boss

`AEvaAdamBossCharacter` と `AEvaAdamBossAIController` を追加。

基準値:

- HP: 2500
- 移動速度: 通常ゾンビ基準の0.85倍
- 近接攻撃力: 25
- 攻撃間隔: 2.0秒
- 突進攻撃: 35
- 突進クールダウン: 8秒
- 咆哮: 周囲ゾンビ召喚
- HP50%以下でPhase 2

Phase 2:

- 移動速度 +20%
- 攻撃間隔 -20%
- 進化個体を1体召喚
- EVA解析率60%以上、またはPhase 2中は咆哮召喚が進化個体寄りになる

撃破時:

- Directorへ通知
- GameModeのStage Clearフラグを立てる
- HUDで戦闘結果を表示

## Cycle 002 実装仕様メモ

- EVA解析率は `UEvaLearningSubsystem::ObservationMass` を0〜100%にClampして扱う。
- 通常イベントの観測精度は初期値0.35、HUNTER観測は1.0。
- `EffectiveWeight = BaseAmount * ObserverAccuracy * LearningSpeedMultiplier`。
- HUNTER生存/再投入中は `LearningSpeedMultiplier = 1.0`。
- HUNTER撃破中は `LearningSpeedMultiplier = 0.3`。
- 解析段階:
  - `Learning`: 20%未満。
  - `Adapting`: 20%以上60%未満。
  - `Evolving`: 60%以上。
- 適応方針:
  - Berserker → `CounterCloseRange`: 距離を取る。
  - Ranger → `CounterLongRange`: 遮蔽物利用。
  - Ghost → `CounterStealth`: 視覚/聴覚範囲増加。
  - Explorer → `CounterExplorer`: 待ち伏せポイント利用。
- 進化スポーン閾値:
  - 20%: `Fast`。移動速度+25%。
  - 40%: `Armored`。最大HP+30%、頭部被ダメージ0.5倍。
  - 60%: `LongArm`。攻撃距離+250cm、攻撃力+15%。
  - 80%: `Composite`。高速/装甲/長腕を同時適用。
- HUNTER投入:
  - 通常ゾンビ撃破3体、または45秒経過。
  - 撃破後30秒でTier+1を再投入。
- Runtime Arenaの仮タグ:
  - `EvaCover`
  - `EvaHideSpot`
  - `EvaEscapeRoute`
  - `EvaAmbushPoint`

## 1. 技術方針

- Unreal Engine 5のC++プロジェクトとして作成する。実際に利用可能なUEバージョンを次タスクで検出し、`.uproject` 生成時に固定する。
- ゲームルール、ダメージ、学習集計、保存はC++を正とする。
- 入力マッピング、Behavior Tree、アニメーション、UI、サウンド、レベル配置はBlueprint／アセットで調整可能にする。
- Enhanced Input、AI Perception、Navigation System、Gameplay Tags、UMG、SaveGameを使用する。
- Gameplay Ability System、Mass、外部ML基盤はVer0.1では採用しない。
- 乱数を使う適応選択にはランのSeedを持たせ、デバッグ時に再現できるようにする。

## 2. ビルド対象

- Primary: Windows 64-bit、Development Editor
- Secondary: Windows Developmentパッケージ
- プレイ人数: 1
- 入力: キーボード／マウス（ゲームパッドは後回し）
- Source control対象外: `Binaries/`、`DerivedDataCache/`、`Intermediate/`、`Saved/`、IDE生成物

正確なEngine Association、Visual Studio toolchain、ターゲットSDKは、ローカル環境を確認してプロジェクト骨格を作るサイクルで確定します。

## 3. モジュール構成

初期は単一Runtimeモジュール `AdaptiveHorror` とし、完成を優先します。責務が増えてから分割を判断します。

```text
Source/AdaptiveHorror/
  AdaptiveHorror.Build.cs
  AdaptiveHorror.cpp/.h
  Core/
    AHGameMode
    AHGameState
    AHGameInstance
    AHGameplayTags
    AHSaveGame
    AHCheckpoint
  Characters/
    AHPlayerCharacter
    AHPlayerController
    AHHealthComponent
    AHInteractionComponent
  Weapons/
    AHWeaponBase
    AHHitscanWeapon
    AHWeaponComponent
  AI/
    AHEnemyBase
    AHZombie
    AHHunter
    AHAdam
    AHEnemyAIController
    EVAAdaptationSubsystem
    PlayerBehaviorTrackerComponent
    AdaptationTypes
  UI/
    AHHUD
```

クラス名は仮称です。既存資産がないため、この命名をプロジェクト骨格の基準にします。

## 4. 主要責務

| 型 | 責務 |
|---|---|
| `AAHPlayerCharacter` | 移動、視点、プレイヤー入力の入口 |
| `UAHHealthComponent` | HP、ダメージ受付、死亡イベント。プレイヤー／敵で共有 |
| `UAHWeaponComponent` | 所持武器、射撃、弾薬、リロード、切替 |
| `AAHEnemyBase` | 敵共通の体力、感知結果、攻撃、進化Modifier適用 |
| `AAHEnemyAIController` | Perception、Blackboard、Behavior Tree起動 |
| `UPlayerBehaviorTrackerComponent` | 生イベントを正規化し学習サブシステムへ送信 |
| `UEVAAdaptationSubsystem` | ラン全体の集計、段階、対策選択、HUNTER状態を保持 |
| `UAHSaveGame` | チェックポイントと学習スナップショットを保存 |

`UEVAAdaptationSubsystem` は `UGameInstanceSubsystem` を継承し、マップ遷移をまたいで同一ランの学習状態を保持します。敵はサブシステムを直接書き換えず、型付き観測イベントを送ります。

## 5. 学習データモデル

体験版では「オンライン機械学習」ではなく、透明性と調整容易性を優先したストリーミング集計＋ルールベース選択を使います。

### 生イベント（デバッグ用リングバッファ）

```cpp
USTRUCT(BlueprintType)
struct FPlayerObservation
{
    FGameplayTag EventType;
    FGameplayTag ObserverType;
    FGameplayTag WeaponType;
    FGameplayTag CauseTag;
    FName EncounterId;
    FName ZoneId;
    float WorldTimeSeconds = 0.0f;
    float CombatDistanceMeters = 0.0f;
    float Value = 0.0f;
    bool bHeadshot = false;
};
```

保存対象は集計値を基本とし、生イベントは直近128件だけをメモリに保持します。個人情報や外部送信はありません。

### 集計プロファイル

- `HeadshotHits / ValidEnemyHits`: HS率。ミス射撃は分母に入れない。
- `CombatShotsByWeapon / TotalCombatShots`: 武器使用率。敵を狙った有効な戦闘中射撃を数える。
- 距離ヒストグラム: 近距離 `<5m`、中距離 `5〜15m`、遠距離 `>15m`。
- 距離EWMA: 最近の戦い方へ緩やかに追従する平均距離。
- `DeathCauseCounts`: 近接、飛び道具、環境、HUNTER、アダム等のGameplay Tag別回数。
- `EscapeRouteCounts`: 区画内の論理ルートIDを通過順で記録し、遭遇離脱時に確定。
- `HideSpotVisitCounts` と滞在秒数: 隠れ場所ID別に記録。
- `ObservationMass`: 有効観測量。成長段階の判定に使う。

0除算、サンプル不足、同率を明示的に処理します。主傾向は最低サンプル数を満たすまで `Unknown` とします。

## 6. 観測強度と段階

観測イベントの有効重みは次で求めます。

```text
EffectiveWeight = EventBaseWeight × ObserverAccuracy × EVA_LearningRate

ObserverAccuracy:
  通常ゾンビ = 0.20
  HUNTER     = 1.00
  固定設備   = 調整用（初期値0.25）

EVA_LearningRate:
  HUNTER生存中 = 1.00
  HUNTER撃破中 = 0.30
  HUNTER再投入 = 1.00
```

通常ゾンビ由来のデータも常に残ります。HUNTERの生死は全体の学習速度を変え、再投入時に100%へ戻します。

初期調整値はData AssetまたはData Tableへ置きます。

| 段階 | 仮条件 | 適用可能範囲 |
|---|---|---|
| Learning | `ObservationMass < 25` または研究棟到達前 | 記録のみ |
| Adapting | `25以上` かつ研究棟以降 | 行動パラメータ／経路変更 |
| Evolving | `70以上` かつ隔離区画以降 | 進化個体スポーン |

閾値はプレイテストで変更します。更新はイベントごとに集計し、実際の敵への反映は遭遇終了、チェックポイント通過、スポーン時だけに行います。

## 7. 適応ルール Ver0.1

各傾向に0〜1の信頼度を持たせ、サンプル不足ペナルティと直近データの重みを加えます。一度に有効化する通常敵向け適応は最大2個、進化は1個体1種です。

| 検出傾向 | 適応 | 進化候補 |
|---|---|---|
| 高HS率 | 頭を振る／遮蔽物接近を増やす | 装甲型 |
| 遠距離戦偏重 | 迂回・疾走開始を早める | 高速型 |
| 近距離すれ違い反復 | 攻撃開始距離と予測旋回を調整 | 長腕型 |
| 同一逃走経路反復 | 先回り用Nav Link／スポーン候補を優先 | 高速型 |
| 同一隠れ場所反復 | 十分な観測信頼度で探索ポイント化 | 長腕型または通常適応 |
| 単一武器偏重 | その武器距離帯への接近方法を変更 | 他傾向との合成で決定 |

死亡原因は難易度緩和にも使います。同じ原因で連続死亡した場合、該当カウンターの信頼度を一段下げ、弾薬または回復の救済候補を有効化します。学習が単純な難易度上昇だけにならないための安全弁です。

## 8. HUNTERライフサイクル

```text
未投入 → 生存（学習100%） → 撃破（30%、解析コアDrop）
      → 再投入待ち → 強化版再投入（100%、世代+1） → 最終撃破／撤退
```

状態はSubsystemで管理し、`Generation`、撃破時刻、再投入可能区画、前回選択した対抗行動を保存します。体験版は最大Generation 2です。実時間だけに依存すると待機悪用が起きるため、再投入は「最短待機時間」と「中央コア進行トリガー」の両方を満たした時に行います。

## 9. ダメージ／武器

- ダメージはUnrealのDamage Eventを入口にし、Hit BoneまたはPhysical Materialで部位を判定する。
- ダメージ計算順: 基礎ダメージ → 部位倍率 → 進化Modifier → 難易度Modifier → Clamp。
- 装甲型の頭部軽減は部位倍率適用後に0.5倍。胴体HP+30%はSpawn時のMaxHP Modifier。
- Hitscanを先に実装し、Projectile武器は体験版に必要になった場合だけ追加する。
- 射撃結果をBehavior Trackerへ通知し、武器側と敵側で二重計上しないよう一意なShot IDを持つ。

## 10. AI実装

- `AIController + AI Perception + Blackboard + Behavior Tree` を基準にする。
- Blackboardの最小キー: `TargetActor`、`LastKnownLocation`、`HomeLocation`、`CurrentState`、`AdaptationProfile`。
- 通常ゾンビ状態: Idle/Patrol → Investigate → Chase → Attack → Search → Return → Dead。
- NavMeshがない場合もゲームがクラッシュしないガードを入れる。
- HUNTERは同じ土台を使い、観測視線、追跡、退路選択、対抗行動を追加する。
- Behavior Tree Taskから学習値を変更せず、適応プロファイルを読み取るだけにする。

## 11. チェックポイント／保存

保存対象:

- チェックポイントID、マップ名、プレイヤーTransform
- HP、武器、マガジン／予備弾、重要取得物
- 撃破済み重要敵、開通済み扉、進行フラグ
- EVA集計プロファイル、成長段階、HUNTER状態、ランSeed

復帰時は保存スナップショットを完全復元します。死亡後の失敗行動だけを永続加算すると難易度が発散するため、Ver0.1では死亡原因を記録してからゲームオーバー画面へ渡し、リトライ時はチェックポイント保存時の学習状態へ戻します。

## 12. アセットとデータ

全ゲームアセットは `/Game/AdaptiveHorror/` 以下に置きます。Blueprint命名例は `BP_`、Widgetは `WBP_`、Behavior Treeは `BT_`、Blackboardは `BB_`、Data Assetは `DA_`、Mapは `L_` です。

調整値をコードに散在させず、次をData AssetまたはData Table化します。

- 武器性能
- 敵基礎性能
- 進化Modifier
- 学習閾値、最低サンプル数、観測者精度
- 適応ルールの有効条件と上限
- ドロップ／救済候補

## 13. テスト戦略

### 自動化可能

- HS率、武器使用率、距離帯、0除算
- HUNTER生死による1.0／0.3切替
- 進化選択の最低サンプル、同率、上限
- SaveGameの往復でプロファイルが一致
- ダメージModifierの計算順

### PIE／手動

- 入力、射撃、リロード、死亡／復帰
- NavMesh上の感知、追跡、攻撃、見失い
- 遭遇終了後にのみ適応が反映されること
- HUNTER撃破、Drop、30%化、強化再投入、100%復帰
- 30〜60分の通しプレイと詰み防止

### 各サイクルの最低確認

1. Project files生成またはEditor Compileが成功する。
2. 対象マップをPIE起動できる。
3. 追加機能の正常系を1回確認する。
4. 既存の主要ループを短いスモークテストする。
5. 実行不能なら理由と未確認範囲を `DEV_LOG.md` に残す。

## 14. 実装ロードマップ

優先順位は固定し、一度に1つの縦切りを完成させます。

| 順 | マイルストーン | 完了の定義 |
|---:|---|---|
| 1 | プロジェクト構成 | `.uproject`、C++モジュール、GameMode、テストMapがEditorで起動 |
| 2 | プレイヤー操作 | 移動、視点、ジャンプ、スプリント、しゃがみをPIE確認 |
| 3 | 武器システム | 拳銃の射撃、弾薬、リロード、命中表示 |
| 4 | 通常ゾンビAI | 感知、追跡、攻撃、見失い、死亡 |
| 5 | ダメージ処理 | 共通HP、部位、死亡、プレイヤーGame Over入口 |
| 6 | AI学習ログ | 6種のデータ集計とデバッグ表示／テスト |
| 7 | 敵適応 | 遭遇間で行動変化し、原因を確認可能 |
| 8 | HUNTER | 初回投入、観測、撃破、Drop、再投入 |
| 9 | 進化個体 | 3種Modifierと視認可能な区別 |
| 10 | アダム | 基本2フェーズ＋適応枠で撃破可能 |
| 11 | 研究施設 | 全区間、鍵、扉、資源、誘導、NavMesh |
| 12 | UI | HUD、通知、Game Over、Clear |
| 13 | チェックポイント | 主要状態と学習状態を保存／復元 |
| 14 | 通し調整 | 30〜60分、進行不能なし、適応を体感可能 |

## 15. 既知リスク

- 適応が見えない: デバッグ可視化を先に作り、演出は後から差し替える。
- 適応が理不尽: 最低サンプル、区画ゲート、最大2適応、救済ルールで抑える。
- C++とBlueprintの責務混在: 状態の正はC++、表現と調整はBlueprintに固定する。
- ステージ制作の肥大化: Grayboxを先に完成させ、区画ごとの戦闘目的を1つに絞る。
- HUNTER再投入待ちの悪用: 時間＋進行条件で制御する。

## 16. Cycle 001 実装差分

ユーザー指定に合わせ、実クラスの接頭辞は計画時の `AH` から `Eva` へ変更した。現在の中心クラスは `AEvaPrototypeGameMode`、`AEvaPlayerCharacter`、`AEvaHitscanWeapon`、`AEvaZombieCharacter`、`UEvaPlayerTelemetryComponent`、`UEvaLearningSubsystem`。責務境界は本書3〜4節を維持する。

Editorを利用できない環境への暫定対応として、次を採用した。理由と移行条件は `DESIGN_DECISIONS.md` に記録する。

- Engine Association未固定
- `/Engine/Maps/Entry` とRuntime Graybox Arena
- C++実行時Enhanced Input Mapping
- Controller Tickによる最小Zombie AI
- C++ `AHUD` による仮UI

Automation Test `AdaptiveHorror.EVA.TelemetryClassification` と `AdaptiveHorror.EVA.HunterLearningMultiplier` を追加済み。UE toolchain導入後の最初の作業は、UHT、Development Editor build、Automation Test、PIE smoke testである。
## Cycle 008 Safe Spawn Specification

Enemy spawning is centralized through `AEvaPrototypeGameMode` for runtime graybox safety.

### Safe Location Search

`FindSafeEnemySpawnLocation(...)` evaluates up to 12 candidates around the requested origin. A candidate is accepted only when:

- it projects to NavMesh when a `UNavigationSystemV1` is available;
- it has a valid floor trace;
- the capsule-adjusted final Z keeps the pawn above the floor;
- capsule overlap against `ECC_Pawn` and `ECC_WorldStatic` is clear;
- player distance is at least the configured minimum;
- nearest living enemy distance is at least the configured minimum.

### Spawn Handling

Enemy spawn collision handling is:

```cpp
ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding
```

`AlwaysSpawn` is not used for enemies. If a post-spawn overlap is detected, the enemy is relocated once to a newly found safe point. If that fails, the enemy is destroyed and the spawn attempt is logged as failed.

### Spawn Telemetry

`LogAdaptiveHorror` records:

- enemy type and spawn reason;
- requested and final location;
- attempt index;
- collision handling;
- success/failure;
- `SpawnActor` return state;
- AI controller/possession state;
- player distance;
- nearest enemy distance;
- NavMesh projection and floor trace state.

### Runtime Debug HUD

The HUD displays active zombie/hunter/ADAM counts, last spawn result, last spawn location, NavMesh availability, fallback movement count, and stuck enemy count.

### Movement Fallback

NavMesh movement remains primary. Direct movement fallback is short-term only, includes nearby enemy separation, and aborts when a forward world-static trace is blocked.

## Cycle 014 Core UI Flow Specification

### Game flow state

The prototype uses `EEvaGameFlowState`:

- `Title`
- `Playing`
- `Paused`
- `PlayerDead`
- `StageCleared`
- `Loading`

`AEvaPrototypeGameMode` owns the authoritative flow state for the current runtime session.

Key helpers:

- `EnterTitleMode()`
- `StartNewGameFlow()`
- `PauseGameFlow()`
- `ResumeGameFlow()`
- `RetryFromCheckpointFlow()`
- `ReturnToTitleFlow()`
- `IsGameplayActive()`
- `CanPlayerTakeDamage()`

Combat spawning is gated so it starts only in `Playing`.

### Widget structure

Menu widgets are implemented in C++ in `EvaMenuWidgets`.

- `UEvaTitleMenuWidget`
- `UEvaPauseMenuWidget`
- `UEvaGameOverWidget`
- `UEvaStageClearWidget`
- `UEvaSettingsWidget`

`AEvaPlayerController` owns widget references as `UPROPERTY` pointers and centralizes:

- widget creation/removal,
- menu input mode,
- gameplay input mode,
- mouse cursor visibility,
- settings load/save/apply,
- UI event tones.

Widgets call controller methods rather than directly mutating GameMode state.

### Input and pause behavior

- `Esc` is bound in `AEvaPlayerController::SetupInputComponent()`.
- `Esc` toggles `PauseGameFlow()` / `ResumeGameFlow()` only from `Playing` or `Paused`.
- Menus use `FInputModeGameAndUI` and visible cursor.
- Gameplay uses `FInputModeGameOnly` and hidden cursor.
- Player movement/fire/reload/jump check `AEvaPrototypeGameMode::IsGameplayActive()`.
- Player damage checks `AEvaPrototypeGameMode::CanPlayerTakeDamage()`.

### New Game reset responsibilities

`StartNewGameFlow()` resets:

- Game Over / Stage Clear flags.
- combat timers.
- active combat actors.
- HUNTER tier/count/current pointer.
- player checkpoint transform and player HP/ammo via checkpoint reset.
- player telemetry.
- EVA learning subsystem.
- research facility director state.
- spawn/debug counters relevant to the session.

### HUD split

`AEvaHUD` now treats normal HUD and debug HUD separately.

Normal HUD:

- HP.
- Ammo.
- EVA analysis/stage.
- HUNTER state.
- objective/zone.
- crosshair.
- Adam Boss HUD during Adam encounter.

Debug HUD:

- kills/accuracy/headshot rate/combat style.
- adaptation/evolution recommendation.
- active enemy counts.
- spawn result/location.
- NavMesh/debug/fallback/stuck counters.
- EVA log count.

Debug HUD is toggled with `F9` or `N`. `F8` remains intentionally unbound because PIE uses it for Eject.

### Settings save

`UEvaSettingsSaveGame` stores prototype settings in slot `EvaPrototypeSettings`.

Applied now:

- Mouse sensitivity.
- Invert Mouse Y.
- Master/SFX volume multiplier for procedural UI tones.

Stored for future application:

- BGM volume.
- Fullscreen/windowed.
- Resolution.
- Graphics quality.

### Audio foundation

No external sound assets are required for Cycle 014.

`AEvaPlayerController::PlayTone()` creates short procedural `USoundWaveProcedural` UI tones for:

- button click,
- title/menu event,
- pause,
- game over,
- stage clear.

Gameplay, enemy, HUNTER, Adam, ambience, and BGM sound events remain TODO and should be implemented with replaceable assets/components later.

### Verification status

- Development Editor / Win64 build: succeeded.
- Automation RunTests `AdaptiveHorror`: 21 succeeded, 0 failed.
- Runtime smoke: succeeded; flow reached `Title`.
- PIE visual/audio confirmation is still required.
