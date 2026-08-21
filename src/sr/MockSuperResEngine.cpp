#include "sr/MockSuperResEngine.h"

namespace medical {

// plane: width=H, height=D(=rows=Z). 输出 width=H, height=upscale*D。
QImage MockSuperResEngine::upsampleImage(const QImage &img)
{
    if (img.isNull()) return {};
    const int up = upscale();
    // 面内两维同时 4× (128×128 -> 512×512)
    return img.scaled(img.width() * up, img.height() * up,
                      Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

} // namespace medical
