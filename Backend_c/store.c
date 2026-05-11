#include "store.h"
#include "cJSON.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

DeviceStore g_store;

void store_init(void) {
    memset(&g_store, 0, sizeof(g_store));
    strcpy(g_store.device_status, "unknown");
    strcpy(g_store.camera_status, "unknown");
    pthread_mutex_init(&g_store.mutex, NULL);
}

void store_lock(void)   { pthread_mutex_lock(&g_store.mutex); }
void store_unlock(void) { pthread_mutex_unlock(&g_store.mutex); }

static void get_timestamp(char *buf, int len) {
    time_t now = time(NULL);
    struct tm *t = gmtime(&now);
    snprintf(buf, len, "%04d-%02d-%02dT%02d:%02d:%02dZ",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec);
}

// MQTT 收到遥测 → 更新 latest + history
void store_update_telemetry(const char *json_str) {
    cJSON *root = cJSON_Parse(json_str);
    if (!root) return;

    store_lock();

    char ts[32];
    get_timestamp(ts, sizeof(ts));

    DeviceSnapshot *s = &g_store.latest;
    g_store.has_latest = 1;

    // 从 MQTT JSON 提取字段
    cJSON *item;
    #define GET_STR(dst, key, def) \
        item = cJSON_GetObjectItem(root, key); \
        strncpy(dst, item && item->valuestring ? item->valuestring : def, ID_LEN)

    #define GET_NUM(dst, key, def) \
        item = cJSON_GetObjectItem(root, key); \
        dst = item ? item->valuedouble : def

    GET_STR(s->device_id, "device_id", "dev001");
    GET_NUM(s->longitude, "longitude", 0);
    GET_NUM(s->latitude, "latitude", 0);
    GET_NUM(s->temperature, "temperature", 0);
    GET_NUM(s->humidity, "humidity", 0);
    GET_NUM(s->fan_speed, "fan_speed", 0);
    s->obstacle_distance = 0;
    strncpy(s->camera_status, g_store.camera_status, 16);
    strncpy(s->timestamp, ts, 32);

    #undef GET_STR
    #undef GET_NUM

    // 追加到历史
    int idx = g_store.telemetry_count % HISTORY_MAX;
    memcpy(&g_store.telemetry_history[idx], s, sizeof(DeviceSnapshot));
    if (g_store.telemetry_count < HISTORY_MAX) {
        g_store.telemetry_count++;
    }

    store_unlock();
    cJSON_Delete(root);

    printf("[store] 遥测: %.1f°C  %.1f%%  %dRPM\n",
           s->temperature, s->humidity, s->fan_speed);
}

// MQTT 收到摄像头 → 更新 camera_status + history
void store_update_camera(const char *json_str) {
    cJSON *root = cJSON_Parse(json_str);
    if (!root) return;

    store_lock();

    cJSON *status = cJSON_GetObjectItem(root, "status");
    if (status && status->valuestring) {
        strncpy(g_store.camera_status, status->valuestring, 16);
        if (g_store.has_latest) {
            strncpy(g_store.latest.camera_status, status->valuestring, 16);
        }
    }

    // 存原始 JSON
    int idx = g_store.camera_count % CAMERA_HIST_MAX;
    strncpy(g_store.camera_history[idx], json_str, 4096);
    if (g_store.camera_count < CAMERA_HIST_MAX) {
        g_store.camera_count++;
    }

    store_unlock();
    cJSON_Delete(root);

    printf("[store] camera: status=%s\n", g_store.camera_status);
}

// MQTT 收到在线状态
void store_update_status(const char *json_str) {
    cJSON *root = cJSON_Parse(json_str);
    if (!root) return;

    store_lock();

    cJSON *status = cJSON_GetObjectItem(root, "status");
    if (status && status->valuestring) {
        strncpy(g_store.device_status, status->valuestring, 16);
    }

    store_unlock();
    cJSON_Delete(root);
    printf("[store] status: %s\n", g_store.device_status);
}

// ==== 查询接口 (返回 JSON 字符串) ====

static void json_response(char *out, int max, int code, const char *msg, const char *data_json) {
    snprintf(out, max, "{\"code\":%d,\"msg\":\"%s\",\"data\":%s}", code, msg, data_json);
}

int store_get_latest(char *json_out, int max_len) {
    store_lock();

    if (!g_store.has_latest) {
        store_unlock();
        json_response(json_out, max_len, -1, "no data", "[]");
        return strlen(json_out);
    }

    DeviceSnapshot *s = &g_store.latest;
    char buf[1024];
    snprintf(buf, sizeof(buf),
        "[{"
        "\"device_id\":\"%s\",\"name\":\"%s\","
        "\"longitude\":%.4f,\"latitude\":%.4f,"
        "\"temperature\":%.1f,\"humidity\":%.1f,"
        "\"fan_speed\":%d,\"obstacle_distance\":%.2f,"
        "\"camera_status\":\"%s\","
        "\"timestamp\":\"%s\""
        "}]",
        s->device_id, s->device_id,
        s->longitude, s->latitude,
        s->temperature, s->humidity,
        s->fan_speed, s->obstacle_distance,
        s->camera_status,
        s->timestamp);

    store_unlock();
    json_response(json_out, max_len, 0, "ok", buf);
    return strlen(json_out);
}

static int store_get_history(char *json_out, int max_len, const char *device_id,
                              int is_humidity) {
    store_lock();

    // 收集匹配 device_id 的最近 20 条
    char items[4096] = "[";
    int found = 0;

    for (int i = g_store.telemetry_count - 1; i >= 0 && found < 20; i--) {
        int idx = (g_store.telemetry_count >= HISTORY_MAX)
            ? ((g_store.telemetry_count % HISTORY_MAX) + i) % HISTORY_MAX
            : i;
        if (idx < 0 || idx >= HISTORY_MAX) continue;

        DeviceSnapshot *s = &g_store.telemetry_history[idx];
        if (s->device_id[0] == '\0') continue;
        if (device_id && strcmp(s->device_id, device_id) != 0) continue;

        char entry[256];
        if (found > 0) strcat(items, ",");
        snprintf(entry, sizeof(entry),
            "{\"time\":\"%s\",\"%s\":%.1f}",
            s->timestamp,
            is_humidity ? "humidity" : "temperature",
            is_humidity ? s->humidity : s->temperature);
        strcat(items, entry);
        found++;
    }
    strcat(items, "]");

    store_unlock();
    json_response(json_out, max_len, 0, "ok", items);
    return strlen(json_out);
}

int store_get_temperature_history(const char *device_id, char *json_out, int max_len) {
    return store_get_history(json_out, max_len, device_id, 0);
}

int store_get_humidity_history(const char *device_id, char *json_out, int max_len) {
    return store_get_history(json_out, max_len, device_id, 1);
}

int store_get_camera_latest(char *json_out, int max_len) {
    store_lock();

    if (g_store.camera_count == 0) {
        store_unlock();
        json_response(json_out, max_len, -1, "no camera data", "null");
        return strlen(json_out);
    }

    int idx = (g_store.camera_count - 1) % CAMERA_HIST_MAX;
    char *raw = g_store.camera_history[idx];

    store_unlock();
    json_response(json_out, max_len, 0, "ok", raw);
    return strlen(json_out);
}
