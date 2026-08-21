# 下载并解包 ONNX Runtime (NuGet) 到 thirdparty/onnxruntime (供 MinGW 链接)
# 用法: bash thirdparty/build_ort.sh
set -e
cd "$(dirname "$0")"
VER=1.20.1
echo "下载 onnxruntime $VER (NuGet)..."
curl -L --retry 3 -o ort.nupkg "https://www.nuget.org/api/v2/package/Microsoft.ML.OnnxRuntime/$VER"
rm -rf onnxruntime ort_extract
unzip -o -q ort.nupkg -d ort_extract
mkdir -p onnxruntime/include onnxruntime/lib
# 头文件 (NuGet 平铺在 build/native/include/)
cp ort_extract/build/native/include/*.h onnxruntime/include/
# DLL
cp ort_extract/runtimes/win-x64/native/onnxruntime.dll onnxruntime/lib/
rm -rf ort_extract ort.nupkg
echo "完成: thirdparty/onnxruntime/{include,lib}"
ls onnxruntime/include/onnxruntime_cxx_api.h onnxruntime/lib/onnxruntime.dll
