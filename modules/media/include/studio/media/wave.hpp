#pragma once
#include <QString>
#include <cstdint>
#include <vector>

namespace studio {
constexpr int ProductionSampleRate = 24000;
struct PcmAudio {
    std::vector<float> samples;
};
struct WaveResult {
    PcmAudio audio;
    QString error;
};
WaveResult readWave(const QString &path);
QString writeWave(const QString &path, const PcmAudio &audio);
void addSoundtrack(PcmAudio &audio, double musicGain, double effectsGain,
                   const std::vector<std::int64_t> &sceneSamples);
} // namespace studio
