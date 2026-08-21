#pragma once

#include <QImage>
#include <QList>
#include <QString>
#include "utils/WindowLevel.h"

namespace medical {

// 一帧 DICOM 影像 (已解析为 16-bit 概念；此处简化为带窗位的 QImage)
struct DicomFrame
{
    QImage       image;        // 渲染用 (按窗位生成的 8-bit 图)
    QVector<unsigned short> rawPixels; // 原始 HU 值 (VTK/ITK 管线使用)
    int          width  = 0;
    int          height = 0;
    float        spacingX = 0.7f;  // 像素间距 mm
    float        spacingY = 0.7f;
    float        sliceThickness = 1.25f; // mm
    WindowLevel  defaultWindow;
    QString      seriesUid;
    // 切片空间定位: ImagePositionPatient(0020,0032) 在切片法线方向上的投影,
    // 用于按真实层位排序(而非按文件名/InstanceNumber 的字典序)。
    double       slicePosition = 0.0;
    bool         hasSlicePosition = false;
    int          instanceNumber = -1;  // InstanceNumber(0020,0013), 数值
    QString      sourceFile;          // 源文件路径, 兜底排序/日志用
};

} // namespace medical
