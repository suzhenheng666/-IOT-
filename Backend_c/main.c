/**
 * IoT Backend (C 语言版)
 * MQTT 订阅 Mosquitto → 内存存储 → HTTP REST API
 *
 * 依赖:
 *   sudo apt install libmosquitto-dev libmicrohttpd-dev
 *
 * 编译: make
 * 运行: ./iot_backend
 *
 * API:
 *   GET /health
 *   GET /api/v1/devices/latest
 *   GET /api/v1/device/temperature?device_id=dev001
 *   GET /api/v1/device/humidity?device_id=dev001
 *   GET /api/v1/device/camera/latest
 */
#include "mqtt_client.h"
#include "http_server.h"
#include "store.h"
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>

#define HTTP_PORT 8080

static void *mqtt_thread(void *arg) {
    (void)arg;
    mqtt_start();
    return NULL;
}

int main(void) {
    printf("=====================================\n");
    printf(" IoT Backend (C) 启动中...\n");
    printf("=====================================\n");

    store_init();
    printf("[main] 内存存储初始化完成\n");

    // MQTT 在独立线程中运行 (mosquitto_loop_forever 会阻塞)
    pthread_t mqtt_tid;
    pthread_create(&mqtt_tid, NULL, mqtt_thread, NULL);
    printf("[main] MQTT 线程已启动\n");

    // HTTP 在主线程运行 (MHD 内部创建线程池)
    http_start(HTTP_PORT);

    printf("[main] 运行中, Ctrl+C 退出\n");
    printf("[main] API: http://localhost:%d/health\n", HTTP_PORT);

    // 阻塞主线程, 等待信号
    sigset_t set;
    int sig;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    sigwait(&set, &sig);

    printf("\n[main] 收到信号 %d, 退出...\n", sig);
    http_stop();
    mqtt_stop();
    printf("[main] 已退出\n");
    return 0;
}
