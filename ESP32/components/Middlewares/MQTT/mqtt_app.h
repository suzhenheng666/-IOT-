#ifndef MQTT_APP_H__
#define MQTT_APP_H__

#include "mqtt_client.h"
#include "esp_log.h"
#include "cJSON.h"

#define MQTT_BROKER_URI  "mqtt://192.168.1.100:1883"
#define DEVICE_ID        "dev001"

// 主题路径
#define TOPIC_TELEMETRY  "device/" DEVICE_ID "/telemetry"
#define TOPIC_CAMERA     "device/" DEVICE_ID "/camera"
#define TOPIC_STATUS     "device/" DEVICE_ID "/status"

void mqtt_app_start(void);
esp_mqtt_client_handle_t mqtt_get_client(void);
void mqtt_publish_telemetry(double temp, double hum, uint16_t fan_speed);
void mqtt_publish_camera(const char *json_payload);
void mqtt_publish_status(const char *status);

#endif
