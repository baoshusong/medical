#pragma once

#include <QString>

#ifdef USE_ONNXRUNTIME
#include "sr/OrtShim.h"

namespace medical {

// 为 ORT 会话配置执行提供者 (EP)。
//   pref: "auto"(默认, GPU 可用则用 GPU, 否则 CPU) / "cpu" / "gpu"
//   返回实际使用的设备标签: "CUDA:0" 或 "CPU"。
// CPU EP 始终默认存在, 故 GPU 不可用时自动回退 CPU (满足"没有 GPU 就用 CPU")。
QString configureOrtProviders(Ort::SessionOptions &opts,
                              const QString &pref = QStringLiteral("auto"));

// 即时查询设备型号, 用于"计算设备"选择后立刻在界面展示 (不真正创建推理会话):
//   describeCpuDevice() -> "CPU (Intel Core i7-11800H ...)" 等本机 CPU 型号
//   describeGpuDevice() -> "CUDA:0 (NVIDIA GeForce RTX 5060 Laptop GPU)"
//                         或驱动不可用时 "CUDA:0 (未检测到 CUDA 设备)"
// 这样用户选择 CPU/GPU 后即可立即看到对应设备的型号, 无需等待重建完成。
QString describeCpuDevice();
QString describeGpuDevice();

} // namespace medical
#endif
