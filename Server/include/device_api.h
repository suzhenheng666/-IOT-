#ifndef DEVICE_API_H
#define DEVICE_API_H

#include <event2/bufferevent.h>
#include "http_parse.h"

/** 设备数据上报处理函数 */
void device_report_handler(struct bufferevent* bev, const http_request_t* req);

/** 设备软件更新处理函数 */
void device_update_handler(struct bufferevent* bev, const http_request_t* req);

/** 设备文件下载处理函数 */
void device_download_handler(struct bufferevent* bev, const http_request_t* req);

#endif /* DEVICE_API_H */