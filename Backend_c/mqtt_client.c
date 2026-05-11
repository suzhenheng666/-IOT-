#include "mqtt_client.h"
#include "store.h"
#include <stdio.h>
#include <string.h>

#define BROKER_HOST  "localhost"
#define BROKER_PORT  1883
#define KEEPALIVE    60

static struct mosquitto *mosq = NULL;

static const char *TOPICS[] = {
    "device/dev001/telemetry",
    "device/dev001/camera",
    "device/dev001/status",
};
static const int N_TOPICS = 3;

static void on_connect(struct mosquitto *mq, void *obj, int rc) {
    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "[mqtt] 连接失败: %s\n", mosquitto_strerror(rc));
        return;
    }
    printf("[mqtt] broker 已连接\n");

    for (int i = 0; i < N_TOPICS; i++) {
        int ret = mosquitto_subscribe(mq, NULL, TOPICS[i], 1);
        if (ret == MOSQ_ERR_SUCCESS) {
            printf("[mqtt] 订阅: %s\n", TOPICS[i]);
        } else {
            fprintf(stderr, "[mqtt] 订阅失败 %s: %s\n", TOPICS[i], mosquitto_strerror(ret));
        }
    }
}

static void on_message(struct mosquitto *mq, void *obj,
                       const struct mosquitto_message *msg) {
    const char *payload_str = (const char *)msg->payload;
    int len = msg->payloadlen;

    // 确保字符串结尾
    char buf[4096];
    if (len >= (int)sizeof(buf)) len = sizeof(buf) - 1;
    memcpy(buf, msg->payload, len);
    buf[len] = '\0';

    if (strstr(msg->topic, "telemetry")) {
        store_update_telemetry(buf);
    } else if (strstr(msg->topic, "camera")) {
        store_update_camera(buf);
    } else if (strstr(msg->topic, "status")) {
        store_update_status(buf);
    }
}

void mqtt_start(void) {
    mosquitto_lib_init();

    mosq = mosquitto_new("iot_backend_c", true, NULL);
    if (!mosq) {
        fprintf(stderr, "[mqtt] 创建客户端失败\n");
        return;
    }

    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);

    // 设置遗嘱消息 (异常断开时 broker 通知订阅者)
    mosquitto_will_set(mosq, "device/dev001/status",
                       7, "{\"status\":\"offline\"}", 1, true);

    int ret = mosquitto_connect(mosq, BROKER_HOST, BROKER_PORT, KEEPALIVE);
    if (ret != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "[mqtt] 连接失败: %s\n", mosquitto_strerror(ret));
        return;
    }

    // 启动 MQTT 网络循环 (阻塞当前线程)
    printf("[mqtt] 开始事件循环...\n");
    mosquitto_loop_forever(mosq, -1, 1);
}

void mqtt_stop(void) {
    if (mosq) {
        mosquitto_disconnect(mosq);
        mosquitto_destroy(mosq);
        mosq = NULL;
    }
    mosquitto_lib_cleanup();
}
