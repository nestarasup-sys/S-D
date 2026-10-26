#pragma once

#include <QJsonObject>
#include <QMutex>
#include <QString>

class QRhi;

// Optional, read-only diagnostics for the adapter attached to Qt's live QRhi.
// DXGI query failures never affect composition or rendering.
class GpuMemoryDiagnostics final {
  public:
    GpuMemoryDiagnostics();
    ~GpuMemoryDiagnostics();
    GpuMemoryDiagnostics(const GpuMemoryDiagnostics &) = delete;
    GpuMemoryDiagnostics &operator=(const GpuMemoryDiagnostics &) = delete;

    void setPhase(QString phase);
    void sample(QRhi *rhi);
    void sampleCurrent(QString phase);
    QJsonObject report() const;

  private:
    bool initializeFromRhi(QRhi *rhi);
    bool querySnapshot(QJsonObject &snapshot, QString &error) const;

    mutable QMutex mutex_;
    QString phase_{QStringLiteral("before")};
    QString backend_;
    QString adapterName_;
    QString initializationError_;
    quint32 vendorId_{0};
    quint32 deviceId_{0};
    quint32 luidHigh_{0};
    quint32 luidLow_{0};
    quint64 dedicatedVideoMemoryBytes_{0};
    quint64 sharedSystemMemoryBytes_{0};
    void *adapter3_{nullptr};
    bool initializationAttempted_{false};
    QJsonObject before_;
    QJsonObject duringPeak_;
    QJsonObject beforeRelease_;
    QJsonObject afterRelease_;
    quint64 sampleCount_{0};
    quint64 queryFailureCount_{0};
};
