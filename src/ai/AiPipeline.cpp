#include "ai/AiPipeline.h"
#include "ai/IInferenceEngine.h"
#include "utils/Logger.h"

namespace medical {

AiPipeline::AiPipeline(QObject *parent)
    : QObject(parent)
    , m_engine(IInferenceEngine::create())
{
    LOG_INFO("ai", QStringLiteral("engine=%1 device=%2")
                    .arg(m_engine->name(), m_engine->device()));
}

AiPipeline::~AiPipeline() = default;

AiResult AiPipeline::run(const DicomFrame &frame, const QString &modelPath,
                         const QString &task)
{
    LOG_INFO("ai", QStringLiteral("run task=%1 model=%2").arg(task, modelPath));
    AiResult r = m_engine->infer(frame, modelPath);
    // 预处理/后处理接入点：
    //   USE_ITK    -> ItkProcessor::normalize / resample
    //   USE_OPENCV -> OpenCVAnnotator::overlay
    emit finished(r);
    return r;
}

QString AiPipeline::engineName() const   { return m_engine->name(); }
QString AiPipeline::engineDevice() const { return m_engine->device(); }

} // namespace medical
