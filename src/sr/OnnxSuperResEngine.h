#pragma once

#include "sr/ISuperResEngine.h"
#include <QString>

#ifdef USE_ONNXRUNTIME
namespace medical {

// ONNX Runtime 真实引擎: 加载 SwinIR-Med (.onnx) 执行面内 4× 超分。
// 模型契约 (对齐 model/onnx.py):
//   输入 1×1×128×128 float32 [-1,1]  (HU clip(-1000,400)->/1400*255 uint8 ->/255 ->(x-0.5)/0.5)
//   输出 1×1×512×512 float32 [-1,1] -> *0.5+0.5 -> [0,1] -> *255 uint8
// 面内推理: 将任意尺寸影像切成 128×128 patch (右/下 pad), 各 patch 4× 超分,
//           拼回 (4W × 4H) —— 面内两维同时放大, 帧数不变。
class OnnxSuperResEngine : public ISuperResEngine
{
public:
    explicit OnnxSuperResEngine(const QString &modelPath,
                                const QString &devicePref = QStringLiteral("auto"));
    ~OnnxSuperResEngine() override;

    QString name() const override   { return QStringLiteral("ONNX SwinIR-Med"); }
    QString device() const override { return m_device; }

    bool ready() const { return m_ready; }

    QImage upsampleImage(const QImage &img) override;

private:
    QImage inferPatch128(const QImage &patch);  // 128×128 -> 512×512
    bool   load(const QString &modelPath, const QString &devicePref);

    struct Impl;
    Impl *m_impl = nullptr;
    bool   m_ready = false;
    QString m_device = QStringLiteral("CPU");
};

} // namespace medical
#endif // USE_ONNXRUNTIME
