#include "sr/InterSliceReconstructor.h"
#include "sr/ISuperResEngine.h"
#include "sr/HuNormalize.h"
#include "utils/Logger.h"

#include <QElapsedTimer>
#include <QImage>
#include <QVector>
#include <atomic>
#include <exception>

namespace medical {
namespace {
constexpr int kInterSliceMaxSlices = 75; // 层间超分: 取前 75 张切片作为一个序列
}

InterSliceReconstructor::InterSliceReconstructor(QObject *parent)
    : BaseReconstructor(parent)
{
}

InterSliceReconstructor::~InterSliceReconstructor()
{
    requestCancel();
    if (m_thread.joinable()) m_thread.join();
}

void InterSliceReconstructor::reconstructAsync(const DicomVolume &in, const QString &modelPath, const QString &devicePref)
{
    if (m_running.exchange(true)) return;
    // A completed std::thread remains joinable until reaped.  The worker has
    // already destroyed its CUDA session before m_running becomes false.
    if (m_thread.joinable()) m_thread.join();
    m_cancelRequested = false;
    m_thread = std::thread([this, input = in, modelPath, devicePref]() mutable {
        run(std::move(input), modelPath, devicePref);
    });
}

void InterSliceReconstructor::run(DicomVolume in, QString modelPath, QString devicePref)
{
    SRStats stats;
    stats.upscale = configuredUpscale();
    QElapsedTimer timer;
    timer.start();

    // Engine/session ownership never crosses the worker boundary.
    std::unique_ptr<ISuperResEngine> engine;

    const auto finish = [this, &stats, &timer, &engine]() {
        const bool hadEngine = bool(engine);
        const qint64 cleanupStart = timer.elapsed();
        engine.reset();
        const qint64 cleanupMs = timer.elapsed() - cleanupStart;
        stats.elapsedMs = timer.elapsed();
        if (hadEngine)
            LOG_INFO("sr", QStringLiteral("inter-slice worker cleanup: %1 ms").arg(cleanupMs));
        m_stats = stats;
        m_running = false;
        emit finished(stats);
    };

    try {
        engine = ISuperResEngine::create(modelPath, devicePref);
        if (engine) {
            stats.upscale = engine->widthUpscale();
            stats.engineName = engine->name();
            stats.engineDevice = engine->device();
        } else {
            stats.engineName = QStringLiteral("none");
            stats.engineDevice = QStringLiteral("-");
        }

        if (in.isEmpty()) {
            stats.error = QStringLiteral("没有可重建的体数据。");
            finish();
            return;
        }

        if (!engine || !engine->ready() || !engine->supportsWidthUpscale()) {
            stats.error = QStringLiteral("层间超分引擎未就绪或不支持层间宽度超分，请检查模型文件 (edsr_invsr_width4x.onnx) 与运行库。");
            LOG_ERR("sr", stats.error);
            finish();
            return;
        }

        const int H = in.rows();
        const int W = in.cols();
        const int Dsrc = in.depth();

        if (Dsrc < 2) {
            stats.error = QStringLiteral("层间超分至少需要 2 张切片（当前 %1 张）。").arg(Dsrc);
            finish();
            return;
        }

        // EDSR uses a fixed sequence width (normally 75); short studies are
        // padded with their final slice to meet the model contract.
        const int modelW = engine->modelInputWidth();
        const int D = (modelW > 0) ? modelW : qMin(Dsrc, kInterSliceMaxSlices);
        const int Dout = D * engine->widthUpscale();

        DicomVolume output;
        if (!output.allocate(Dout, H, W, in.spacingX(), in.spacingY())) {
            stats.error = QStringLiteral("无法分配层间超分输出体数据。");
            finish();
            return;
        }
        if (in.spacingZ() > 0.0f)
            output.setSpacingZ(in.spacingZ() / static_cast<float>(engine->widthUpscale()));

        QVector<float> T(static_cast<long long>(W) * H * D);
        for (int w = 0; w < W; ++w)
            for (int h = 0; h < H; ++h)
                for (int d = 0; d < D; ++d)
                    T[static_cast<long long>(w) * H * D + static_cast<long long>(h) * D + d]
                        = in.voxel(d < Dsrc ? d : Dsrc - 1, h, w);

        int done = 0;
        for (int w = 0; w < W; ++w) {
            if (m_cancelRequested) {
                stats.cancelled = true;
                stats.error = QStringLiteral("重建已取消。");
                finish();
                return;
            }

            QImage sheet(D, H, QImage::Format_Grayscale8);
            for (int h = 0; h < H; ++h) {
                uchar *row = sheet.scanLine(h);
                for (int d = 0; d < D; ++d)
                    row[d] = hu::unitTo8(
                        T[static_cast<long long>(w) * H * D + static_cast<long long>(h) * D + d]);
            }

            const QImage sr = engine->upsampleWidth(sheet);
            if (sr.isNull() || sr.width() != Dout || sr.height() != H) {
                stats.error = QStringLiteral("层间超分输出尺寸异常：期望 %1×%2，实际 %3×%4。")
                                  .arg(Dout).arg(H).arg(sr.width()).arg(sr.height());
                LOG_ERR("sr", stats.error);
                finish();
                return;
            }

            for (int h = 0; h < H; ++h) {
                const uchar *row = sr.constScanLine(h);
                for (int d2 = 0; d2 < Dout; ++d2)
                    output.setVoxel(d2, h, w, hu::u8ToUnit(row[d2]));
            }

            emit progress(++done, W);
        }

        stats.inSlices = in.depth();
        stats.outSlices = output.depth();
        stats.ok = true;
        m_result = std::move(output);
        LOG_INFO("sr", QStringLiteral("reconstruct inter-slice 4x: %1 slices -> %2 slices (%3×%4×%5), engine=%6")
                        .arg(D).arg(Dout).arg(W).arg(H).arg(Dout).arg(stats.engineName));
        emit progress(W, W);
    } catch (const std::bad_alloc &) {
        stats.error = QStringLiteral("内存不足，无法完成层间超分重建。");
        LOG_ERR("sr", stats.error);
    } catch (const std::exception &e) {
        stats.error = QStringLiteral("层间超分异常: %1").arg(e.what());
        LOG_ERR("sr", stats.error);
    } catch (...) {
        stats.error = QStringLiteral("层间超分发生未知异常。");
        LOG_ERR("sr", stats.error);
    }
    finish();
}

} // namespace medical
