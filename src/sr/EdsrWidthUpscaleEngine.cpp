#include "sr/EdsrWidthUpscaleEngine.h"
#include "sr/OrtProviders.h"
#include "utils/Logger.h"

#include <QFileInfo>

#ifdef USE_ONNXRUNTIME

#include <vector>
#include <string>
#include <memory>

namespace medical {

// ── 归一化约定 (对齐 EDSR_models_4x.py) ────────────────────
// 体素在 DicomVolume 中以 unit∈[0,1] 存储, unit = huToUnit(hu) = (clip(hu,-1000,400)+1000)/1400.
// 模型 (EDSR-InvSR) 训练时的预处理链:
//   uint8 = unit*255  ->  ToTensor(/255)=unit  ->  Normalize(mean=0.5,std=0.5)  =>  2*unit - 1  (∈[-1,1])
// 后处理链:
//   unit = (hr + 1) / 2  ->  uint8 = unit*255   (取输出通道 0, 与 python [... ,0] 一致)
// 因此本引擎把 8-bit 灰度(=unit*255) 按 2*(p/255)-1 送入模型, 输出按 (y+1)/2 还原.
static constexpr float kUnitToModel = 2.0f;   // unit -> model 斜率
static constexpr float kUnitToModelBias = -1.0f; // unit -> model 偏置
static constexpr float kModelToUnit = 0.5f;   // model -> unit 斜率
static constexpr float kModelToUnitBias = 0.5f; // model -> unit 偏置

namespace {
void gatherIo(Ort::Session &s, std::string &inName, std::string &outName, int &inChannels, int &inH, int &inW)
{
    Ort::AllocatorWithDefaultOptions alloc;
    { auto in = s.GetInputNameAllocated(0, alloc); inName = in.get(); }
    { auto out = s.GetOutputNameAllocated(0, alloc); outName = out.get(); }
    try {
        auto tinfo = s.GetInputTypeInfo(0).GetTensorTypeAndShapeInfo();
        auto shp = tinfo.GetShape();
        if (shp.size() >= 2 && shp[1] > 0)
            inChannels = static_cast<int>(shp[1]);
        // 记录固定输入尺寸 (EDSR 为固定 1×C×512×75; 动态模型此处为 -1).
        if (shp.size() >= 4) {
            if (shp[2] > 0) inH = static_cast<int>(shp[2]);
            if (shp[3] > 0) inW = static_cast<int>(shp[3]);
        }
    } catch (...) { /* 保持默认 3 通道 */ }
}

// 用真实模型输入尺寸做一次推理自测, 验证 EP 是否真能执行 (CUDA 缺 cuDNN/CUDA 运行库时会抛异常).
// 注意: EDSR 模型输入尺寸是固定的 (如 512×75), 不可用 8×8 占位张量, 否则会因尺寸不符直接抛异常,
// 被误判为 "CUDA 自测失败" 而回退 CPU (这正是此前 "选了 GPU 却没用上 GPU" 的原因).
// 动态尺寸模型 (inH/inW<=0) 才回退到 8×8 占位.
bool probeSession(Ort::Session &s, const std::string &inName, const std::string &outName, int channels, int inH, int inW)
{
    try {
        const int c = channels;
        const int H = inH > 0 ? inH : 8;
        const int W = inW > 0 ? inW : 8;
        std::vector<float> xin(static_cast<size_t>(H) * W * c, 0.f);
        const int64_t inDims[4] = {1, c, H, W};
        Ort::MemoryInfo mi = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value inT = Ort::Value::CreateTensor<float>(mi, xin.data(), xin.size(), inDims, 4);
        const char *inN[1] = {inName.c_str()};
        const char *outN[1] = {outName.c_str()};
        const int outH = H, outW = W * 4;
        std::vector<float> yout(static_cast<size_t>(c) * outH * outW, 0.f);
        const int64_t outDims[4] = {1, c, outH, outW};
        Ort::Value outT = Ort::Value::CreateTensor<float>(mi, yout.data(), yout.size(), outDims, 4);
        s.Run(Ort::RunOptions{nullptr}, inN, &inT, 1, outN, &outT, 1);
        return true;
    } catch (...) { return false; }
}
} // namespace

EdsrWidthUpscaleEngine::EdsrWidthUpscaleEngine(const QString &modelPath, const QString &devicePref)
{
    if (!QFileInfo::exists(modelPath)) {
        LOG_ERR("edsr-sr", QStringLiteral("model not found: %1").arg(modelPath));
        return;
    }
    try {
        Ort::SessionOptions opts;
        opts.SetIntraOpNumThreads(4);
        opts.SetGraphOptimizationLevel(ORT_ENABLE_ALL);
        // 选择执行提供者: GPU(CUDA) 优先, 不可用时自动回退 CPU
        m_deviceLabel = medical::configureOrtProviders(opts, devicePref);
        const auto wpath = modelPath.toStdWString();
        Ort::Session s(m_env, wpath.c_str(), opts);
        gatherIo(s, m_inName, m_outName, m_inChannels, m_inH, m_inW);
        m_session = std::make_unique<Ort::Session>(std::move(s));

        // 自测 GPU 是否真能执行; 若 CUDA EP 运行期失败 (如缺 cuDNN/CUDA 运行库), 回退 CPU 重建.
        if (m_deviceLabel.startsWith(QStringLiteral("CUDA"))) {
            if (!probeSession(*m_session, m_inName, m_outName, m_inChannels, m_inH, m_inW)) {
                LOG_WARN("edsr-sr", QStringLiteral("CUDA 自测失败（可能缺少 cuDNN 或 CUDA 运行库），已回退到 CPU。"));
                m_session.reset();
                Ort::SessionOptions copts;
                copts.SetIntraOpNumThreads(4);
                copts.SetGraphOptimizationLevel(ORT_ENABLE_ALL);
                Ort::Session cs(m_env, wpath.c_str(), copts);
                gatherIo(cs, m_inName, m_outName, m_inChannels, m_inH, m_inW);
                m_session = std::make_unique<Ort::Session>(std::move(cs));
                m_deviceLabel = QStringLiteral("CPU");
            }
        }
        m_ready = true;
    } catch (const std::exception &e) {
        LOG_ERR("edsr-sr", QStringLiteral("session failed: %1").arg(QString::fromLocal8Bit(e.what())));
    }
}

QImage EdsrWidthUpscaleEngine::upsampleWidth(const QImage &in) const
{
    if (!m_ready) return in.copy();

    QImage img = in.convertToFormat(QImage::Format_Grayscale8);
    const int H = img.height();
    const int W = img.width();
    const int C = m_inChannels;   // 1 或 3

    // 灰度 uint8 -> 归一化 float (NCHW: [1,C,H,W]); 灰度复制到各通道
    std::vector<float> xin(static_cast<size_t>(H) * W * C);
    for (int y = 0; y < H; ++y) {
        const uchar *row = img.constScanLine(y);
        for (int x = 0; x < W; ++x) {
            const float m = (float(row[x]) / 255.0f) * kUnitToModel + kUnitToModelBias;
            for (int c = 0; c < C; ++c)
                xin[static_cast<size_t>((c * H + y) * W + x)] = m;
        }
    }

    const int64_t inDims[4] = {1, C, H, W};
    Ort::MemoryInfo mi = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    Ort::Value inT = Ort::Value::CreateTensor<float>(mi, xin.data(), xin.size(), inDims, 4);
    const char *inNames[1]  = {m_inName.c_str()};
    const char *outNames[1] = {m_outName.c_str()};

    // 宽度方向 4× 超分: 输出 [1,C,H,W*4]. 显式提供 CPU 输出张量, ORT 会把 GPU 计算结果自动拷回 CPU,
    // 避免直接读取 GPU 显存导致的崩溃 (CUDA EP 默认把输出放在显存).
    const int outH = H;
    const int outW = W * widthUpscale();
    std::vector<float> yout(static_cast<size_t>(C) * outH * outW);
    const int64_t outDims[4] = {1, C, outH, outW};
    Ort::Value outT = Ort::Value::CreateTensor<float>(mi, yout.data(), yout.size(), outDims, 4);

    m_session->Run(Ort::RunOptions{nullptr}, inNames, &inT, 1, outNames, &outT, 1);
    const float *outData = yout.data();

    // 模型输出(通道0, [-1,1]) -> unit [0,1] -> 灰度 uint8
    QImage out(outW, outH, QImage::Format_Grayscale8);
    for (int y = 0; y < outH; ++y) {
        uchar *row = out.scanLine(y);
        for (int x = 0; x < outW; ++x) {
            const float unit = outData[static_cast<size_t>(y) * outW + x] * kModelToUnit + kModelToUnitBias;
            int p = static_cast<int>(unit * 255.0f);
            row[x] = static_cast<uchar>(p < 0 ? 0 : p > 255 ? 255 : p);
        }
    }
    return out;
}

QImage EdsrWidthUpscaleEngine::upsampleImage(const QImage &img)
{
    // 该模型只做宽度方向超分; 面内重建不使用本引擎.
    return upsampleWidth(img);
}

} // namespace medical

#endif // USE_ONNXRUNTIME
