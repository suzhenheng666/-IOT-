#ifndef YOLO_DETECT_H__
#define YOLO_DETECT_H__

#include <stdint.h>
#include "cJSON.h"

// 故障灯类别
typedef enum {
    FAULT_GREEN_NORMAL  = 0,
    FAULT_YELLOW_WARNING = 1,
    FAULT_RED_FAULT     = 2
} FaultLightClass;

// 单次检测结果
typedef struct {
    FaultLightClass class_id;
    float confidence;
    float bbox[4];    // x, y, w, h (归一化)
} DetectionResult;

// 完整推理结果
typedef struct {
    DetectionResult detections[10];
    uint8_t num_detections;
    float inference_time_ms;
    float green_prob;
    float yellow_prob;
    float red_prob;
} YOLO_Result;

// 初始化 ESP-DL 模型
int yolo_init(void);

// 执行推理
int yolo_detect(const uint8_t *jpeg_buf, size_t jpeg_len, YOLO_Result *result);

// 将结果序列化为 JSON 字符串 (调用者需要 free)
char* yolo_result_to_json(const YOLO_Result *result);

// 获取故障状态字符串
const char* yolo_status_string(const YOLO_Result *result);

#endif
