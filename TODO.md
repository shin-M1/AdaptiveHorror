# TODO — Adaptive Horror FPS Demo

## Cycle 009 状態メモ

障害物回避リカバリ、頭上ラベル、敵タイプ識別、全Automation導線を改善した。

- [x] 作業前に `git status` / branch / logを確認した。
- [x] 通常ゾンビの詰まり検知後に側方迂回を試すようにした。
- [x] Direct fallback前方ブロック時に左右迂回候補を試すようにした。
- [x] 頭上ラベルの固定Yaw 180度を廃止した。
- [x] 頭上ラベルをプレイヤーカメラ方向へのYaw-only Billboardにした。
- [x] LongArmの見た目をActorScaleではなく腕パーツで表現するようにした。
- [x] Fast / Armored / LongArm / Compositeの体格差を相対パーツで強化した。
- [x] HUNTER / ADAM / ADAM Phase2の仮モデル差を強化した。
- [x] `RunBuildCheck.ps1` のAutomation対象を `AdaptiveHorror` 全体へ拡張した。
- [x] Automation Testを15件に拡張し、全件Successを確認した。
- [x] Development Editor / Win64 Build成功を確認した。
- [x] Runtime smokeでGameMode起動、Navigation Build、初期ゾンビBeginPlay、Fatalなしを確認した。

### 次回最優先TODO

- [ ] UE5.8 PIEで、障害物を挟んだ通常ゾンビが停止せず迂回・再接近することを確認する。
- [ ] PIEで頭上ラベルが左右反転せず読めることを確認する。
- [ ] PIEでFast / Armored / LongArm / Composite / HUNTER / ADAMの見た目差が分かるか確認する。
- [ ] F2でHUNTERの追跡、攻撃、撃破、解析コアDrop、30秒Tier+1再投入を確認する。
- [ ] F4でADAMの追跡、近接、突進、攻撃後再追跡、Phase2、撃破Stage Clearを確認する。
- [ ] HUD NAV DEBUGの視認性とF9切替をPIEで確認する。
- [ ] Runtime debug lightの明るさが強すぎないか、敵/NAV確認がしやすいか調整する。
- [ ] 可能なら障害物回避のPIE結果に応じてAcceptanceRadius / stuck判定秒数 / sidestep距離を微調整する。

## Cycle 007 状態メモ

ゲームループ安定化として、Runtime GrayboxのNavMesh問題とAI追跡停止リスクを修正した。

- [x] Development Editor / Win64 Buildを再実行した。
- [x] `AdaptiveHorror.EVA` Automation Test 8件Successを確認した。
- [x] Standalone Game相当の短時間起動で `/Engine/Maps/Entry` が `EvaPrototypeGameMode` で起動することを確認した。
- [x] Runtime Navigation Build開始ログを確認した。
- [x] Runtime Spawnした空 `ANavMeshBoundsVolume` 由来のempty bounds警告を解消した。
- [x] `bWholeWorldNavigable` によるRuntime Graybox向けNavigation Buildに変更した。
- [x] 通常ゾンビ/HUNTER/ADAMのNavMesh失敗時Direct movement fallbackを追加した。
- [x] GameOver / StageClear / Debug Restore周辺のTimer重複リスクを低減した。

### 次回最優先TODO

- [ ] UE5.8 PIEで通常ゾンビがスポーン、感知、追跡、攻撃し、プレイヤーHPが減ることを手動確認する。
- [ ] 射撃、通常ダメージ、ヘッドショット倍率、ゾンビ死亡、Kill/Telemetry更新をPIEで確認する。
- [ ] プレイヤー死亡、GAME OVER、3秒Checkpoint復帰、複数回死亡時のTimer重複なしをPIEで確認する。
- [ ] F2でHUNTER強制出現、撃破、解析コアDrop、学習倍率0.3、30秒再投入を確認する。
- [ ] F1/F3/F7でEVA解析率、進化個体、Telemetry Snapshotを確認する。
- [ ] 研究施設6区画を通しで移動し、Zone Trigger、Objective、Checkpoint、EVAログ5件を確認する。
- [ ] F4または通常進行でADAM戦に入り、Phase 2、撃破、Stage Clearを確認する。
- [ ] F1〜F7 DebugキーをすべてPIEで目視確認する。
- [ ] Runtime Grayboxから正式 `.umap` へ移行する際、Editor上にNavMeshBoundsVolumeを配置・保存する。

## Cycle 006 状態メモ

UE5.8 PIEでマウス上下視点が逆になる問題を修正した。

- [x] `AEvaPlayerCharacter::Look` を確認した。
- [x] Enhanced InputのLook Action / MouseY Mappingを確認した。
- [x] MouseY Mapping側にNegateがないことを確認した。
- [x] 上下反転管理をC++側に一本化した。
- [x] `bInvertMouseY` を追加した。
  - [x] デフォルト `false`。
  - [x] `false` で通常FPS操作。
  - [x] `true` で上下反転。
  - [x] Blueprint/設定画面から変更可能。
- [x] 静的検証PASS。
- [x] `EvaPlayerCharacter.cpp` を含むC++ Compile段階はPASS。
- [ ] Development Editor / Win64 Link完了。
  - 現在はUE Editorが `UnrealEditor-AdaptiveHorror.dll` を掴んでいるため `LNK1104` で停止。
- [ ] Automation Test再実行。
- [ ] PIEでマウス上移動/下移動/左右方向を確認。

### 次回優先TODO

- [ ] UE EditorとLiveCodingConsoleを閉じる。
- [ ] `powershell -ExecutionPolicy Bypass -File .\Scripts\RunBuildCheck.ps1 -MaxParallelActions 4` を再実行する。
- [ ] `AdaptiveHorror.EVA` Automation Test成功を確認する。
- [ ] PIEで通常FPSの上下視点を確認する。
- [ ] `bInvertMouseY=true` の上下反転も確認する。

## Cycle 005 状態メモ

UE5.8 Development Editor / Win64の初回ビルドエラーを修正し、Automation Testまで通過した。

- [x] UE5.8 / MSVC toolchainを検出した。
- [x] Generate Project Filesが成功した。
- [x] Development Editor / Win64 Buildが成功した。
- [x] `AdaptiveHorror.EVA` Automation Test 8件が成功した。
- [x] `DefaultBuildSettings = BuildSettingsVersion.V7` へ更新した。
- [x] モジュールinclude pathをUE5.8向けに補正した。
- [x] UE5.8 API差分 `APointLight::GetPointLightComponent()` を修正した。
- [x] V7でエラー化されたshadowingを修正した。
- [x] `RunBuildCheck.ps1` に `-MaxParallelActions` を追加した。

### 次回優先TODO

- [ ] UE5.8 EditorでPIEを起動する。
- [ ] F1〜F7 DebugキーをPIEで確認する。
- [ ] 研究施設6区画の導線を実プレイで調整する。
- [ ] HUNTER出現・再投入の体感を調整する。
- [ ] ADAM戦の難易度を調整する。
- [ ] 最低限のサウンドと画面演出を追加する。
- [ ] 体験版としての開始・終了フローを整備する。

## Cycle 004 状態メモ

今回、UE5実環境ビルド確認を最優先で再実行したが、この環境ではUE/MSVCツールチェーンが未検出だったため、静的検証・ビルド導線強化・Debugキー・クラッシュ予防を実装した。

- [x] `Scripts/RunBuildCheck.ps1` を実行した。
  - [x] `.uproject` 検出成功。
  - [x] Static source sanity PASS。
  - [ ] `UnrealEditor.exe` 検出。
  - [ ] `UnrealBuildTool.exe` 検出。
  - [ ] `MSBuild.exe` 検出。
  - [ ] `cl.exe` 検出。
  - [ ] Generate Project Files実行。
  - [ ] Development Editor Build実行。
  - [ ] Automation Test実行。
  - [ ] PIE起動確認。
- [x] `.generated.h` include順序を静的確認した。
- [x] `.uproject` JSONを静的確認した。
- [x] `RunBuildCheck.ps1` に静的sanity checkを追加した。
- [x] `RunBuildCheck.ps1` にAutomation Test実行導線を追加した。
- [x] 非Shipping Debugキーを追加した。
  - [x] F1: EVA解析率+20。
  - [x] F2: HUNTER強制出現。
  - [x] F3: ゾンビWave強制スポーン。
  - [x] F4: ADAM戦へワープ。
  - [x] F5: プレイヤー全回復・弾薬補充。
  - [x] F6: Stage Clear強制。
  - [x] F7: Telemetry Snapshot表示。
- [x] HUD Debug通知を追加した。
- [x] Checkpoint復帰時の敵AIターゲット解除を追加した。
- [x] HUD/Player/Weapon/Zombie/HUNTER/ADAM/Director周辺のnull保護を強化した。

### 次回優先TODO

- [ ] UE5実環境で `Scripts/RunBuildCheck.ps1` を再実行し、UHT/Compile/Automationエラーを実測する。
- [ ] UE5 PIEでF1〜F7 Debugキーの実動作を確認する。
- [ ] 研究施設6区画の導線をプレイ調整する。
- [ ] HUNTER出現・再投入の体感を調整する。
- [ ] ADAM戦の難易度を調整する。
- [ ] 最低限のサウンドと画面演出を追加する。
- [ ] 体験版としての開始・終了フローを整備する。

## Cycle 003 状態メモ

今回、研究施設体験版ステージとアダム基盤をC++ Runtime Grayboxとして追加した。

- [x] `BUILD_CHECK.md` を作成した。
- [x] `Scripts/RunBuildCheck.ps1` を作成した。
- [x] UnrealEditor.exe / UnrealBuildTool.exe / MSBuild.exe / cl.exe の探索導線を整備した。
- [x] 実環境ビルド不可時の原因切り分け表を作成した。
- [x] 研究施設6区画GrayboxをRuntime生成するようにした。
  - [x] Entry Lobby
  - [x] Security Corridor
  - [x] Observation Lab
  - [x] Containment Ward
  - [x] Data Core Room
  - [x] Adam Arena
- [x] `AEvaResearchFacilityDirector` を追加した。
- [x] 区画トリガー `AEvaFacilityZoneTrigger` を追加した。
- [x] EVAログPickup `AEvaStoryLogPickup` を追加した。
- [x] EVAログ5種を配置した。
- [x] Boss `AEvaAdamBossCharacter` を追加した。
- [x] Boss AI `AEvaAdamBossAIController` を追加した。
- [x] Adam HP50%以下のPhase 2基盤を実装した。
- [x] Adam撃破でStage Clear扱いにした。
- [x] HUDにZone / Objective / EVA Logs / Stage Clear Resultを表示した。
- [x] Director / StoryLog / Adam Phase / Adam DefeatのAutomation Testコードを追加した。

未完了/要確認:

- [ ] UE5実環境でGenerate Project Filesを実行する。
- [ ] UE5実環境でDevelopment Editor Buildを実行する。
- [ ] UE5実環境で `AdaptiveHorror.EVA` Automation Testを実行する。
- [ ] PIEで6区画進行、区画トリガー、NavMesh移動を確認する。
- [ ] PIEでAdam Phase 2とStage Clear HUDを確認する。
- [ ] 研究施設Grayboxのサイズ、導線、敵密度、アイテム配置を調整する。
- [ ] HUNTER/Adam/進化個体の演出とサウンドを追加する。

## Cycle 002 状態メモ

今回の目的だった「ただのゾンビFPS」から「プレイヤーを学習するゲーム」への移行は、C++実装上は到達した。

- [x] HUNTER本格AIを追加した。
  - [x] 撃破数3体または45秒経過でHUNTER出現。
  - [x] Berserker / Ranger / Ghost / Explorer への対抗行動。
  - [x] HUNTER観測でEVA解析率が高精度に上昇。
  - [x] HUNTER撃破で解析コアをドロップ。
  - [x] HUNTER撃破で学習倍率1.0から0.3へ低下。
  - [x] HUNTER再投入で学習倍率0.3から1.0へ復帰。
- [x] EVA解析率をHUD表示へ追加した。
  - [x] 0〜100%。
  - [x] 命中、HS、キル、被ダメージ、HUNTER観測で上昇。
  - [x] 解析段階 `Learning / Adapting / Evolving` を追加。
- [x] 敵適応システムを実装した。
  - [x] 近距離型対策: 距離を取る。
  - [x] 遠距離型対策: 遮蔽物利用。
  - [x] 隠密型対策: 索敵範囲増加。
  - [x] 探索型対策: 待ち伏せ増加。
- [x] 進化個体を実際にスポーンするようにした。
  - [x] 20%: 高速型。
  - [x] 40%: 装甲型。
  - [x] 60%: 長腕型。
  - [x] 80%: 複合型。
- [x] HUDにHUNTER状態、学習倍率、適応方針、次進化型を追加した。
- [x] Automation Testを追加した。
  - [x] HUNTER撃破倍率。
  - [x] 解析率増加。
  - [x] 進化閾値。

未完了/要確認:

- [ ] UE5実環境でDevelopment Editor Buildを実行する。
- [ ] UE5実環境で `AdaptiveHorror.EVA` Automation Testを実行する。
- [ ] PIEでHUNTER出現、撃破、再投入、解析コアDropを確認する。
- [ ] PIEで解析率20/40/60/80%到達時の進化スポーンを確認する。
- [ ] PIEで通常ゾンビの適応行動がプレイヤーから体感できるか調整する。
- [ ] HUNTER/進化個体のMaterial、音、警告演出を追加する。
- [ ] 研究施設6区画ステージへタグ付きCover/Hide/Route/Ambushを移植する。
- [ ] アダムボス基盤を実装する。

チェックは実装と静的検証まで終わった時だけ付けます。Editor／PIEが必要な確認は明記し、実行するまで未完了のままにします。

## P0: プロジェクト構成

- [x] ローカルのUnreal EngineとC++ toolchainを確認する（未導入と判定）
- [x] `AdaptiveHorror.uproject` と単一Runtime C++モジュールを作成する
- [x] 基本プラグイン／モジュール依存（Enhanced Input、Gameplay Tags、AIModule、UMG等）を設定する
- [x] `AEvaPrototypeGameMode`、`AEvaPlayerController`、`AEvaPlayerCharacter` を作る
- [x] `/Engine/Maps/Entry` 上にRuntime `L_DevGym` 相当のArenaを生成し、Default GameModeを設定する
- [ ] Editorで `/Game/AdaptiveHorror/Maps/L_DevGym.umap` を保存する
- [x] Unreal向け `.gitignore` と基本 `Config` を整備する
- [ ] Development EditorビルドとPIE起動を確認する

## P1: プレイヤー操作

- [x] Enhanced Input Actions／Mapping ContextをC++実行時生成する
- [x] 移動、視点、ジャンプを実装する
- [x] スプリントを実装する
- [ ] しゃがみを実装する（最小縦切り後）
- [x] FPSカメラとカプセルを実装する
- [ ] 入力喪失／フォーカス復帰のスモークテストを行う

## P2: 武器システム

- [x] Blueprint拡張可能な `AEvaWeaponBase` と編集可能な武器値を作る
- [x] 拳銃Hitscan、発射間隔、命中結果を実装する
- [x] マガジン、予備弾、1.5秒リロードを実装する
- [ ] リロード中断を実装する
- [ ] 射撃／命中に一意なShot IDを付ける
- [ ] ショットガンと武器切替を追加する
- [ ] 仮マズル、反動、発射／空撃ち音を追加する

## P3: 通常ゾンビAI

- [x] 敵基底、Zombie、AIControllerを作る
- [x] AI Perceptionの視覚15m／聴覚10mを設定する
- [ ] Gameplay Team判定を設定する
- [ ] Blackboard／Behavior Treeの最小状態を作る
- [x] 感知、NavMesh追跡、近接攻撃、見失いを最小実装する
- [ ] Patrol／Search／ReturnとBehavior Treeを実装する
- [ ] NavMesh欠落時の安全動作を確認する

## P4: ダメージ処理

- [x] 共有Health Componentを作る
- [x] プレイヤー／敵の被ダメージ、死亡イベントを統一する
- [x] 仮Head／Torso Hitboxによる部位判定を作る
- [ ] Skeletal Mesh導入後にHit Bone／Physical Material判定へ移行する
- [ ] ダメージ計算順とModifierテストを作る
- [x] 入力停止、GAME OVER表示、3秒後のメモリCheckpoint復帰を接続する

## P5: AI学習ログ

- [ ] Gameplay Tagsと生イベント `FPlayerObservation` を定義する
- [x] `UEvaPlayerTelemetryComponent` を作る
- [x] `UEvaLearningSubsystem` と集計プロファイルを作る
- [x] HS率と武器使用率を記録する
- [x] 戦闘距離と死亡原因を記録する
- [x] 逃走経路と隠れ場所の記録APIを作る
- [ ] 通常ゾンビ由来観測へ低精度Weightを適用する
- [x] 集計／倍率のAutomation TestコードとC++ HUD表示を作る
- [ ] UE上で `AdaptiveHorror.EVA` Automation Testを実行する

## P6: 敵適応システム

- [ ] Learning／Adapting／Evolvingの段階ゲートを実装する
- [ ] 最低サンプル、信頼度、同率処理を実装する
- [ ] 高HS、距離偏重、経路反復、隠れ場所反復を判定する
- [ ] 遭遇終了／Spawn時だけ適応を反映する
- [ ] 通常ゾンビの経路、回避、攻撃タイミング適応を最低2種作る
- [ ] 連続死亡時の難易度安全弁を実装する
- [ ] 適応の原因と結果をデバッグ表示する

## P7: HUNTER α

- [x] `AEvaHunterCharacter` 仮クラス、HP、解析率、倍率、Tierを作る
- [ ] HUNTER本格AIと高精度観測を実装する
- [x] 生存中1.0／撃破中0.3の学習倍率ロジックとAutomation Testコードを実装する
- [ ] HUNTER撃破をPIEし、倍率1.0／0.3を実測する
- [ ] 撃破時の解析コアDropを実装する
- [ ] 世代、撃破時刻、再投入状態を保持する
- [ ] 時間＋進行条件による強化再投入を実装する
- [ ] 前回の主戦術に対する対抗行動を1〜2種反映する
- [ ] 再投入時に学習倍率が100%へ戻ることを確認する

## P8: 進化個体

- [x] `None/Fast/Armored/LongArm` 型とZombieの適用接続点を作る
- [ ] 進化ModifierのData Asset／Tableを作る
- [ ] 高速型（移動速度+25%）を実装する
- [ ] 装甲型（頭部-50%、胴体HP+30%）を実装する
- [ ] 長腕型（射程+2.5m、攻撃+15%）を実装する
- [ ] 各型を仮メッシュ色、音、エフェクトで識別可能にする
- [ ] 1個体1種、同時出現上限、区画ゲートを確認する

## P9: ボス「アダム」

- [ ] ボスアリーナのGrayboxを作る
- [ ] 基本フェーズ1／2を実装する
- [ ] 弱点露出と明確な攻撃予告を実装する
- [ ] 最上位傾向に応じた適応枠を最低3候補作る
- [ ] 撃破からコア停止／脱出へ接続する

## P10: 研究施設ステージ

- [ ] 6区間のGrayboxと通し導線を作る
- [ ] 鍵、電源、扉、インタラクトを実装する
- [ ] 戦闘遭遇、逃走ルート、隠れ場所に論理IDを付ける
- [ ] 資源配置とショートカットを調整する
- [ ] NavMesh、Spawn、詰み、落下不能箇所を検証する
- [ ] 仮照明、環境音、EVA通知で誘導を作る

## P11: UI

- [x] C++ HUDでHP、弾薬、照準、Kill、命中率、HS率、Style、EVA解析率を表示する
- [ ] インタラクト、取得、リロード、チェックポイント通知を作る
- [ ] HUNTER観測強度と適応兆候の演出を作る
- [x] 仮Game Over表示を作る
- [ ] Stage Clear画面とプレイ結果を作る
- [ ] 開発ビルド限定のAIデバッグHUDを完成させる

## P12: チェックポイント

- [x] `AEvaCheckpoint` 接触で復帰Transformをメモリ保存する
- [x] 死亡3秒後にHP／弾薬を初期化して最後のCheckpointへ戻す
- [ ] SaveGameスキーマとVersionを定義する
- [ ] プレイヤー、進行、重要敵、EVA、HUNTER状態を保存する
- [ ] 主要区画入口とボス前へ配置する
- [ ] 死亡復帰、アプリ再起動、スキーマ不一致を検証する

## P13: 通しプレイ調整

- [ ] タイトルからクリアまで進行不能なしで通す
- [ ] 初見相当30〜60分、基準45分へ調整する
- [ ] 通常戦→適応→進化→HUNTER再投入→アダムの理解曲線を確認する
- [ ] 弾薬／回復／敵密度／チェックポイントを調整する
- [ ] 主要クラッシュ、警告、入力不能、Save破損を解消する
- [ ] Windows Developmentパッケージを作成し別起動確認する
- [ ] 既知問題と操作説明をREADMEへ追記する

## 保留（Ver0.1後）

- [ ] ゲームパッド対応
- [ ] アクセシビリティ設定
- [ ] 製品版用アート／音声／ローカライズ
- [ ] マルチプレイのAuthority／Replication設計レビュー（実装はしない）
- [ ] 複数ステージと周回メタ進行
## Cycle 008 Safe Spawn Follow-up

- [x] Replace enemy `AlwaysSpawn` paths with safe spawn flow for initial zombie, zone waves, F3 wave, HUNTER, ADAM, and ADAM roar minions.
- [x] Add spawn telemetry logs and HUD debug values.
- [x] Add runtime graybox directional/sky lighting.
- [x] Add AI direct movement separation and obstacle guard.
- [x] Add automation coverage for safe spawn and debug counters.
- [ ] Close Unreal Editor / Live Coding and re-run `Scripts\RunBuildCheck.ps1 -MaxParallelActions 2` so the editor DLL can link.
- [ ] Run `AdaptiveHorror.Spawn.*` and `AdaptiveHorror.EVA.*` automation tests after the DLL lock is cleared.
- [ ] PIE: confirm Entry Lobby spawns exactly one visible normal zombie in front/near the player and it can chase, attack, take damage, die, and update telemetry.
- [ ] PIE: press F3 repeatedly and confirm wave zombies spawn separated, not stacked.
- [ ] PIE: press P and confirm NavMesh coverage near spawn locations.
- [ ] PIE: press F2 and F4 and confirm HUNTER/ADAM spawns are safe and not embedded.
- [ ] PIE: confirm repeated `is stuck and failed to move` logs no longer appear during ordinary pursuit.

## Cycle 009 Obstacle pursuit / label consistency follow-up

- [x] Add path-state diagnostics for stopped obstacle pursuit.
- [x] Log MoveRequest result, PathFollowing state, PathValid, IsPartial, PathPoints, CurrentPathPointIndex.
- [x] Log player/enemy NavProjection results.
- [x] Log left/right detour candidate NavProjection results.
- [x] Log enemy-to-player trace result.
- [x] Log MoveCompleted ResultCode and distinguish Success / Blocked / Aborted / Invalid / OffPath.
- [x] Keep Path Following preferred whenever a valid NavPath exists.
- [x] Restrict Direct fallback to clear line-of-sight and clear-forward cases only.
- [x] Prevent Direct fallback from overwriting accepted MoveTo detours.
- [x] Accept sidestep destinations only after successful NavProjection.
- [x] Return to MoveToActor(Player) after sidestep recovery.
- [x] Use partial path end as an alternate detour search origin when available.
- [x] Avoid overwriting MoveTo every tick.
- [x] Log capsule radius versus Nav Agent radius.
- [x] Add common enemy debug label initialization.
- [x] Ensure HUNTER/ADAM subclass label text is logged after subclass initialization.
- [x] Log label state for BeginPlay and ConfigureEvolution paths.
- [x] Document F2/F3/F4/F7/F9/N/P/F8 debug key status.
- [x] Development Editor / Win64 build without Live Coding.
- [x] Automation RunTests AdaptiveHorror: 15 tests succeeded.
- [x] Runtime smoke with NullRHI commandlet launch.
- [ ] PIE: confirm a zombie routes around an obstacle when the NavMesh contains a valid detour.
- [ ] PIE: confirm the zombie stops instead of forcing through when the NavMesh has no valid detour.
- [ ] PIE: confirm labels are visible for InitialVisibleZombie, Wave Spawn, AdaptiveSpawn, evolved zombie, HUNTER, ADAM, ADAM minions, and HUNTER reinsertion.
- [ ] PIE: press F2/F3/F4/F7/F9/N and confirm runtime debug behavior.
- [ ] Consider adding an automation test that directly exercises projected sidestep recovery with a synthetic obstacle/NavPath setup.

## Cycle 010 Stationary target repath / ADAM debug start follow-up

- [x] Add stationary-target NoProgress monitoring independent of target movement.
- [x] Reissue `MoveToActor(Player)` when a valid path is accepted but the pawn has no meaningful progress.
- [x] Log Repath reasons: TargetMoved / NoProgress / MoveCompleted / PathInvalid / SidestepFinished / PeriodicRefresh.
- [x] Log elapsed time since last MoveTo, elapsed time since last progress, recent movement distance, DirectFallback active, Sidestep active, PathFollowing status, CurrentMoveRequestID, and reissue result.
- [x] Ensure Direct fallback is deactivated when Path Following MoveTo is accepted.
- [x] Ensure sidestep completion/timeout returns to normal player pursuit.
- [x] Make F4 explicitly start the ADAM encounter, not only teleport.
- [x] Prevent ADAM spawn failure from leaving Director in a false active state.
- [x] Prevent generic evolution setup from resetting ADAM into normal zombie visuals.
- [x] Clean non-boss arena enemies during F4 ADAM debug start.
- [x] Suppress ordinary AdaptiveSpawn while ADAM encounter is active.
- [x] Add ADAM debug logs for spawn state, class, count, nav projection, controller, possession, target, health, phase, and failure reason.
- [x] Development Editor / Win64 build without Live Coding.
- [x] Automation RunTests AdaptiveHorror: 15 tests succeeded.
- [x] Runtime smoke with NullRHI commandlet launch.
- [ ] PIE: with player fully stationary and not jumping, confirm zombie routes around an obstacle using NoProgress repath.
- [ ] PIE: confirm zombie does not press into the wall for several seconds.
- [ ] PIE: confirm zombie reaches melee range after obstacle reroute.
- [ ] PIE: confirm multiple zombies do not break pursuit around the obstacle.
- [ ] PIE: press F4 and confirm visible ADAM exists in Adam Arena.
- [ ] PIE: confirm F4 does not duplicate ADAM when pressed repeatedly.
- [ ] PIE: confirm ADAM AIController is possessed and targets player.
- [ ] PIE: confirm ADAM label is visible.
- [ ] PIE: confirm ADAM summoned enemy labels are visible.
- [ ] PIE: confirm FAST label.
- [ ] PIE: confirm ARMORED label.
- [ ] PIE: confirm LONG ARM label.

## Cycle 011 Adam chase crash follow-up

- [x] Inspect crash artifacts for Adam chase crash.
- [x] Record that WinDbg/cdb were not available locally and UE CrashContext/logs were used for symbolicated analysis.
- [x] Identify first AdaptiveHorror frame: `AEvaZombieAIController::OnMoveCompleted`.
- [x] Identify repeated recursive stack: `OnMoveCompleted -> ReissueMoveToTarget -> MoveToActor -> OnMoveCompleted`.
- [x] Fix only the reentrant MoveTo reissue path with an in-flight guard and pre-call cooldown timestamp.
- [x] Development Editor / Win64 build without Live Coding.
- [x] Automation RunTests AdaptiveHorror: 15 tests succeeded.
- [x] Runtime smoke with NullRHI commandlet launch.
- [ ] PIE: reproduce Adam chase for at least 60 seconds and confirm Unreal Editor no longer exits.
- [ ] PIE: inspect logs and confirm there is no same-frame flood of `MoveCompleted ResultCode=Invalid`.
- [ ] PIE: verify Adam charge, roar summon, Phase 2, and defeat still work after the reissue guard.

## Cycle 013 Stage Clear / Player Death conflict follow-up

- [x] Make Stage Clear reject late Player Death requests.
- [x] Make player damage return `0` after Stage Clear.
- [x] Prevent movement, sprint, jump, fire, and reload after Stage Clear while leaving look input available.
- [x] Clear respawn, wave, adaptive, HUNTER timed spawn, and HUNTER reinsertion timers on Stage Clear.
- [x] Stop existing zombie/evolved/HUNTER/ADAM/minion AI combat on Stage Clear.
- [x] Hide enemy overhead labels and HP bars after Stage Clear.
- [x] Skip post-clear spawn requests.
- [x] Keep Director/GameMode from diverging when Player Death wins before Adam defeat.
- [x] Add `[StageClear]` and `[PlayerDeath]` diagnostic logs.
- [x] Add automation tests for post-clear death rejection, spawn skipping, AI stop, and Stage Clear idempotence.
- [x] Development Editor / Win64 build without Live Coding.
- [x] Automation RunTests AdaptiveHorror: 19 tests succeeded.
- [x] Runtime smoke with NullRHI commandlet launch.
- [ ] PIE: defeat ADAM and confirm Stage Clear appears.
- [ ] PIE: after Stage Clear, stand still 60 seconds and confirm player HP does not decrease.
- [ ] PIE: confirm residual enemies no longer chase or attack after Stage Clear.
- [ ] PIE: confirm GAME OVER / checkpoint respawn does not start after Stage Clear.
- [ ] PIE: confirm camera look remains available after Stage Clear.
- [ ] PIE: confirm movement/shooting remain disabled after Stage Clear.
- [ ] PIE: confirm Boss HUD and enemy overhead HP bars are hidden after Stage Clear.

## Cycle 014 Core game UI flow

- [x] Create dedicated branch `feature/core-game-ui-flow`.
- [x] Add `Title / Playing / Paused / PlayerDead / StageCleared / Loading` flow state.
- [x] Add title menu with NEW GAME / disabled CONTINUE / SETTINGS / EXIT.
- [x] Add New Game reset flow for player, telemetry, EVA learning, Director, combat timers, enemies, HUNTER, Adam, Stage Clear, and Game Over state.
- [x] Add Esc pause/resume flow and pause menu.
- [x] Add Game Over menu and remove the legacy auto-respawn-only death loop.
- [x] Add Stage Clear menu and remove old canvas `STAGE CLEAR TODO` overlay.
- [x] Add Return to Title from Pause / Game Over / Stage Clear.
- [x] Add settings save object and settings widget for Master/BGM/SFX volume, mouse sensitivity, and invert Y.
- [x] Add placeholder settings fields for fullscreen/windowed, resolution, and graphics quality.
- [x] Add simple procedural UI tones for click/menu/game over/stage clear feedback.
- [x] Split normal HUD and debug HUD; move detailed debug stats behind F9/N.
- [x] Keep F8 unassigned because it conflicts with PIE Eject.
- [x] Set runtime point light to Movable.
- [x] Add automation tests for title blocking combat, new-game reset, retry flow, and settings defaults.
- [x] Live Codingなし Development Editor / Win64 build succeeded.
- [x] Automation RunTests `AdaptiveHorror`: 21 tests succeeded.
- [x] Runtime smoke with NullRHI succeeded; game loads into Title state.
- [x] `git diff --check` passed with CRLF warnings only.
- [ ] PIE: confirm title screen is visible and readable.
- [ ] PIE: confirm NEW GAME starts gameplay and hides cursor.
- [ ] PIE: confirm Esc pause/resume does not create duplicate widgets.
- [ ] PIE: confirm Settings values apply and persist after returning to menu.
- [ ] PIE: confirm Player Death opens Game Over and Retry restores checkpoint safely.
- [ ] PIE: confirm Adam defeat opens the new Stage Clear widget.
- [ ] PIE: confirm Return to Title works from Pause / Game Over / Stage Clear.
- [ ] PIE: confirm second New Game has no stale Adam / HUNTER / Stage Clear / telemetry state.
- [ ] PIE: confirm UI tones are audible.
- [ ] Implement real BGM/SFX assets or structured sound components for gameplay/enemy/boss events.
- [ ] Apply fullscreen/window mode, resolution, and graphics quality settings for real.
- [ ] Add confirmation dialog for Return to Title / Exit Game if needed.

## Cycle 015 Title widget display / PIE input flow fix

- [x] Investigate PIE lock state where Title flow starts but no Title buttons are visible.
- [x] Move C++ menu root creation from `NativeConstruct()` to `RebuildWidget()`.
- [x] Add native menu root / NativeConstruct / initial focus debug helpers.
- [x] Assign focus to primary menu buttons.
- [x] Add `[TitleUI]` logs for class validity, CreateWidget, AddToViewport, IsInViewport, visibility, opacity, root validity, NativeConstruct, focus, LocalPlayer, viewport client, and failure reason.
- [x] Add detailed `[GameFlow]` logs with world/net/controller/local player/pawn details.
- [x] Add `[InputState]` logs for input mode, cursor, ignore move/look, pause, and time dilation.
- [x] Add `[Player]` log after New Game possession/input recovery.
- [x] Make Title/Settings menu input explicitly block gameplay input.
- [x] Add Development fallback from failed local Title UI display to playable mode.
- [x] Keep controller-less automation Title tests from triggering fallback.
- [x] Live Codingなし Development Editor / Win64 build succeeded.
- [x] Automation RunTests `AdaptiveHorror`: 21 tests succeeded.
- [x] Runtime smoke succeeded.
- [x] Runtime log confirmed Title Widget creation and viewport attachment:
  - `TitleWidgetClassValid=true`
  - `CreateWidgetResult=true`
  - `RootWidgetValid=true`
  - `IsInViewport=true`
  - `Visibility=Visible`
  - `FocusAssigned=true`
  - `InputMode=GameAndUI`
  - `ShowMouseCursor=true`
  - `IgnoreMoveInput=true`
  - `IgnoreLookInput=true`
- [ ] PIE: confirm title text/buttons are visible.
- [ ] PIE: click NEW GAME and confirm `Title -> Playing`.
- [ ] PIE: after NEW GAME confirm cursor hidden, GameOnly input, IgnoreMove=false, IgnoreLook=false.
- [ ] PIE: confirm possessed pawn is `EvaPlayerCharacter`.
- [ ] PIE: confirm WASD, mouse look, shooting, and Esc Pause work after NEW GAME.
# TODO — Adaptive Horror FPS Demo

## Cycle 016 Visual / Audio Pass 1

- [x] Create `feature/visual-audio-pass1` from stable `main`.
- [x] Improve enemy silhouettes using Engine standard primitive meshes only.
- [x] Add visual body parts for legs and shoulders without changing capsules/nav agents.
- [x] Add simple idle/walk/attack motion through component rotation.
- [x] Make ADAM Attack / Charge / Roar visually distinct through temporary poses.
- [x] Add shared procedural audio helper for replaceable prototype tones.
- [x] Add temporary tones for UI reuse, gun, reload, damage, enemy attack/death, HUNTER spawn, ADAM cues, and facility ambience.
- [x] Darken runtime facility lighting and add Movable emergency lights.
- [x] Preserve runtime graybox geometry mobility to avoid NavMesh behavior changes.
- [x] Development Editor / Win64 build without Live Coding succeeded.
- [x] Automation RunTests `AdaptiveHorror` succeeded: 23 success, 0 failures.
- [x] Runtime smoke succeeded: exit code 0.

### Next manual PIE checks

- [ ] Confirm Zombie / FAST / ARMORED / LONG ARM / COMPOSITE / HUNTER / ADAM silhouettes are distinguishable in viewport.
- [ ] Confirm idle/walk/attack motion is visible without breaking combat.
- [ ] Confirm ADAM Attack / Charge / Roar are visually distinguishable.
- [ ] Confirm temporary UI/gameplay/enemy/boss tones are audible and not too loud.
- [ ] Confirm emergency lighting is darker/readable and no unwanted “Lighting needs to be rebuilt” warning appears.
- [ ] Confirm NavMesh / zombie chase / Stage Clear / Game Flow remain unchanged in PIE.
