#pragma once

#include "ai/AnalysisProviderStatus.h"

namespace medical {

// 分析 Provider 的最小描述接口。具体推理服务可在后续实现。
class IAnalysisProvider
{
public:
    virtual ~IAnalysisProvider() = default;
    virtual AnalysisProviderStatus status() const = 0;
};

} // namespace medical
