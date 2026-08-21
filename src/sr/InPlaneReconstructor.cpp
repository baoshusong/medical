#include "sr/InPlaneReconstructor.h"
#include "sr/BaseReconstructor.h"
#include "sr/ISuperResEngine.h"
#include "sr/HuNormalize.h"
#include "utils/Logger.h"

#include <QElapsedTimer>
#include <QImage>
#include <atomic>
#include <exception>
#include <limits>

namespace medical {
namespace {
constexpr quint64 kMaxOutputBytes = 384ULL * 1024ULL * 1024ULL;

bool outputFitsBudget(const DicomVolume &in, int upscale, int &newH, int &newW,
                      quint64 &bytes, QString &error)
{
    if (in.isEmpty() || upscale <= 0) {
        error = QStringLiteral("输入体数据或超分倍率无效。");
        return false;
    }
    const quint64 h = quint64(in.rows()) * quint64(upscale);
    const quint64 w = quint64(in.cols()) * quint64(upscale);
    const quint64 voxels = quint64(in.depth()) * h * w;
    if (h > quint64(std::numeric_limits<int>::max()) || w > quint64(std::numeric_limits<int>::max())
        || voxels > quint64(std::numeric_limits<int>::max())) {
        error = QStringLiteral("超分输出尺寸超出程序支持范围。");
        return false;
    }
    bytes = voxels * sizeof(float);
    if (bytes > kMaxOutputBytes) {
        error = QStringLiteral("请求输出 %1×%2×%3，约 %4 MiB，超过 384 MiB 安全限制。请导入更小序列或使用快速预览。")
                    .arg(in.depth()).arg(w).arg(h).arg(bytes / 1024 / 1024);
        return false;
    }
    newH = int(h);
    newW = int(w);
    return true;
}
}

InPlaneReconstructor::InPlaneReconstructor(QObject *parent)
    : BaseReconstructor(parent)
{
}

InPlaneReconstructor::~InPlaneReconstructor()
{
    requestCancel();
    if (m_thread.joinable()) m_thread.join();
}

void InPlaneReconstructor::reconstructAsync(const DicomVolume &in, const QString &modelPath, const QString &devicePref)
{
    if (m_running.exchange(true)) return;
    // A completed std::thread remains joinable until reaped.  At this point
    // all GPU cleanup has already happened on the worker, so this join is short.
    if (m_thread.joinable()) m_thread.join();
    m_cancelRequested = false;
    m_thread = std::thread([this, input = in, modelPath, devicePref]() mutable {
        run(std::move(input), modelPath, devicePref);
    });
}

void InPlaneReconstructor::run(DicomVolume in, QString modelPath, QString devicePref)
{
    SRStats stats;
    stats.inSlices = in.depth();
    QElapsedTimer timer;
    timer.start();

    // The inference engine is strictly worker-owned. In particular, its
    // CUDA session must be destroyed before the GUI is told that the job is idle.
    std::unique_ptr<ISuperResEngine> engine;

    const auto finish = [this, &stats, &timer, &engine]() {
        const bool hadEngine = bool(engine);
        const qint64 cleanupStart = timer.elapsed();
        engine.reset();
        const qint64 cleanupMs = timer.elapsed() - cleanupStart;
        stats.elapsedMs = timer.elapsed();
        if (hadEngine)
            LOG_INFO("sr", QStringLiteral("in-plane worker cleanup: %1 ms").arg(cleanupMs));
        m_stats = stats;
        m_running = false;
        emit finished(stats);
    };

    try {
        engine = ISuperResEngine::create(modelPath, devicePref);
        if (engine) {
            stats.upscale = engine->upscale();
            stats.engineName = engine->name();
            stats.engineDevice = engine->device();
        } else {
            stats.upscale = configuredUpscale();
            stats.engineName = QStringLiteral("none");
            stats.engineDevice = QStringLiteral("-");
        }

        if (in.isEmpty()) {
            stats.error = QStringLiteral("没有可重建的体数据。");
            finish();
            return;
        }
        if (!engine) {
            stats.error = QStringLiteral("超分引擎无法加载，请检查模型文件和运行库。");
            finish();
            return;
        }

        if (!m_result.isEmpty()) m_result = DicomVolume();
        int newH = 0, newW = 0;
        quint64 outputBytes = 0;
        if (!outputFitsBudget(in, stats.upscale, newH, newW, outputBytes, stats.error)) {
            finish();
            return;
        }

        DicomVolume output;
        if (!output.allocate(in.depth(), newH, newW,
                             in.spacingX() / stats.upscale, in.spacingY() / stats.upscale)) {
            stats.error = QStringLiteral("无法分配超分输出体数据。");
            finish();
            return;
        }

        const int stride = qMax(1, m_stride);
        const int total = (in.depth() + stride - 1) / stride;
        int done = 0;
        int lastDone = -1;
        for (int z = 0; z < in.depth(); z += stride) {
            if (m_cancelRequested) {
                stats.cancelled = true;
                stats.error = QStringLiteral("重建已取消。");
                finish();
                return;
            }
            const QImage slice = in.axialSlice(z);
            if (slice.isNull()) {
                stats.error = QStringLiteral("第 %1 层输入图像为空 (%2×%3)。")
                                  .arg(z + 1).arg(in.cols()).arg(in.rows());
                finish();
                return;
            }
            const QImage upImg = engine->upsampleImage(slice);
            if (upImg.isNull() || upImg.width() != newW || upImg.height() != newH) {
                stats.error = QStringLiteral("第 %1 层超分输出无效：输入 %2×%3，实际输出 %4×%5，期望 %6×%7。引擎：%8")
                                  .arg(z + 1).arg(slice.width()).arg(slice.height())
                                  .arg(upImg.width()).arg(upImg.height())
                                  .arg(newW).arg(newH).arg(stats.engineName);
                finish();
                return;
            }
            for (int y = 0; y < newH; ++y) {
                const auto *src = upImg.constBits() + y * upImg.bytesPerLine();
                for (int x = 0; x < newW; ++x)
                    output.setVoxel(z, y, x, hu::u8ToUnit(src[x]));
            }
            lastDone = z;
            emit progress(++done, total);
        }

        if (stride > 1) {
            int previous = -1;
            for (int z = 0; z < in.depth(); ++z) {
                if (m_cancelRequested) {
                    stats.cancelled = true;
                    stats.error = QStringLiteral("重建已取消。");
                    finish();
                    return;
                }
                if (z % stride == 0 && z <= lastDone) {
                    previous = z;
                    continue;
                }
                if (previous < 0) continue;
                for (int y = 0; y < newH; ++y)
                    for (int x = 0; x < newW; ++x)
                        output.setVoxel(z, y, x, output.voxel(previous, y, x));
            }
        }

        stats.outSlices = output.depth();
        stats.ok = true;
        m_result = std::move(output);
        LOG_INFO("sr", QStringLiteral("reconstruct in-plane %1x: %2 slices %3×%4 -> %5×%6, stride=%7, %8/%9")
                        .arg(stats.upscale).arg(in.depth()).arg(in.cols()).arg(in.rows())
                        .arg(newW).arg(newH).arg(stride).arg(stats.engineName, stats.engineDevice));
        emit progress(total, total);
    } catch (const std::bad_alloc &) {
        stats.error = QStringLiteral("内存不足，无法完成超分重建。");
        LOG_ERR("sr", stats.error);
    } catch (const std::exception &e) {
        stats.error = QStringLiteral("超分重建异常: %1").arg(e.what());
        LOG_ERR("sr", stats.error);
    } catch (...) {
        stats.error = QStringLiteral("超分重建发生未知异常。");
        LOG_ERR("sr", stats.error);
    }
    finish();
}

} // namespace medical
