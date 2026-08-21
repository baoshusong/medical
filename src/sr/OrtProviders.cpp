#include <thread>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>

#include <QLibrary>
#include <QSettings>
#include <QThread>
#include <QtGlobal>

#include <limits>

#include "utils/Logger.h"

#include "OrtShim.h"
#include "OrtProviders.h"

namespace fs = std::filesystem;

namespace medical {

namespace {

constexpr quint64 kDefaultCudaArenaMiB = 4096;

size_t cudaArenaLimitBytes()
{
    bool ok = false;
    const QByteArray configured = qgetenv("AI_MEDICAL_CUDA_ARENA_MIB");
    quint64 mib = configured.toULongLong(&ok);
    if (!ok || mib < 512)
        mib = kDefaultCudaArenaMiB;
    constexpr quint64 kMiB = 1024ULL * 1024ULL;
    const quint64 maxMiB = quint64(std::numeric_limits<size_t>::max()) / kMiB;
    return size_t(qMin(mib, maxMiB) * kMiB);
}

// 检查 cuDNN 运行库是否存在（CUDA 执行提供者必需，否则 onnxruntime_providers_cuda.dll 无法加载）
// 返回空串表示已找到；否则返回缺失说明。
std::string findCudnn() {
    std::vector<fs::path> dirs;

    // 1) CUDA_PATH\bin（用户按文档把 cuDNN 放这里）
    if (const char* cudaPath = std::getenv("CUDA_PATH"))
        dirs.push_back(fs::path(cudaPath) / "bin");

    // 2) 环境变量 PATH 中的各目录
    if (const char* pathEnv = std::getenv("PATH")) {
        std::string_view sv(pathEnv);
        size_t start = 0, pos;
        while ((pos = sv.find(';', start)) != std::string_view::npos) {
            std::string token = std::string(sv.substr(start, pos - start));
            if (!token.empty()) dirs.push_back(fs::path(token));
            start = pos + 1;
        }
        if (start < sv.size()) dirs.push_back(fs::path(std::string(sv.substr(start))));
    }

    for (const auto& d : dirs) {
        std::error_code ec;
        if (!fs::is_directory(d, ec)) continue;
        for (const auto& e : fs::directory_iterator(d, ec)) {
            const std::string name = e.path().filename().string();
            if (name.find("cudnn") != std::string::npos) {
                return "";  // 找到 cuDNN 运行库
            }
        }
    }
    return "在 CUDA_PATH\\bin 及 PATH 中均未找到 cudnn*.dll（CUDA 执行提供者依赖 cuDNN）";
}

// 动态加载 nvcuda.dll (CUDA 驱动, 位于系统目录), 取 device_id 对应的 GPU 名称.
// 不链接 CUDA SDK, 失败则返回空串(不影响 GPU 选择).
QString getCudaDeviceName(int device_id) {
    QLibrary cudaLib(QStringLiteral("nvcuda"));
    if (!cudaLib.load()) return {};
    using CuInitFn      = int (*)(unsigned int);
    using CuDevNameFn   = int (*)(char*, int, int);
    auto cuInit          = reinterpret_cast<CuInitFn>(cudaLib.resolve("cuInit"));
    auto cuDeviceGetName = reinterpret_cast<CuDevNameFn>(cudaLib.resolve("cuDeviceGetName"));
    if (!cuInit || !cuDeviceGetName) return {};
    if (cuInit(0) != 0) return {};
    char name[256] = {0};
    if (cuDeviceGetName(name, int(sizeof(name)), device_id) != 0) return {};
    return QString::fromLocal8Bit(name);
}

} // namespace

// 即时返回本机 CPU 型号 (Windows: 注册表 ProcessorNameString), 不创建任何推理会话.
QString describeCpuDevice() {
    QSettings reg(QStringLiteral(
        "HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"),
        QSettings::NativeFormat);
    QString cpu = reg.value(QStringLiteral("ProcessorNameString")).toString().trimmed();
    if (!cpu.isEmpty()) {
        // 去掉 (R)/(TM)/(C) 等商标标记, 让显示更干净
        cpu.replace(QStringLiteral("(R)"), QString())
            .replace(QStringLiteral("(TM)"), QString())
            .replace(QStringLiteral("(C)"), QString());
        cpu = cpu.simplified();
        return QStringLiteral("CPU (%1)").arg(cpu);
    }
    // 回退: 仅显示逻辑核数
    return QStringLiteral("CPU (%1 逻辑核)").arg(QThread::idealThreadCount());
}

// 即时返回 GPU 设备串 (CUDA:0 + 名称), 用于 UI 选择 GPU 后立刻展示型号.
QString describeGpuDevice() {
    QString name = getCudaDeviceName(0);
    if (name.isEmpty())
        return QStringLiteral("CUDA:0 (未检测到 CUDA 设备)");
    return QStringLiteral("CUDA:0 (%1)").arg(name);
}

QString configureOrtProviders(Ort::SessionOptions& opts, const QString& pref) {
    opts.SetIntraOpNumThreads(std::max(1u, std::thread::hardware_concurrency() / 2));
    opts.SetInterOpNumThreads(std::max(1u, std::thread::hardware_concurrency() / 2));
    opts.SetExecutionMode(ORT_PARALLEL);

    const bool preferGpu = pref == QStringLiteral("gpu") || pref == QStringLiteral("auto");
    if (preferGpu) {
        bool hasCuda = false;
        try {
            std::vector<std::string> avail = Ort::GetAvailableProviders();
            for (const auto& p : avail)
                if (p == "CUDAExecutionProvider") { hasCuda = true; break; }
        } catch (...) {
            hasCuda = false;
        }

        if (!hasCuda) {
            std::string detail = findCudnn();
            if (detail.empty())
                detail = "GetAvailableProviders() 未返回 CUDA（onnxruntime_providers_cuda.dll 加载失败，通常缺少 cuDNN 或 CUDA 运行库）";
            LOG_WARN("ort", QStringLiteral(
                "请求 GPU/自动计算，但当前环境无法加载 CUDA 执行提供者。%1。"
                "引擎将回退到 CPU。请安装与 CUDA 12.x 匹配的 cuDNN 9.x 运行库"
                "（将其 bin 目录加入 PATH 或放入 exe 所在目录）。").arg(QString::fromStdString(detail)));
            return QStringLiteral("CPU");
        }

        try {
            OrtCUDAProviderOptions cuda{};
            cuda.device_id = 0;
            // Heuristic avoids the expensive per-shape exhaustive cuDNN benchmark.
            cuda.cudnn_conv_algo_search = OrtCudnnConvAlgoSearchHeuristic;
            // The CUDA arena is bounded so that Windows DWM and Qt retain GPU
            // headroom. Override in MiB with AI_MEDICAL_CUDA_ARENA_MIB (>=512).
            cuda.gpu_mem_limit = cudaArenaLimitBytes();
            cuda.arena_extend_strategy = 1; // kSameAsRequested: avoid power-of-two over-allocation
            opts.AppendExecutionProvider_CUDA(cuda);
            LOG_INFO("ort", QStringLiteral("CUDA arena limit: %1 MiB")
                                .arg(qulonglong(cuda.gpu_mem_limit / (1024ULL * 1024ULL))));
            QString dev = QStringLiteral("CUDA:0");
            QString gpuName = getCudaDeviceName(0);
            if (!gpuName.isEmpty())
                dev = QStringLiteral("CUDA:0 (%1)").arg(gpuName);
            return dev;
        } catch (const Ort::Exception& e) {
            LOG_WARN("ort", QStringLiteral(
                "请求 GPU/自动计算，但追加 CUDA 执行提供者失败：%1 引擎将回退到 CPU。")
                .arg(QString::fromStdString(std::string(e.what()))));
            return QStringLiteral("CPU");
        } catch (...) {
            LOG_WARN("ort", QStringLiteral(
                "请求 GPU/自动计算，但追加 CUDA 执行提供者时发生未知错误，引擎将回退到 CPU。"));
            return QStringLiteral("CPU");
        }
    }

    // CPU EP is the implicit/default fallback in ONNX Runtime. Do not append it
    // before CUDA, because provider registration order defines priority.
    return QStringLiteral("CPU");
}

} // namespace medical
