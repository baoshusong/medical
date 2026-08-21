#pragma once

#include "ai/AnalysisProviderStatus.h"
#include "core/AiResult.h"
#include "core/Study.h"
#include "sr/DicomVolume.h"
#include <QWidget>

class QComboBox;
class QLabel;
class QListWidget;
class QPlainTextEdit;
class QPushButton;

namespace medical {

class AiPipeline;

// AI 分析工作区：展示可验证的检查上下文，并为后续分析模型保留入口。
// - 无分析模型时，提供传统定量摘要 (HU 统计 / 体积 / 低密度占比)，无需模型。
// - model/ 中放置分析 ONNX 模型后，下拉自动出现并接入 OnnxInferenceEngine 推理。
class AiAnalysisPanel : public QWidget
{
    Q_OBJECT
public:
    explicit AiAnalysisPanel(QWidget *parent = nullptr);

    void setSource(const DicomVolume &volume);
    void setStudy(const Study &study);
    void runAnalysis();

private slots:
    void onRunAnalysis();

private:
    void populateProviders();        // 从 model/ 动态探测可用分析模型
    void updateSummary();
    void updateProviderState();
    void appendLog(const QString &text);
    QString valueOrDash(const QString &value) const;

    void computeQuantitativeSummary();  // 传统算法 (无需模型)
    void runModelInference(const QString &modelPath); // 真实 ONNX 推理
    void showResult(const AiResult &result);
    DicomFrame makeRepresentativeFrame() const;

    DicomVolume m_volume;
    Study m_study;

    QLabel *m_patient = nullptr;
    QLabel *m_studyInfo = nullptr;
    QLabel *m_geometry = nullptr;
    QLabel *m_engine = nullptr;
    QLabel *m_workspaceStatus = nullptr;
    QLabel *m_modalityStatus = nullptr;
    QComboBox *m_provider = nullptr;
    QLabel *m_resultStatus = nullptr;
    QLabel *m_dataSummary = nullptr;
    QListWidget *m_tasks = nullptr;
    QPlainTextEdit *m_log = nullptr;
    QPushButton *m_run = nullptr;
    AiPipeline *m_pipeline = nullptr;
};

} // namespace medical
