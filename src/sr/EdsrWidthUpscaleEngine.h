#pragma once

#include "sr/ISuperResEngine.h"
#include <memory>

#ifdef USE_ONNXRUNTIME
#include "sr/OrtShim.h"
#endif

namespace medical {

#ifdef USE_ONNXRUNTIME
// 层间超分专用引擎: 仅沿"宽度"方向 4× 超分, 高度不变 (其余不变)。
// 对应模型 edsr_invsr_width4x.onnx (EDSR 系, 把切片序列当宽度维做超分)。
//   输入 2D 影像: 宽 = 切片数 (如 75), 高 = 512
//   输出 2D 影像: 宽 = 切片数 × 4 (如 300), 高 = 512
// 由 ISuperResEngine::create 按模型文件名 (含 edsr/invsr/width) 自动分流到此。
class EdsrWidthUpscaleEngine : public ISuperResEngine {
public:
    explicit EdsrWidthUpscaleEngine(const QString &modelPath,
                                    const QString &devicePref = QStringLiteral("auto"));

    QString name() const override { return QStringLiteral("ONNX EDSR-InvSR (width ×4)"); }
    QString device() const override { return m_deviceLabel; }
    int     widthUpscale() const override { return 4; }
    bool    supportsWidthUpscale() const override { return true; }
    bool    ready() const override { return m_ready; }
    int     modelInputHeight() const override { return m_inH; }
    int     modelInputWidth()  const override { return m_inW; }

    // 该模型仅支持宽度方向超分; 面内重建不会调用此方法, 这里转调 upsampleWidth.
    QImage upsampleImage(const QImage &img) override;
    // 宽度方向 4× 超分 (核心).
    QImage upsampleWidth(const QImage &img) const override;

private:
    Ort::Env m_env{ORT_LOGGING_LEVEL_WARNING, "EDSRI"};
    std::unique_ptr<Ort::Session> m_session;
    std::string m_inName, m_outName;
    int m_inChannels = 3;   // 输入通道数 (EDSR 模型导出自 3 通道灰度, 运行时查询确认)
    int m_inH = -1;         // 模型固定输入高度 (动态模型为 -1)
    int m_inW = -1;         // 模型固定输入宽度 (EDSR 为 75 = 切片数; 动态模型为 -1)
    QString m_deviceLabel = QStringLiteral("CPU");  // 实际使用的设备 (CUDA:0 / CPU)
    bool m_ready = false;
};
#endif

} // namespace medical
