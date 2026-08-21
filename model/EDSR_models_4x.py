import os
import cv2
import csv
import torch
import torch.nn as nn
import torch.optim as optim
import torch.utils.data as data
import numpy as np
from torchvision import transforms
from tqdm import tqdm
from skimage.metrics import peak_signal_noise_ratio as psnr
from skimage.metrics import structural_similarity as ssim
import re
import pydicom
from pydicom.uid import ExplicitVRLittleEndian, CTImageStorage
from pydicom.dataset import Dataset, FileDataset
import datetime
import time


# ===================== 1. 适配4倍超分的EDSR模型（严格512×300输出） =====================
class ResBlock(nn.Module):
    """EDSR经典残差块（无BN，保留CT高频细节）"""

    def __init__(self, channels=64):
        super(ResBlock, self).__init__()
        self.conv1 = nn.Conv2d(channels, channels, kernel_size=3, padding=1, bias=False)
        self.relu = nn.ReLU(inplace=True)
        self.conv2 = nn.Conv2d(channels, channels, kernel_size=3, padding=1, bias=False)

    def forward(self, x):
        residual = x
        out = self.relu(self.conv1(x))
        out = self.conv2(out)
        out += residual
        return out


class EDSRInvSR(nn.Module):
    """融合invSR的EDSR超分模型（仅宽度4x超分，严格输出512×300）"""

    def __init__(self, upscale_factor=4, in_channels=3, out_channels=3, num_res_blocks=32):
        super(EDSRInvSR, self).__init__()
        self.upscale_factor = upscale_factor
        self.target_h, self.target_w = 512, 300  # 目标尺寸（512×300）

        # EDSR特征提取（适配512×75输入）
        self.input_conv = nn.Conv2d(in_channels, 64, kernel_size=3, padding=1, bias=False)
        self.res_blocks = nn.Sequential(*[ResBlock(channels=64) for _ in range(num_res_blocks)])
        self.mid_conv = nn.Conv2d(64, 64, kernel_size=3, padding=1, bias=False)

        # ========== 核心修改：4倍超分转置卷积参数配置 ==========
        self.upsample = nn.ConvTranspose2d(
            in_channels=64,
            out_channels=64,
            kernel_size=(3, 8),  # 高度核=3，宽度核=8（适配512输入）
            stride=(1, 4),       # 高度步长=1，宽度步长=4（仅宽度4x超分）
            padding=(1, 2),      # 精准填充（适配4倍超分尺寸计算）
            output_padding=(0, 0)# 无额外输出填充
        )

        # 通道映射+噪声预测（invSR核心）
        self.channel_mapper = nn.Conv2d(64, 3, kernel_size=3, padding=1, bias=False)
        self.noise_predictor = nn.Sequential(
            nn.Conv2d(3, 32, kernel_size=3, padding=1, bias=False),
            nn.ReLU(inplace=True),
            nn.Conv2d(32, 64, kernel_size=3, padding=1, bias=False),
            nn.ReLU(inplace=True),
            nn.Conv2d(64, out_channels, kernel_size=3, padding=1, bias=False)
        )

        # 输出层
        self.output_conv = nn.Conv2d(3, out_channels, kernel_size=3, padding=1, bias=False)

    def align_size(self, x):
        """尺寸对齐函数：不足填充，超出裁剪，确保输出严格为target_h×target_w"""
        b, c, h, w = x.shape

        # 裁剪超出部分
        x = x[:, :, :self.target_h, :self.target_w]

        # 计算填充量（上下左右对称填充）
        pad_h = self.target_h - h if h < self.target_h else 0
        pad_w = self.target_w - w if w < self.target_w else 0

        if pad_h > 0 or pad_w > 0:
            pad_top = pad_h // 2
            pad_bottom = pad_h - pad_top
            pad_left = pad_w // 2
            pad_right = pad_w - pad_left
            # 用0填充（CT背景为0，不影响）
            x = nn.functional.pad(x, (pad_left, pad_right, pad_top, pad_bottom), mode='constant', value=0)

        return x

    def forward(self, x, timestep=None):
        # EDSR特征提取
        x = self.input_conv(x)
        residual = x
        x = self.res_blocks(x)
        x = self.mid_conv(x)
        x += residual

        # 仅宽度4x上采样
        x_upscale = self.upsample(x)

        # ========== 关键：强制尺寸对齐 ==========
        x_upscale = self.align_size(x_upscale)

        # 通道映射+噪声预测
        x_3ch = self.channel_mapper(x_upscale)
        noise = self.noise_predictor(x_3ch)

        # 噪声尺寸也对齐
        noise = self.align_size(noise)

        # 逆扩散生成HR图像
        hr_img = self.output_conv(x_3ch - noise)

        # 最终尺寸对齐
        hr_img = self.align_size(hr_img)

        return hr_img, noise


# ===================== 2. 扩散损失函数（无修改） =====================
class InvSRLoss(nn.Module):
    """invSR损失函数：结合像素损失 + 噪声估计损失"""

    def __init__(self):
        super(InvSRLoss, self).__init__()
        self.l1_loss = nn.L1Loss()
        self.mse_loss = nn.MSELoss()

    def forward(self, hr_pred, hr_gt, noise_pred, noise_gt):
        # 尺寸校验
        assert hr_pred.shape == hr_gt.shape, f"预测尺寸{hr_pred.shape}与目标尺寸{hr_gt.shape}不匹配！"
        assert noise_pred.shape == noise_gt.shape, f"噪声预测尺寸{noise_pred.shape}与目标尺寸{noise_gt.shape}不匹配！"

        pixel_loss = self.l1_loss(hr_pred, hr_gt)
        noise_loss = self.mse_loss(noise_pred, noise_gt)
        total_loss = pixel_loss + 0.1 * noise_loss
        return total_loss, pixel_loss, noise_loss


# ===================== 3. 生成扩散噪声（无修改） =====================
def generate_diffusion_noise(hr_img, noise_level=0.02):
    """模拟扩散前向过程：适配CT影像低噪声特性"""
    noise = torch.randn_like(hr_img) * noise_level
    noisy_hr = hr_img + noise
    return noisy_hr, noise


# ===================== 4. 数据集（修改LR尺寸为512×75） =====================
def read_ima_image(path):
    """读取IMA/DICOM，还原HU值并归一化，返回像素+原始DICOM"""
    ds = pydicom.dcmread(path)
    # 提取像素并转换为HU值（CT核心）
    img = ds.pixel_array.astype(np.float32)
    if hasattr(ds, 'RescaleSlope') and hasattr(ds, 'RescaleIntercept'):
        img = img * ds.RescaleSlope + ds.RescaleIntercept
    # 临床CT窗宽窗位裁剪（-1000~400 HU）
    img = np.clip(img, -1000, 400)
    # 归一化到[0,255]（适配模型输入）
    img = (img - (-1000)) / (400 - (-1000)) * 255.0
    img = img.astype(np.uint8)
    return img, ds


def natural_sort_key(s):
    """自然排序辅助函数"""
    return [int(text) if text.isdigit() else text.lower() for text in re.split(r'(\d+)', s)]


class SRDataset(data.Dataset):
    def __init__(self, lr_dir, hr_dir, transform=None):
        self.lr_dir = os.path.abspath(lr_dir)
        self.hr_dir = os.path.abspath(hr_dir)
        self.transform = transform
        self.valid_ext = ['.ima', '.dcm']

        # 递归收集9个子文件夹（0-8）的所有有效文件
        def collect_files(root_dir):
            file_list = []
            # 按0-8顺序遍历序列文件夹
            for seq_idx in range(9):
                seq_dir = os.path.join(root_dir, str(seq_idx))
                if not os.path.isdir(seq_dir):
                    raise ValueError(f"序列文件夹 {seq_dir} 不存在！请检查数据集结构")
                # 收集当前序列的有效文件并自然排序
                seq_files = sorted(
                    [os.path.join(str(seq_idx), f) for f in os.listdir(seq_dir)
                     if os.path.splitext(f)[1].lower() in self.valid_ext and os.path.isfile(os.path.join(seq_dir, f))],
                    key=natural_sort_key
                )
                if len(seq_files) == 0:
                    raise ValueError(f"序列文件夹 {seq_dir} 中无有效IMA/DICOM文件！")
                file_list.extend(seq_files)
            return file_list

        # 收集LR和HR的所有文件（带子文件夹路径）
        self.lr_file_paths = collect_files(self.lr_dir)
        self.hr_file_paths = collect_files(self.hr_dir)

        # 校验文件数量
        if len(self.lr_file_paths) == 0:
            print(f"\n===== 数据集加载失败 =====")
            print(f"LR根文件夹：{self.lr_dir}")
            print(f"未找到任何有效影像文件，请检查子文件夹（0-8）中是否有IMA/DICOM文件")
            raise AssertionError(f"LR文件夹 {self.lr_dir} 中无有效医学影像！")

        if len(self.lr_file_paths) != len(self.hr_file_paths):
            raise ValueError(f"LR文件数({len(self.lr_file_paths)})与HR文件数({len(self.hr_file_paths)})不匹配！")

    def __len__(self):
        return len(self.lr_file_paths)

    def __getitem__(self, idx):
        # 拼接完整路径（根目录+子文件夹+文件名）
        lr_rel_path = self.lr_file_paths[idx]
        hr_rel_path = self.hr_file_paths[idx]
        lr_path = os.path.join(self.lr_dir, lr_rel_path)
        hr_path = os.path.join(self.hr_dir, hr_rel_path)

        # 读取CT影像
        lr_img, _ = read_ima_image(lr_path)
        hr_img, _ = read_ima_image(hr_path)

        # ========== 核心修改：LR尺寸改为512×75 ==========
        if lr_img.shape[:2] != (512, 75):
            raise ValueError(f"LR影像 {lr_rel_path} 尺寸错误，需为512×75，当前: {lr_img.shape[:2]}")
        if hr_img.shape[:2] != (512, 300):
            raise ValueError(f"HR影像 {hr_rel_path} 尺寸错误，需为512×300，当前: {hr_img.shape[:2]}")

        # 单通道→3通道
        lr_img = np.stack([lr_img] * 3, axis=-1)
        hr_img = np.stack([hr_img] * 3, axis=-1)

        if self.transform:
            lr_img = self.transform(lr_img)
            hr_img = self.transform(hr_img)

        return lr_img, hr_img


# ===================== 5. 验证函数（修改超分倍数参数） =====================
def validate_model(model, val_loader, device, upscale_factor=4):
    model.eval()
    total_psnr, total_ssim, count = 0.0, 0.0, 0
    with torch.no_grad():
        for lr_imgs, hr_imgs in val_loader:
            lr_imgs = lr_imgs.to(device)
            hr_imgs = hr_imgs.to(device)

            hr_pred, noise_pred = model(lr_imgs)

            # 反归一化到[0,1]
            hr_pred_np = (hr_pred.cpu().numpy() + 1) / 2.0
            hr_imgs_np = (hr_imgs.cpu().numpy() + 1) / 2.0

            for pred, gt in zip(hr_pred_np, hr_imgs_np):
                # 转为单通道灰度（CT评估标准），适配512×300
                pred_gray = pred.transpose(1, 2, 0)[..., 0].clip(0, 1)  # 512×300
                gt_gray = gt.transpose(1, 2, 0)[..., 0].clip(0, 1)  # 512×300
                total_psnr += psnr(gt_gray, pred_gray, data_range=1.0)
                # SSIM适配非正方形图像
                total_ssim += ssim(gt_gray, pred_gray, data_range=1.0, win_size=3)
                count += 1

    avg_psnr = total_psnr / count
    avg_ssim = total_ssim / count
    return avg_psnr, avg_ssim


# ===================== 6. 训练函数（修改超分倍数和尺寸日志） =====================
def train_model(lr_dir, hr_dir, save_model_path, csv_save_path,
                batch_size=2, epochs=100, learning_rate=1e-4, upscale_factor=4, val_ratio=0.1):
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    print(f"=" * 50)
    print(f"训练配置：")
    print(f"设备: {device} | 超分倍数：4x (512×75→512×300)")
    print(f"LR文件夹：{lr_dir}")
    print(f"HR文件夹：{hr_dir}")
    print(f"=" * 50)

    # 预处理（无随机变换，避免CT解剖结构失真）
    transform = transforms.Compose([
        transforms.ToPILImage(),
        transforms.ToTensor(),
        transforms.Normalize(mean=[0.5, 0.5, 0.5], std=[0.5, 0.5, 0.5])
    ])

    # 加载数据集（9个子文件夹序列）
    try:
        full_dataset = SRDataset(lr_dir, hr_dir, transform)
        print(f"✅ 数据集加载成功，总样本数：{len(full_dataset)}")
    except Exception as e:
        print(f"\n❌ 数据集加载失败：{str(e)}")
        return

    # 划分训练/验证集
    n_val = int(len(full_dataset) * val_ratio)
    n_train = len(full_dataset) - n_val
    train_dataset, val_dataset = data.random_split(
        full_dataset, [n_train, n_val],
        generator=torch.Generator().manual_seed(42)
    )

    # 数据加载器（适配大尺寸图像，减小batch_size）
    train_loader = data.DataLoader(train_dataset, batch_size=batch_size, shuffle=True, num_workers=0)
    val_loader = data.DataLoader(val_dataset, batch_size=1, shuffle=False, num_workers=0)

    # 初始化模型和优化器
    model = EDSRInvSR(upscale_factor=upscale_factor, num_res_blocks=32).to(device)
    loss_fn = InvSRLoss()
    optimizer = optim.AdamW(model.parameters(), lr=learning_rate, weight_decay=1e-6)
    scheduler = optim.lr_scheduler.StepLR(optimizer, step_size=40, gamma=0.6)

    # 断点续训
    start_epoch = 0
    best_psnr = 0.0
    best_model_path = save_model_path.replace('.pth', '_best.pth')

    if os.path.exists(best_model_path):
        best_checkpoint = torch.load(best_model_path, map_location=device, weights_only=False)
        best_psnr = best_checkpoint.get('val_psnr', 0.0)
        print(f"✅ 加载最佳模型基准 PSNR: {best_psnr:.2f} dB")

    if os.path.exists(save_model_path):
        checkpoint = torch.load(save_model_path, map_location=device, weights_only=False)
        model.load_state_dict(checkpoint['model_state_dict'])
        optimizer.load_state_dict(checkpoint['optimizer_state_dict'])
        start_epoch = checkpoint['epoch']
        print(f"✅ 加载已有模型，从第 {start_epoch} 轮继续训练")

    # 初始化训练日志
    os.makedirs(os.path.dirname(csv_save_path), exist_ok=True)
    csv_header = ['epoch', 'train_loss', 'pixel_loss', 'noise_loss', 'val_psnr', 'val_ssim']
    if not os.path.exists(csv_save_path):
        with open(csv_save_path, 'w', newline='', encoding='utf-8') as f:
            csv.writer(f).writerow(csv_header)

    # 训练循环
    for epoch in range(start_epoch, epochs):
        model.train()
        epoch_loss = 0.0
        epoch_pixel_loss = 0.0
        epoch_noise_loss = 0.0
        pbar = tqdm(train_loader, desc=f"Epoch {epoch + 1}/{epochs}")

        for batch_idx, (lr_imgs, hr_imgs) in enumerate(pbar):
            lr_imgs = lr_imgs.to(device)
            hr_imgs = hr_imgs.to(device)

            # 打印第一批数据的尺寸（调试用）
            if batch_idx == 0 and epoch == start_epoch:
                print(f"\n📌 第一批数据尺寸：")
                print(f"LR输入尺寸：{lr_imgs.shape} (预期: [batch, 3, 512, 75])")
                print(f"HR目标尺寸：{hr_imgs.shape} (预期: [batch, 3, 512, 300])")

            # 生成CT专属低噪声
            noisy_hr, noise_gt = generate_diffusion_noise(hr_imgs, noise_level=0.02)

            # 模型前向传播
            hr_pred, noise_pred = model(lr_imgs)

            # 打印第一批输出尺寸（调试用）
            if batch_idx == 0 and epoch == start_epoch:
                print(f"模型输出尺寸：{hr_pred.shape} (预期: [batch, 3, 512, 300])")
                print(f"噪声预测尺寸：{noise_pred.shape}")
                print(f"噪声目标尺寸：{noise_gt.shape}")

            # 计算损失
            total_loss, pixel_loss, noise_loss = loss_fn(hr_pred, hr_imgs, noise_pred, noise_gt)

            # 反向传播
            optimizer.zero_grad()
            total_loss.backward()
            optimizer.step()

            # 累计损失
            epoch_loss += total_loss.item()
            epoch_pixel_loss += pixel_loss.item()
            epoch_noise_loss += noise_loss.item()
            pbar.set_postfix({'total_loss': total_loss.item(), 'pixel_loss': pixel_loss.item()})

        # 学习率更新
        scheduler.step()

        # 计算平均损失
        avg_train_loss = epoch_loss / len(train_loader)
        avg_pixel_loss = epoch_pixel_loss / len(train_loader)
        avg_noise_loss = epoch_noise_loss / len(train_loader)

        # 验证
        val_psnr, val_ssim = validate_model(model, val_loader, device, upscale_factor)
        print(f"\nEpoch {epoch + 1} 训练结果：")
        print(f"训练总损失：{avg_train_loss:.4f} | 像素损失：{avg_pixel_loss:.4f} | 噪声损失：{avg_noise_loss:.4f}")
        print(f"验证PSNR：{val_psnr:.2f} dB | 验证SSIM：{val_ssim:.4f}")

        # 保存训练日志
        with open(csv_save_path, 'a', newline='', encoding='utf-8') as f:
            csv.writer(f).writerow([epoch + 1, avg_train_loss, avg_pixel_loss, avg_noise_loss, val_psnr, val_ssim])

        # 保存最佳模型
        if val_psnr > best_psnr:
            best_psnr = val_psnr
            torch.save({
                'epoch': epoch + 1,
                'model_state_dict': model.state_dict(),
                'optimizer_state_dict': optimizer.state_dict(),
                'val_psnr': val_psnr
            }, best_model_path)
            print(f"🏆 新最佳模型已保存 (PSNR: {val_psnr:.2f} dB)")

        # 定期保存检查点
        if (epoch + 1) % 10 == 0 or (epoch + 1) == epochs:
            torch.save({
                'epoch': epoch + 1,
                'model_state_dict': model.state_dict(),
                'optimizer_state_dict': optimizer.state_dict(),
                'val_psnr': val_psnr
            }, save_model_path)
            print(f"💾 检查点模型已保存至: {save_model_path}")


# ===================== 7. 保存IMA文件（适配512×75输入） =====================
def save_as_ima(pixel_array, original_ds, save_path):
    """保存为CT专属IMA文件，适配512×300分辨率，修复HU值映射"""
    # 输入校验（新增512×75的支持）
    if pixel_array.shape not in [(512, 75), (512, 300)]:
        raise ValueError(f"像素数组尺寸必须为512×75/512×300，当前：{pixel_array.shape}")
    if pixel_array.dtype != np.uint8:
        pixel_array = pixel_array.astype(np.uint8)

    # 创建DICOM数据集
    ds = Dataset()
    file_meta = Dataset()

    # 继承原始元信息
    if hasattr(original_ds, 'PatientID'):
        ds.PatientID = original_ds.PatientID
    if hasattr(original_ds, 'StudyID'):
        ds.StudyID = original_ds.StudyID
    ds.SeriesNumber = original_ds.SeriesNumber if hasattr(original_ds, 'SeriesNumber') else 1
    ds.Modality = original_ds.Modality if hasattr(original_ds, 'Modality') else 'CT'
    if hasattr(original_ds, 'Manufacturer'):
        ds.Manufacturer = original_ds.Manufacturer

    # CT专属核心标签
    ds.SOPClassUID = CTImageStorage
    ds.SOPInstanceUID = pydicom.uid.generate_uid()
    ds.SliceThickness = 1.0  # 1mm层厚
    now = datetime.datetime.now()
    ds.StudyDate = now.strftime('%Y%m%d')
    ds.StudyTime = now.strftime('%H%M%S.%f')[:-3]

    # HU值映射（核心）
    ds.RescaleSlope = 5.5556  # (400 - (-1000)) / 255
    ds.RescaleIntercept = -1000.0

    # 像素几何参数
    if hasattr(original_ds, 'PixelSpacing'):
        pixel_spacing = original_ds.PixelSpacing
        pixel_spacing = [float(pixel_spacing)] * 2 if isinstance(pixel_spacing, (str, float, int)) else pixel_spacing
        ds.PixelSpacing = [float(x) for x in pixel_spacing[:2]]
    else:
        ds.PixelSpacing = [0.5, 0.5]

    # 像素数据标签（适配512×300）
    ds.Rows = pixel_array.shape[0]  # 512
    ds.Columns = pixel_array.shape[1]  # 300（输出）或75（输入）
    ds.SamplesPerPixel = 1
    ds.PhotometricInterpretation = "MONOCHROME2"
    ds.BitsAllocated = 8
    ds.BitsStored = 8
    ds.HighBit = 7
    ds.PixelRepresentation = 0
    ds.PixelData = pixel_array.tobytes()

    # 文件元信息（兼容所有pydicom版本）
    file_meta.FileMetaInformationVersion = b'\x00\x01'
    file_meta.MediaStorageSOPClassUID = ds.SOPClassUID
    file_meta.MediaStorageSOPInstanceUID = ds.SOPInstanceUID
    file_meta.TransferSyntaxUID = ExplicitVRLittleEndian
    file_meta.ImplementationClassUID = pydicom.uid.generate_uid()

    # 保存文件
    fd = FileDataset(
        save_path,
        ds,
        file_meta=file_meta,
        preamble=b"\0" * 128
    )
    fd.save_as(save_path, write_like_original=False)
    print(f"✅ 保存IMA文件：{save_path}")


# ===================== 8. 批量测试函数（修改超分倍数和LR尺寸） =====================
def test_batch_images(model_path, input_folder, output_folder, upscale_factor=4):
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    print(f"=" * 50)
    print(f"测试配置：")
    print(f"设备: {device} | 超分倍数：4x (512×75→512×300)")
    print(f"输入文件夹：{input_folder}")
    print(f"输出文件夹：{output_folder}")
    print(f"=" * 50)

    # 加载模型
    model = EDSRInvSR(upscale_factor=upscale_factor, num_res_blocks=32).to(device)
    checkpoint = torch.load(model_path, map_location=device, weights_only=False)
    model.load_state_dict(checkpoint['model_state_dict'])
    model.eval()
    print(f"✅ 模型加载成功")

    # 预处理
    transform = transforms.Compose([
        transforms.ToPILImage(),
        transforms.ToTensor(),
        transforms.Normalize(mean=[0.5, 0.5, 0.5], std=[0.5, 0.5, 0.5])
    ])

    # 收集9个子文件夹的测试文件
    input_folder = os.path.abspath(input_folder)
    img_paths = []
    for seq_idx in range(1):
        seq_dir = os.path.join(input_folder, str(seq_idx))
        if os.path.isdir(seq_dir):
            seq_files = sorted(
                [os.path.join(str(seq_idx), f) for f in os.listdir(seq_dir)
                 if os.path.splitext(f)[1].lower() in ['.ima', '.dcm'] and os.path.isfile(os.path.join(seq_dir, f))],
                key=natural_sort_key
            )
            img_paths.extend(seq_files)

    if not img_paths:
        raise AssertionError(f"测试文件夹 {input_folder} 中无有效IMA/DICOM影像！")
    print(f"✅ 找到 {len(img_paths)} 个测试文件")

    # 创建输出目录
    os.makedirs(output_folder, exist_ok=True)

    # 批量推理
    with torch.no_grad():
        for img_rel_path in tqdm(img_paths, desc="CT影像4x超分推理"):
            input_path = os.path.join(input_folder, img_rel_path)
            try:
                lr_img, original_ds = read_ima_image(input_path)
            except Exception as e:
                print(f"\n⚠️  无法读取 {img_rel_path}，错误：{e}，跳过")
                continue

            # ========== 核心修改：LR尺寸改为512×75 ==========
            h, w = lr_img.shape[:2]
            if (h != 512) or (w != 75):
                print(f"\nℹ️  {img_rel_path} 尺寸为 {h}×{w}，自动缩放到512×75")
                lr_img = cv2.resize(lr_img, (75, 512), interpolation=cv2.INTER_LINEAR)

            # 预处理+推理
            lr_img_3ch = np.stack([lr_img] * 3, axis=-1)
            lr_tensor = transform(lr_img_3ch).unsqueeze(0).to(device)
            hr_pred, _ = model(lr_tensor)

            # 后处理：反归一化+转单通道
            hr_pred_np = (hr_pred.squeeze(0).cpu().numpy() + 1) / 2.0
            hr_pred_np = np.transpose(hr_pred_np, (1, 2, 0)).clip(0, 1)
            hr_pred_gray = (hr_pred_np[..., 0] * 255).astype(np.uint8)

            # 保存文件（保留子文件夹结构）
            save_dir = os.path.join(output_folder, os.path.dirname(img_rel_path))
            os.makedirs(save_dir, exist_ok=True)
            save_name = os.path.splitext(os.path.basename(img_rel_path))[0] + '_4x_SR.ima'
            save_path = os.path.join(save_dir, save_name)
            save_as_ima(hr_pred_gray, original_ds, save_path)

    print(f"\n🎉 CT影像4x超分完成！结果保存至: {output_folder}")
    print(f"输出规格：512×300 | IMA格式 | HU值范围：-1000~400")


# ===================== 9. 主函数（配置+启动训练/测试） =====================
if __name__ == "__main__":
    # ========== 请修改为你的实际路径 ==========
    LR_DIR = r"E:\code\Python\CT_LR\dataset\512x75"  # 512×75 LR文件夹（含0-8子文件夹）
    HR_DIR = r"E:\code\Python\CT_LR\dataset\512x300"  # 512×300 HR文件夹（含0-8子文件夹）
    SAVE_MODEL_PATH = r"E:\code\Python\CT_LR\result_1\edsr_invSR_4x_ct\edsr_invSR_4x_sr.pth"
    CSV_SAVE_PATH = r"E:\code\Python\CT_LR\result_1\edsr_invSR_4x_ct\training_metrics.csv"
    TEST_BATCH_INPUT = r"E:\code\Python\CT_LR\dataset\test_4x"  # 测试用512×75文件夹（含0-8子文件夹）
    TEST_BATCH_OUTPUT = r"E:\code\Python\CT_LR\result_1\edsr_invSR_4x_ct\test_output"

    # 创建输出目录
    os.makedirs(os.path.dirname(SAVE_MODEL_PATH), exist_ok=True)
    os.makedirs(TEST_BATCH_OUTPUT, exist_ok=True)

    # ========== 训练模型 ==========
    train_model(
        lr_dir=LR_DIR,
        hr_dir=HR_DIR,
        save_model_path=SAVE_MODEL_PATH,
        csv_save_path=CSV_SAVE_PATH,
        batch_size=2,  # 512×75图像建议batch_size=2（避免显存溢出）
        epochs=10,  # 4x超分建议至少100轮
        learning_rate=1e-4,
        upscale_factor=4,  # 4倍超分（75→300）
        val_ratio=0.1  # 验证集比例10%
    )

    # ========== 测试模型（训练完成后执行） ==========
    test_batch_images(
        model_path=SAVE_MODEL_PATH.replace('.pth', '_best.pth'),  # 加载最佳模型
        input_folder=TEST_BATCH_INPUT,
        output_folder=TEST_BATCH_OUTPUT,
        upscale_factor=4
    )