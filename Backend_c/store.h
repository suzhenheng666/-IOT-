#ifndef STORE_H__
#define STORE_H__

#include <stdint.h>
#include <pthread.h>

#define HISTORY_MAX     1000
#define CAMERA_HIST_MAX 500
#define ID_LEN          64

// 单条遥测记录
typedef struct {
    char    device_id[ID_LEN];
    double  longitude;
    double  latitude;
    double  temperature;
    double  humidity;
    uint16_t fan_speed;
    double  obstacle_distance;
    char    camera_status[16];
    char    timestamp[32];
} DeviceSnapshot;

// 线程安全的内存存储
typedef struct {
    DeviceSnapshot latest;
    int            has_latest;

    DeviceSnapshot telemetry_history[HISTORY_MAX];
    int            telemetry_count;

    char           camera_status[16];
    char           camera_history[CAMERA_HIST_MAX][4096];
    int            camera_count;

    char           device_status[16];

    pthread_mutex_t mutex;
} DeviceStore;

extern DeviceStore g_store;

void  store_init(void);
void  store_lock(void);
void  store_unlock(void);

// 更新接口 (MQTT 回调中调用)
void  store_update_telemetry(const char *json_str);
void  store_update_camera(const char *json_str);
void  store_update_status(const char *json_str);

// 查询接口 (HTTP handler 中调用)
int   store_get_latest(char *json_out, int max_len);
int   store_get_temperature_history(const char *device_id, char *json_out, int max_len);
int   store_get_humidity_history(const char *device_id, char *json_out, int max_len);
int   store_get_camera_latest(char *json_out, int max_len);

#endif
