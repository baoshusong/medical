#include "ai/AiModelDiscovery.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>

namespace medical {

static QStringList modelDirCandidates()
{
    return {
        QCoreApplication::applicationDirPath() + QStringLiteral("/model"),
        QDir::currentPath() + QStringLiteral("/model"),
        QCoreApplication::applicationDirPath() + QStringLiteral("/../model"),
        QCoreApplication::applicationDirPath() + QStringLiteral("/../../model")
    };
}

// 超分重建模型命名约定 (swinir / edsr / invsr / 4x / sr / recon ...)，
// 这些不是诊断/分割分析模型，不进入分析下拉。
static bool isSuperResolutionName(const QString &name)
{
    const QString l = name.toLower();
    return l.contains(QStringLiteral("swinir")) ||
           l.contains(QStringLiteral("edsr"))   ||
           l.contains(QStringLiteral("invsr"))  ||
           l.contains(QStringLiteral("width4x"))||
           l.contains(QStringLiteral("superres")) ||
           l.contains(QStringLiteral("super_res"))||
           l.contains(QStringLiteral("_4x"))    ||
           l.contains(QStringLiteral("4x_"))    ||
           l.contains(QStringLiteral("recon"))  ||
           l.contains(QStringLiteral("_sr"))    ||
           l.contains(QStringLiteral("sr_"));
}

static bool modelFileExists(const QString &basePath)
{
    const QFileInfo f(basePath);
    return f.exists() || QFileInfo(basePath + QStringLiteral(".data")).exists();
}

QList<AiModelInfo> discoverModels()
{
    QList<AiModelInfo> out;
    for (const QString &dir : modelDirCandidates()) {
        QDir d(dir);
        if (!d.exists()) continue;
        const QStringList names = d.entryList(QStringList() << QStringLiteral("*.onnx"), QDir::Files);
        for (const QString &name : names) {
            const QString full = d.absoluteFilePath(name);
            if (!modelFileExists(full)) continue;
            const QFileInfo fi(full);
            AiModelInfo info;
            info.id = full;
            info.name = fi.baseName();
            info.path = full;
            info.sizeBytes = fi.size();
            info.modified = fi.lastModified().toString(Qt::ISODate);
            info.isSuperResolution = isSuperResolutionName(name);
            out.append(info);
        }
    }
    return out;
}

QList<AiModelInfo> discoverAnalysisModels()
{
    QList<AiModelInfo> all = discoverModels();
    QList<AiModelInfo> out;
    out.reserve(all.size());
    for (const AiModelInfo &m : all)
        if (!m.isSuperResolution) out.append(m);
    return out;
}

bool hasOnnxRuntime()
{
#ifdef USE_ONNXRUNTIME
    return true;
#else
    return false;
#endif
}

} // namespace medical
