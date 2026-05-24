#ifndef FRONTEND_API_H
#define FRONTEND_API_H

#include <event2/bufferevent.h>
#include "http_parse.h"

/** 获取所有设备最新数据处理函数 */
void frontend_devices_latest_handler(struct bufferevent* bev, const http_request_t* req);

/** 获取设备温度历史数据处理函数 */
void frontend_device_temperature_handler(struct bufferevent* bev, const http_request_t* req);

/** 获取设备湿度历史数据处理函数 */
void frontend_device_humidity_handler(struct bufferevent* bev, const http_request_t* req);

#endif /* FRONTEND_API_H */