#include "yolo_detect.h"
#include "esp_log.h"
#include "dl_image_preprocessor.hpp"
#include "dl_detect_yolo11_nano.hpp"

static const char *TAG = "YOLO";
static dl::detect::Yolo11Nano *detector = nullptr;
static dl::image::ImagePreprocessor *preprocessor = nullptr;

// 模型类别名
static const char* CLASS_NAMES[] = {"green_normal", "yellow_warning", "red_fault"};

int yolo_init(void) {
    ESP_LOGI(TAG, "加载 YOLO 模型...");

    // 使用 ESP-DL 内置的 YOLOv11 Nano 架构
    detector = new dl::detect::Yolo11Nano();

    // 配置预处理: QVGA RGB888 → 模型输入
    dl::image::ImagePreprocessorConfig pp_cfg = {
        .input_size = {320, 240, 3},      // QVGA RGB
        .output_size = {320, 320, 3},      // 模型输入 320x320
        .mean = {0.485f, 0.456f, 0.406f},
        .std  = {0.229f, 0.224f, 0.225f},
        .resize_method = dl::image::ResizeMethod::BILINEAR,
        .pixel_format = dl::image::PixelFormat::RGB888,
    };
    preprocessor = new dl::image::ImagePreprocessor(pp_cfg);

    ESP_LOGI(TAG, "YOLO 模型加载完成");
    return 0;
}

int yolo_detect(const uint8_t *jpeg_buf, size_t jpeg_len, YOLO_Result *result) {
    memset(result, 0, sizeof(YOLO_Result));

    int64_t t_start = esp_timer_get_time();

    // 1. JPEG → RGB888 解码 (ESP-IDF 内置 JPEG 解码器)
    dl::image::Image<uint8_t> input_img = dl::image::jpeg_decode(jpeg_buf, jpeg_len);
    if (input_img.data == nullptr) {
        ESP_LOGE(TAG, "JPEG 解码失败");
        return -1;
    }

    // 2. 预处理
    dl::TensorBase *input_tensor = preprocessor->run(input_img);
    input_img.free();

    // 3. 推理
    std::vector<dl::detect::DetectionResult> dets = detector->run(input_tensor);
    delete input_tensor;

    int64_t t_end = esp_timer_get_time();
    result->inference_time_ms = (t_end - t_start) / 1000.0f;

    // 4. 汇总类别概率
    float class_probs[3] = {0};
    result->num_detections = 0;

    for (const auto &det : dets) {
        if (result->num_detections >= 10) break;

        int cls = det.class_id;
        if (cls >= 0 && cls < 3 && det.score > class_probs[cls]) {
            class_probs[cls] = det.score;
        }

        result->detections[result->num_detections].class_id = (FaultLightClass)cls;
        result->detections[result->num_detections].confidence = det.score;
        result->detections[result->num_detections].bbox[0] = det.box[0];
        result->detections[result->num_detections].bbox[1] = det.box[1];
        result->detections[result->num_detections].bbox[2] = det.box[2];
        result->detections[result->num_detections].bbox[3] = det.box[3];
        result->num_detections++;
    }

    result->green_prob = class_probs[0];
    result->yellow_prob = class_probs[1];
    result->red_prob = class_probs[2];

    ESP_LOGI(TAG, "检测完成: %d 目标, 耗时 %.1f ms, 状态=%s",
             result->num_detections, result->inference_time_ms,
             yolo_status_string(result));

    return 0;
}

char* yolo_result_to_json(const YOLO_Result *result) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "device_id", "dev001");
    cJSON_AddStringToObject(root, "status", yolo_status_string(result));
    cJSON_AddNumberToObject(root, "inference_time_ms", result->inference_time_ms);
    cJSON_AddNumberToObject(root, "num_detections", result->num_detections);

    cJSON *probs = cJSON_CreateObject();
    cJSON_AddNumberToObject(probs, "green_normal", result->green_prob);
    cJSON_AddNumberToObject(probs, "yellow_warning", result->yellow_prob);
    cJSON_AddNumberToObject(probs, "red_fault", result->red_prob);
    cJSON_AddItemToObject(root, "class_probabilities", probs);

    cJSON *dets_arr = cJSON_CreateArray();
    for (int i = 0; i < result->num_detections; i++) {
        cJSON *det = cJSON_CreateObject();
        cJSON_AddStringToObject(det, "class", CLASS_NAMES[result->detections[i].class_id]);
        cJSON_AddNumberToObject(det, "confidence", result->detections[i].confidence);
        cJSON *box = cJSON_CreateFloatArray(result->detections[i].bbox, 4);
        cJSON_AddItemToObject(det, "bbox", box);
        cJSON_AddItemToArray(dets_arr, det);
    }
    cJSON_AddItemToObject(root, "detections", dets_arr);

    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_str;
}

const char* yolo_status_string(const YOLO_Result *result) {
    if (result->red_prob > 0.5f) return "fault";
    if (result->yellow_prob > 0.5f) return "warning";
    return "normal";
}
