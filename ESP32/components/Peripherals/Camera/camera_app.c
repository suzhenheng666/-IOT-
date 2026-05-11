#include "camera_app.h"

static const char *TAG = "CAMERA";

esp_err_t camera_init(void) {
    camera_config_t config = {
        .pin_pwdn    = CAM_PIN_PWDN,
        .pin_reset   = CAM_PIN_RESET,
        .pin_xclk    = CAM_PIN_XCLK,
        .pin_sccb_sda = CAM_PIN_SIOD,
        .pin_sccb_scl = CAM_PIN_SIOC,
        .pin_d7      = CAM_PIN_D7,
        .pin_d6      = CAM_PIN_D6,
        .pin_d5      = CAM_PIN_D5,
        .pin_d4      = CAM_PIN_D4,
        .pin_d3      = CAM_PIN_D3,
        .pin_d2      = CAM_PIN_D2,
        .pin_d1      = CAM_PIN_D1,
        .pin_d0      = CAM_PIN_D0,
        .pin_vsync   = CAM_PIN_VSYNC,
        .pin_href    = CAM_PIN_HREF,
        .pin_pclk    = CAM_PIN_PCLK,

        .xclk_freq_hz = 20000000,       // 20MHz XCLK
        .ledc_timer   = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,

        .pixel_format = PIXFORMAT_JPEG,
        .frame_size   = FRAMESIZE_QVGA,  // 320x240
        .jpeg_quality = 12,
        .fb_count     = 2,
        .grab_mode    = CAMERA_GRAB_WHEN_EMPTY,
    };

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "摄像头初始化失败: 0x%x", err);
        return err;
    }

    ESP_LOGI(TAG, "OV2640 初始化成功");
    return ESP_OK;
}

camera_fb_t* camera_capture(void) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE("CAMERA", "图像获取失败");
    }
    return fb;
}

void camera_return_fb(camera_fb_t *fb) {
    if (fb) {
        esp_camera_fb_return(fb);
    }
}
