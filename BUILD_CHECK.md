# BUILD_CHECK — UE5実環境ビルド検証

## Cycle 007 実行結果 — ゲームループ安定化後

2026-07-11に、UE5.8環境で以下を実行しました。

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\RunBuildCheck.ps1 -MaxParallelActions 2
```

結果:

- Static source sanity: PASS
- Generate Project Files: Succeeded
- Development Editor / Win64 Build: Succeeded
- `AdaptiveHorror.EVA` Automation Test: 8件すべてSuccess
- 短時間Standalone起動: `/Engine/Maps/Entry` が `AEvaPrototypeGameMode` で起動し、Fatal/Crashなし

補足:

- `Saved/Logs/AdaptiveHorror.log` または直後にStandalone起動した場合の `Saved/Logs/AdaptiveHorror-backup-*.log` で、`AdaptiveHorror.EVA.*` の8件がSuccess、`TEST COMPLETE. EXIT CODE: 0` を確認しました。
- AutomationログにはUEエンジン側の内部自己検証ログが混ざることがありますが、プロジェクト対象テストの終了コードは0でした。
- Runtime GrayboxのNavigation初期化を修正したため、以前出ていた `Navigation bounds update` のempty bounds警告は短時間起動ログ上では再発していません。
- この環境からEditor viewportを直接操作するPIE手動検証はできないため、通常ゾンビ/HUNTER/EVA進化/6区画進行/ADAM戦の完全なPIE手動確認は未完了です。Editor上で下記の「PIE手動確認」を続行してください。
- UBAやコンパイルプロセスがメモリ不足でkill/retryされる場合は、`-MaxParallelActions 2` を優先してください。余裕がある環境では4でも可です。

### Cycle 007 PIE手動確認で重点確認する項目

1. 通常ゾンビがプレイヤーを追跡・攻撃できること。
2. プレイヤー死亡後、3秒復帰と入力再開が重複Timerなしで成立すること。
3. HUNTER出現・撃破・30秒後Tier+1再投入が重複なしで成立すること。
4. EVA解析率20/40/60/80%で進化個体が重複せず出現すること。
5. Entry LobbyからAdam Arenaまで進行不能なしで到達できること。
6. ADAM撃破でStage Clearが一度だけ発火すること。

このプロジェクトをWindows上のUnreal Engine 5環境で検証するための手順です。

## Cycle 004 実行結果

2026-07-10に現在のCodex環境で以下を実行しました。

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\RunBuildCheck.ps1
```

結果:

- Project: `AdaptiveHorror.uproject` 検出成功。
- Static source sanity: PASS。
- `UnrealEditor.exe`: NOT FOUND。
- `UnrealBuildTool.exe`: NOT FOUND。
- `MSBuild.exe`: NOT FOUND。
- `cl.exe`: NOT FOUND。
- UE toolchain未検出のため、Generate Project Files / Development Editor Build / Automation Test / PIEは未実行。

今回のスクリプトは、UE未検出でも `.generated.h` include順序と `.uproject` JSONを静的確認します。UE実環境ではBuild後に `AdaptiveHorror.EVA` Automation Testも実行します。

## Cycle 005 / UE5.8 実行結果

2026-07-11にUE5.8実環境で以下を実行しました。

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\RunBuildCheck.ps1 -MaxParallelActions 4
```

結果:

- Generate Project Files: Succeeded。
- Development Editor Build / Win64: Succeeded。
- `AdaptiveHorror.EVA` Automation Test: 8件すべてSuccess。
- PIE Smoke Test: 未実行。Editorで手動確認が必要。

UE5.8では `DefaultBuildSettings = BuildSettingsVersion.V7` を使用します。メモリ逼迫でUBAがcompile processをkill/retryする場合があるため、検証時は `-MaxParallelActions 4` を推奨します。

## 1. 前提

- Windows 10/11
- Unreal Engine 5.x
- Visual Studio 2022
- `Desktop development with C++`
- Windows 10/11 SDK
- MSVC v143 toolchain

gitはこの作業では不要です。

## 2. 自動チェック

PowerShellでプロジェクトルートから実行します。

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\RunBuildCheck.ps1 -MaxParallelActions 4
```

スクリプトは以下を順に試します。

1. `.uproject` の存在確認
2. Static source sanity
   - `.generated.h` include順序確認
   - `.uproject` JSON確認
3. `UnrealEditor.exe` 探索
4. `UnrealBuildTool.exe` 探索
5. `MSBuild.exe` 探索
6. `cl.exe` 探索
7. Generate Project Files
8. `AdaptiveHorrorEditor` Development Editor Build
9. `AdaptiveHorror.EVA` Automation Test
10. PIE Smoke Test確認手順の提示

UEやMSVCが見つからない場合、そこで停止せず、見つからない理由と次の手動確認先を表示します。

## 3. UnrealEditor.exe 探索候補

代表的な候補:

- `C:\Program Files\Epic Games\UE_5.0\Engine\Binaries\Win64\UnrealEditor.exe`
- `C:\Program Files\Epic Games\UE_5.1\Engine\Binaries\Win64\UnrealEditor.exe`
- `C:\Program Files\Epic Games\UE_5.2\Engine\Binaries\Win64\UnrealEditor.exe`
- `C:\Program Files\Epic Games\UE_5.3\Engine\Binaries\Win64\UnrealEditor.exe`
- `C:\Program Files\Epic Games\UE_5.4\Engine\Binaries\Win64\UnrealEditor.exe`
- `C:\Program Files\Epic Games\UE_5.5\Engine\Binaries\Win64\UnrealEditor.exe`
- `C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor.exe`

環境変数 `UE_ROOT` がある場合は、以下も確認します。

- `%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe`

## 4. MSBuild / cl.exe 確認

Visual Studio Installerで以下が入っているか確認します。

- Desktop development with C++
- MSVC v143
- Windows SDK

手動確認:

```powershell
where MSBuild.exe
where cl.exe
```

見つからない場合は、Developer PowerShell for VS 2022から再実行してください。

## 5. Generate Project Files

`UnrealBuildTool.exe` が見つかった場合:

```powershell
& "<UE>\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" `
  -projectfiles `
  -project="C:\path\to\AdaptiveHorror.uproject" `
  -game `
  -engine
```

## 6. Development Editor Build

```powershell
& "<UE>\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" `
  AdaptiveHorrorEditor Win64 Development `
  -Project="C:\path\to\AdaptiveHorror.uproject" `
  -WaitMutex `
  -FromMsBuild
```

## 7. Automation Test

EditorまたはCommandletで以下を実行します。

```powershell
& "<UE>\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "C:\path\to\AdaptiveHorror.uproject" `
  -ExecCmds="Automation RunTests AdaptiveHorror.EVA; Quit" `
  -unattended `
  -nop4 `
  -nosplash
```

対象テスト:

- `AdaptiveHorror.EVA.TelemetryClassification`
- `AdaptiveHorror.EVA.HunterLearningMultiplier`
- `AdaptiveHorror.EVA.AnalysisRateIncrease`
- `AdaptiveHorror.EVA.EvolutionThresholds`
- `AdaptiveHorror.EVA.DirectorProgression`
- `AdaptiveHorror.EVA.StoryLogPickup`
- `AdaptiveHorror.EVA.AdamPhaseTransition`
- `AdaptiveHorror.EVA.AdamDefeatStageClear`

## 8. PIE Smoke Test

Unreal Editorで `AdaptiveHorror.uproject` を開き、Playを押します。

確認項目:

- Entry Lobbyから開始する。
- FPS移動、視点操作、射撃、リロードができる。
- 6区画を順に進める。
- EVAログを拾うとHUDに表示される。
- HUNTERが出現し、撃破で学習倍率が0.3になる。
- HUNTERが再投入され、倍率が1.0に戻る。
- 解析率に応じて進化個体が出現する。
- Adam Arenaでアダムが出現する。
- アダムHP50%以下でPhase 2になる。
- アダム撃破でStage Clear表示が出る。

### Debugキー確認

Development / DebugGame / Editor環境で以下を確認します。Shippingでは無効化する前提です。

| キー | 確認内容 |
|---|---|
| F1 | EVA解析率が+20され、HUD/Debug表示に反映される |
| F2 | HUNTERが強制出現する |
| F3 | ゾンビWaveが強制スポーンする |
| F4 | Adam Arenaへワープし、ADAM戦が開始される |
| F5 | プレイヤーHPと弾薬が回復し、入力が戻る |
| F6 | Stage Clear表示になる |
| F7 | Shot/Hit/HS/Kill/Accuracy/EVA解析率のTelemetry Snapshotが表示される |

## 9. 原因切り分け表

| 症状 | 可能性 | 対応 |
|---|---|---|
| `UnrealEditor.exe` が見つからない | UE未インストール、または標準パス外 | Epic Games LauncherでUE5をインストール。`UE_ROOT` を設定 |
| `UnrealBuildTool.exe` が見つからない | UEインストール不完全 | UEを修復、または別バージョンのEngineパスを指定 |
| `MSBuild.exe` が見つからない | Visual Studio Build Tools不足 | Visual Studio InstallerでC++ workloadを追加 |
| `cl.exe` が見つからない | Developer環境未ロード | Developer PowerShell for VS 2022から実行 |
| Generate Project Files失敗 | `.uproject` JSON、Module名、Plugin不足 | `.uproject` と `Source/*.Target.cs` を確認 |
| UHT失敗 | `generated.h`順序、UCLASS/USTRUCT宣言ミス | エラー箇所のHeaderを修正 |
| Compile失敗 | UE minor API差分、include不足 | 最初のC++エラーから順に修正 |
| PIEでAIが動かない | NavMesh未生成、Runtime Bounds更新不足 | `Show Navigation` で確認。NavMeshBoundsVolumeを調整 |
| Automation Testが0件 | Test Filter不一致、WITH_DEV_AUTOMATION_TESTS無効 | `AdaptiveHorror.EVA` filterを確認 |

## 10. 記録ルール

検証結果は必ず `DEV_LOG.md` に残します。

- 実行したコマンド
- 成功/失敗
- 失敗した場合の最初のエラー
- 次の修正対象
## Cycle 008 Build Check Note - 2026-07-12

Command used:

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\RunBuildCheck.ps1 -MaxParallelActions 2
```

Observed result:

- Static source sanity: PASS.
- Generate Project Files: Succeeded.
- C++ compile reached the modified safe-spawn files.
- First pass found one compile error in `EvaPrototypeGameMode.cpp`: `ADirectionalLight::GetLightComponent()` returns `ULightComponent*`, so the code now casts to `UDirectionalLightComponent`.
- Second pass compiled the modified file, then failed at link because `UnrealEditor.exe` was holding `Binaries/Win64/UnrealEditor-AdaptiveHorror.dll`.

Required follow-up:

1. Close Unreal Editor and Live Coding Console.
2. Re-run `Scripts\RunBuildCheck.ps1 -MaxParallelActions 2`.
3. Run automation tests.
4. Perform PIE manual verification for safe enemy spawning and stuck-log reduction.
