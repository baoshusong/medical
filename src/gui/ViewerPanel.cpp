#include "gui/ViewerPanel.h"
#include "gui/SrViewer.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace medical {

ViewerPanel::ViewerPanel(QWidget *parent) : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);
    setObjectName(QStringLiteral("viewerPanel"));

    // ── Study info and common actions ──
    auto *header = new QWidget(this);
    header->setObjectName(QStringLiteral("viewerToolbar"));
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(4, 2, 4, 2);
    headerLayout->setSpacing(6);

    m_studyInfo = new QLabel(QStringLiteral("未载入检查信息"), header);
    m_studyInfo->setObjectName(QStringLiteral("viewerStudyInfo"));
    m_studyInfo->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_studyInfo->setWordWrap(true);
    m_studyInfo->setProperty("role", "muted");
    headerLayout->addWidget(m_studyInfo, 1);

    auto *importButton = new QPushButton(QStringLiteral("导入"), header);
    importButton->setProperty("role", "small");
    importButton->setToolTip(QStringLiteral("导入 DICOM / IMA 序列"));
    connect(importButton, &QPushButton::clicked, this, &ViewerPanel::importRequested);
    headerLayout->addWidget(importButton);

    auto *exportButton = new QPushButton(QStringLiteral("导出 PNG"), header);
    exportButton->setProperty("role", "small");
    exportButton->setToolTip(QStringLiteral("导出当前层面 PNG"));
    connect(exportButton, &QPushButton::clicked, this, &ViewerPanel::exportPngRequested);
    headerLayout->addWidget(exportButton);
    root->addWidget(header);

    m_viewer = new SrViewer(this);
    m_viewer->setCompareMode(false);   // Single viewport for pure viewing
    root->addWidget(m_viewer, 1);
}

void ViewerPanel::setSource(const DicomVolume &vol)
{
    m_viewer->setVolumes(vol, DicomVolume());
}

void ViewerPanel::setStudy(const Study &study)
{
    const QString patientName = study.patient.name.isEmpty() ? QStringLiteral("--") : study.patient.name;
    const QString patientId = study.patient.patientId.isEmpty() ? QStringLiteral("--") : study.patient.patientId;
    const QString sex = study.patient.sex.isEmpty() ? QStringLiteral("--") : study.patient.sex;
    const QString age = study.patient.age > 0 ? QStringLiteral("%1岁").arg(study.patient.age)
                                                : QStringLiteral("--");
    const QString modality = study.modality.isEmpty() ? QStringLiteral("--") : study.modality;
    const QString description = study.description.isEmpty() ? QStringLiteral("--") : study.description;
    const QString accession = study.accessionNumber.isEmpty() ? QStringLiteral("--") : study.accessionNumber;
    const QString series = study.seriesCount > 0 ? QString::number(study.seriesCount) : QStringLiteral("--");
    const QString frames = study.frameCount > 0 ? QString::number(study.frameCount) : QStringLiteral("--");

    m_studyInfo->setText(QStringLiteral("患者: %1  |  ID: %2  |  %3 / %4  |  %5  |  %6  |  检查号: %7  |  %8序列 / %9帧")
        .arg(patientName, patientId, sex, age, modality, description, accession, series, frames));
}


} // namespace medical