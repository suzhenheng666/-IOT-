#include "http_server.h"
#include "store.h"
#include <microhttpd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static struct MHD_Daemon *daemon = NULL;

// 简易 URL 参数解析: 从 query 中提取 key=value
static int get_query_param(const char *query, const char *key, char *value, int max_len) {
    if (!query) return 0;
    char search[64];
    snprintf(search, sizeof(search), "%s=", key);
    const char *p = strstr(query, search);
    if (!p) return 0;
    p += strlen(search);
    int i = 0;
    while (*p && *p != '&' && i < max_len - 1) {
        value[i++] = *p++;
    }
    value[i] = '\0';
    return i > 0;
}

static enum MHD_Result handle_request(void *cls,
    struct MHD_Connection *conn, const char *url,
    const char *method, const char *version,
    const char *upload_data, size_t *upload_size, void **ptr) {

    static int dummy;
    if (&dummy != *ptr) {
        *ptr = &dummy;
        return MHD_YES;
    }
    *ptr = NULL;

    if (strcmp(method, "GET") != 0) {
        const char *err = "{\"code\":-1,\"msg\":\"only GET\"}";
        struct MHD_Response *resp = MHD_create_response_from_buffer(
            strlen(err), (void *)err, MHD_RESPMEM_PERSISTENT);
        int ret = MHD_queue_response(conn, 405, resp);
        MHD_destroy_response(resp);
        return ret;
    }

    char json_buf[8192];
    int status_code = 200;

    // 解析 query string
    const char *query = strchr(url, '?');
    char path[256] = {0};
    if (query) {
        int plen = query - url;
        if (plen > 255) plen = 255;
        strncpy(path, url, plen);
        path[plen] = '\0';
        query++; // 跳过 '?'
    } else {
        strncpy(path, url, 255);
    }

    // === 路由 ===
    if (strcmp(path, "/health") == 0) {
        snprintf(json_buf, sizeof(json_buf), "{\"status\":\"ok\"}");
    }
    else if (strcmp(path, "/api/v1/devices/latest") == 0 ||
             strcmp(path, "/frontend/devices/latest") == 0) {
        store_get_latest(json_buf, sizeof(json_buf));
    }
    else if (strcmp(path, "/api/v1/device/temperature") == 0) {
        char device_id[64] = {0};
        get_query_param(query, "device_id", device_id, sizeof(device_id));
        store_get_temperature_history(device_id[0] ? device_id : "dev001",
                                      json_buf, sizeof(json_buf));
    }
    else if (strcmp(path, "/api/v1/device/humidity") == 0) {
        char device_id[64] = {0};
        get_query_param(query, "device_id", device_id, sizeof(device_id));
        store_get_humidity_history(device_id[0] ? device_id : "dev001",
                                   json_buf, sizeof(json_buf));
    }
    else if (strcmp(path, "/api/v1/device/camera/latest") == 0) {
        store_get_camera_latest(json_buf, sizeof(json_buf));
    }
    else {
        snprintf(json_buf, sizeof(json_buf), "{\"code\":-1,\"msg\":\"not found\"}");
        status_code = 404;
    }

    struct MHD_Response *resp = MHD_create_response_from_buffer(
        strlen(json_buf), json_buf, MHD_RESPMEM_MUST_COPY);
    MHD_add_response_header(resp, "Content-Type", "application/json; charset=utf-8");
    MHD_add_response_header(resp, "Access-Control-Allow-Origin", "*");
    int ret = MHD_queue_response(conn, status_code, resp);
    MHD_destroy_response(resp);
    return ret;
}

void http_start(unsigned short port) {
    daemon = MHD_start_daemon(
        MHD_USE_AUTO | MHD_USE_INTERNAL_POLLING_THREAD,
        port,
        NULL, NULL,
        &handle_request, NULL,
        MHD_OPTION_END);
    if (!daemon) {
        fprintf(stderr, "[http] 启动失败 (端口 %d)\n", port);
        return;
    }
    printf("[http] 启动: http://0.0.0.0:%d\n", port);
}

void http_stop(void) {
    if (daemon) {
        MHD_stop_daemon(daemon);
        daemon = NULL;
    }
}
