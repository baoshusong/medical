#include "sr/ISuperResEngine.h"
#include "sr/MockSuperResEngine.h"
#include "utils/Logger.h"

#ifdef USE_ONNXRUNTIME
#  include "sr/OnnxSuperResEngine.h"
#  include "sr/EdsrWidthUpscaleEngine.h"
#endif

#include <memory>

namespace medical {

std::unique_ptr<ISuperResEngine> ISuperResEngine::create(const QString &modelPath, const QString &devicePref)
{
#ifdef USE_ONNXRUNTIME
    if (!modelPath.isEmpty()) {
        // 层间超分模型 (EDSR-InvSR, 仅宽度方向 4×) 与面内模型 (SwinIR-Med) 走不同引擎
        const QString lower = modelPath.toLower();
        const bool isEdsr = lower.contains(QLatin1String("edsr")) ||
                            lower.contains(QLatin1String("invsr")) ||
                            lower.contains(QLatin1String("width4x")) ||
                            lower.contains(QLatin1String("width_4x"));
        std::unique_ptr<ISuperResEngine> eng = isEdsr
            ? std::unique_ptr<ISuperResEngine>(new EdsrWidthUpscaleEngine(modelPath, devicePref))
            : std::unique_ptr<ISuperResEngine>(new OnnxSuperResEngine(modelPath, devicePref));
        if (eng && eng->ready())
            return eng;
        LOG_ERR("sr", QStringLiteral("ONNX model failed to load: %1").arg(modelPath));
        return nullptr;
    }
#endif
    return std::make_unique<MockSuperResEngine>();
}

} // namespace medical
