#pragma once

#include "core/AiResult.h"
#include "core/DicomFrame.h"
#include "core/Study.h"
#include <QObject>
#include <memory>

namespace medical {

class IInferenceEngine;

// AI 推理管线：编排"预处理 → 推理 → 后处理/标注"，供主程序调用。
// 当前用 Mock 引擎；接入真实库后自动切换 (IInferenceEngine::create)。
class AiPipeline : public QObject
{
    Q_OBJECT
public:
    explicit AiPipeline(QObject *parent = nullptr);
    ~AiPipeline();

    // 对当前帧执行所选任务，返回结果并发出 finished 信号。
    AiResult run(const DicomFrame &frame, const QString &modelPath,
                 const QString &task);

    QString engineName() const;
    QString engineDevice() const;

signals:
    void finished(const medical::AiResult &result);

private:
    std::unique_ptr<IInferenceEngine> m_engine;
};

} // namespace medical
