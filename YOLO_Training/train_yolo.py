"""
训练机柜故障灯检测 YOLOv8n 模型 (3 分类: 正常绿/告警黄/故障红)

用法:
  1. python generate_dataset.py    # 生成合成数据集 (500张)
  2. cd dataset && python ../train_yolo.py  # 训练
  3. python export_espdl.py        # 量化导出 ESP-DL 格式

输出: best.pt (6MB) + best.onnx (12MB)
mAP50: 0.995 | mAP50-95: 0.990 (合成数据集)
"""
from ultralytics import YOLO
import os
import subprocess
import sys

def main():
    # Step 1: 如果没有 dataset, 先生成
    if not os.path.exists("dataset/train/images") or len(os.listdir("dataset/train/images")) == 0:
        print("数据集不存在，先生成合成数据...")
        subprocess.run([sys.executable, "generate_dataset.py"], check=True)

    # Step 2: 确保在 dataset 目录下运行 (避免中文路径问题)
    os.chdir("dataset")

    model = YOLO("../yolov8n.pt")

    results = model.train(
        data="data.yaml",
        epochs=50,
        imgsz=320,
        batch=16,
        name="fault_light",
        device="cuda",
        patience=10,
        lr0=0.001,
        augment=True,
    )

    # 验证
    metrics = model.val()
    print(f"mAP50: {metrics.box.map50:.4f}")
    print(f"mAP50-95: {metrics.box.map:.4f}")

    # 导出 ONNX 供后续量化
    model.export(format="onnx", imgsz=320, opset=12)

    print("训练完成: best.pt + best.onnx 已生成")

if __name__ == "__main__":
    main()
