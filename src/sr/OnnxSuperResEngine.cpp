#include "sr/OnnxSuperResEngine.h"

#ifdef USE_ONNXRUNTIME
#include "sr/HuNormalize.h"
#include "sr/OrtProviders.h"
#include "utils/Logger.h"

#include <QImage>
#include <QFileInfo>
#include <QDir>
#include <QElapsedTimer>
#include <vector>
#include <algorithm>
#include <cmath>
#include <memory>
#include <string>

// ORT 头兼容 (见 sr/OrtShim.h)
#include "sr/OrtShim.h"

namespace medical {

struct OnnxSuperResEngine::Impl
{
    Ort::Env        env{ORT_LOGGING_LEVEL_WARNING, "SwinIR-Med"};
    Ort::Session    session{nullptr};
    std::string inName, outName;
};

namespace {
// 用 128×128 零张量做一次推理自测, 验证 EP 是否真能执行 (CUDA 缺 cuDNN/CUDA 运行库时会抛异常).
// 输出张量放在 CPU, 避免 GPU 显存读取问题.
bool probeSession(Ort::Session &s, const std::string &inName, const std::string &outName)
{
    try {
        constexpr int S = 128, E = 512;
        std::vector<float> xin(size_t(S) * S, 0.f);
        int64_t inDims[4] = {1, 1, S, S};
        Ort::MemoryInfo mi = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value inT = Ort::Value::CreateTensor<float>(mi, xin.data(), xin.size(), inDims, 4);
        const char *inN[1] = {inName.c_str()};
        const char *outN[1] = {outName.c_str()};
        std::vector<float> yout(size_t(E) * E, 0.f);
        int64_t outDims[4] = {1, 1, E, E};
        Ort::Value outT = Ort::Value::CreateTensor<float>(mi, yout.data(), yout.size(), outDims, 4);
        s.Run(Ort::RunOptions{nullptr}, inN, &inT, 1, outN, &outT, 1);
        return true;
    } catch (...) { return false; }
}
} // namespace

static Ort::Session makeSession(Ort::Env &env, const QString &path,
                                std::string &inName, std::string &outName,
                                QString &device, const QString &devicePref)
{
    auto build = [&](bool gpu) -> Ort::Session {
        Ort::SessionOptions opts;
        opts.SetIntraOpNumThreads(4);
        opts.SetGraphOptimizationLevel(ORT_ENABLE_ALL);
        if (gpu) {
            // 选择执行提供者: GPU(CUDA) 优先, 不可用时 configureOrtProviders 内部已回退 CPU
            device = medical::configureOrtProviders(opts, devicePref);
        } else {
            device = QStringLiteral("CPU");
        }
        const auto wpath = path.toStdWString();
        return Ort::Session(env, wpath.c_str(), opts);
    };

    Ort::Session s = build(true);
    Ort::AllocatorWithDefaultOptions alloc;
    {
        auto in = s.GetInputNameAllocated(0, alloc);
        inName = in.get();
    }
    {
        auto out = s.GetOutputNameAllocated(0, alloc);
        outName = out.get();
    }

    // 若挂载了 CUDA, 自测其是否真能执行; 失败 (如缺 cuDNN/CUDA 运行库) 则回退 CPU 重建
    if (device.startsWith(QStringLiteral("CUDA"))) {
        if (!probeSession(s, inName, outName)) {
            LOG_WARN("onnx-sr", QStringLiteral("CUDA 自测失败（可能缺少 cuDNN 或 CUDA 运行库），已回退到 CPU。"));
            s = build(false);
            {
                auto in = s.GetInputNameAllocated(0, alloc);
                inName = in.get();
            }
            {
                auto out = s.GetOutputNameAllocated(0, alloc);
                outName = out.get();
            }
        }
    }
    return s;
}

bool OnnxSuperResEngine::load(const QString &modelPath, const QString &devicePref)
{
    if (!QFileInfo::exists(modelPath)) {
        LOG_ERR("onnx-sr", QStringLiteral("model not found: %1").arg(modelPath));
        return false;
    }
    // ORT 按模型文件所在目录加载外部权重 (.onnx.data), 无需改 cwd
    try {
        auto impl = std::make_unique<Impl>();
        impl->session = makeSession(impl->env, modelPath, impl->inName, impl->outName, m_device, devicePref);
        m_impl = impl.release();
        m_ready = true;
        LOG_INFO("onnx-sr", QStringLiteral("loaded %1  in=%2 out=%3")
                            .arg(modelPath)
                            .arg(QString::fromStdString(m_impl->inName))
                            .arg(QString::fromStdString(m_impl->outName)));
    } catch (const Ort::Exception &e) {
        LOG_ERR("onnx-sr", QStringLiteral("load failed: %1").arg(e.what()));
        delete m_impl;
        m_impl = nullptr;
        m_ready = false;
    } catch (const std::exception &e) {
        LOG_ERR("onnx-sr", QStringLiteral("load failed: %1").arg(e.what()));
        delete m_impl;
        m_impl = nullptr;
        m_ready = false;
    }
    return m_ready;
}

OnnxSuperResEngine::OnnxSuperResEngine(const QString &modelPath, const QString &devicePref) { load(modelPath, devicePref); }
OnnxSuperResEngine::~OnnxSuperResEngine() { delete m_impl; }

// 128×128 [0,1] -> 512×512 [0,1] (8-bit)
QImage OnnxSuperResEngine::inferPatch128(const QImage &patch)
{
    constexpr int S = 128;
    QImage in = patch.convertToFormat(QImage::Format_Grayscale8);
    if (!m_ready || !m_impl || in.width() != S || in.height() != S) return {};

    try {
        std::vector<float> xin(size_t(S) * S);
        for (int y = 0; y < S; ++y) {
            const auto *src = in.constBits() + y * in.bytesPerLine();
            for (int x = 0; x < S; ++x) {
                const float unit = hu::u8ToUnit(src[x]);
                xin[size_t(y) * S + x] = (unit - 0.5f) / 0.5f;
            }
        }

        int64_t inShape[4] = {1, 1, S, S};
        Ort::MemoryInfo mem = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        auto inT = Ort::Value::CreateTensor<float>(mem, xin.data(), xin.size(), inShape, 4);
        const char *inN[1] = {m_impl->inName.c_str()};
        const char *outN[1] = {m_impl->outName.c_str()};
        // 显式提供 CPU 输出张量, 让 ORT 把 (可能的) GPU 计算结果自动拷回 CPU,
        // 避免直接读取 GPU 显存导致的崩溃 (CUDA EP 默认把输出放在显存).
        const int expected = S * upscale();
        std::vector<float> yout(size_t(expected) * expected);
        int64_t outShape[4] = {1, 1, expected, expected};
        Ort::Value outT = Ort::Value::CreateTensor<float>(mem, yout.data(), yout.size(), outShape, 4);
        try {
            m_impl->session.Run(Ort::RunOptions{nullptr}, inN, &inT, 1, outN, &outT, 1);
        } catch (const Ort::Exception &e) {
            LOG_ERR("onnx-sr", QStringLiteral("inference failed: %1").arg(e.what()));
            return {};
        } catch (const std::exception &e) {
            LOG_ERR("onnx-sr", QStringLiteral("inference failed: %1").arg(e.what()));
            return {};
        }
        const float *hr = yout.data();
        if (!hr) return {};
        QImage out(expected, expected, QImage::Format_Grayscale8);
        for (int y = 0; y < expected; ++y) {
            auto *dst = out.scanLine(y);
            for (int x = 0; x < expected; ++x) {
                const float value = hr[size_t(y) * expected + x] * 0.5f + 0.5f;
                if (!std::isfinite(value)) {
                    LOG_ERR("onnx-sr", QStringLiteral("model returned non-finite pixel data"));
                    return {};
                }
                dst[x] = hu::unitTo8(value);
            }
        }
        return out;
    } catch (const Ort::Exception &e) {
        LOG_ERR("onnx-sr", QStringLiteral("inference failed: %1").arg(e.what()));
    } catch (const std::exception &e) {
        LOG_ERR("onnx-sr", QStringLiteral("inference failed: %1").arg(e.what()));
    }
    return {};
}

QImage OnnxSuperResEngine::upsampleImage(const QImage &img)
{
    if (!m_ready || img.isNull()) return img;

    const int H = img.height();
    const int W = img.width();
    constexpr int S = 128;
    const int up = upscale();

    QImage src = img.convertToFormat(QImage::Format_Grayscale8);
    const int nY = (H + S - 1) / S;
    const int nX = (W + S - 1) / S;
    const int bigH = nY * S * up;   // = 4H' (含 pad)
    const int bigW = nX * S * up;

    QImage big(bigW, bigH, QImage::Format_Grayscale8);
    big.fill(0);
    QImage patch(S, S, QImage::Format_Grayscale8);

    QElapsedTimer t; t.start();
    int calls = 0;
    for (int py = 0; py < nY; ++py) {
        for (int px = 0; px < nX; ++px) {
            patch.fill(0);
            for (int y = 0; y < S; ++y) {
                const int sy = py * S + y;
                if (sy >= H) break;
                const auto *s = src.constBits() + sy * src.bytesPerLine();
                auto *d = patch.scanLine(y);
                for (int x = 0; x < S; ++x) {
                    const int sx = px * S + x;
                    if (sx >= W) break;
                    d[x] = s[sx];
                }
            }
            QImage hr = inferPatch128(patch);
            ++calls;
            if (hr.isNull()) {
                LOG_ERR("onnx-sr", QStringLiteral("patch inference failed at (%1,%2) for %3×%4 image")
                                    .arg(px).arg(py).arg(W).arg(H));
                return {};
            }
            const int ow = hr.width(), oh = hr.height();
            const int ox = px * S * up, oy = py * S * up;
            for (int y = 0; y < oh; ++y) {
                const int ty = oy + y;
                if (ty >= bigH) break;
                const auto *s = hr.constBits() + y * hr.bytesPerLine();
                auto *d = big.scanLine(ty);
                for (int x = 0; x < ow; ++x) {
                    const int tx = ox + x;
                    if (tx >= bigW) break;
                    d[tx] = s[x];
                }
            }
        }
    }
    LOG_INFO("onnx-sr", QStringLiteral("img %1×%2, %3 patches, %4 ms")
                       .arg(W).arg(H).arg(calls).arg(t.elapsed()));

    // 面内两维同时 4×: 裁掉 pad, 输出精确 up*W × up*H
    return big.copy(0, 0, up * W, up * H);
}

} // namespace medical
#endif // USE_ONNXRUNTIME
