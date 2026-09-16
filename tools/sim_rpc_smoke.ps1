<#
.SYNOPSIS
    Дымовая проверка JSON-RPC демо-режима GRAMs (контракт grams.sim/1).

.DESCRIPTION
    hello → catalog → validate → run → status ×N → pause → resume → goto → stop.
    GRAMs должен быть запущен с --sim на машине без подключённого железа.

.EXAMPLE
    .\build\src\GRAMs.exe --sim
    .\tools\sim_rpc_smoke.ps1
    .\tools\sim_rpc_smoke.ps1 -Port 8779 -Token s3cret -Profile tests\sim\profiles\h2_to_chamber.json
#>
param(
    [int]$Port = 8770,
    [string]$Token = $env:GRAMS_SIM_TOKEN,
    [string]$Profile = (Join-Path $PSScriptRoot "..\tests\sim\profiles\vacuum_fore_turbo.json"),
    [string]$GotoPhase = "",
    [int]$StatusPolls = 3
)

$ErrorActionPreference = "Stop"
$script:id = 0
$script:failed = $false

function Invoke-SimRpc([string]$Method, $Params = $null) {
    $script:id++
    $body = @{ jsonrpc = "2.0"; id = $script:id; method = $Method }
    if ($null -ne $Params) { $body.params = $Params }
    $headers = @{}
    if ($Token) { $headers["X-Grams-Sim-Token"] = $Token }
    $json = $body | ConvertTo-Json -Depth 50 -Compress
    $reply = Invoke-RestMethod -Uri "http://127.0.0.1:$Port/rpc" -Method Post -Headers $headers `
        -ContentType "application/json; charset=utf-8" -Body ([Text.Encoding]::UTF8.GetBytes($json)) -TimeoutSec 5
    if ($reply.error) {
        Write-Host ("  ✗ {0}: {1} {2}" -f $Method, $reply.error.code, $reply.error.message) -ForegroundColor Red
        $script:failed = $true
    } else {
        Write-Host ("  ✓ {0}" -f $Method) -ForegroundColor Green
    }
    return $reply
}

Write-Host "GRAMs sim RPC на 127.0.0.1:$Port"

$hello = (Invoke-SimRpc "sim.hello").result
Write-Host ("    api={0} appVersion={1} simAllowed={2} reason={3}" -f $hello.api, $hello.appVersion, $hello.simAllowed, $hello.reason)
if (-not $hello.simAllowed) { Write-Host "Симуляция запрещена — дальше не идём."; exit 2 }

$catalog = (Invoke-SimRpc "sim.catalog").result
Write-Host ("    каналов {0}, клапанов {1}, участков {2}" -f $catalog.channels.Count, $catalog.valves.Count, $catalog.zones.Count)

$profileJson = Get-Content $Profile -Raw -Encoding UTF8 | ConvertFrom-Json
$validate = (Invoke-SimRpc "sim.validate" @{ profile = $profileJson }).result
Write-Host ("    ok={0} errors={1} warnings={2}" -f $validate.ok, $validate.errors.Count, $validate.warnings.Count)
if (-not $validate.ok) { $validate.errors | Format-Table path, code, message; exit 1 }

$runId = (Invoke-SimRpc "sim.run" @{ profile = $profileJson }).result.runId
Write-Host "    runId=$runId"

function Show-Status {
    $s = (Invoke-SimRpc "sim.status").result
    $phase = if ($s.phase) { "{0} ({1:N1}/{2} с)" -f $s.phase.id, $s.phase.elapsedSec, $s.phase.durationSec } else { "—" }
    Write-Host ("    machine={0} phase={1} history={2}" -f $s.machine, $phase, $s.history.Count)
    return $s
}

for ($i = 0; $i -lt $StatusPolls; $i++) { Show-Status | Out-Null; Start-Sleep -Seconds 1 }

Invoke-SimRpc "sim.pause" @{ runId = $runId } | Out-Null
$paused = Show-Status
Start-Sleep -Seconds 2
$still = Show-Status
if ($paused.phase -and $still.phase -and $still.phase.elapsedSec -ne $paused.phase.elapsedSec) {
    Write-Host "  ✗ на паузе время фазы идёт" -ForegroundColor Red; $script:failed = $true
}
Invoke-SimRpc "sim.resume" @{ runId = $runId } | Out-Null

if (-not $GotoPhase) { $GotoPhase = $profileJson.phases[-1].id }
Invoke-SimRpc "sim.goto" @{ runId = $runId; phaseId = $GotoPhase } | Out-Null
Start-Sleep -Milliseconds 700
Show-Status | Out-Null

Invoke-SimRpc "sim.stop" @{ runId = $runId } | Out-Null
Start-Sleep -Milliseconds 700
$final = Show-Status
if ($final.machine -ne "idle") { Write-Host "  ✗ после stop машина не в idle" -ForegroundColor Red; $script:failed = $true }

if ($script:failed) { Write-Host "ИТОГ: есть ошибки" -ForegroundColor Red; exit 1 }
Write-Host "ИТОГ: всё прошло" -ForegroundColor Green
