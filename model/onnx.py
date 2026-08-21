# -*- coding: utf-8 -*-
"""
批量推理测试脚本【ONNX版本】
替换原pth推理，和pth版本预处理/后处理严格对齐
依赖：model.py 内 read_ima_image, save_as_ima
"""
import os
import numpy as np
import cv2
import onnxruntime as ort
from tqdm import tqdm

from model import (
    read_ima_image,
    save_as_ima
)


def test_batch_images_onnx(onnx_path, input_folder, output_folder, upscale_factor=4):
    # 初始化ONNX Runtime
    sess_options = ort.SessionOptions()
    # 可选开启优化
    sess_options.graph_optimization_level = ort.GraphOptimizationLevel.ORT_ENABLE_ALL
    session = ort.InferenceSession(onnx_path, sess_options, providers=["CPUExecutionProvider"])
    input_name = session.get_inputs()[0].name   # "lr"
    output_name = session.get_outputs()[0].name # "hr"

    img_names = [f for f in os.listdir(input_folder)
                 if f.lower().endswith(('ima', 'dcm'))]
    assert len(img_names) > 0, "测试文件夹中无有效IMA/DICOM影像！"
    os.makedirs(output_folder, exist_ok=True)

    for img_name in tqdm(img_names, desc="SwinIR-Med CT 4x超分推理 ONNX"):
        input_path = os.path.join(input_folder, img_name)
        try:
            lr_img, original_ds = read_ima_image(input_path)
        except Exception as e:
            print(f"警告：无法读取 {img_name}，错误：{e}，跳过")
            continue

        h, w = lr_img.shape[:2]
        if (h != 128) or (w != 128):
            print(f"提示：{img_name} 尺寸为 {h}×{w}，自动缩放到128×128")
            lr_img = cv2.resize(lr_img, (128, 128), interpolation=cv2.INTER_LINEAR)

        # ========== 和原torch预处理完全对齐 ==========
        # ToTensor() → [0,1]; Normalize(mean=0.5,std=0.5) → (x-0.5)/0.5
        lr_norm = lr_img.astype(np.float32) / 255.0
        lr_norm = (lr_norm - 0.5) / 0.5
        # 构造输入: [1,1,H,W] float32
        lr_tensor = np.expand_dims(np.expand_dims(lr_norm, axis=0), axis=0)

        # ONNX推理
        pred = session.run([output_name], {input_name: lr_tensor})[0]

        # ========== 和原torch后处理完全对齐 ==========
        hr_pred_np = pred.squeeze()    # (512,512)
        hr_pred_np = hr_pred_np * 0.5 + 0.5
        hr_pred_np = np.clip(hr_pred_np, 0, 1)
        hr_pred_gray = (hr_pred_np * 255).astype(np.uint8)

        save_name = os.path.splitext(img_name)[0] + '_4x_SR_ONNX.ima'
        save_path = os.path.join(output_folder, save_name)
        save_as_ima(hr_pred_gray, original_ds, save_path)

    print(f"\nONNX SwinIR-Med CT影像4x超分完成！结果保存至: {output_folder}")
    print(f"输出格式：IMA（DICOM）| 尺寸：512×512")


if __name__ == "__main__":
    TEST_BATCH_INPUT = "dataset/test_4x"
    TEST_BATCH_OUTPUT = "result/test_output_onnx"
    ONNX_MODEL = "swinir_med_4x.onnx" # 改成你导出的onnx路径

    os.makedirs(TEST_BATCH_OUTPUT, exist_ok=True)

    test_batch_images_onnx(
        onnx_path=ONNX_MODEL,
        input_folder=TEST_BATCH_INPUT,
        output_folder=TEST_BATCH_OUTPUT,
        upscale_factor=4
    )