#pragma once

#include <QImage>
#include <QString>
#include <memory>

namespace medical {

// 超分推理引擎接口: 对一张 2D 影像执行面内 4×(可配) 超分 (两维同时放大)。
// 实现: MockSuperResEngine(双三次,默认) / OnnxSuperResEngine(USE_ONNXRUNTIME,真实SwinIR-Med)
//   输入:  灰度 QImage (任意尺寸, 像素=HU 归一化 uint8[0,255], 即 read_ima_image 输出)
//   输出:  灰度 QImage, 两维均放大 upscale 倍 (128×128 -> 512×512)
class ISuperResEngine
{
public:
    virtual ~ISuperResEngine() = default;
    virtual QString name() const = 0;
    virtual QString device() const = 0;
    virtual int     upscale() const { return 4; }

    // 层间超分: 仅沿"宽度"方向放大 widthUpscale 倍, 高度不变. 默认等于 upscale().
    virtual int     widthUpscale() const { return upscale(); }

    // 模型固定输入尺寸 (EDSR 层间模型为固定 1×C×512×75); 动态模型返回 -1.
    // 用于层间重建时把切片序列填充/裁剪到模型要求的固定宽度, 避免 GPU/CPU 推理尺寸错误.
    virtual int     modelInputHeight() const { return -1; }
    virtual int     modelInputWidth()  const { return -1; }

    // 引擎是否就绪 (模型加载成功). 默认 true.
    virtual bool    ready() const { return true; }

    // 面内 4× 超分: 输入 W×H, 输出 (upscale*W)×(upscale*H)
    virtual QImage upsampleImage(const QImage &img) = 0;

    // Whether this engine implements learned inter-slice width upscaling.
    // The default Qt interpolation helper is intentionally not accepted by
    // InterSliceReconstructor as a medical SR model.
    virtual bool supportsWidthUpscale() const { return false; }

    // 沿宽度方向超分 (层间超分用): 输入 W×H, 输出 (widthUpscale*W)×H.
    // 默认 Mock 实现为双线性放大, 子类(如 EDSR 引擎)重写为真实推理.
    virtual QImage upsampleWidth(const QImage &img) const
    {
        return img.scaled(img.width() * widthUpscale(), img.height(),
                          Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    static std::unique_ptr<ISuperResEngine> create(const QString &modelPath = {},
                                                    const QString &devicePref = QStringLiteral("auto"));
};

} // namespace medical
