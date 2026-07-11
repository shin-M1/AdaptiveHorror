param(
    [string]$ProjectPath = "$PSScriptRoot\..\AdaptiveHorror.uproject",
    [switch]$SkipAutomation,
    [int]$MaxParallelActions = 4
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

function Find-UnrealEditor {
    $Candidates = @()
    if ($env:UE_ROOT) {
        $Candidates += Join-Path $env:UE_ROOT "Engine\Binaries\Win64\UnrealEditor.exe"
    }
    $Candidates += Get-ChildItem -Path "C:\Program Files\Epic Games" -Directory -Filter "UE_5*" -ErrorAction SilentlyContinue |
        ForEach-Object { Join-Path $_.FullName "Engine\Binaries\Win64\UnrealEditor.exe" }
    $Candidates += Get-Command UnrealEditor.exe -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source
    return Resolve-FirstExistingPath $Candidates
}

function Find-UnrealBuildTool {
    param([string]$UnrealEditorPath)
    $Candidates = @()
    if ($UnrealEditorPath) {
        $EngineRoot = Split-Path (Split-Path (Split-Path $UnrealEditorPath -Parent) -Parent) -Parent
        $Candidates += Join-Path $EngineRoot "Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"
        $Candidates += Join-Path $EngineRoot "Binaries\DotNET\UnrealBuildTool.exe"
    }
    if ($env:UE_ROOT) {
        $Candidates += Join-Path $env:UE_ROOT "Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"
        $Candidates += Join-Path $env:UE_ROOT "Engine\Binaries\DotNET\UnrealBuildTool.exe"
    }
    return Resolve-FirstExistingPath $Candidates
}

function Find-MSBuild {
    $Candidates = @()
    $VsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path -LiteralPath $VsWhere) {
        $Candidates += & $VsWhere -latest -products * -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe"
    }
    $Candidates += Get-Command MSBuild.exe -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source
    return Resolve-FirstExistingPath $Candidates
}

function Find-CL {
    $Candidates = @()
    $VsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path -LiteralPath $VsWhere) {
        $Candidates += & $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find "VC\Tools\MSVC\**\bin\Hostx64\x64\cl.exe"
    }
    $Candidates += Get-Command cl.exe -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source
    return Resolve-FirstExistingPath $Candidates
}

function Format-FoundPath {
    param([string]$Path)
    if ($Path) {
        return $Path
    }
    return "NOT FOUND"
}

function Test-StaticSourceSanity {
    param([string]$RootPath)

    $Failures = New-Object System.Collections.Generic.List[string]
    $SourceRoot = Join-Path $RootPath "Source"
    if (-not (Test-Path -LiteralPath $SourceRoot)) {
        $Failures.Add("Source directory not found: $SourceRoot")
        return $Failures
    }

    foreach ($Header in Get-ChildItem -LiteralPath $SourceRoot -Recurse -Filter "*.h") {
        $Text = Get-Content -LiteralPath $Header.FullName -Raw -Encoding UTF8
        $NeedsGeneratedHeader = $Text -match "UCLASS\s*\(" -or $Text -match "USTRUCT\s*\(" -or $Text -match "UENUM\s*\("
        $IncludeLines = @()
        $Lines = $Text -split "`r?`n"
        for ($Index = 0; $Index -lt $Lines.Count; ++$Index) {
            if ($Lines[$Index] -match "^\s*#include\s+") {
                $IncludeLines += [pscustomobject]@{
                    Number = $Index + 1
                    Text = $Lines[$Index]
                }
            }
        }
        $GeneratedIncludes = @($IncludeLines | Where-Object { $_.Text -match "\.generated\.h" })
        if ($NeedsGeneratedHeader -and $GeneratedIncludes.Count -ne 1) {
            $Failures.Add("$($Header.FullName): expected exactly one .generated.h include, found $($GeneratedIncludes.Count)")
        }
        if ($NeedsGeneratedHeader -and $GeneratedIncludes.Count -eq 1) {
            $LastInclude = $IncludeLines | Select-Object -Last 1
            if ($LastInclude.Number -ne $GeneratedIncludes[0].Number) {
                $Failures.Add("$($Header.FullName): .generated.h must be the last include")
            }
        }
    }

    try {
        $null = Get-Content -LiteralPath (Join-Path $RootPath "AdaptiveHorror.uproject") -Raw -Encoding UTF8 | ConvertFrom-Json
    } catch {
        $Failures.Add("AdaptiveHorror.uproject JSON parse failed: $($_.Exception.Message)")
    }

    return $Failures
}

Write-Step "Project"
$ResolvedProject = Resolve-Path -LiteralPath $ProjectPath -ErrorAction SilentlyContinue
if (-not $ResolvedProject) {
    Write-Host "FAIL: Project file not found: $ProjectPath" -ForegroundColor Red
    exit 1
}
Write-Host "Project: $($ResolvedProject.Path)"
$ProjectRoot = Split-Path $ResolvedProject.Path -Parent

Write-Step "Static source sanity"
$StaticFailures = Test-StaticSourceSanity $ProjectRoot
if ($StaticFailures.Count -gt 0) {
    foreach ($Failure in $StaticFailures) {
        Write-Host "FAIL: $Failure" -ForegroundColor Red
    }
    exit 1
}
Write-Host "PASS: generated.h order and .uproject JSON sanity checks passed."

Write-Step "Tool discovery"
$UnrealEditor = Find-UnrealEditor
$UBT = Find-UnrealBuildTool $UnrealEditor
$MSBuild = Find-MSBuild
$CL = Find-CL

Write-Host "UnrealEditor.exe: $(Format-FoundPath $UnrealEditor)"
Write-Host "UnrealBuildTool.exe: $(Format-FoundPath $UBT)"
Write-Host "MSBuild.exe: $(Format-FoundPath $MSBuild)"
Write-Host "cl.exe: $(Format-FoundPath $CL)"

if (-not $UnrealEditor -or -not $UBT) {
    Write-Host "SKIP: UE toolchain was not found. See BUILD_CHECK.md for installation/path setup." -ForegroundColor Yellow
    exit 0
}

Write-Step "Generate Project Files"
& $UBT -projectfiles -project="$($ResolvedProject.Path)" -game -engine
if ($LASTEXITCODE -ne 0) {
    Write-Host "FAIL: Generate Project Files failed with exit code $LASTEXITCODE" -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Step "Development Editor Build"
& $UBT AdaptiveHorrorEditor Win64 Development -Project="$($ResolvedProject.Path)" -WaitMutex -FromMsBuild -NoHotReload -NoHotReloadFromIDE "-MaxParallelActions=$MaxParallelActions"
if ($LASTEXITCODE -ne 0) {
    Write-Host "FAIL: Development Editor Build failed with exit code $LASTEXITCODE" -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Step "Automation Test"
$EditorCmd = Join-Path (Split-Path $UnrealEditor -Parent) "UnrealEditor-Cmd.exe"
if ((Test-Path -LiteralPath $EditorCmd) -and -not $SkipAutomation) {
    & $EditorCmd "$($ResolvedProject.Path)" -ExecCmds="Automation RunTests AdaptiveHorror.EVA; Quit" -unattended -nop4 -nosplash
    if ($LASTEXITCODE -ne 0) {
        Write-Host "FAIL: Automation Test failed with exit code $LASTEXITCODE" -ForegroundColor Red
        exit $LASTEXITCODE
    }
} elseif (Test-Path -LiteralPath $EditorCmd) {
    Write-Host "SKIP: Automation disabled by -SkipAutomation."
} else {
    Write-Host "UnrealEditor-Cmd.exe not found next to UnrealEditor.exe. Run tests from Editor Session Frontend." -ForegroundColor Yellow
}

Write-Step "PIE Smoke Test"
Write-Host "Manual PIE check is still required in Unreal Editor:"
Write-Host "1. Open $($ResolvedProject.Path)"
Write-Host "2. Press Play in Editor"
Write-Host "3. Verify movement, shooting, zombie combat, HUNTER, ADAM arena, and Stage Clear."

Write-Step "Done"
Write-Host "Build check completed. Open the editor and run the PIE smoke test from BUILD_CHECK.md."
