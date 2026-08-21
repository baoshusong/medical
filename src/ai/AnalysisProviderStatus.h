#pragma once

#include <QString>
#include <QStringList>

namespace medical {

// 分析 Provider 的能力与可用性描述。这里只描述能力，不启动外部服务。
struct AnalysisProviderStatus
{
    QString id;
    QString name;
    QStringList supportedModalities;
    QString state;
    QString reason;
    bool available = false;

    bool supports(const QString &modality) const
    {
        return supportedModalities.contains(modality.trimmed(), Qt::CaseInsensitive);
    }
};

inline AnalysisProviderStatus ctModelUnavailable()
{
    return {
        QStringLiteral("ct-model"),
        QStringLiteral("CT 专用分析模型"),
        {QStringLiteral("CT")},
        QStringLiteral("未配置"),
        QStringLiteral("当前版本未安装经过验证的 CT 检测/分割模型。"),
        false
    };
}

inline AnalysisProviderStatus medRaxUnavailable()
{
    return {
        QStringLiteral("medrax"),
        QStringLiteral("MedRAX（胸部 X-ray）"),
        {QStringLiteral("CR"), QStringLiteral("DX"), QStringLiteral("DR"), QStringLiteral("XR")},
        QStringLiteral("未连接"),
        QStringLiteral("MedRAX 需要单独启动 Python 服务；它只支持胸部 X-ray，不支持当前 CT 体数据。"),
        false
    };
}

inline AnalysisProviderStatus superResolutionOnly()
{
    return {
        QStringLiteral("swinir-med"),
        QStringLiteral("SwinIR-Med 4×"),
        {QStringLiteral("CT")},
        QStringLiteral("可用（仅超分）"),
        QStringLiteral("该模型用于面内超分重建，不属于诊断分析引擎。"),
        false
    };
}

} // namespace medical
