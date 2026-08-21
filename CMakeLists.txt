cmake_minimum_required(VERSION 3.21)
project(AiMedicalWorkstation VERSION 0.1.0 LANGUAGES CXX)

# ---------------------------------------------------------------------------
# 放射 AI 阅片工作站 —— 工业级商用医疗软件标准栈 (见 step1.txt)
#   GUI : Qt6 QWidget
#   医学图像 : DCMTK / ITK(SimpleITK) / VTK / OpenCV
#   AI 推理   : ONNX Runtime / TensorRT / OpenVINO  (本地离线)
#   存储     : SQLite (QtSql) / 可选 PostgreSQL 局域网中间服务
#   网络     : DCMTK C-FIND/C-MOVE / 可选 REST 后端
#
# 设计原则：仅安装 Qt6 即可编译运行(Mock 实现)；安装对应依赖后打开
#           USE_* 开关即可接入真实算法库，互不破坏。
# ---------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# ---- Qt6 (必需) ----
find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets Gui Sql Concurrent)
qt_standard_project_setup()

# ---- 可选依赖开关 (与 step1.txt 对应) ----
option(USE_DCMTK        "启用 DCMTK (DICOM 解析/C-FIND/C-MOVE)"                     OFF)
option(USE_ITK         "启用 ITK/SimpleITK (预处理/配准/分割)"                     OFF)
option(USE_VTK         "启用 VTK 9.x (2D/MPR/VR 三维重建)"                         OFF)
option(USE_OPENCV      "启用 OpenCV (病灶可视化/标注绘制)"                          OFF)
option(USE_ONNXRUNTIME "启用 ONNX Runtime (跨平台推理)"                            OFF)
option(USE_TENSORRT    "启用 TensorRT (NVIDIA GPU 加速)"                            OFF)
option(USE_OPENVINO    "启用 OpenVINO (Intel CPU/核显加速)"                         OFF)
option(USE_HTTPLIB     "启用 cpp-httplib (局域网脱敏 REST 后端)"                     OFF)

# AI 推理封装为独立动态库 (.dll/.so)，Qt 主程序调用，算法可独立迭代
option(BUILD_AI_SHARED "把 AI 推理构建为独立动态库 AiInference"                       OFF)

# ---- DCMTK (本地静态构建, thirdparty/dcmtk-install) ----
set(DCMTK_ROOT "${CMAKE_SOURCE_DIR}/thirdparty/dcmtk-install" CACHE PATH "DCMTK install prefix")
if(USE_DCMTK AND EXISTS "${DCMTK_ROOT}/include/dcmtk/config/osconfig.h")
    message(STATUS "DCMTK (local build): ${DCMTK_ROOT}")
    file(GLOB DCMTK_STATIC_LIBS "${DCMTK_ROOT}/lib/lib*.a")
endif()

# ---- 可选依赖探测 ----
if(USE_DCMTK)
    # 不调用 find_package(DCMTK); 改用上面显式定位的本地静态库
endif()
if(USE_ITK)
    find_package(ITK REQUIRED)
endif()
if(USE_VTK)
    find_package(VTK 9 REQUIRED COMPONENTS GUISupportQt)
endif()
if(USE_OPENCV)
    find_package(OpenCV REQUIRED)
endif()
if(USE_ONNXRUNTIME)
    # 不调用 find_package(ONNXRuntime); 用本地 thirdparty/onnxruntime (NuGet 解包)
endif()
if(USE_TENSORRT)
    find_package(CUDAToolkit REQUIRED)
    find_package(TensorRT REQUIRED)
endif()
if(USE_OPENVINO)
    find_package(OpenVINO REQUIRED)
endif()
if(USE_HTTPLIB)
    find_package(httplib REQUIRED)
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/config.h"
    @ONLY)

add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x060000)

# ---- 源码 ----
set(APP_SOURCES
    src/app/MainWindow.cpp
    src/gui/AiToolPanel.cpp
    src/gui/ReportPanel.cpp
    src/gui/AppToolBar.cpp
    src/gui/AppStatusBar.cpp
    src/gui/SrViewer.cpp
    src/gui/SrImageView.cpp
    src/gui/BrightnessContrastDialog.cpp
    src/gui/DocumentWindow.cpp
    src/gui/HistogramDialog.cpp
    src/gui/ResultsWindow.cpp
    src/gui/SrControlPanel.cpp
    src/gui/ReconPanel.cpp
    src/gui/ViewerPanel.cpp
    src/gui/AiAnalysisPanel.cpp
    src/gui/PrimarySidebar.cpp
    src/gui/ImageJToolBar.cpp
    src/gui/PatientBanner.cpp
    src/gui/EditorTabBar.cpp
    src/gui/BottomPanel.cpp
    src/core/Patient.cpp
    src/core/Study.cpp
    src/dicom/IDicomLoader.cpp
    src/dicom/MockDicomLoader.cpp
    src/sr/DicomVolume.cpp
    src/sr/ISuperResEngine.cpp
    src/sr/MockSuperResEngine.cpp
    src/sr/BaseReconstructor.cpp
    src/sr/InPlaneReconstructor.cpp
    src/sr/InterSliceReconstructor.cpp
    src/imaging/ItkProcessor.cpp
    src/imaging/VtkSceneFactory.cpp
    src/imaging/OpenCVAnnotator.cpp
    src/ai/AiPipeline.cpp
    src/ai/IInferenceEngine.cpp
    src/ai/AiModelDiscovery.cpp
    src/ai/MockInferenceEngine.cpp
    src/storage/Database.cpp
    src/storage/ReportStore.cpp
    src/storage/DicomExporter.cpp
    src/network/PacsClient.cpp
    src/network/RestClient.cpp
    src/utils/Logger.cpp
)

if(USE_DCMTK)
    list(APPEND APP_SOURCES src/dicom/DcmtkLoader.cpp src/network/DcmtkPacsClient.cpp)
endif()
if(USE_ITK)
    list(APPEND APP_SOURCES)
endif()
if(USE_VTK)
    list(APPEND APP_SOURCES src/imaging/VtkViewer.cpp)
endif()
if(USE_OPENCV)
    list(APPEND APP_SOURCES)
endif()
if(USE_ONNXRUNTIME)
    list(APPEND APP_SOURCES src/ai/OnnxInferenceEngine.cpp src/sr/OnnxSuperResEngine.cpp src/sr/EdsrWidthUpscaleEngine.cpp src/sr/OrtProviders.cpp)
endif()
if(USE_TENSORRT)
    list(APPEND APP_SOURCES src/ai/TensorRtInferenceEngine.cpp)
endif()
if(USE_OPENVINO)
    list(APPEND APP_SOURCES src/ai/OpenVinoInferenceEngine.cpp)
endif()

# Windows application icon (shown on the .exe and in the taskbar)
set(APP_ICON_RC "")
if(WIN32)
    # make the absolute icon path available to the .rc template
    set(APP_ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/resources/ai_medical_icon.ico")
    configure_file(resources/appicon.rc.in
        "${CMAKE_CURRENT_BINARY_DIR}/appicon.rc" @ONLY)
    set(APP_ICON_RC "${CMAKE_CURRENT_BINARY_DIR}/appicon.rc")
endif()

qt_add_executable(AiMedicalWorkstation WIN32
    src/main.cpp
    resources/resources.qrc
    ${APP_ICON_RC}
    ${APP_SOURCES}
)

target_include_directories(AiMedicalWorkstation PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
)

target_link_libraries(AiMedicalWorkstation PRIVATE
    Qt6::Widgets
    Qt6::Gui
    Qt6::Sql
    Qt6::Concurrent
)

if(USE_DCMTK AND DCMTK_STATIC_LIBS)
    target_include_directories(AiMedicalWorkstation PRIVATE "${DCMTK_ROOT}/include")
    # 静态库间存在循环依赖, 用 --start-group/--end-group 解决
    target_link_libraries(AiMedicalWorkstation PRIVATE
        -Wl,--start-group ${DCMTK_STATIC_LIBS} -Wl,--end-group
        ws2_32 wsock32 iphlpapi secur32 netapi32)
endif()
if(USE_ITK)
    target_link_libraries(AiMedicalWorkstation PRIVATE ${ITK_LIBRARIES})
    target_include_directories(AiMedicalWorkstation PRIVATE ${ITK_INCLUDE_DIRS})
endif()
if(USE_VTK)
    target_link_libraries(AiMedicalWorkstation PRIVATE ${VTK_LIBRARIES})
    target_include_directories(AiMedicalWorkstation PRIVATE ${VTK_INCLUDE_DIRS})
endif()
if(USE_OPENCV)
    target_link_libraries(AiMedicalWorkstation PRIVATE ${OpenCV_LIBS})
    target_include_directories(AiMedicalWorkstation PRIVATE ${OpenCV_INCLUDE_DIRS})
endif()
if(USE_ONNXRUNTIME)
    set(ORT_ROOT "${CMAKE_SOURCE_DIR}/thirdparty/onnxruntime" CACHE PATH "ONNX Runtime (NuGet) root")
    if(EXISTS "${ORT_ROOT}/include/onnxruntime_cxx_api.h" AND EXISTS "${ORT_ROOT}/lib/onnxruntime.dll")
        message(STATUS "ONNX Runtime (local): ${ORT_ROOT}")
        target_include_directories(AiMedicalWorkstation PRIVATE "${ORT_ROOT}/include")
        target_link_libraries(AiMedicalWorkstation PRIVATE "${ORT_ROOT}/lib/onnxruntime.dll")
        # 运行时所需的 ONNX Runtime DLL（CPU 必备 + GPU(CUDA)/TensorRT provider + cuDNN 等，复制 lib 下全部 dll）
        file(GLOB ORT_RUNTIME_DLLS "${ORT_ROOT}/lib/*.dll")
        add_custom_command(TARGET AiMedicalWorkstation POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:AiMedicalWorkstation>/model"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${ORT_RUNTIME_DLLS} "$<TARGET_FILE_DIR:AiMedicalWorkstation>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/model/swinir_med_4x.onnx"
                "$<TARGET_FILE_DIR:AiMedicalWorkstation>/model/swinir_med_4x.onnx"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/model/swinir_med_4x.onnx.data"
                "$<TARGET_FILE_DIR:AiMedicalWorkstation>/model/swinir_med_4x.onnx.data"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/model/edsr_invsr_width4x.onnx"
                "$<TARGET_FILE_DIR:AiMedicalWorkstation>/model/edsr_invsr_width4x.onnx"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/model/edsr_invsr_width4x.onnx.data"
                "$<TARGET_FILE_DIR:AiMedicalWorkstation>/model/edsr_invsr_width4x.onnx.data")
    else()
        message(FATAL_ERROR "USE_ONNXRUNTIME=ON 但 thirdparty/onnxruntime 未就绪 (需 include/onnxruntime_cxx_api.h 与 lib/onnxruntime.dll)。")
    endif()
endif()
if(USE_TENSORRT)
    target_link_libraries(AiMedicalWorkstation PRIVATE ${TensorRT_LIBRARIES} CUDA::cudart)
    target_include_directories(AiMedicalWorkstation PRIVATE ${TensorRT_INCLUDE_DIRS})
endif()
if(USE_OPENVINO)
    target_link_libraries(AiMedicalWorkstation PRIVATE ${OpenVINO_LIBRARIES})
    target_include_directories(AiMedicalWorkstation PRIVATE ${OpenVINO_INCLUDE_DIRS})
endif()
if(USE_HTTPLIB)
    target_link_libraries(AiMedicalWorkstation PRIVATE httplib)
endif()

set_target_properties(AiMedicalWorkstation PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

# 将 USE_* 开关作为编译宏传入源码 (#ifdef USE_DCMTK ...)
foreach(opt USE_DCMTK USE_ITK USE_VTK USE_OPENCV USE_ONNXRUNTIME USE_TENSORRT USE_OPENVINO USE_HTTPLIB BUILD_AI_SHARED)
    if(${opt})
        target_compile_definitions(AiMedicalWorkstation PRIVATE ${opt})
    endif()
endforeach()

install(TARGETS AiMedicalWorkstation
    RUNTIME DESTINATION bin
    BUNDLE  DESTINATION .
)

if(USE_ONNXRUNTIME)
    file(GLOB ORT_INSTALL_DLLS "${ORT_ROOT}/lib/*.dll")
    install(FILES ${ORT_INSTALL_DLLS} DESTINATION bin)
    install(FILES
        "${CMAKE_SOURCE_DIR}/model/swinir_med_4x.onnx"
        "${CMAKE_SOURCE_DIR}/model/swinir_med_4x.onnx.data"
        "${CMAKE_SOURCE_DIR}/model/edsr_invsr_width4x.onnx"
        "${CMAKE_SOURCE_DIR}/model/edsr_invsr_width4x.onnx.data"
        DESTINATION bin/model)
endif()
