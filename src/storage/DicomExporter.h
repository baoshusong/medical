#pragma once

#include "sr/DicomVolume.h"
#include "core/Study.h"
#include <QString>

namespace medical {

class DicomExporter
{
public:
    static bool exportDicomSeries(const DicomVolume &vol, const Study &study,
                                  const QString &outDir, int upscale);
    static bool exportPngSlice(const DicomVolume &vol, int z, const QString &path);
    static bool exportPngPlane(const DicomVolume &vol, int plane, int slice,
                               float windowCenter, float windowWidth, const QString &path);
};

} // namespace medical
