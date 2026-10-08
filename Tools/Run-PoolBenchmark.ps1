param(
    [ValidateSet('validate', 'reuse', 'fresh')]
    [string]$Mode = 'validate',
    [ValidateRange(1, 200)][int]$Batch = 20,
    [ValidateRange(1, 100)][int]$Cycles = 10,
    [ValidateRange(0, 10)][int]$Warmup = 2,
    [switch]$Headless,
    [switch]$Trace,
    [switch]$WorkspaceCache,
    [Parameter(Mandatory = $true)]
    [string]$EngineRoot
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$projectFile = Join-Path $projectRoot 'Warrior.uproject'
$editorExe = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if (!(Test-Path -LiteralPath $editorExe)) { throw "Unreal executable not found: $editorExe" }
if (Get-Process -Name UnrealEditor, UnrealEditor-Cmd -ErrorAction SilentlyContinue) {
    throw 'Save and close Unreal Editor / other benchmark runs before starting a standalone comparison.'
}
$logDirectory = Join-Path $projectRoot 'Saved\Logs'
[void](New-Item -ItemType Directory -Path $logDirectory -Force)
$runId = (Get-Date -Format 'yyyyMMdd_HHmmss') + '_' + $Mode + '_' + ([guid]::NewGuid().ToString('N').Substring(0, 8))
$logFile = Join-Path $logDirectory ('PoolTest_' + $runId + '.log')
# Use explicit concatenation: PowerShell can interpret '?' as part of a variable name.
$mapUrl = '/Game/Maps/GameModeTestMap?game=/Script/Warrior.WarriorPoolBenchmarkGameMode' +
    '?PoolMode=' + $Mode + '?PoolBatch=' + $Batch + '?PoolCycles=' + $Cycles + '?PoolWarmup=' + $Warmup + '?PoolQuit'
$launchArgs = @($projectFile, $mapUrl, '-game', '-unattended', '-nosplash', "-abslog=$logFile")
if ($WorkspaceCache) {
    $cacheDirectory = Join-Path $projectRoot 'Saved\PoolBenchmarkDDC'
    $launchArgs += @('-ddc=NoZenLocalFallback', "-LocalDataCachePath=$cacheDirectory")
}
if ($Headless) {
    $launchArgs += @('-nullrhi', '-nosound')
    Write-Host 'Headless correctness check only. Do not use these timings as gameplay performance results.'
} else {
    $launchArgs += @('-windowed', '-ResX=1280', '-ResY=720')
}
if ($Trace) {
    $traceDirectory = Join-Path $projectRoot 'Saved\Profiling\PoolBenchmark'
    [void](New-Item -ItemType Directory -Path $traceDirectory -Force)
    $traceFile = Join-Path $traceDirectory ($runId + '.utrace')
    $launchArgs += @('-trace=cpu,frame,bookmark', "-tracefile=$traceFile")
}
Write-Host "Starting $Mode. Log: $logFile"
& $editorExe @launchArgs
$runExitCode = $LASTEXITCODE
if ($runExitCode -ne 0) { throw "Benchmark failed (exit $runExitCode). Inspect $logFile" }
if (!(Select-String -LiteralPath $logFile -SimpleMatch 'LogWarriorPoolTest: Display: PASS:' -Quiet)) {
    throw "Process exited without a benchmark PASS. Inspect $logFile"
}
Write-Host 'PASS. Reports: Saved\Profiling\PoolBenchmark (one timestamped folder per run).'
