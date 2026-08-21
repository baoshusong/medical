#pragma once

#include <QString>
#include <QList>

namespace medical {

// 探测 model/ 目录下的 ONNX 模型，区分"超分重建模型"与"可接入的分析模型"。
struct AiModelInfo
{
    QString id;                 // 绝对路径 (唯一)
    QString name;               // 显示名 (文件基名)
    QString path;               // 绝对路径
    qint64  sizeBytes = 0;
    QString modified;           // 最后修改时间 (ISO)
    bool    isSuperResolution = false;
    QString note;
};

QList<AiModelInfo> discoverModels();           // model/ 下全部 *.onnx
QList<AiModelInfo> discoverAnalysisModels();   // 排除超分后的分析模型候选
bool hasOnnxRuntime();                          // 构建时是否启用 USE_ONNXRUNTIME

} // namespace medical
