#include "mqtt_app.h"
#include <string.h>

static const char *TAG = "MQTT_APP";
static esp_mqtt_client_handle_t mqtt_client = NULL;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                                int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT 已连接");
            mqtt_publish_status("online");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT 断开，自动重连中...");
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT 错误");
            break;
        default:
            break;
    }
}

void mqtt_app_start(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
        .network.disable_auto_reconnect = false,
        .session.keepalive = 60,
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID,
                                    mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

esp_mqtt_client_handle_t mqtt_get_client(void) {
    return mqtt_client;
}

void mqtt_publish_telemetry(double temp, double hum, uint16_t fan_speed) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "device_id", DEVICE_ID);
    cJSON_AddNumberToObject(root, "longitude", 121.112345);
    cJSON_AddNumberToObject(root, "latitude", 31.245678);
    cJSON_AddNumberToObject(root, "temperature", temp);
    cJSON_AddNumberToObject(root, "humidity", hum);
    cJSON_AddNumberToObject(root, "fan_speed", fan_speed);

    char *json_str = cJSON_PrintUnformatted(root);
    esp_mqtt_client_publish(mqtt_client, TOPIC_TELEMETRY, json_str, 0, 1, 0);
    free(json_str);
    cJSON_Delete(root);

    ESP_LOGI(TAG, "遥测已发布: temp=%.1f hum=%.1f rpm=%d", temp, hum, fan_speed);
}

void mqtt_publish_camera(const char *json_payload) {
    esp_mqtt_client_publish(mqtt_client, TOPIC_CAMERA, json_payload, 0, 1, 0);
    ESP_LOGI(TAG, "摄像头数据已发布");
}

void mqtt_publish_status(const char *status) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "device_id", DEVICE_ID);
    cJSON_AddStringToObject(root, "status", status);
    cJSON_AddNumberToObject(root, "timestamp", (double)time(NULL));

    char *json_str = cJSON_PrintUnformatted(root);
    esp_mqtt_client_publish(mqtt_client, TOPIC_STATUS, json_str, 0, 1, 0);
    free(json_str);
    cJSON_Delete(root);
}
