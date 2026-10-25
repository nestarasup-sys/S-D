. "$PSScriptRoot/environment.ps1"
$StudioSmokeDir = Join-Path $StudioRoot ('artifacts/audio-tracks-smoke-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
if (!(New-Item -ItemType Directory -Path $StudioSmokeDir -ErrorAction Stop)) { throw 'Pasta temporaria indisponivel.' }
$StudioVoice = Join-Path $StudioSmokeDir 'voice.wav'
$StudioMain = Join-Path $StudioSmokeDir 'main.wav'
$StudioBed = Join-Path $StudioSmokeDir 'bed.wav'
foreach ($StudioPair in @(
    @{ Path = $StudioVoice; Freq = 880; Duration = 1; Gain = 0.30 },
    @{ Path = $StudioBed; Freq = 220; Duration = 2; Gain = 0.45 }
)) {
    & $StudioEncoder -nostdin -y -hide_banner -loglevel error -f lavfi -i "sine=frequency=$($StudioPair.Freq):duration=$($StudioPair.Duration)" -af "volume=$($StudioPair.Gain)" -ar 24000 -ac 1 -c:a pcm_s16le $StudioPair.Path
    if ($LASTEXITCODE -ne 0) { throw 'Não foi possível gerar as fontes do teste de áudio.' }
}
& $StudioEncoder -nostdin -y -hide_banner -loglevel error -i $StudioVoice -af 'volume=0.95' -ar 24000 -ac 1 -c:a pcm_s16le $StudioMain
if ($LASTEXITCODE -ne 0) { throw 'Não foi possível gerar a mix base do teste.' }
$StudioBase = @{
    format = 'studio-dark'; schemaVersion = 4; id = 'audio-smoke'; name = 'Audio smoke'
    width = 640; height = 360; fps = @{ numerator = 30; denominator = 1 }; duration = 60
    nodes = @(); scenes = @(); shots = @(); visualBeats = @(); camera = @()
    audioSource = $StudioMain; artDirection = @{}
    voiceDuckingSource = $StudioVoice
    audioTracks = @(@{ id = 'bed'; name = 'Music bed'; source = $StudioBed; start = 0; duration = 60; gain = 0.8; muted = $false; duckUnderVoice = $true; duckAmount = 0.85 })
}
$StudioOutputs = @()
foreach ($StudioDuck in @($false, $true)) {
    $StudioBase.audioTracks[0].duckUnderVoice = $StudioDuck
    $StudioProject = Join-Path $StudioSmokeDir ("project-$StudioDuck.json")
    $StudioMovie = Join-Path $StudioSmokeDir ("mix-$StudioDuck.mp4")
    $StudioWave = Join-Path $StudioSmokeDir ("mix-$StudioDuck.wav")
    $StudioJson = $StudioBase | ConvertTo-Json -Depth 8
    [System.IO.File]::WriteAllText($StudioProject, $StudioJson, [System.Text.UTF8Encoding]::new($false))
    $StudioRenderLog = & (Join-Path $StudioBin 'studio_render.exe') --project $StudioProject --output $StudioMovie --ffmpeg $StudioEncoder 2>&1
    if ($LASTEXITCODE -ne 0) { throw "Exportação de mix (duck=$StudioDuck) falhou: $($StudioRenderLog -join "`n")" }
    & $StudioEncoder -nostdin -y -hide_banner -loglevel error -i $StudioMovie -vn -ar 24000 -ac 1 -c:a pcm_s16le $StudioWave
    if ($LASTEXITCODE -ne 0) { throw 'Não foi possível decodificar o áudio exportado.' }
    $StudioOutputs += $StudioWave
}
function Get-StudioRms([string]$StudioWavePath, [double]$StudioStart, [double]$StudioEnd) {
    $StudioBytes = [System.IO.File]::ReadAllBytes($StudioWavePath)
    $StudioFirst = 44 + [int]($StudioStart * 24000) * 2
    $StudioLast = [Math]::Min($StudioBytes.Length, 44 + [int]($StudioEnd * 24000) * 2)
    $StudioSum = 0.0; $StudioCount = 0
    for ($StudioOffset = $StudioFirst; $StudioOffset + 1 -lt $StudioLast; $StudioOffset += 2) {
        $StudioSample = [BitConverter]::ToInt16($StudioBytes, $StudioOffset) / 32768.0
        $StudioSum += $StudioSample * $StudioSample; $StudioCount++
    }
    return [Math]::Sqrt($StudioSum / [Math]::Max(1, $StudioCount))
}
$StudioPlainActive = Get-StudioRms $StudioOutputs[0] 0.2 0.8
$StudioDuckedActive = Get-StudioRms $StudioOutputs[1] 0.2 0.8
$StudioPlainTail = Get-StudioRms $StudioOutputs[0] 1.2 1.8
$StudioDuckedTail = Get-StudioRms $StudioOutputs[1] 1.2 1.8
if ($StudioDuckedActive -ge $StudioPlainActive * 0.92) { throw 'Ducking não reduziu a trilha durante o áudio principal.' }
if ([Math]::Abs($StudioDuckedTail - $StudioPlainTail) -gt 0.025) { throw 'A trilha não retornou ao volume após o áudio principal.' }
[pscustomobject]@{
    status = 'passed'; artifacts = $StudioSmokeDir
    plainActiveRms = [Math]::Round($StudioPlainActive, 4); duckedActiveRms = [Math]::Round($StudioDuckedActive, 4)
    plainTailRms = [Math]::Round($StudioPlainTail, 4); duckedTailRms = [Math]::Round($StudioDuckedTail, 4)
} | ConvertTo-Json
