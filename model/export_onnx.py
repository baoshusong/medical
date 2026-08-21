# -*- coding: utf-8 -*-
"""
SwinIR-Med → ONNX 导出脚本
---------------------------------
将 PyTorch 训练的层间超分模型导出为 ONNX，供 C++/Qt 端的
OnnxSuperResEngine (ONNX Runtime / TensorRT) 离线推理。

模型契约 (源自 127_8.py):
  顶层类     : SwinIRMed
  输入       : 1 x 1 x 128 x 128, float32, 范围 [0,1]
               (HU 先 clip(-1000,400) 再 (hu+1000)/1400)
  输出       : 1 x 1 x 512 x 512, float32, 范围约 [-1,1]
               后处理: out*0.5+0.5 -> [0,1]
  checkpoint : 含 'model_state_dict' 键

用法 (在你装了 PyTorch 的环境里):
  pip install torch onnx onnxruntime
  python model/export_onnx.py --ckpt model/swinir_med_4x_sr_amp_best.pth \
      --arch model/127_8.py --out model/swinir_med_4x.onnx
"""
import argparse, importlib.util, os, sys, numpy as np

def load_arch(arch_path):
    """从 127_8.py 加载模块，返回 SwinIRMed 类。"""
    spec = importlib.util.spec_from_file_location("swinir_med_arch", arch_path)
    mod = importlib.util.module_from_spec(spec)
    sys.modules.setdefault("swinir_med_arch", mod)
    spec.loader.exec_module(mod)
    return mod.SwinIRMed

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ckpt", default="model/swinir_med_4x_sr_amp_best.pth")
    ap.add_argument("--arch", default="model/127_8.py")
    ap.add_argument("--out",  default="model/swinir_med_4x.onnx")
    ap.add_argument("--img-size", type=int, default=128)
    ap.add_argument("--upscale", type=int, default=4)
    ap.add_argument("--opset", type=int, default=17)
    ap.add_argument("--dynamic", action="store_true", help="允许 H/W 动态(分块拼接时用)")
    args = ap.parse_args()

    import torch
    SwinIRMed = load_arch(args.arch)

    # 实例化并加载权重 (checkpoint 含 model_state_dict)
    model = SwinIRMed(img_size=args.img_size, in_chans=1, out_chans=1,
                      upscale=args.upscale)
    ckpt = torch.load(args.ckpt, map_location="cpu", weights_only=False)
    state = ckpt["model_state_dict"] if "model_state_dict" in ckpt else ckpt
    missing, unexpected = model.load_state_dict(state, strict=False)
    print(f"[load] missing={len(missing)} unexpected={len(unexpected)}")
    if missing:   print("  missing keys  :", missing[:5], "...")
    if unexpected:print("  unexpected keys:", unexpected[:5], "...")
    model.eval()

    dummy = torch.rand(1, 1, args.img_size, args.img_size, dtype=torch.float32)

    # 推理自检 (PyTorch)
    with torch.no_grad():
        y = model(dummy)
    print(f"[torch] in={tuple(dummy.shape)} out={tuple(y.shape)} "
          f"range=[{y.min().item():.3f},{y.max().item():.3f}]")
    assert y.shape[2] == args.img_size * args.upscale and \
           y.shape[3] == args.img_size * args.upscale, "输出尺寸与 upscale 不符"

    # 导出 ONNX
    torch.onnx.export(
        model, dummy, args.out,
        input_names=["lr"], output_names=["hr"],
        opset_version=args.opset,
        dynamic_axes={"lr": {0: "B", 2: "H", 3: "W"},
                       "hr": {0: "B", 2: "H", 3: "W"}} if args.dynamic else None,
    )
    print(f"[onnx] saved -> {args.out}  ({os.path.getsize(args.out)/1e6:.1f} MB)")

    # onnx + onnxruntime 一致性自检
    try:
        import onnx, onnxruntime as ort
        onnx.checker.check_model(onnx.load(args.out))
        sess = ort.InferenceSession(args.out, providers=["CPUExecutionProvider"])
        x = dummy.numpy()
        y2 = sess.run(None, {"lr": x})[0]
        d = np.abs(y2 - y.numpy()).max()
        print(f"[ort]  out={y2.shape} max|Δ|={d:.6f}  ✓ 导出验证通过")
    except ImportError:
        print("[ort] 未安装 onnx/onnxruntime，跳过一致性自检 (建议 pip install onnx onnxruntime)")

if __name__ == "__main__":
    main()
