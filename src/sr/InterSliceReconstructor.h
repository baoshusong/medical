#pragma once

#include "sr/BaseReconstructor.h"
#include <thread>
#include <atomic>

namespace medical {

class ISuperResEngine;

// 层间超分重建器: 沿 z 方向做 4× 超分 (帧数 D -> 4D)。
//
// 恒定 4×, 由 EDSR 宽度超分引擎 (edsr_invsr_width4x.onnx) 实现。流程:
//   1) 取前 75 张轴位切片 (按加载顺序) 作为一个序列;
//   2) 维度转换 (H,W,D) -> (H,D,W): 把"切片维"当作 2D 影像的"宽度"维;
//   3) 对每个 x 列, 用模型把宽度 75 -> 300 (×4), 高度 H 不变;
//   4) 维度还原 (H,D',W) -> (H,W,D'), 得到 D'=300 张 512×512 切片 (Z 层间距细化)。
// 接口与 InPlaneReconstructor 保持一致 (经 BaseReconstructor), 可 drop-in 接入 UI。
class InterSliceReconstructor : public BaseReconstructor
{
    Q_OBJECT
public:
    explicit InterSliceReconstructor(QObject *parent = nullptr);
    ~InterSliceReconstructor() override;

    bool isRunning() const override { return m_running; }
    void requestCancel() override { m_cancelRequested = true; }
    void setStride(int s) override { m_stride = qMax(1, s); }
    int configuredUpscale() const override { return 4; }

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
