# Design Decisions

## DD-007 — Runtime GrayboxではWhole World NavigationとAI直接移動フォールバックを併用する

- 日付: 2026-07-11
- 状態: 採用
- 背景: UE5.8の短時間起動検証で、Runtime Spawnした `ANavMeshBoundsVolume` がempty bounds扱いになり、AIの `MoveTo` が失敗してゾンビ/HUNTER/ADAMの追跡が止まる可能性が見つかった。
- 決定: 体験版Ver0.1のRuntime Grayboxでは、`AEvaPrototypeGameMode` が `UNavigationSystemV1::bWholeWorldNavigable = true` を設定してNavigation Buildを行う。さらにAI Controller側で、PathFollowingが失敗した場合だけ `AddMovementInput()` による直接移動フォールバックを使う。
- 影響: NavMeshの有無に依存しすぎず、PIE検証時に敵が完全停止しにくくなる。一方で、正式な `.umap` 化後はEditorで保存されたNavMesh BoundsとNavMesh確認を優先し、直接移動フォールバックは最後の保険として扱う。

## DD-005 — 研究施設6区画もRuntime Grayboxで生成する

- 日付: 2026-06-23
- 状態: 採用
- 理由: 現在の開発環境ではUnreal Editorが検出できず、`.umap` を安全に作成・保存できないため。
- 決定: `AEvaPrototypeGameMode` が `/Engine/Maps/Entry` 上に6区画の研究施設GrayboxをRuntime生成する。進行管理は `AEvaResearchFacilityDirector` に分離する。
- 影響: Editor導入後はRuntime生成ロジックを参考に `L_ResearchFacility_Prototype.umap` へ移植できる。Cover / HideSpot / EscapeRoute / AmbushPoint / Zone Trigger / Checkpoint の配置ルールはそのまま利用する。

## DD-006 — アダムBossはゾンビ基盤を継承して最小実装する

- 日付: 2026-06-23
- 状態: 採用
- 理由: 体験版Ver0.1では、専用Skeletal MeshやBehavior Treeよりも「追跡、攻撃、Phase 2、撃破クリア」が成立することを優先するため。
- 決定: `AEvaAdamBossCharacter` は `AEvaZombieCharacter` を継承し、HP/Phase/咆哮召喚/Stage Clear通知を追加する。AIは `AEvaAdamBossAIController` のTick制御で実装する。
- 影響: 見た目と演出は仮。次回以降、専用Material、弱点露出、咆哮演出、サウンド、Behavior Tree化を行う。

## DD-001 — Engine Associationを固定しない

- 日付: 2026-06-23
- 状態: 採用
- 理由: Epic Launcher manifest、一般的なUE配置、レジストリにEngineがなく、実在しないバージョンの関連付けを推測しないため。
- 決定: `.uproject` の `EngineAssociation` を省略し、UE5.4以降へ手動関連付けできる形にする。
- 影響: 初回起動時にEngine選択が必要。Engine導入後、採用版を確認して固定する。

## DD-002 — 最初のMapはRuntime Grayboxとする

- 日付: 2026-06-23
- 状態: 暫定採用
- 理由: `.umap` はバイナリアセットであり、Unreal Editorがない環境で安全に生成・保存できないため。
- 決定: `/Engine/Maps/Entry` を起動し、`AEvaPrototypeGameMode` が床、壁、遮蔽物、照明、NavMesh Bounds、PlayerStart、Zombie、Checkpoint、Pickupを生成する。
- 影響: C++だけで縦切りを開始できる。Editor導入後は `L_DevGym.umap` に保存し、Runtime生成を無効化する。

## DD-003 — Enhanced Input MappingをC++で仮生成する

- 日付: 2026-06-23
- 状態: 暫定採用
- 理由: Input Action／Mapping ContextアセットをEditorなしで作成できないため。
- 決定: `AEvaPlayerCharacter` が実行時にEnhanced Input ActionとMapping Contextを生成する。
- 影響: 基本操作はContent依存なしで動く。Editor利用可能後、同じ入力を `/Game/AdaptiveHorror/Core/Input/` のData Assetへ移し、キー設定拡張を可能にする。

## DD-004 — 最小AIはController Tickで成立させる

- 日付: 2026-06-23
- 状態: 暫定採用
- 理由: Behavior Tree／Blackboardアセットなしでも、感知→追跡→攻撃の縦切りを成立させるため。
- 決定: `AEvaZombieAIController` がAI PerceptionとNavMesh MoveToを直接制御する。
- 影響: 通常ゾンビは動作可能な最小構成になる。本格HUNTER実装時にBlackboard／Behavior Treeへ移行し、学習プロファイル読取点をTask／Serviceへ分離する。
## DD-008 - Centralize Enemy Spawning Through Safe Spawn Search

- Date: 2026-07-12
- Status: Adopted
- Context: PIE logs showed multiple zombies becoming stuck with capsule penetration. Existing enemy paths used fixed transforms and `AdjustIfPossibleButAlwaysSpawn`, so waves and boss/minion spawns could stack pawns or place them inside runtime graybox collision.
- Decision: Route initial zombie, zone waves, adaptive waves, HUNTER, ADAM, and ADAM roar minions through `AEvaPrototypeGameMode::FindSafeEnemySpawnLocation(...)` and `SpawnEnemyNearLocation(...)`.
- Collision policy: enemy spawns use `AdjustIfPossibleButDontSpawnIfColliding`. A post-spawn overlap can relocate once; otherwise the spawned enemy is destroyed and the failure is logged.
- Debug policy: every spawn attempt logs requested/final locations, NavMesh/floor/overlap state, controller possession, player distance, and nearest enemy distance. HUD exposes the last spawn result and enemy counts.
- Consequence: runtime graybox combat is more deterministic and easier to debug. Long-term, a saved map with authored NavMesh bounds should replace the whole-world navigation fallback.

## DD-009 - Keep Direct Movement as Short-Term Fallback Only

- Date: 2026-07-12
- Status: Adopted
- Context: direct `AddMovementInput` fallback kept enemies moving when runtime NavMesh failed, but could push them into other enemies/walls.
- Decision: keep NavMesh `MoveTo` as primary. Direct fallback now mixes player direction with a separation vector, performs a short forward obstacle trace, and increments debug counters when used.
- Consequence: enemies should no longer deepen capsule penetration while fallback is active. The counter remains visible in HUD so fallback reliance can be reduced during the future authored-map pass.
