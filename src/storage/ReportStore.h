#pragma once

#include "core/AiResult.h"
#include "core/Study.h"
#include <QString>

namespace medical {

// 报告生成与导出：依据检查信息 + AI 结果生成结构化文字报告。
class ReportStore
{
public:
    static QString buildContent(const Study &study, const AiResult &ai);

    static bool exportToFile(const QString &content, const QString &filePath);
};

} // namespace medical
