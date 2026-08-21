#pragma once

#include "core/DicomFrame.h"

namespace medical {

// ITK/SimpleITK 预处理/配准/分割接入点。
// USE_ITK 开启后填充实现：归一化、重采样到等距、多模态配准、肺实质分割。
class ItkProcessor
{
public:
    // 归一化 HU 并 resample 到目标 spacing，返回处理后的帧。
    static DicomFrame normalize(const DicomFrame &in);
};

} // namespace medical
