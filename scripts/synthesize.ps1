param([Parameter(Mandatory=$true)][string]$InputFile,[Parameter(Mandatory=$true)][string]$OutputDirectory,[string]$CancelFile)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Speech
$StudioInput = Get-Content -LiteralPath $InputFile -Raw -Encoding UTF8 | ConvertFrom-Json
$StudioSpeech = New-Object System.Speech.Synthesis.SpeechSynthesizer
try {
    $StudioVoices = @($StudioSpeech.GetInstalledVoices() | Where-Object { $_.Enabled -and $_.VoiceInfo.Culture.Name -eq 'pt-BR' })
    if ($StudioInput.voice -eq 'auto') {
        if ($StudioVoices.Count -eq 0) { throw 'No installed pt-BR speech voice.' }
        $StudioChosen = $StudioVoices | Where-Object { $_.VoiceInfo.Gender.ToString() -eq 'Male' } | Sort-Object { $_.VoiceInfo.Name } | Select-Object -First 1
        if ($null -eq $StudioChosen) { $StudioChosen = $StudioVoices | Sort-Object { $_.VoiceInfo.Name } | Select-Object -First 1 }
        $StudioVoiceName = $StudioChosen.VoiceInfo.Name
    } else { $StudioVoiceName = [string]$StudioInput.voice }
    $StudioSpeech.SelectVoice($StudioVoiceName)
    $StudioSpeech.Rate = [int]$StudioInput.speechRate
    $StudioSpeech.Volume = 100
    $StudioFormat = [System.Speech.AudioFormat.SpeechAudioFormatInfo]::new(24000,[System.Speech.AudioFormat.AudioBitsPerSample]::Sixteen,[System.Speech.AudioFormat.AudioChannel]::Mono)
    $StudioUtterances = @()
    $StudioIndex = 0
    $StudioCurrentWords = [System.Collections.Generic.List[object]]::new()
    $StudioProgressHandler = [System.EventHandler[System.Speech.Synthesis.SpeakProgressEventArgs]] {
        param($sender, $eventArgs)
        $StudioCurrentWords.Add(@{
            text = [string]$eventArgs.Text
            characterPosition = [int]$eventArgs.CharacterPosition
            characterCount = [int]$eventArgs.CharacterCount
            audioPositionSamples = [long][math]::Round($eventArgs.AudioPosition.TotalSeconds * 24000)
        })
    }
    $StudioSpeech.add_SpeakProgress($StudioProgressHandler)
    foreach ($StudioLine in $StudioInput.utterances) {
        if ($CancelFile -and (Test-Path -LiteralPath $CancelFile)) { throw 'Production cancelled.' }
        $StudioWavePath = Join-Path $OutputDirectory ('speech-{0:D3}.wav' -f $StudioIndex)
        $StudioCurrentWords.Clear()
        $StudioSpeech.SetOutputToWaveFile($StudioWavePath,$StudioFormat)
        $StudioSpeech.Speak([string]$StudioLine.text)
        $StudioSpeech.SetOutputToNull()
        $StudioUtterances += @{ id = [string]$StudioLine.id; path = $StudioWavePath; words = @($StudioCurrentWords.ToArray()) }
        $StudioIndex++
        Write-Output ('Voice {0}/{1}' -f $StudioIndex,$StudioInput.utterances.Count)
    }
    $StudioManifest = @{ voice = $StudioVoiceName; sampleRate = 24000; utterances = $StudioUtterances } | ConvertTo-Json -Depth 6
    [System.IO.File]::WriteAllText((Join-Path $OutputDirectory 'speech-manifest.json'),$StudioManifest,[System.Text.UTF8Encoding]::new($false))
} finally {
    if ($StudioProgressHandler) { $StudioSpeech.remove_SpeakProgress($StudioProgressHandler) }
    $StudioSpeech.Dispose()
}
