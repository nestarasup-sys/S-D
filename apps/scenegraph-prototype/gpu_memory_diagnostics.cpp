#include "gpu_memory_diagnostics.hpp"

#include <QJsonObject>
#include <QMutexLocker>
#include <rhi/qrhi.h>
#include <algorithm>

#ifdef Q_OS_WIN
#include <d3d11.h>
#include <dxgi1_4.h>
#include <rhi/qrhi.h>
#include <rhi/qrhi_platform.h>
#endif

GpuMemoryDiagnostics::GpuMemoryDiagnostics() = default;

GpuMemoryDiagnostics::~GpuMemoryDiagnostics() {
#ifdef Q_OS_WIN
    if (adapter3_)
        static_cast<IDXGIAdapter3 *>(adapter3_)->Release();
#endif
}

void GpuMemoryDiagnostics::setPhase(QString phase) {
    QMutexLocker lock(&mutex_);
    phase_ = std::move(phase);
}

bool GpuMemoryDiagnostics::initializeFromRhi(QRhi *rhi) {
    if (initializationAttempted_)
        return adapter3_ != nullptr;
    if (!rhi) {
        initializationError_ = QStringLiteral("Qt has not created a QRhi device yet");
        return false;
    }
    backend_ = QString::fromLatin1(rhi->backendName());
#ifdef Q_OS_WIN
    if (backend_.compare(QStringLiteral("D3D11"), Qt::CaseInsensitive) != 0 &&
        backend_.compare(QStringLiteral("Direct3D 11"), Qt::CaseInsensitive) != 0) {
        initializationAttempted_ = true;
        initializationError_ = QStringLiteral("DXGI probe requires the active QRhi backend to be D3D11");
        return false;
    }
    const auto *native = static_cast<const QRhiD3D11NativeHandles *>(rhi->nativeHandles());
    if (!native || !native->dev) {
        initializationError_ = QStringLiteral("QRhi did not expose its active D3D11 device");
        return false;
    }
    initializationAttempted_ = true;
    auto *device = static_cast<ID3D11Device *>(native->dev);
    IDXGIDevice *dxgiDevice = nullptr;
    HRESULT result = device->QueryInterface(IID_PPV_ARGS(&dxgiDevice));
    if (FAILED(result) || !dxgiDevice) {
        initializationError_ = QStringLiteral("ID3D11Device could not be queried as IDXGIDevice (0x%1)")
                                   .arg(quint32(result), 8, 16, QLatin1Char('0'));
        return false;
    }
    IDXGIAdapter *adapter = nullptr;
    result = dxgiDevice->GetAdapter(&adapter);
    dxgiDevice->Release();
    if (FAILED(result) || !adapter) {
        initializationError_ = QStringLiteral("The QRhi device could not identify its DXGI adapter (0x%1)")
                                   .arg(quint32(result), 8, 16, QLatin1Char('0'));
        return false;
    }
    IDXGIAdapter3 *adapter3 = nullptr;
    result = adapter->QueryInterface(IID_PPV_ARGS(&adapter3));
    DXGI_ADAPTER_DESC description{};
    const HRESULT descResult = adapter->GetDesc(&description);
    adapter->Release();
    if (FAILED(result) || !adapter3) {
        initializationError_ = QStringLiteral("IDXGIAdapter3 is unavailable for the active adapter (0x%1)")
                                   .arg(quint32(result), 8, 16, QLatin1Char('0'));
        return false;
    }
    if (FAILED(descResult)) {
        adapter3->Release();
        initializationError_ = QStringLiteral("DXGI could not read the active adapter descriptor (0x%1)")
                                   .arg(quint32(descResult), 8, 16, QLatin1Char('0'));
        return false;
    }
    adapter3_ = adapter3;
    adapterName_ = QString::fromWCharArray(description.Description);
    vendorId_ = description.VendorId;
    deviceId_ = description.DeviceId;
    luidHigh_ = quint32(description.AdapterLuid.HighPart);
    luidLow_ = description.AdapterLuid.LowPart;
    dedicatedVideoMemoryBytes_ = quint64(description.DedicatedVideoMemory);
    sharedSystemMemoryBytes_ = quint64(description.SharedSystemMemory);
    return true;
#else
    initializationError_ = QStringLiteral("DXGI diagnostics are only available on Windows");
    return false;
#endif
}

bool GpuMemoryDiagnostics::querySnapshot(QJsonObject &snapshot, QString &error) const {
#ifdef Q_OS_WIN
    auto *adapter = static_cast<IDXGIAdapter3 *>(adapter3_);
    if (!adapter) {
        error = initializationError_.isEmpty() ? QStringLiteral("No active DXGI adapter") : initializationError_;
        return false;
    }
    DXGI_QUERY_VIDEO_MEMORY_INFO local{};
    DXGI_QUERY_VIDEO_MEMORY_INFO nonLocal{};
    const HRESULT localResult = adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &local);
    const HRESULT nonLocalResult = adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &nonLocal);
    if (FAILED(localResult) || FAILED(nonLocalResult)) {
        error = QStringLiteral("QueryVideoMemoryInfo failed (LOCAL=0x%1, NON_LOCAL=0x%2)")
                    .arg(quint32(localResult), 8, 16, QLatin1Char('0'))
                    .arg(quint32(nonLocalResult), 8, 16, QLatin1Char('0'));
        return false;
    }
    const auto asObject = [](const DXGI_QUERY_VIDEO_MEMORY_INFO &info) {
        return QJsonObject{{QStringLiteral("budgetBytes"), qint64(info.Budget)},
                           {QStringLiteral("currentUsageBytes"), qint64(info.CurrentUsage)},
                           {QStringLiteral("currentReservationBytes"), qint64(info.CurrentReservation)},
                           {QStringLiteral("availableForReservationBytes"), qint64(info.AvailableForReservation)}};
    };
    snapshot = {{QStringLiteral("local"), asObject(local)},
                {QStringLiteral("nonLocal"), asObject(nonLocal)}};
    return true;
#else
    Q_UNUSED(snapshot)
    error = QStringLiteral("DXGI diagnostics are only available on Windows");
    return false;
#endif
}

void GpuMemoryDiagnostics::sample(QRhi *rhi) {
    QMutexLocker lock(&mutex_);
    if (!initializeFromRhi(rhi)) {
        ++queryFailureCount_;
        return;
    }
    QJsonObject sample;
    QString error;
    if (!querySnapshot(sample, error)) {
        initializationError_ = error;
        ++queryFailureCount_;
        return;
    }
    ++sampleCount_;
    sample.insert(QStringLiteral("sampleCount"), qint64(sampleCount_));
    if (phase_ == QStringLiteral("before")) {
        if (before_.isEmpty())
            before_ = sample;
    } else if (phase_ == QStringLiteral("during")) {
        const auto local = sample.value(QStringLiteral("local")).toObject();
        const auto peak = duringPeak_.value(QStringLiteral("local")).toObject();
        if (duringPeak_.isEmpty() ||
            local.value(QStringLiteral("currentUsageBytes")).toDouble() >=
                peak.value(QStringLiteral("currentUsageBytes")).toDouble())
            duringPeak_ = sample;
    } else if (phase_ == QStringLiteral("beforeRelease")) {
        beforeRelease_ = sample;
    } else if (phase_ == QStringLiteral("afterRelease")) {
        afterRelease_ = sample;
    }
}

void GpuMemoryDiagnostics::sampleCurrent(QString phase) {
    QMutexLocker lock(&mutex_);
    QJsonObject sample;
    QString error;
    if (!querySnapshot(sample, error)) {
        initializationError_ = error;
        ++queryFailureCount_;
        return;
    }
    ++sampleCount_;
    sample.insert(QStringLiteral("sampleCount"), qint64(sampleCount_));
    if (phase == QStringLiteral("before"))
        before_ = before_.isEmpty() ? sample : before_;
    else if (phase == QStringLiteral("beforeRelease"))
        beforeRelease_ = sample;
    else if (phase == QStringLiteral("afterRelease"))
        afterRelease_ = sample;
}

QJsonObject GpuMemoryDiagnostics::report() const {
    QMutexLocker lock(&mutex_);
    const bool available = !adapterName_.isEmpty() && sampleCount_ > 0;
    const QJsonObject adapter{
        {QStringLiteral("name"), adapterName_},
        {QStringLiteral("vendorIdHex"), QStringLiteral("0x%1").arg(vendorId_, 4, 16, QLatin1Char('0'))},
        {QStringLiteral("deviceIdHex"), QStringLiteral("0x%1").arg(deviceId_, 4, 16, QLatin1Char('0'))},
        {QStringLiteral("luidHighHex"), QStringLiteral("0x%1").arg(luidHigh_, 8, 16, QLatin1Char('0'))},
        {QStringLiteral("luidLowHex"), QStringLiteral("0x%1").arg(luidLow_, 8, 16, QLatin1Char('0'))},
        {QStringLiteral("dedicatedVideoMemoryPhysicalBytes"), qint64(dedicatedVideoMemoryBytes_)},
        {QStringLiteral("sharedSystemMemoryBytes"), qint64(sharedSystemMemoryBytes_)}};
    const QJsonObject phases{
        {QStringLiteral("before"), before_},
        {QStringLiteral("duringPeak"), duringPeak_},
        {QStringLiteral("beforeRelease"), beforeRelease_},
        {QStringLiteral("afterRelease"), afterRelease_}};
    return QJsonObject{
        {QStringLiteral("status"), available ? QStringLiteral("available") : QStringLiteral("unavailable")},
        {QStringLiteral("backend"), backend_},
        {QStringLiteral("adapter"), adapter},
        {QStringLiteral("phases"), phases},
        {QStringLiteral("sampleCount"), qint64(sampleCount_)},
        {QStringLiteral("queryFailureCount"), qint64(queryFailureCount_)},
        {QStringLiteral("error"), initializationError_},
        {QStringLiteral("currentUsageMeaning"),
         QStringLiteral("DXGI process usage for the adapter segment, not a sum of Studio Dark textures")},
        {QStringLiteral("physicalMemoryMeaning"),
         QStringLiteral("DedicatedVideoMemoryPhysicalBytes is adapter-reported physical capacity, separate from the Windows process budget")}};
}
