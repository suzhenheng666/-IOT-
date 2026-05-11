#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "led.h"
#include "uart.h"
#include "Protocol.h"
#include "wifi_app.h"
#include "mqtt_app.h"
#include "camera_app.h"
#include "yolo_detect.h"

static const char *TAG = "MAIN";

QueueHandle_t sensor_data_queue;

#define RX_BUF_SIZE 1024
extern QueueHandle_t uart0_queue;
extern QueueHandle_t uart1_queue;

// ─── MQTT 上传任务 ───
static void mqtt_telemetry_task(void *pvParameters) {
    SensorPayload_t sensor_data;

    for (;;) {
        if (xQueueReceive(sensor_data_queue, &sensor_data, portMAX_DELAY)) {
            double temperature = sensor_data.temp_int + (sensor_data.temp_dec / 100.0);
            double humidity   = sensor_data.humi_int + (sensor_data.humi_dec / 10.0);

            ESP_LOGI(TAG, "温度: %.1f°C  湿度: %.1f%%  风扇: %d RPM",
                     temperature, humidity, sensor_data.fan_speed);

            mqtt_publish_telemetry(temperature, humidity, sensor_data.fan_speed);
        }
    }
}

// ─── 摄像头 + YOLO 任务 ───
static void camera_yolo_task(void *pvParameters) {
    if (camera_init() != ESP_OK) {
        ESP_LOGE(TAG, "摄像头初始化失败，跳过视觉任务");
        vTaskDelete(NULL);
        return;
    }

    if (yolo_init() != 0) {
        ESP_LOGE(TAG, "YOLO 初始化失败");
        vTaskDelete(NULL);
        return;
    }

    for (;;) {
        camera_fb_t *fb = camera_capture();
        if (fb && fb->len > 0) {
            YOLO_Result yolo_result;
            int ret = yolo_detect(fb->buf, fb->len, &yolo_result);

            if (ret == 0) {
                char *json_str = yolo_result_to_json(&yolo_result);
                mqtt_publish_camera(json_str);
                free(json_str);
            }

            camera_return_fb(fb);
        }

        vTaskDelay(pdMS_TO_TICKS(5000));  // 每 5 秒检测一次
    }
}

// ─── UART 协议解析 (UART1 ← STM32) ───
static uint16_t find_head(uint8_t *rx_buffer, uint16_t rx_len) {
    while (rx_len >= sizeof(DataFrame_t)) {
        int header_idx = -1;
        for (uint16_t i = 0; i < rx_len - 1; i++) {
            if (rx_buffer[i] == FRAME_HEADER1 && rx_buffer[i + 1] == FRAME_HEADER2) {
                header_idx = i;
                break;
            }
        }

        if (header_idx == -1) {
            if (rx_buffer[rx_len - 1] == FRAME_HEADER1) {
                rx_buffer[0] = FRAME_HEADER1;
                return 1;
            }
            return 0;
        }

        if (header_idx > 0) {
            rx_len -= header_idx;
            memmove(rx_buffer, &rx_buffer[header_idx], rx_len);
        }

        if (rx_len >= sizeof(DataFrame_t)) {
            DataFrame_t *frame = (DataFrame_t *)rx_buffer;
            uint8_t *calc_start = (uint8_t *)&frame->cmd;
            uint8_t calc_len = 1 + 1 + sizeof(frame->payload);
            uint8_t cal_sum = Calc_Checksum(calc_start, calc_len);

            if (cal_sum == frame->checksum && frame->cmd == CMD_SENSOR_REPORT) {
                ESP_LOGI(TAG, "STM32 → 温度:%d.%d°C 湿度:%d.%d%% 风扇:%d RPM",
                         frame->payload.temp_int, frame->payload.temp_dec,
                         frame->payload.humi_int, frame->payload.humi_dec,
                         frame->payload.fan_speed);

                SensorPayload_t copy = frame->payload;
                xQueueSend(sensor_data_queue, &copy, 0);

                rx_len -= sizeof(DataFrame_t);
                memmove(rx_buffer, rx_buffer + sizeof(DataFrame_t), rx_len);
            } else {
                ESP_LOGE(TAG, "校验失败! 收到:%02X 计算:%02X", frame->checksum, cal_sum);
                rx_len -= 1;
                memmove(rx_buffer, rx_buffer + 1, rx_len);
            }
        }
    }
    return rx_len;
}

static void uart1_event_task(void *pvParameters) {
    uart_event_t event;
    static uint8_t rx_buffer[256];
    static uint16_t rx_len = 0;

    for (;;) {
        if (xQueueReceive(uart1_queue, (void *)&event, portMAX_DELAY)) {
            if (event.type == UART_DATA) {
                int read_len = uart_read_bytes(STM_UART_NUM, rx_buffer + rx_len, event.size, portMAX_DELAY);
                rx_len += read_len;
                rx_len = find_head(rx_buffer, rx_len);
            } else if (event.type == UART_FIFO_OVF || event.type == UART_BUFFER_FULL) {
                uart_flush_input(STM_UART_NUM);
                xQueueReset(uart1_queue);
                rx_len = 0;
            }
        }
    }
    vTaskDelete(NULL);
}

// ─── 心跳任务 ───
static void heartbeat_task(void *pvParameters) {
    for (;;) {
        mqtt_publish_status("online");
        vTaskDelay(pdMS_TO_TICKS(30000));  // 每 30 秒心跳
    }
}

// ─── 入口 ───
void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    led_init();
    usart_init();
    wifi_init_sta();

    // 等待 WiFi 连接稳定
    vTaskDelay(pdMS_TO_TICKS(3000));

    mqtt_app_start();

    sensor_data_queue = xQueueCreate(5, sizeof(SensorPayload_t));

    xTaskCreate(uart1_event_task,     "uart1_task",   4096, NULL, 12, NULL);
    xTaskCreate(mqtt_telemetry_task,  "mqtt_tele",    4096, NULL, 5,  NULL);
    xTaskCreate(camera_yolo_task,     "camera_yolo",  8192, NULL, 4,  NULL);
    xTaskCreate(heartbeat_task,       "heartbeat",    2048, NULL, 3,  NULL);

    ESP_LOGI(TAG, "====================================");
    ESP_LOGI(TAG, " IoT 网关启动 (MQTT + YOLO 3-Class)");
    ESP_LOGI(TAG, "====================================");
}
