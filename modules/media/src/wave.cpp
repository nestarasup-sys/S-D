#include <QDataStream>
#include <QFile>
#include <QSaveFile>
#include <QtEndian>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <studio/media/wave.hpp>

namespace studio {
WaveResult readWave(const QString &path) {
    WaveResult r;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        r.error = "Áudio ausente: " + path;
        return r;
    }
    if (file.size() > 64 * 1024 * 1024) {
        r.error = "WAV excede 64 MiB.";
        return r;
    }
    auto b = file.readAll();
    auto u16 = [&](qsizetype i) { return qFromLittleEndian<quint16>(b.constData() + i); };
    auto u32 = [&](qsizetype i) { return qFromLittleEndian<quint32>(b.constData() + i); };
    if (b.size() < 44 || b.first(4) != "RIFF" || b.mid(8, 4) != "WAVE") {
        r.error = "WAV inválido.";
        return r;
    }
    bool format = false;
    qsizetype dataOffset = 0, dataSize = 0;
    for (qsizetype i = 12; i + 8 <= b.size();) {
        auto length = qsizetype(u32(i + 4));
        if (length > b.size() - i - 8) {
            r.error = "WAV truncado.";
            return r;
        }
        auto id = b.mid(i, 4);
        if (id == "fmt ") {
            if (length < 16 || u16(i + 8) != 1 || u16(i + 10) != 1 || u32(i + 12) != ProductionSampleRate ||
                u16(i + 22) != 16) {
                r.error = "Use WAV PCM mono, 24 kHz, 16 bits.";
                return r;
            }
            format = true;
        }
        if (id == "data") {
            dataOffset = i + 8;
            dataSize = length;
        }
        i += 8 + length + (length % 2);
    }
    if (!format || dataSize < 2 || dataSize % 2) {
        r.error = "WAV sem amostras válidas.";
        return r;
    }
    r.audio.samples.resize(dataSize / 2);
    for (qsizetype i = 0; i < dataSize / 2; ++i)
        r.audio.samples[i] = float(qFromLittleEndian<qint16>(b.constData() + dataOffset + i * 2)) / 32768.f;
    return r;
}
QString writeWave(const QString &path, const PcmAudio &audio) {
    if (audio.samples.empty() || audio.samples.size() > ProductionSampleRate * 600)
        return "Duração de áudio inválida.";
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return file.errorString();
    QDataStream out(&file);
    out.setByteOrder(QDataStream::LittleEndian);
    quint32 size = quint32(audio.samples.size() * 2);
    out.writeRawData("RIFF", 4);
    out << quint32(36 + size);
    out.writeRawData("WAVEfmt ", 8);
    out << quint32(16) << quint16(1) << quint16(1) << quint32(ProductionSampleRate)
        << quint32(ProductionSampleRate * 2) << quint16(2) << quint16(16);
    out.writeRawData("data", 4);
    out << size;
    for (auto sample : audio.samples)
        out << qint16(std::clamp(sample, -1.f, 1.f) * 32767);
    if (out.status() != QDataStream::Ok || !file.commit())
        return "Falha ao gravar WAV: " + file.errorString();
    return {};
}
void addSoundtrack(PcmAudio &audio, double musicGain, double effectsGain,
                   const std::vector<std::int64_t> &sceneSamples) {
    constexpr double pi = std::numbers::pi;
    const double notes[]{130.8128, 164.8138, 195.9977, 246.9417};
    for (std::size_t i = 0; i < audio.samples.size(); ++i) {
        double t = double(i) / ProductionSampleRate;
        double beat = std::fmod(t, 0.625);
        int note = int(t / 0.625) % 4;
        double envelope = std::exp(-beat * 5) * (1 - std::exp(-beat * 120));
        double bed =
            (std::sin(2 * pi * notes[note] * t) + .25 * std::sin(2 * pi * notes[note] * 2 * t)) * envelope;
        double fade = std::min({1., t / 1.5, double(audio.samples.size() - i) / ProductionSampleRate / 1.5});
        audio.samples[i] += float(musicGain * bed * fade);
    }
    for (auto start : sceneSamples)
        for (int j = 0; j < ProductionSampleRate / 4 && start + j < std::int64_t(audio.samples.size()); ++j) {
            if (start + j < 0)
                continue;
            double t = double(j) / ProductionSampleRate;
            double envelope = std::sin(pi * t / .25) * std::exp(-t * 12);
            double s = std::sin(2 * pi * (650 * t - 800 * t * t));
            audio.samples[start + j] += float(effectsGain * s * envelope);
        }
    // Preserve headroom; this soundtrack is deliberately quiet under speech.
    float peak = 0;
    for (float s : audio.samples)
        peak = std::max(peak, std::abs(s));
    if (peak > .95f)
        for (auto &s : audio.samples)
            s *= .95f / peak;
}
} // namespace studio
