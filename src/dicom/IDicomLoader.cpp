#include "dicom/IDicomLoader.h"
#include "utils/Logger.h"

#ifdef USE_DCMTK
#include "dicom/DcmtkLoader.h"
#endif
#include "dicom/MockDicomLoader.h"

#include <memory>

namespace medical {

std::unique_ptr<IDicomLoader> IDicomLoader::create()
{
#ifdef USE_DCMTK
    LOG_INFO("dicom", "using DcmtkLoader (DCMTK)");
    return std::make_unique<DcmtkLoader>();
#else
    LOG_INFO("dicom", "using MockDicomLoader (no DCMTK)");
    return std::make_unique<MockDicomLoader>();
#endif
}

} // namespace medical
