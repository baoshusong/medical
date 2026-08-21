#pragma once

#include "sr/DicomVolume.h"
#include <QObject>
#include <QString>

namespace medical {

// 超分重建统计 (面内/层间共用)。
struct SRStats
{
    int   inSlices  = 0;
    int   outSlices = 0;
    int   upscale   = 4;
    qint64 elapsedMs = 0;
    QString engineName;
    QString engineDevice;
    bool    ok = false;
    bool    cancelled = false;
    QString error;
};

// 重建器抽象基类: 统一 InPlaneReconstructor(面内 2D) 与
// InterSliceReconstructor(层间) 的对外接口 + Qt 信号, 供 ReconPanel
// 用同一份代码承载两种重建器。
//
// 子类实现 reconstructAsync(); 进度与完成通过基类 progress/finished 信号上报,
// 结果写入受保护的 m_result/m_stats, 由 result()/lastStats() 暴露。
class BaseReconstructor : public QObject
{
    Q_OBJECT
public:
    explicit BaseReconstructor(QObject *parent = nullptr) : QObject(parent) {}
    ~BaseReconstructor() override = default;

    virtual void reconstructAsync(const DicomVolume &in, const QString &modelPath,
                                  const QString &devicePref = QStringLiteral("auto")) = 0;
    virtual bool isRunning() const = 0;
    virtual void requestCancel() = 0;
    virtual void setStride(int s) = 0;                       // 1=全质量, 4/8=预览
    virtual int configuredUpscale() const { return 4; }

    const DicomVolume &result() const { return m_result; }
    const SRStats &lastStats() const { return m_stats; }

signals:
    void progress(int done, int total);
    void finished(const medical::SRStats &stats);

protected:
    DicomVolume m_result;
    SRStats     m_stats;
};

} // namespace medical

Q_DECLARE_METATYPE(medical::SRStats)
