"""
生成合成服务器机柜故障灯检测数据集
3 分类: green_normal(0) / yellow_warning(1) / red_fault(2)
每张图包含 1-5 个随机位置的 LED 指示灯，自动生成 YOLO 格式标签
"""
import cv2
import numpy as np
import os
import random
from pathlib import Path

# ============ 配置 ============
IMG_SIZE = 320
NUM_TRAIN = 400  # 训练图片数
NUM_VAL = 100    # 验证图片数
LED_RADIUS_MIN = 6
LED_RADIUS_MAX = 12

# 数据集目录
DATASET_DIR = Path("dataset")
for split in ["train", "val"]:
    for sub in ["images", "labels"]:
        (DATASET_DIR / split / sub).mkdir(parents=True, exist_ok=True)


def generate_server_panel():
    """生成一个暗色服务器面板背景 (320x320)"""
    img = np.random.randint(15, 45, (IMG_SIZE, IMG_SIZE, 3), dtype=np.uint8)

    # 加金属拉丝纹理
    for y in range(0, IMG_SIZE, 3):
        noise = np.random.randint(-8, 8)
        img[y:y+1, :, :] = np.clip(img[y:y+1, :, :].astype(np.int16) + noise, 0, 255).astype(np.uint8)

    # 加竖直线条(模拟前面板接缝)
    for _ in range(np.random.randint(2, 6)):
        x = np.random.randint(20, IMG_SIZE - 20)
        cv2.line(img, (x, 0), (x, IMG_SIZE), (60, 60, 60), 1)

    # 加横线
    for _ in range(np.random.randint(1, 4)):
        y = np.random.randint(30, IMG_SIZE - 30)
        cv2.line(img, (0, y), (IMG_SIZE, y), (50, 50, 50), 1)

    # 加螺丝/铆钉装饰
    for _ in range(np.random.randint(2, 8)):
        sx = np.random.randint(10, IMG_SIZE - 10)
        sy = np.random.randint(10, IMG_SIZE - 10)
        cv2.circle(img, (sx, sy), 3, (80, 80, 80), -1)
        cv2.circle(img, (sx, sy), 4, (40, 40, 40), 1)

    return img


def draw_led(img, cx, cy, radius, color, glow=True):
    """在面板上画一个LED指示灯(带辉光效果)"""
    color_bgr = {
        "green":  (0, 200, 0),
        "yellow": (0, 220, 220),
        "red":    (0, 0, 220),
    }[color]

    glow_color = {
        "green":  (0, 80, 0),
        "yellow": (0, 80, 80),
        "red":    (0, 0, 60),
    }[color]

    # 辉光
    if glow:
        glow_r = radius * 2 + np.random.randint(1, 4)
        overlay = img.copy()
        cv2.circle(overlay, (cx, cy), glow_r, glow_color, -1)
        cv2.addWeighted(overlay, 0.35, img, 0.65, 0, img)

    # LED外圈(暗色边框)
    cv2.circle(img, (cx, cy), radius + 1, (30, 30, 30), -1)
    # LED亮色核心
    cv2.circle(img, (cx, cy), radius, color_bgr, -1)
    # 高光点
    highlight_r = max(2, radius // 3)
    cv2.circle(img, (cx - radius//3, cy - radius//3), highlight_r, (255, 255, 255), -1)


def add_text_label(img, x, y, text):
    """在LED旁边添加文字标签"""
    font = cv2.FONT_HERSHEY_SIMPLEX
    cv2.putText(img, text, (x + 15, y + 4), font, 0.35, (180, 180, 180), 1, cv2.LINE_AA)


def generate_image(img_id, split):
    """生成一张图片 + YOLO 标签"""
    img = generate_server_panel()
    h, w = img.shape[:2]
    labels = []

    # 随机放 1-5 个 LED
    num_leds = np.random.randint(1, 6)

    # 选择 LED 颜色组合(模拟真实场景)
    # 大部分是绿色(正常), 少数黄/红
    colors_pool = (["green"] * 5 + ["yellow"] * 2 + ["red"] * 1)  # 加权
    chosen_colors = [random.choice(colors_pool) for _ in range(num_leds)]

    # 确保 red 出现在大约 20% 的图片中, yellow 30%
    if np.random.random() < 0.2:
        chosen_colors[np.random.randint(0, len(chosen_colors))] = "red"
    if np.random.random() < 0.3:
        chosen_colors[np.random.randint(0, len(chosen_colors))] = "yellow"

    placed = []
    for color in chosen_colors:
        # 尝试放置 LED, 避免重叠
        for attempt in range(20):
            radius = np.random.randint(LED_RADIUS_MIN, LED_RADIUS_MAX + 1)
            cx = np.random.randint(radius + 10, w - radius - 10)
            cy = np.random.randint(radius + 10, h - radius - 10)

            # 检查与已有LED的距离
            too_close = False
            for (px, py, pr) in placed:
                if np.sqrt((cx - px)**2 + (cy - py)**2) < (radius + pr + 8):
                    too_close = True
                    break

            if not too_close:
                placed.append((cx, cy, radius))
                break
        else:
            # 20次都没放下, 随机放一个
            radius = np.random.randint(LED_RADIUS_MIN, LED_RADIUS_MAX + 1)
            cx = np.random.randint(radius + 10, w - radius - 10)
            cy = np.random.randint(radius + 10, h - radius - 10)
            placed.append((cx, cy, radius))

        draw_led(img, cx, cy, radius, color)

        # YOLO 格式: class x_center y_center width height (归一化)
        bw = (2 * radius + 4) / w  # bbox 略大于LED
        bh = (2 * radius + 4) / h
        class_id = {"green": 0, "yellow": 1, "red": 2}[color]
        labels.append(f"{class_id} {cx/w:.6f} {cy/h:.6f} {bw:.6f} {bh:.6f}")

    # 添加一些随机噪声模拟真实摄像头
    if np.random.random() < 0.5:
        noise = np.random.randint(0, 15, img.shape, dtype=np.uint8)
        img = cv2.add(img, noise)

    # 轻微模糊(模拟运动/对焦不准)
    if np.random.random() < 0.3:
        ksize = np.random.choice([3, 5])
        img = cv2.GaussianBlur(img, (ksize, ksize), 0)

    # 亮度变化
    if np.random.random() < 0.4:
        factor = np.random.uniform(0.7, 1.3)
        img = np.clip(img.astype(np.float32) * factor, 0, 255).astype(np.uint8)

    # 保存
    img_name = f"{split}_{img_id:04d}"
    img_path = DATASET_DIR / split / "images" / f"{img_name}.jpg"
    lbl_path = DATASET_DIR / split / "labels" / f"{img_name}.txt"

    cv2.imwrite(str(img_path), img)
    with open(lbl_path, "w") as f:
        f.write("\n".join(labels))

    return len(labels)


# ============ 生成数据集 ============
print("生成训练集...")
total_train_boxes = 0
for i in range(NUM_TRAIN):
    n = generate_image(i, "train")
    total_train_boxes += n
    if (i + 1) % 100 == 0:
        print(f"  训练: {i+1}/{NUM_TRAIN}")

print(f"\n训练集: {NUM_TRAIN} 张图片, {total_train_boxes} 个标注框")

print("生成验证集...")
total_val_boxes = 0
for i in range(NUM_VAL):
    n = generate_image(i, "val")
    total_val_boxes += n
    if (i + 1) % 50 == 0:
        print(f"  验证: {i+1}/{NUM_VAL}")

print(f"\n验证集: {NUM_VAL} 张图片, {total_val_boxes} 个标注框")

# ============ 生成 data.yaml ============
yaml_content = f"""path: {DATASET_DIR.absolute().as_posix()}
train: train/images
val: val/images

names:
  0: green_normal
  1: yellow_warning
  2: red_fault

nc: 3
"""
with open(DATASET_DIR / "data.yaml", "w") as f:
    f.write(yaml_content)

print(f"\n数据集生成完成!")
print(f"总图片: {NUM_TRAIN + NUM_VAL} 张")
print(f"总标注: {total_train_boxes + total_val_boxes} 个")
print(f"data.yaml 已生成")
print(f"\n目录结构:")
print(f"  dataset/")
print(f"    train/images/  ({NUM_TRAIN} jpg)")
print(f"    train/labels/  ({NUM_TRAIN} txt)")
print(f"    val/images/    ({NUM_VAL} jpg)")
print(f"    val/labels/    ({NUM_VAL} txt)")
print(f"    data.yaml")
