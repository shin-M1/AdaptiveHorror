# Adaptive Horror FPS Demo

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
