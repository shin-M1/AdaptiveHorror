# BUILD_CHECK — UE5実環境ビルド検証

## Cycle 009 実行結果 — 全Automation導線更新後

2026-07-12に、UE5.8環境で以下を実行しました。

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\RunBuildCheck.ps1 -MaxParallelActions 1
```

結果:

- Static source sanity: PASS
- Generate Project Files: Succeeded
- Development Editor / Win64 Build: Succeeded
- Automation RunTests `AdaptiveHorror`: 15件すべてSuccess
- `TEST COMPLETE. EXIT CODE: 0`

補足:

- `RunBuildCheck.ps1` のAutomation対象を `AdaptiveHorror.EVA` から `AdaptiveHorror` 全体へ変更しました。
- `-MaxParallelActions 2` 実行時にUBAがLow memoryでcompile processをkill/retryしたため、メモリに余裕がない場合は `-MaxParallelActions 1` を推奨します。
- 初回ビルドでは `GameFramework/PlayerCameraManager.h` includeパス不備が出ましたが、UE5.8向けに `Camera/PlayerCameraManager.h` へ修正済みです。
- Runtime smokeでは `/Engine/Maps/Entry` 起動、GameMode、Navigation Build、初期ゾンビBeginPlay、Fatalなしを確認しました。
- PIE目視確認はEditor上で継続してください。特に障害物回避、頭上ラベル、敵タイプ識別、HUNTER、ADAMはAutomation成功だけでは完了扱いにしません。

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

今回のスクリプトは、UE未検出でも `.generated.h` include順序と `.uproject` JSONを静的確認します。UE実環境ではBuild後に `AdaptiveHorror` Automation Testも実行します。

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
9. `AdaptiveHorror` Automation Test
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
  -ExecCmds="Automation RunTests AdaptiveHorror; Quit" `
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
| Automation Testが0件 | Test Filter不一致、WITH_DEV_AUTOMATION_TESTS無効 | `AdaptiveHorror` filterを確認 |

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

## Cycle 011 Build Check Note - 2026-07-12

Command used:

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\RunBuildCheck.ps1 -MaxParallelActions 2
```

Observed result:

- Static source sanity: PASS.
- Generate Project Files: Succeeded.
- Development Editor / Win64 build without Live Coding: Succeeded.
- Automation RunTests `AdaptiveHorror`: exit code 0.
- Latest automation log confirmed 15 successful tests and 0 failures.
- Runtime smoke commandlet launch with `-game -NullRHI -ExecCmds="Quit"` exited with code 0.

Crash-specific note:

- The Adam chase crash was classified by UE CrashContext as `EXCEPTION_STACK_OVERFLOW`.
- First AdaptiveHorror frame: `AEvaZombieAIController::OnMoveCompleted`.
- Repeating stack: `OnMoveCompleted -> ReissueMoveToTarget -> MoveToActor -> OnMoveCompleted`.
- Fix: guard reentrant MoveTo reissue and stamp `LastMoveRequestTime` before calling `MoveToActor`.
- PIE visual verification is still required in Unreal Editor; Codex did not visually confirm the Adam chase regression.

Troubleshooting if Adam chase still crashes:

1. Search the latest PIE log for `EXCEPTION_STACK_OVERFLOW`.
2. Search for same-frame floods of `[AIPath] MoveCompleted` and `[AIRepath] Reason=MoveCompleted`.
3. If the stack still includes `AEvaZombieAIController::OnMoveCompleted` and `ReissueMoveToTarget`, inspect any MoveTo calls that are not protected by `bIssuingRepathMove`.
4. If the stack moves to Adam-specific functions, inspect `AEvaAdamBossAIController::TryChargeAttack`, `TryRoarSummon`, and `AEvaAdamBossCharacter::SpawnRoarMinions` separately.

## Cycle 013 Build Check Note - 2026-07-12

Command used:

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\RunBuildCheck.ps1 -MaxParallelActions 2
```

Observed result:

- Static source sanity: PASS.
- Generate Project Files: Succeeded.
- First compile pass found one include issue in the newly added automation test:
  - `EvaLearningTests.cpp` spawned `AEvaPlayerCharacter` without including `Characters/EvaPlayerCharacter.h`.
  - Added the include and rebuilt.
- Development Editor / Win64 build without Live Coding: Succeeded.
- Automation RunTests `AdaptiveHorror`: exit code 0.
- Latest automation log confirmed 19 successful tests and 0 failures.

Runtime smoke:

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "C:\Users\shinn\Documents\Codex\2026-06-23\unreal-engine-5-fps-30-60\AdaptiveHorror.uproject" `
  -game `
  -Unattended `
  -NullRHI `
  -NoSound `
  -NoSplash `
  -ExecCmds="Quit" `
  -log
```

- Result: exit code 0.
- Note: a prior relative-path command using `.\\AdaptiveHorror.uproject` failed to locate the descriptor. Use the absolute `.uproject` path for commandlet smoke checks.
- Runtime log confirmed `/Engine/Maps/Entry` loaded, runtime navigation built, initial zombie spawned, and the engine exited cleanly via `Quit`.

Stage Clear regression checks added:

- `AdaptiveHorror.StageClear.RejectsPlayerDeath`
- `AdaptiveHorror.StageClear.SkipsSpawns`
- `AdaptiveHorror.StageClear.StopsEnemyCombat`
- `AdaptiveHorror.StageClear.Idempotent`

Manual PIE checks still required:

1. Defeat ADAM and confirm Stage Clear appears.
2. Wait after Stage Clear and confirm HP no longer decreases.
3. Confirm residual enemies do not chase/attack after Stage Clear.
4. Confirm no GAME OVER / checkpoint respawn starts after Stage Clear.
5. Confirm look input remains available while movement/shooting are disabled.
6. Confirm Boss HUD and enemy overhead HP bars hide after Stage Clear.

## Cycle 014 execution result - core UI flow

Date: 2026-07-14

Commands:

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\RunBuildCheck.ps1
```

Runtime smoke:

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "C:\Users\shinn\Documents\Codex\2026-06-23\unreal-engine-5-fps-30-60\AdaptiveHorror.uproject" `
  -game -Unattended -NullRHI -NoSound -NoSplash -ExecCmds="Quit" -log
```

Results:

- Static source sanity: PASS.
- Generate Project Files: Succeeded.
- Development Editor / Win64 build without Live Coding: Succeeded.
- Automation RunTests `AdaptiveHorror`: Succeeded.
  - Tests run: 21.
  - Success: 21.
  - Failures: 0.
  - Latest automation log: `**** TEST COMPLETE. EXIT CODE: 0 ****`.
- Runtime smoke: exit code 0.
  - `EvaPrototypeGameMode` loaded.
  - Game flow transitioned `Loading -> Title`.
- `git diff --check`: no whitespace errors; CRLF conversion warnings only.

Notes:

- UE5.8 commandlet launch still prints SDK validation warnings for non-Win64 platforms such as LinuxArm64 and VisionOS. Win64 is valid and the final build/test/runtime commands returned exit code 0.
- PIE viewport confirmation is still required for visual menu layout, cursor/input modes, audible procedural UI tones, and the complete Title -> New Game -> Pause -> Game Over -> Stage Clear -> Title loop.

## Cycle 015 execution result - title widget display / PIE input flow

Date: 2026-07-14

Command:

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\RunBuildCheck.ps1
```

Runtime smoke:

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "C:\Users\shinn\Documents\Codex\2026-06-23\unreal-engine-5-fps-30-60\AdaptiveHorror.uproject" `
  -game -Unattended -NullRHI -NoSound -NoSplash -ExecCmds="Quit" -log
```

Results:

- Static source sanity: PASS.
- Generate Project Files: Succeeded.
- Development Editor / Win64 build without Live Coding: Succeeded.
- Automation RunTests `AdaptiveHorror`: Succeeded.
  - Tests run: 21.
  - Success: 21.
  - Failures: 0.
  - Latest automation log: `**** TEST COMPLETE. EXIT CODE: 0 ****`.
- Runtime smoke: exit code 0.
- Runtime log confirmed Title UI creation path:
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

Notes:

- UE5.8 commandlets still emit non-Win64 SDK validation warnings; Win64 remains valid and the command exits with code 0.
- PIE viewport visual confirmation is still required for the actual title screen and NEW GAME click path.
