# CT 超分重建系统（SwinIR-Med）

基于 Qt6 Widgets、C++17、DCMTK 和 ONNX Runtime 的 CT 影像查看与面内超分重建工作站。项目面向 DICOM/IMA 序列，提供三平面 MPR 查看、窗宽窗位、测量标注、真实 SwinIR-Med 4× 面内超分、AI 分析工作区，以及 PNG/DICOM 导出能力。

> 当前已实现的是 **2D 面内超分**：每个轴位切片的 X/Y 分辨率放大 4 倍，Z 方向帧数保持不变。层间超分模块目前仍为占位实现。

## 主要功能

### 四个并列板块

主窗口使用四个 Tab，并共享一次导入的源序列：

1. **DICOM 查看器**
   - 轴位、矢状位、冠状位 MPR
   - 滚轮翻层，Ctrl+滚轮缩放，右键拖动平移
   - 窗宽窗位调整和肺窗、纵隔窗、骨窗、脑窗预设
   - HU 值实时显示
   - 距离、角度、箭头和方框标注
   - 可控制十字虚线显示/隐藏

2. **超分重建**
   - 重建前/重建后双视图对比
   - 使用 `model/swinir_med_4x.onnx` 执行真实 SwinIR-Med 4× 推理
   - 支持全质量、快速预览和极速预览模式
   - 后台线程执行，不阻塞界面
   - 显示重建进度、耗时、引擎和设备信息

3. **层间超分重建**
   - 界面和流程已预留
   - 当前返回“开发中”，尚未增加 Z 方向帧数

### 4. AI 分析工作区
   - 使用 VS Code 风格的资源侧栏、分析概览和结果/活动面板
   - 显示患者、检查、体数据尺寸、像素间距和当前模态
   - 区分 CT 专用分析模型、MedRAX 胸部 X-ray Provider 和 SwinIR-Med 超分模型
   - 未配置或未连接真实分析引擎时，运行入口保持禁用，不生成随机病灶或诊断报告
   - MedRAX 是独立的 Python/Gradio 胸部 X-ray Agent，不适用于当前 CT 体数据；后续如接入，应通过模态检查和独立服务适配器调用

> AI 分析结果（如未来接入）只能作为辅助信息，不能替代执业医师判断。模型权重、外部服务和第三方许可证需要单独核查。

### 2D 面内超分数据流

```text
输入体数据：D × H × W
       │
       ├─ 对每一层轴位图像执行 4× 超分
       │  128×128 → 512×512（模型内部按 128×128 patch 推理）
       │
输出体数据：D × 4H × 4W
```

其中 `D` 为导入成功的帧数，2D 面内超分不会增加或减少帧数。输出体数据的 X/Y 像素间距相应缩小为原来的 1/4，Z 向采样保持不变。

## 技术栈

| 功能 | 技术/模块 | 构建开关 |
|---|---|---|
| GUI | Qt6 Widgets / Qt6 Gui / Qt6 Sql | 必需 |
| DICOM 解析 | DCMTK 3.6.8 | `USE_DCMTK` |
| 真实超分推理 | ONNX Runtime + `OnnxSuperResEngine` | `USE_ONNXRUNTIME` |
| 默认演示引擎 | Qt 双三次 `MockSuperResEngine` | 无 |
| 三平面查看 | `SrViewer` + `SrImageView` | 内置 |
| 数据处理 | `DicomVolume` | 内置 |
| 导出 | DICOM/PNG | 内置 |
| 存储 | SQLite | QtSql |
| PACS/网络 | DCMTK C-FIND/C-MOVE/C-STORE、REST 接入点 | 可选 |
| 后续加速 | TensorRT、OpenVINO、INT8、剪枝 | 规划中 |

## 目录结构

```text
model/
├── swinir_med_4x.onnx       已导出的 SwinIR-Med 模型
├── swinir_med_4x.onnx.data   ONNX 外部权重
├── export_onnx.py            PyTorch → ONNX 导出脚本
├── onnx.py                   预处理/后处理说明
└── 127_8.py                  SwinIR-Med 模型结构

resources/
├── ai_medical_icon.svg       项目应用图标
└── resources.qrc             Qt 资源清单

src/
├── app/                      MainWindow 主窗口
├── gui/                      三平面查看器、控制面板和各功能板块
├── core/                     Study、Patient、DicomFrame
├── dicom/                    DICOM 接口、Mock 和 DCMTK 加载器
├── sr/                       DicomVolume、2D 超分、层间超分占位、ONNX 引擎
├── storage/                  DICOM/PNG 导出、SQLite 存储
├── network/                  PACS 和 REST 接入点
├── ai/                       通用 AI 推理接口与 Provider 能力描述
├── imaging/                  ITK/VTK/OpenCV 接入点
└── utils/                    日志、HU 归一化和窗宽窗位

thirdparty/
├── dcmtk-install/            本地 DCMTK 安装目录
├── onnxruntime/              本地 ONNX Runtime 头文件和 DLL
└── test_dcm/                 演示 DICOM 序列
```

## 环境要求

- Windows 10/11
- Qt 6.4 或更高版本（开发环境使用 Qt 6.11.1）
- CMake 3.21 或更高版本
- Ninja
- MinGW 64 位编译器
- DCMTK 3.6.8（启用真实 DICOM 解析时）
- ONNX Runtime DLL 和 C++ 头文件（启用真实 SwinIR-Med 推理时）

项目当前已包含以下 ONNX Runtime 文件：

```text
thirdparty/onnxruntime/include/onnxruntime_cxx_api.h
thirdparty/onnxruntime/lib/onnxruntime.dll
```

### 启用 GPU (CUDA) 加速

当前 `thirdparty/onnxruntime/lib/` 只提供了 **CPU 版**的 `onnxruntime.dll`，**未包含** `onnxruntime_providers_cuda.dll`，因此程序在默认情况下只会在 CPU 上执行超分推理。

程序左侧控制面板中的「计算设备」提供了 **CPU / GPU 两个按钮**（默认高亮 GPU）。选择 GPU 时，如果本机未部署 CUDA 执行提供者，引擎会自动回退到 CPU，并在状态栏 / 引擎信息处显示实际使用的设备（例如 `CUDA:0` 或 `CPU`）。也就是说，即使界面选了 GPU，在仅含 CPU 版 ONNX Runtime 的环境里也会安全运行在 CPU 上。

要真正使用 GPU 加速，需要把当前 CPU 版替换为 **GPU 版 ONNX Runtime**：

1. 前往 [onnxruntime releases](https://github.com/microsoft/onnxruntime/releases) 下载与当前头文件版本一致的 **`onnxruntime-<ver>-windows-x64-gpu`** 压缩包（版本必须与 `thirdparty/onnxruntime/include` 中的头文件版本一致，否则可能出现 ABI 不兼容）。
2. 将 GPU 版压缩包 `lib/` 目录下的以下文件复制到 `thirdparty/onnxruntime/lib/`：
   - `onnxruntime.dll`
   - `onnxruntime_providers_cuda.dll`
   - `onnxruntime_providers_shared.dll`
   - 以及同目录下其他 CUDA / cuDNN 相关的运行期 DLL（如 `cublas*.dll`、`cudnn*.dll`、`tensorrt*.dll` 等，具体随 ORT 版本而定）
3. 在运行环境中安装与本机显卡驱动匹配、且与所用 ONNX Runtime 版本兼容的 **NVIDIA CUDA** 和 **cuDNN** 运行库（其路径需位于 `PATH` 中）。
4. 重新运行程序（**无需重新编译**，CMake 已经链接 `onnxruntime` 导入库；只需确保上述 DLL 随 `AiMedicalWorkstation.exe` 一起位于运行目录，或将 `thirdparty/onnxruntime/lib` 加入 `PATH`）。在「计算设备」中选择 GPU，状态栏应显示 `CUDA:0`，表示已成功在 GPU 上推理。

> 如果 GPU 初始化失败（例如 CUDA 版本不匹配或驱动缺失），程序会回退到 CPU 并输出告警日志，不会崩溃。

## 构建项目

### 启用真实 DICOM 和 ONNX 推理

在 Git Bash 中执行：

```bash
export PATH="/e/ProgramFiles/Qt/Tools/CMake_64/bin:/e/ProgramFiles/Qt/Tools/Ninja:/e/ProgramFiles/Qt/Tools/mingw1310_64/bin:$PATH"

cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DUSE_DCMTK=ON \
  -DUSE_ONNXRUNTIME=ON \
  -DCMAKE_PREFIX_PATH="E:/ProgramFiles/Qt/6.11.1/mingw_64" \
  -DCMAKE_C_COMPILER="E:/ProgramFiles/Qt/Tools/mingw1310_64/bin/gcc.exe" \
  -DCMAKE_CXX_COMPILER="E:/ProgramFiles/Qt/Tools/mingw1310_64/bin/g++.exe"

cmake --build build -j
```

也可以使用项目中已生成的 Ninja：

```bash
/e/ProgramFiles/Qt/Tools/Ninja/ninja.exe -C build
```

构建成功后，CMake 会自动将以下运行文件复制到 `build/`：

```text
build/AiMedicalWorkstation.exe
build/onnxruntime.dll
build/model/swinir_med_4x.onnx
build/model/swinir_med_4x.onnx.data
```

### 仅使用 Mock 引擎

如果没有 ONNX Runtime，关闭 `USE_ONNXRUNTIME` 仍可构建和运行程序。此时 2D 超分使用 `MockSuperResEngine` 的 Qt 双三次放大，不是真实 AI 推理。

```bash
cmake -S . -B build-mock -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DUSE_DCMTK=ON \
  -DUSE_ONNXRUNTIME=OFF
cmake --build build-mock -j
```

## 运行程序

```bash
export PATH="/e/ProgramFiles/Qt/6.11.1/mingw_64/bin:$PATH"
./build/AiMedicalWorkstation.exe
```

程序启动后会尝试自动加载：

```text
thirdparty/test_dcm/
```

如果存在可用的模型和 ONNX Runtime，程序会使用 `ONNX SwinIR-Med`；否则在关闭 ONNX 开关的构建中使用 Mock 引擎。也可以通过界面左侧的“导入 DICOM / IMA 序列”按钮加载自己的序列。

## 模型说明

模型由 `model/export_onnx.py` 导出，默认契约为：

```text
输入：1 × 1 × 128 × 128 float32
输出：1 × 1 × 512 × 512 float32
```

C++ 端的 `OnnxSuperResEngine` 会：

1. 将任意尺寸轴位图像切分为 128×128 patch；
2. 对右侧和底部不足区域进行填充；
3. 逐 patch 调用 ONNX Runtime；
4. 将 512×512 高分辨率 patch 拼接；
5. 裁剪为原始图像尺寸的 4 倍；
6. 写回新的 `DicomVolume`。

模型使用外部权重时，以下两个文件必须位于同一目录：

```text
swinir_med_4x.onnx
swinir_med_4x.onnx.data
```

## 查看器操作

| 操作 | 功能 |
|---|---|
| 鼠标滚轮 | 当前平面翻层 |
| Ctrl + 鼠标滚轮 | 缩放 |
| 左键拖动 | 调整窗宽窗位 |
| 左键单击 | 三平面联动定位 |
| 右键拖动 | 平移 |
| 双击 | 聚焦当前平面 |
| 右键菜单 | 在独立窗口打开当前平面 |
| 距离/角度/箭头/方框工具 | 绘制医学影像标注 |
| 显示十字虚线 | 控制十字定位线显示 |

## 导出

- **PNG/JPG**：导出当前选中的轴位、矢状位或冠状位。
- **DICOM**：导出当前重建结果序列。实际临床使用前应由专业人员验证导出的 DICOM 元数据和几何信息。
- 导出结果不能替代医生诊断，AI 结果必须经过执业医师确认。

## Windows 应用图标

项目应用图标位于 `resources/ai_medical_icon.svg`，主题包含：

- CT 断层圆环
- MPR 十字定位线
- 4× AI 超分标识
- 深蓝色医疗科技背景

图标通过 Qt Resource System 编译到程序中，并在 `src/main.cpp` 中设置为窗口图标。

## 当前状态

- ✅ Qt6 Widgets 主界面和三 Tab 工作流
- ✅ 共享 DICOM/IMA 导入
- ✅ DCMTK 3.6.8 DICOM 解析
- ✅ 轴位、矢状位、冠状位 MPR
- ✅ 窗宽窗位、HU 显示、测量和标注
- ✅ 十字虚线显示/隐藏控制
- ✅ 2D 面内 4× 超分流程
- ✅ ONNX Runtime + SwinIR-Med 真实推理
- ✅ ONNX 模型及外部权重自动部署
- ✅ DICOM/PNG 导出接口
- ✅ 项目应用图标
- 🟡 层间超分：接口和界面已预留，算法尚未实现
- 🟡 TensorRT/INT8/剪枝加速：已预留接入点，待后续优化

## 注意事项

本项目是医学影像软件原型和研究工具，不构成医疗器械或诊断结论。请在真实临床环境使用前完成数据安全、DICOM 合规、算法性能、模型泛化性和医疗器械相关验证。
