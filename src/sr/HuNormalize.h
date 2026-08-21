#pragma once

#include <QImage>

namespace medical {

// HU 值归一化 (与 127_8.py read_ima_image 对齐):
//   clip(-1000, 400) -> (hu + 1000) / 1400 -> [0,1]
// 反归一化: hu = v*1400 - 1000
namespace hu {

constexpr float HU_MIN   = -1000.0f;
constexpr float HU_MAX   =   400.0f;
constexpr float HU_SPAN  =  1400.0f;

inline float huToUnit(float hu)  { return (std::clamp(hu, HU_MIN, HU_MAX) - HU_MIN) / HU_SPAN; }
inline float unitToHu(float v)   { return v * HU_SPAN + HU_MIN; }
inline quint8 unitTo8(float v)   { return static_cast<quint8>(std::clamp(v, 0.0f, 1.0f) * 255.0f); }
inline float  u8ToUnit(quint8 v) { return v / 255.0f; }

} // namespace hu

} // namespace medical
