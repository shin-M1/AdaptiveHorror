param(
    [string]$ProjectPath = "$PSScriptRoot\..\AdaptiveHorror.uproject",
    [int]$MaxParallelActions = 4,
    [switch]$SkipBuildCheck,
    [switch]$SkipAutomation,
    [switch]$SkipRuntimeSmoke,
    [switch]$SkipLogScan,
    [switch]$SkipDiffCheck
)

$ErrorActionPreference = "Continue"

function Write-Step {
    param([string]$Message)
    Write-Host ""
    Write-Host "== $Message ==" -ForegroundColor Cyan
}

function Resolve-FirstExistingPath {
    param([string[]]$Candidates)
    foreach ($Candidate in $Candidates) {
        if ($Candidate -and (Test-Path -LiteralPath $Candidate)) {
            return (Resolve-Path -LiteralPath $Candidate).Path
        }
    }
    return $null
}

function Find-UnrealEditorCmd {
    $Candidates = @()
    if ($env:UE_ROOT) {
        $Candidates += Join-Path $env:UE_ROOT "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
    }
    $Candidates += Get-ChildItem -Path "C:\Program Files\Epic Games" -Directory -Filter "UE_5*" -ErrorAction SilentlyContinue |
        Sort-Object Name -Descending |
        ForEach-Object { Join-Path $_.FullName "Engine\Binaries\Win64\UnrealEditor-Cmd.exe" }
    $Candidates += Get-Command UnrealEditor-Cmd.exe -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source
    return Resolve-FirstExistingPath $Candidates
}

function Invoke-CheckedCommand {
    param(
        [string]$Label,
        [scriptblock]$Command
    )

    Write-Step $Label
    & $Command
    $ExitCode = $LASTEXITCODE
    if ($null -eq $ExitCode) {
        $ExitCode = 0
    }
    if ($ExitCode -ne 0) {
        Write-Host "FAIL: $Label exited with code $ExitCode" -ForegroundColor Red
        exit $ExitCode
    }
    Write-Host "PASS: $Label"
}

Write-Step "Project"
$ResolvedProject = Resolve-Path -LiteralPath $ProjectPath -ErrorAction SilentlyContinue
if (-not $ResolvedProject) {
    Write-Host "FAIL: Project file not found: $ProjectPath" -ForegroundColor Red
    exit 1
}
$ProjectRoot = Split-Path $ResolvedProject.Path -Parent
Write-Host "Project: $($ResolvedProject.Path)"

if (-not $SkipBuildCheck) {
    $BuildCheckScript = Join-Path $ProjectRoot "Scripts\RunBuildCheck.ps1"
    if (-not (Test-Path -LiteralPath $BuildCheckScript)) {
        Write-Host "FAIL: Build check script not found: $BuildCheckScript" -ForegroundColor Red
        exit 1
    }

    Invoke-CheckedCommand "Build check" {
        $Arguments = @(
            "-ExecutionPolicy", "Bypass",
            "-File", $BuildCheckScript,
            "-ProjectPath", $ResolvedProject.Path,
            "-MaxParallelActions", $MaxParallelActions
        )
        if ($SkipAutomation) {
            $Arguments += "-SkipAutomation"
        }
        & powershell @Arguments
    }
} else {
    Write-Step "Build check"
    Write-Host "SKIP: -SkipBuildCheck was provided." -ForegroundColor Yellow
}

if (-not $SkipRuntimeSmoke) {
    $EditorCmd = Find-UnrealEditorCmd
    if (-not $EditorCmd) {
        Write-Step "Runtime Smoke"
        Write-Host "FAIL: UnrealEditor-Cmd.exe not found. Runtime Smoke cannot run." -ForegroundColor Red
        exit 1
    }

    Invoke-CheckedCommand "Runtime Smoke" {
        & $EditorCmd $ResolvedProject.Path -game -Unattended -NullRHI -NoSound -NoSplash -ExecCmds="Quit" -log
    }
} else {
    Write-Step "Runtime Smoke"
    Write-Host "SKIP: -SkipRuntimeSmoke was provided." -ForegroundColor Yellow
}

if (-not $SkipLogScan) {
    Write-Step "Log scan"
    $LogPath = Join-Path $ProjectRoot "Saved\Logs\AdaptiveHorror.log"
    if (-not (Test-Path -LiteralPath $LogPath)) {
        Write-Host "FAIL: Runtime log not found: $LogPath" -ForegroundColor Red
        exit 1
    }

    $Pattern = "Fatal error|LogOutputDevice: Error: Ensure|Ensure condition failed|Ensure failed|Assertion failed|EXCEPTION|Stack overflow|Access violation|NavReady=false|Result=\{Fail|Automation Test failed"
    $Matches = @(Select-String -Path $LogPath -Pattern $Pattern -CaseSensitive:$false)
    if ($Matches.Count -gt 0) {
        Write-Host "FAIL: log scan found blocking pattern(s)." -ForegroundColor Red
        $Matches | Select-Object -Last 80 | ForEach-Object { Write-Host $_.Line }
        exit 1
    }
    Write-Host "PASS: no blocking runtime log patterns found in $LogPath"
} else {
    Write-Step "Log scan"
    Write-Host "SKIP: -SkipLogScan was provided." -ForegroundColor Yellow
}

if (-not $SkipDiffCheck) {
    Invoke-CheckedCommand "git diff --check" {
        Push-Location $ProjectRoot
        try {
            & git diff --check
        } finally {
            Pop-Location
        }
    }
} else {
    Write-Step "git diff --check"
    Write-Host "SKIP: -SkipDiffCheck was provided." -ForegroundColor Yellow
}

Write-Step "git status"
Push-Location $ProjectRoot
try {
    & git status --short --branch
} finally {
    Pop-Location
}

Write-Step "Done"
Write-Host "Codex validation completed."
