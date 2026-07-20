# TEST_PLAN.md — Adaptive Horror Verification

This file lists commands that exist in this repository or have been verified in prior logs. Do not mark PIE/visual/audio feel as passed unless a human confirms it.

## Environment

- Project root: repository root containing `AdaptiveHorror.uproject`.
- Primary platform: Windows / UE5.8 / Development Editor / Win64.
- Preferred shell: PowerShell.
- Close Unreal Editor and Live Coding Console before Development Editor builds.
- If memory pressure occurs, lower `-MaxParallelActions` to `2` or `1`.

## 1. Development Editor Build

Command:

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\RunBuildCheck.ps1 -MaxParallelActions 4 -SkipAutomation
```

Prerequisites:

- Unreal Editor/UBT installed or discoverable via standard Epic path or `UE_ROOT`.
- Visual Studio C++ toolchain installed.

Success conditions:

- Static source sanity passes.
- Generate Project Files succeeds.
- `AdaptiveHorrorEditor Win64 Development` build succeeds.
- No C++/UHT/link error.

Logs:

- UBT logs: `%LOCALAPPDATA%\UnrealBuildTool\Log.txt`
- Project logs: `Saved\Logs\AdaptiveHorror.log`

Timeout guide:

- 5-20 minutes depending on machine and cache state.

If impossible:

- Record missing tool path (`UnrealEditor.exe`, `UnrealBuildTool.exe`, `MSBuild.exe`, `cl.exe`) and do not mark build as passed.

## 2. AdaptiveHorror Automation

Command:

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\RunBuildCheck.ps1 -MaxParallelActions 4
```

This runs build plus:

```powershell
Automation RunTests AdaptiveHorror
```

Success conditions:

- Command exits 0.
- Latest automation log contains `**** TEST COMPLETE. EXIT CODE: 0 ****`.
- No project test result contains `Result={Fail}`.
- Current project count at time of this document: 43 tests. Treat lower counts as suspicious unless intentionally changed.

Logs:

- `Saved\Logs\AdaptiveHorror.log`
- `Saved\Logs\AdaptiveHorror-backup-*.log` if runtime smoke later overwrites the active log.

Timeout guide:

- 5-15 minutes after build is hot.

Failure triage:

- Fix the first real project failure.
- Engine/internal automation self-test errors can appear in old logs; use the `Automation RunTests AdaptiveHorror` session result and exit code as primary evidence.

## 3. Runtime Smoke

Command:

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "C:\Users\shinn\Documents\Codex\2026-06-23\unreal-engine-5-fps-30-60\AdaptiveHorror.uproject" `
  -game -Unattended -NullRHI -NoSound -NoSplash -ExecCmds="Quit" -log
```

Success conditions:

- Exit code 0.
- Game loads `/Engine/Maps/Entry`.
- `AEvaPrototypeGameMode` initializes.
- No Fatal / Ensure / Assertion / EXCEPTION / access violation.

Expected limitation:

- This starts and exits without player input. It cannot verify visual readability, audio, controls, combat feel, or full PIE progression.

Timeout guide:

- 1-5 minutes.

## 4. Log validation

Recommended scan concepts:

- `Fatal`
- `Ensure`
- `Assertion`
- `EXCEPTION`
- `Stack overflow`
- `Access violation`
- `NavReady=false`
- `Result={Fail}`
- `Automation Test failed`
- `Stage Clear` followed by unexpected death/game-over transition
- Runtime actor duplicate warnings relevant to the task

PowerShell example:

```powershell
Select-String -Path .\Saved\Logs\AdaptiveHorror*.log `
  -Pattern 'Fatal error|LogOutputDevice: Error: Ensure|Ensure condition failed|Ensure failed|Assertion failed|EXCEPTION|Stack overflow|Access violation|NavReady=false|Result=\{Fail|Automation Test failed' |
  Select-Object -Last 120
```

Do not treat normal context strings such as `AfterEnsurePrototypePlayer` as an ensure failure.

Success conditions:

- No current-run project fatal/crash/assert/ensure.
- No current-run AdaptiveHorror automation failure.
- Task-specific required logs are present when the task defines them.

## 5. Whitespace diff check

Command:

```powershell
git diff --check
```

Success conditions:

- Exit code 0.
- CRLF conversion warnings are acceptable.
- Whitespace errors are not acceptable.

## 6. Git status

Command:

```powershell
git status --short --branch
```

Success conditions at handoff:

- Correct feature/chore branch.
- Clean working tree after commit.
- Branch pushed to origin when the task requires it.

## 7. Markdown/document validation

Use this for documentation-only tasks when the task does not require a full UE build.

Commands:

```powershell
git diff --check
git diff --name-only
```

Minimum checks:

- No whitespace errors from `git diff --check`.
- No game source/config files changed when the task is documentation-only.
- Markdown fenced code blocks are balanced in edited `.md` files.
- No leftover citation artifact markers from copied assistant output.

If a dedicated Markdown linter is introduced later, record the real command here before requiring it in tasks.

## 8. One-command Codex validation

Command:

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\RunCodexValidation.ps1 -MaxParallelActions 4
```

This script runs:

1. `Scripts/RunBuildCheck.ps1`
2. Runtime Smoke
3. Log scan
4. `git diff --check`

Optional skips:

```powershell
-SkipBuildCheck
-SkipAutomation
-SkipRuntimeSmoke
-SkipLogScan
-SkipDiffCheck
```

Success conditions:

- Script exit code 0.
- Any skipped item is reported as skipped, not passed.

## 9. Human PIE verification

Human-only checks include:

- Actual title/pause/settings/game-over/stage-clear layout.
- Mouse/keyboard feel.
- Weapon feel.
- Horror readability and darkness.
- Enemy spawn presentation.
- Zone visual identity and sightline guidance.
- Full New Game to Stage Clear playthrough.

Codex may commit a completion candidate when all automated criteria pass and the task explicitly allows PIE items to remain unverified. Main merge should wait for human PIE acceptance.
