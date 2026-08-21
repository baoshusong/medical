#!/usr/bin/env bash
# 单图超分: 128×128 .IMA/.dcm -> 512×512 (严格对齐 model/onnx.py)
# 用法: ./run_sr_single.sh [输入.IMA] [输出前缀] [模型.onnx]
cd "$(dirname "$0")"
export PATH="/e/ProgramFiles/Qt/Tools/mingw1310_64/bin:$PATH"
cp -n thirdparty/onnxruntime/lib/onnxruntime.dll . 2>/dev/null
IN="${1:-model/test.IMA}"; OUT="${2:-model/test}"; MDL="${3:-model/swinir_med_4x.onnx}"
./sr_single.exe "$IN" "$OUT" "$MDL"
