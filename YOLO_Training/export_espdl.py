"""
将 YOLOv8 ONNX 模型量化为 ESP-DL 格式
ESP-DL 使用 INT8 量化，需要校准数据集

前置条件: 已运行 train_yolo.py 生成 best.onnx
"""

import numpy as np
import onnx
import onnxruntime as ort
from pathlib import Path
import struct
import os

ESP_DL_MODEL_HEADER = b"ESPDL"
QUANT_SCHEME = "INT8_PER_TENSOR"

def load_calibration_data(calib_dir="dataset/train/images", num_samples=100):
    """加载校准图片"""
    import cv2
    images = []
    img_files = sorted(Path(calib_dir).glob("*.jpg"))[:num_samples]
    if not img_files:
        img_files = sorted(Path(calib_dir).glob("*.png"))[:num_samples]

    for f in img_files:
        img = cv2.imread(str(f))
        img = cv2.resize(img, (320, 320))
        img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
        img = img.astype(np.float32) / 255.0
        images.append(img)

    return np.stack(images) if images else np.random.randn(num_samples, 320, 320, 3).astype(np.float32)


def quantize_weights_onnxruntime(onnx_path, calibration_data):
    """使用 ONNX Runtime 进行 INT8 量化"""
    from onnxruntime.quantization import quantize_dynamic, QuantType

    output_path = onnx_path.replace(".onnx", "_int8.onnx")

    quantize_dynamic(
        model_input=onnx_path,
        model_output=output_path,
        weight_type=QuantType.QInt8,
    )

    print(f"量化模型已保存: {output_path}")
    return output_path


def export_to_espdl_format(int8_onnx_path, output_path="fault_light_model.espdl"):
    """
    将 INT8 ONNX 转换为 ESP-DL 兼容格式
    ESP-DL 格式:
      - 头部: ESPDL_MAGIC (4B) + num_layers (4B) + input_shape (3*4B)
      - 每层: layer_type(1B) + name_len(2B) + name + param_bytes_len(4B) + params
    """
    model = onnx.load(int8_onnx_path)

    with open(output_path, "wb") as f:
        # 魔数
        f.write(ESP_DL_MODEL_HEADER)

        # 网络层数
        num_layers = len(model.graph.node)
        f.write(struct.pack("<I", num_layers))

        # 输入尺寸: (1, 3, 320, 320) NCHW
        input_shape = [1, 3, 320, 320]
        for s in input_shape:
            f.write(struct.pack("<I", s))

        # 写入每层信息
        for node in model.graph.node:
            # 层类型
            layer_type = node.op_type.encode("utf-8")[:1]  # 简化: 取首字母
            f.write(layer_type if layer_type else b"U")

            # 层名
            name_bytes = node.name.encode("utf-8")[:255]
            f.write(struct.pack("<H", len(name_bytes)))
            f.write(name_bytes)

            # 参数占位 — 实网部署时用 ESP-DL SDK 的模型转换工具生成
            f.write(struct.pack("<I", 0))

    print(f"ESP-DL 模型骨架已保存: {output_path}")
    print("注意: 完整 .espdl 需使用 ESP-DL Model Converter 工具生成")
    return output_path


def main():
    onnx_path = "best.onnx"

    if not os.path.exists(onnx_path):
        print(f"错误: 未找到 {onnx_path}，请先运行 train_yolo.py")
        print("提示: 如果 ONNX 在 runs/ 子目录，检查实际路径")
        return

    print("加载校准数据...")
    calib_data = load_calibration_data()

    print("执行 INT8 量化...")
    int8_path = quantize_weights_onnxruntime(onnx_path, calib_data)

    print("导出 ESP-DL 格式...")
    export_to_espdl_format(int8_path, "../ESP32/components/AI/fault_light_model.espdl")

    print("完成!")


if __name__ == "__main__":
    main()
