#include "dicom/MockDicomLoader.h"
#include "utils/Logger.h"

#include <QImage>
#include <QPainter>
#include <QDateTime>
#include <QtMath>
#include <algorithm>
#include <cmath>

namespace medical {

bool MockDicomLoader::load(const QString &path, Study &studyOut)
{
    LOG_INFO("mock-dicom", QStringLiteral("load synthetic study (path=%1)").arg(path));

    studyOut.studyUid     = QStringLiteral("1.2.840 MOCK SYNTHETIC");
    studyOut.accessionNumber = QStringLiteral("ACC-000001");
    studyOut.modality     = QStringLiteral("CT");
    studyOut.description  = QStringLiteral("胸部高分辨 CT (合成演示数据)");
    studyOut.dateTime     = QDateTime::currentDateTime();
    studyOut.seriesCount  = 1;
    studyOut.frameCount   = 12;

    studyOut.patient.patientId = QStringLiteral("P-100024");
    studyOut.patient.name      = QStringLiteral("演示·张三");
    studyOut.patient.sex       = QStringLiteral("男");
    studyOut.patient.age       = 54;

    generateSyntheticCt(studyOut);
    return true;
}

// 生成 12 帧胸部 CT 切片：肺区暗、胸壁亮、随切片位置变化，
// 并在中部切片植入一枚模拟肺结节。
void MockDicomLoader::generateSyntheticCt(Study &study)
{
    const int W = 512, H = 512;
    const int slices = study.frameCount;

    m_frames.clear();
    m_frames.reserve(slices);

    for (int z = 0; z < slices; ++z) {
        DicomFrame f;
        f.width = W; f.height = H;
        f.defaultWindow = WindowLevel::lung();
        f.seriesUid = QStringLiteral("1.2.840 MOCK SERIES");
        f.rawPixels.resize(W * H);

        QImage img(W, H, QImage::Format_Grayscale8);
        const float t = float(z) / float(slices - 1);   // 0..1 切片进度

        // 肺结节中心 (中部切片出现)
        const QPointF noduleCenter(W * 0.62, H * 0.42);
        const float noduleR = 10.0f * (t > 0.35f && t < 0.65f ? 1.0f : 0.0f);

        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                const float dx = x - W * 0.5f;
                const float dy = y - H * 0.5f;
                const float r  = std::sqrt(dx * dx + dy * dy);
                const float bodyR = W * (0.30f + 0.05f * std::sin(t * 3.14f));

                // HU 近似：背景 -1000(空气) / 肺 -700 / 软组织 40 / 骨 600
                float hu = -1000.0f;
                if (r < bodyR) {
                    hu = -700.0f; // 肺实质
                    // 胸壁软组织
                    if (r > bodyR - 26.0f) hu = 40.0f;
                    // 脊柱
                    const float spineDx = x - W * 0.5f;
                    const float spineDy = y - H * 0.72f;
                    if (std::sqrt(spineDx * spineDx + spineDy * spineDy) < 30.0f) hu = 600.0f;
                }
                // 模拟肺结节
                const float nd = std::sqrt((x - noduleCenter.x()) * (x - noduleCenter.x())
                                         + (y - noduleCenter.y()) * (y - noduleCenter.y()));
                if (noduleR > 0 && nd < noduleR) hu = 80.0f;

                f.rawPixels[y * W + x] = static_cast<unsigned short>(hu + 1000);

                // 按 Lung 窗渲染为 8-bit
                const float wc = -600.0f, ww = 1500.0f;
                float v = (hu - (wc - ww * 0.5f)) / ww;
                v = std::clamp(v, 0.0f, 1.0f);
                img.setPixel(x, y, static_cast<quint8>(v * 255.0f));
            }
        }
        f.image = img;
        m_frames.append(std::move(f));
    }
}

} // namespace medical
