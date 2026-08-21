#pragma once

#include "sr/BaseReconstructor.h"
#include <thread>
#include <atomic>

namespace medical {

class ISuperResEngine;

// 面内 2D 超分重建器: 对每个轴位切片做面内 N× 超分 (如 128×128 -> 512×512),
// 写回新体数据同一切片; 帧数 D 不变, 仅面内两维放大。
// 异步执行 (工作线程), 逐切片 emit progress, 完成后 emit finished;
// 主线程只读 result()/lastStats()。stride>1 时只对每 stride 切片做推理,
// 其余切片复制相邻已重建切片, 实现"快速预览"(stride 倍提速, 轻微质量损失)。
class InPlaneReconstructor : public BaseReconstructor
{
    Q_OBJECT
public:
    explicit InPlaneReconstructor(QObject *parent = nullptr);
    ~InPlaneReconstructor() override;

    bool isRunning() const override { return m_running; }
    void requestCancel() override { m_cancelRequested = true; }
    void setStride(int s) override { m_stride = qMax(1, s); }   // 1=全质量, 4=预览
    int configuredUpscale() const override { return 4; }

    // 异步启动; 立即返回。结果通过 finished 信号 + result() 获取。
    void reconstructAsync(const DicomVolume &in, const QString &modelPath,
                          const QString &devicePref = QStringLiteral("auto")) override;

private:
    void run(DicomVolume in, QString modelPath, QString devicePref);

    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_cancelRequested{false};
    int m_stride = 1;
};

} // namespace medical
