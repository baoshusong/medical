#include "ai/OnnxInferenceEngine.h"

#ifdef USE_ONNXRUNTIME
#include "utils/Logger.h"
#include "sr/HuNormalize.h"
#include "sr/OrtProviders.h"
#include "sr/OrtShim.h"

#include <QFileInfo>
#include <QElapsedTimer>
#include <vector>
#include <string>
#include <memory>
#include <cmath>

namespace medical {

struct OnnxInferenceEngine::Impl
{
    Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "AiAnalysis"};
    Ort::Session session{nullptr};
    std::string inName;
    std::string outName;
    QString path;
    bool ok = false;
};

OnnxInferenceEngine::OnnxInferenceEngine() = default;
OnnxInferenceEngine::~OnnxInferenceEngine() { delete m_impl; }

static Ort::Session makeSession(Ort::Env &env, const QString &path,
                                std::string &inName, std::string &outName,
                                QString &device, bool gpu)
{
    Ort::SessionOptions opts;
    opts.SetIntraOpNumThreads(4);
    opts.SetGraphOptimizationLevel(ORT_ENABLE_ALL);
    if (gpu)
        device = medical::configureOrtProviders(opts, QStringLiteral("gpu"));
    else
        device = QStringLiteral("CPU");
    const auto wpath = path.toStdWString();
    Ort::Session s(env, wpath.c_str(), opts);
    Ort::AllocatorWithDefaultOptions alloc;
    {
        auto in = s.GetInputNameAllocated(0, alloc);
        inName = in.get();
    }
    {
        auto out = s.GetOutputNameAllocated(0, alloc);
        outName = out.get();
    }
    return s;
}

AiResult OnnxInferenceEngine::infer(const DicomFrame &frame, const QString &modelPath)
{
    AiResult out;
    out.modelName = QStringLiteral("ONNX Runtime");
    if (modelPath.isEmpty() || !QFileInfo::exists(modelPath)) {
        LOG_WARN("onnx-ai", QStringLiteral("model path empty or not found: ") + modelPath);
        out.summary = QStringLiteral("模型文件缺失");
        return out;
    }
    try {
        if (!m_impl || m_impl->path != modelPath) {
            auto impl = std::make_unique<Impl>();
            QString device;
            impl->session = makeSession(impl->env, modelPath, impl->inName, impl->outName, device, true);
            impl->path = modelPath;
            impl->ok = true;
            delete m_impl;
            m_impl = impl.release();
            m_deviceStr = device;
        }
        Impl *im = m_impl;
        if (!im->ok) { out.summary = QStringLiteral("模型加载失败"); return out; }

        const int H = frame.height, W = frame.width;
        if (H <= 0 || W <= 0) { out.summary = QStringLiteral("帧尺寸无效"); return out; }

        // 输入: 单帧 [1,1,H,W], 由窗位 8-bit 图转 [0,1]
        const QImage img = frame.image.convertToFormat(QImage::Format_Grayscale8);
        std::vector<float> inData(size_t(H) * W);
        for (int y = 0; y < H; ++y) {
            const uchar *s = img.constScanLine(y);
            for (int x = 0; x < W; ++x)
                inData[size_t(y) * W + x] = hu::u8ToUnit(s[x]);
        }
        int64_t inDims[4] = {1, 1, H, W};
        Ort::MemoryInfo mem = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        auto inT = Ort::Value::CreateTensor<float>(mem, inData.data(), inData.size(), inDims, 4);

        // 读取输出形状 → 分配输出缓冲
        auto outType = im->session.GetOutputTypeInfo(0);
        auto tensorInfo = outType.GetTensorTypeAndShapeInfo();
        std::vector<int64_t> outShape = tensorInfo.GetShape();
        size_t outTotal = 1;
        for (int64_t d : outShape)
            if (d > 0) outTotal *= size_t(d);
        std::vector<float> yout(outTotal);
        auto outT = Ort::Value::CreateTensor<float>(mem, yout.data(), yout.size(),
                                                    outShape.data(), outShape.size());

        const char *inN[1] = {im->inName.c_str()};
        const char *outN[1] = {im->outName.c_str()};
        QElapsedTimer t; t.start();
        im->session.Run(Ort::RunOptions{nullptr}, inN, &inT, 1, outN, &outT, 1);
        const double elapsed = t.elapsed() / 1000.0;

        const double sx = frame.spacingX, sy = frame.spacingY;

        // 启发式解析:
        if (outShape.size() == 4 && outShape[1] == 1) {
            // 单通道概率掩码 [1,1,OH,OW] (可能已与输入同尺寸)
            size_t pos = 0;
            for (float v : yout) if (v > 0.5f) ++pos;
            const double areaMm2 = double(pos) * sx * sy;
            out.totalPositive = int(pos);
            out.summary = QStringLiteral("单帧分割：阳性区域 %1 像素，面积约 %2 mm²（用时 %3 s）")
                .arg(pos).arg(areaMm2, 0, 'f', 1).arg(elapsed, 0, 'f', 2);
        } else if (outShape.size() == 2 && outShape[1] >= 5) {
            // 检测输出 [N, k]，每行 x,y,w,h,score[,class]
            const int n = int(outShape[0]);
            const int k = int(outShape[1]);
            for (int i = 0; i < n; ++i) {
                const float *row = yout.data() + size_t(i) * k;
                Annotation a;
                a.type = Annotation::Box;
                a.rect = QRectF(row[0], row[1], row[2], row[3]);
                a.score = k >= 5 ? row[4] : 0.f;
                a.label = k >= 6 ? QString::number(int(row[5])) : QStringLiteral("检测框");
                out.detections.append(a);
            }
            out.totalPositive = n;
            out.summary = QStringLiteral("检出 %1 个目标（用时 %2 s）")
                .arg(n).arg(elapsed, 0, 'f', 2);
        } else {
            out.summary = QStringLiteral("输出形状 [%1] 暂不支持解析（用时 %2 s）")
                .arg(outShape.size()).arg(elapsed, 0, 'f', 2);
        }
        LOG_INFO("onnx-ai", QStringLiteral("infer done: ") + modelPath + QStringLiteral(", outputs=") + QString::number(outTotal));
    } catch (const std::exception &e) {
        LOG_WARN("onnx-ai", QStringLiteral("infer failed: ") + QString::fromLocal8Bit(e.what()));
        out.summary = QStringLiteral("推理失败：%1").arg(QString::fromLocal8Bit(e.what()));
    }
    return out;
}

} // namespace medical
#endif // USE_ONNXRUNTIME
