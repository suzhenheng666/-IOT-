#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "uart.h"
#include <string.h>
#include "esp_log.h"
#include "Protocol.h"
#include "wifi_app.h"
#include "cJSON.h"
#include "esp_http_client.h"
#include "nvs.h"

QueueHandle_t sensor_data_queue;

static const char *TAG = "UART_INTR_DEMO";

/* ========== NVS 离线缓存（环形缓冲区） ========== */
#define CACHE_NS    "s_cache"
#define MAX_CACHE   50

static void cache_sensor_data(const char *json_str) {
    nvs_handle_t h;
    if (nvs_open(CACHE_NS, NVS_READWRITE, &h) != ESP_OK) return;

    uint8_t head = 0, tail = 0, cnt = 0;
    nvs_get_u8(h, "head", &head);
    nvs_get_u8(h, "tail", &tail);
    nvs_get_u8(h, "cnt",  &cnt);

    if (cnt >= MAX_CACHE) {
        // 丢弃最旧条目，为新数据腾位置
        char old_key[8];
        snprintf(old_key, sizeof(old_key), "c%d", tail);
        nvs_erase_key(h, old_key);
        tail = (tail + 1) % MAX_CACHE;
        cnt--;
    }

    char key[8];
    snprintf(key, sizeof(key), "c%d", head);
    nvs_set_str(h, key, json_str);
    head = (head + 1) % MAX_CACHE;
    cnt++;

    nvs_set_u8(h, "head", head);
    nvs_set_u8(h, "tail", tail);
    nvs_set_u8(h, "cnt",  cnt);
    nvs_commit(h);
    nvs_close(h);

    ESP_LOGI(TAG, "[CACHE] 已缓存 (共 %d 条)", cnt);
}

static void flush_cached_data(void) {
    nvs_handle_t h;
    if (nvs_open(CACHE_NS, NVS_READWRITE, &h) != ESP_OK) return;

    uint8_t tail = 0, cnt = 0;
    nvs_get_u8(h, "tail", &tail);
    nvs_get_u8(h, "cnt",  &cnt);
    if (cnt == 0) { nvs_close(h); return; }

    ESP_LOGI(TAG, "[CACHE] 正在上传 %d 条缓存...", cnt);

    char api_url[] = "http://10.175.1.126:8080/device/report";
    int uploaded = 0;

    for (int i = 0; i < cnt; i++) {
        char key[8];
        snprintf(key, sizeof(key), "c%d", tail);

        size_t len;
        if (nvs_get_str(h, key, NULL, &len) != ESP_OK) break;
        char *json_str = malloc(len);
        nvs_get_str(h, key, json_str, &len);

        esp_http_client_config_t config = {
            .url = api_url,
            .method = HTTP_METHOD_POST,
            .timeout_ms = 5000,
        };
        esp_http_client_handle_t client = esp_http_client_init(&config);
        esp_http_client_set_header(client, "Content-Type", "application/json");
        esp_http_client_set_post_field(client, json_str, strlen(json_str));

        esp_err_t err = esp_http_client_perform(client);
        esp_http_client_cleanup(client);
        free(json_str);

        if (err != ESP_OK) {
            ESP_LOGW(TAG, "[CACHE] 上传中断，保留 %d 条", cnt - uploaded);
            break;
        }

        nvs_erase_key(h, key);
        tail = (tail + 1) % MAX_CACHE;
        uploaded++;
    }

    cnt -= uploaded;
    nvs_set_u8(h, "tail", tail);
    nvs_set_u8(h, "cnt",  cnt);
    nvs_commit(h);
    nvs_close(h);

    ESP_LOGI(TAG, "[CACHE] 上传完成: %d 条成功, 剩余 %d 条", uploaded, cnt);
}

/* ========== HTTP 上传任务 ========== */
static void http_upload_task(void *pvParameters)
{
    SensorPayload_t sensor_data;
    char api_url[] = "http://10.175.1.126:8080/device/report";

    for(;;) {
        if (xQueueReceive(sensor_data_queue, &sensor_data, portMAX_DELAY)) {

            double temperature = sensor_data.temp_int + (sensor_data.temp_dec / 100.0);
            double humidity = sensor_data.humi_int + (sensor_data.humi_dec / 10.0);

            ESP_LOGI(TAG, "队列收到数据，准备上传...");

            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "device_id", "dev001");
            cJSON_AddNumberToObject(root, "longitude", 121.112345);
            cJSON_AddNumberToObject(root, "latitude", 31.245678);
            cJSON_AddNumberToObject(root, "temperature", temperature);
            cJSON_AddNumberToObject(root, "humidity", humidity);
            cJSON_AddNumberToObject(root, "distance", 1.2);

            char *json_string = cJSON_PrintUnformatted(root);

            esp_http_client_config_t config = {
                .url = api_url,
                .method = HTTP_METHOD_POST,
                .timeout_ms = 5000,
            };
            esp_http_client_handle_t client = esp_http_client_init(&config);
            esp_http_client_set_header(client, "Content-Type", "application/json");
            esp_http_client_set_post_field(client, json_string, strlen(json_string));

            esp_err_t err = esp_http_client_perform(client);

            if (err == ESP_OK) {
                int status_code = esp_http_client_get_status_code(client);
                ESP_LOGI(TAG, "上传成功, HTTP %d", status_code);

                // 网络通了，尝试把缓存的历史数据也发上去
                flush_cached_data();
            } else {
                ESP_LOGE(TAG, "上传失败: %s，缓存到本地 flash", esp_err_to_name(err));
                cache_sensor_data(json_string);
            }

            esp_http_client_cleanup(client);
            cJSON_Delete(root);
            free(json_string);
        }
    }
}

/* ========== UART 事件任务 ========== */
static void uart0_event_task(void *pvParameters)
{
    uart_event_t event;
    uint8_t* dtmp = (uint8_t*) malloc(RX_BUF_SIZE);
    for(;;) {
        if(xQueueReceive(uart0_queue, (void * )&event, (TickType_t)portMAX_DELAY)) {
            bzero(dtmp, RX_BUF_SIZE);
            if(event.type == UART_DATA) {
                uart_read_bytes(USART_UX, dtmp, event.size, portMAX_DELAY);
                ESP_LOGI(TAG, "[HOST -> ESP32]: %s", dtmp);
            }
        }
    }
    free(dtmp);
    vTaskDelete(NULL);
}

uint16_t find_head(uint8_t *rx_buffer, uint16_t rx_len)
{
    while(rx_len >= sizeof(DataFrame_t))
    {
        int header_idx = -1;

        for(uint8_t i = 0; i < rx_len - 1; i++)
        {
            if(rx_buffer[i] == 0xA5 && rx_buffer[i+1] == 0x5A)
            {
                header_idx = i;
                break;
            }
        }

        if(header_idx == -1)
        {
            if(rx_buffer[rx_len-1] == 0xA5)
            {
                header_idx = rx_len - 1;
                rx_buffer[0] = 0xA5;
                rx_len = 1;
            }
            else
            {
                rx_len = 0;
            }
            return rx_len;
        }

        if(header_idx)
        {
            rx_len -= header_idx;
            memmove(rx_buffer, &rx_buffer[header_idx], rx_len);
        }

        if(rx_len >= sizeof(DataFrame_t))
        {
            DataFrame_t *frame = (DataFrame_t *)rx_buffer;
            uint8_t *calc_start_ptr = (uint8_t*)&frame->cmd;
            uint8_t len = 1 + 1 + sizeof(frame->payload);
            uint8_t cal_sum = Calc_Checksum(calc_start_ptr, len);

            if (cal_sum == frame->checksum) {
                ESP_LOGI(TAG, "收到有效数据帧！温度: %d.%d C | 湿度: %d.%d %% | 风扇状态: %d",
                            frame->payload.temp_int, frame->payload.temp_dec,
                            frame->payload.humi_int, frame->payload.humi_dec,
                            frame->payload.fan_state);

                SensorPayload_t payload_copy = frame->payload;
                xQueueSend(sensor_data_queue, &payload_copy, 0);

                rx_len -= sizeof(DataFrame_t);
                memmove(rx_buffer, rx_buffer + sizeof(DataFrame_t), rx_len);
            } else {
                ESP_LOGE(TAG, "校验和错误! 收到: %02X, 计算应为: %02X", frame->checksum, cal_sum);
                rx_len -= 1;
                memmove(rx_buffer, rx_buffer + 1, rx_len);
            }
        }
    }
    return rx_len;
}

static void uart1_event_task(void *pvParameters)
{
    uart_event_t event;

    static uint8_t rx_buffer[256];
    static uint16_t rx_len = 0;
    for(;;)
    {
        if(xQueueReceive(uart1_queue, (void*)&event, (TickType_t)portMAX_DELAY))
        {
            if(event.type == UART_DATA)
            {
                ESP_LOGW(TAG, "==== 底层串口被触发！收到了 %d 个字节 ====", event.size);
                int read_len = uart_read_bytes(STM_UART_NUM, rx_buffer + rx_len, event.size, (TickType_t)portMAX_DELAY);
                rx_len += read_len;

                rx_len = find_head(rx_buffer, rx_len);
            }
            else if(event.type == UART_FIFO_OVF || event.type == UART_BUFFER_FULL)
            {
                ESP_LOGW(TAG, "UART1 缓冲区溢出，已重置！");
                uart_flush_input(STM_UART_NUM);
                xQueueReset(uart1_queue);
                rx_len = 0;
            }
        }
    }
    vTaskDelete(NULL);
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    led_init();
    usart_init();
    wifi_init_sta();

    sensor_data_queue = xQueueCreate(5, sizeof(SensorPayload_t));

    xTaskCreate(uart0_event_task, "uart0_event_task", 4096, NULL, 12, NULL);
    xTaskCreate(uart1_event_task, "uart1_event_task", 4096, NULL, 12, NULL);
    xTaskCreate(http_upload_task, "http_upload_task", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, " 双串口 IoT 网关 + Wi-Fi + 边缘缓存 启动成功！");
    ESP_LOGI(TAG, "===========================================");
}
