#pragma once

#include "core/DicomFrame.h"
#include <QImage>
#include <QVector>
#include <QtGlobal>

namespace medical {

// 3D CT 体数据 (depth D × rows H × cols W)，存储为 HU 归一化后的 [0,1] float。
// 由 DicomFrame 序列构造；提供轴位/矢状位/冠状位 MPR 切片提取。
class DicomVolume
{
public:
    DicomVolume() = default;

    void fromFrames(const QVector<DicomFrame> &frames);
    bool allocate(int depth, int rows, int cols, float spacingX, float spacingY, float spacingZ = 0.0f);

    int depth()  const { return m_d; }   // Z (slice)
    int rows()   const { return m_h; }  // Y
    int cols()   const { return m_w; }  // X
    bool isEmpty() const { return m_data.isEmpty(); }

    float spacingX() const { return m_sx; }  // mm/pixel
    float spacingY() const { return m_sy; }
    float spacingZ() const { return m_sz; }  // mm/slice (层间距)
    void  setSpacingZ(float sz) { m_sz = sz; }
    void  setSpacing(float sx, float sy) { m_sx = sx; m_sy = sy; }
    void  setSpacing(float sx, float sy, float sz) { m_sx = sx; m_sy = sy; m_sz = sz; }

    // 体素 [0,1]，越界返回 0
    float voxel(int z, int y, int x) const;
    void  setVoxel(int z, int y, int x, float v);

    // 直接扫描底层 [0,1] 体素数组，避免逐体素边界检查/索引重算 (批量统计/直方图用)
    template <typename F>
    void forEachUnit(F &&fn) const {
        for (const float u : m_data) fn(u);
    }

    // 扫描指定切面 (plane,slice) 的体素 (unit 域)；qBound 仅对固定下标执行一次
    template <typename F>
    void forEachUnitInPlane(int plane, int slice, F &&fn) const {
        if (m_data.isEmpty()) return;
        if (plane == 0) {                        // 轴位: 固定 z
            const int z = qBound(0, slice, m_d - 1);
            const size_t base = size_t(z) * m_h * m_w;
            for (int y = 0; y < m_h; ++y)
                for (int x = 0; x < m_w; ++x)
                    fn(m_data[base + size_t(y) * m_w + x]);
        } else if (plane == 1) {                // 矢状位: 固定 x
            const int x = qBound(0, slice, m_w - 1);
            for (int z2 = 0; z2 < m_d; ++z2)
                for (int y = 0; y < m_h; ++y)
                    fn(m_data[size_t(z2) * m_h * m_w + size_t(y) * m_w + x]);
        } else {                                // 冠状位: 固定 y
            const int y = qBound(0, slice, m_h - 1);
            for (int z2 = 0; z2 < m_d; ++z2)
                for (int x = 0; x < m_w; ++x)
                    fn(m_data[size_t(z2) * m_h * m_w + size_t(y) * m_w + x]);
        }
    }

    // MPR: 返回 8-bit 灰度 QImage (按 [0,1] 映射, 可叠加窗位 preset)
    // 轴位: 固定 z, 返回 H×W
    QImage axialSlice(int z) const;
    // 矢状位: 固定 x, 返回 D(rows=Z) × H(cols=Y)
    QImage sagittalPlane(int x) const;
    // 冠状位: 固定 y, 返回 D(rows=Z) × W(cols=X)
    QImage coronalPlane(int y) const;

    // 按 lung 窗重新映射用于显示 (可选)
    void setDisplayWindow(float center, float width);

private:
    QImage makeImage(int w, int h) const;
    float  mapDisplay(float unit) const;

    QVector<float> m_data;
    int m_d = 0, m_h = 0, m_w = 0;
    float m_sx = 0.7f, m_sy = 0.7f;   // mm/pixel
    float m_sz = 0.0f;                // mm/slice (层间距)
    bool  m_useWindow = false;
    float m_wc = -600.0f, m_ww = 1500.0f; // lung 默认 (HU 域)
};

} // namespace medical
