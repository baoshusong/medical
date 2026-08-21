#include "sr/DicomVolume.h"
#include "sr/HuNormalize.h"
#include <QtMath>
#include <cmath>
#include <limits>
#include <vector>

namespace medical {

void DicomVolume::fromFrames(const QVector<DicomFrame> &frames)
{
    m_d = frames.size();
    if (m_d == 0) { m_data.clear(); return; }
    m_h = frames[0].height;
    m_w = frames[0].width;
    m_sx = frames[0].spacingX > 0 ? frames[0].spacingX : 0.7f;
    m_sy = frames[0].spacingY > 0 ? frames[0].spacingY : 0.7f;
    // 层间距: 优先用相邻切片 ImagePositionPatient 投影差; 否则回退 SliceThickness
    if (m_d >= 2 && frames[0].hasSlicePosition && frames[1].hasSlicePosition)
        m_sz = std::fabs(frames[1].slicePosition - frames[0].slicePosition);
    else
        m_sz = frames[0].sliceThickness > 0 ? frames[0].sliceThickness : 0.0f;
    m_data.fill(0.0f, size_t(m_d) * m_h * m_w);

    for (int z = 0; z < m_d; ++z) {
        const auto &f = frames[z];
        const int h = qMin(f.height, m_h);
        const int w = qMin(f.width,  m_w);
        for (int y = 0; y < h; ++y) {
            const auto *src = (f.rawPixels.isEmpty()) ? nullptr
                            : f.rawPixels.constData() + y * f.width;
            for (int x = 0; x < w; ++x) {
                // rawPixels 存的是 hu + 1000
                float hu = src ? (float(src[x]) - 1000.0f) : 0.0f;
                m_data[size_t(z) * m_h * m_w + y * m_w + x] = hu::huToUnit(hu);
            }
        }
    }
}

bool DicomVolume::allocate(int depth, int rows, int cols, float spacingX, float spacingY, float spacingZ)
{
    if (depth <= 0 || rows <= 0 || cols <= 0) return false;
    const quint64 count = quint64(depth) * quint64(rows) * quint64(cols);
    if (count > quint64(std::numeric_limits<int>::max())) return false;
    m_d = depth;
    m_h = rows;
    m_w = cols;
    m_sx = spacingX > 0 ? spacingX : 0.7f;
    m_sy = spacingY > 0 ? spacingY : 0.7f;
    m_sz = spacingZ > 0 ? spacingZ : 0.0f;
    m_data.fill(0.0f, int(count));
    return true;
}

float DicomVolume::voxel(int z, int y, int x) const
{
    if (z < 0 || z >= m_d || y < 0 || y >= m_h || x < 0 || x >= m_w) return 0.0f;
    return m_data[size_t(z) * m_h * m_w + y * m_w + x];
}

void DicomVolume::setVoxel(int z, int y, int x, float v)
{
    if (z < 0 || z >= m_d || y < 0 || y >= m_h || x < 0 || x >= m_w) return;
    m_data[size_t(z) * m_h * m_w + y * m_w + x] = v;
}

QImage DicomVolume::makeImage(int w, int h) const
{
    return QImage(w, h, QImage::Format_Grayscale8);
}

float DicomVolume::mapDisplay(float unit) const
{
    if (!m_useWindow) return unit;
    // unit -> hu -> lung 窗 -> [0,1]
    const float hu = hu::unitToHu(unit);
    const float lo = m_wc - m_ww * 0.5f;
    return std::clamp((hu - lo) / std::max(1.0f, m_ww), 0.0f, 1.0f);
}

QImage DicomVolume::axialSlice(int z) const
{
    QImage img = makeImage(m_w, m_h);
    if (z < 0) z = 0; if (z >= m_d) z = m_d - 1;
    for (int y = 0; y < m_h; ++y) {
        auto *dst = img.scanLine(y);
        for (int x = 0; x < m_w; ++x)
            dst[x] = hu::unitTo8(mapDisplay(voxel(z, y, x)));
    }
    return img;
}

// 行=Z(depth), 列=Y(rows)
QImage DicomVolume::sagittalPlane(int x) const
{
    QImage img = makeImage(m_h, m_d);  // width=H, height=D
    if (x < 0) x = 0; if (x >= m_w) x = m_w - 1;
    for (int z = 0; z < m_d; ++z) {
        auto *dst = img.scanLine(z);
        for (int y = 0; y < m_h; ++y)
            dst[y] = hu::unitTo8(mapDisplay(voxel(z, y, x)));
    }
    return img;
}

// 行=Z(depth), 列=X(cols)
QImage DicomVolume::coronalPlane(int y) const
{
    QImage img = makeImage(m_w, m_d);  // width=W, height=D
    if (y < 0) y = 0; if (y >= m_h) y = m_h - 1;
    for (int z = 0; z < m_d; ++z) {
        auto *dst = img.scanLine(z);
        for (int x = 0; x < m_w; ++x)
            dst[x] = hu::unitTo8(mapDisplay(voxel(z, y, x)));
    }
    return img;
}

void DicomVolume::setDisplayWindow(float center, float width)
{
    m_useWindow = true;
    m_wc = center; m_ww = width;
}

} // namespace medical
