#ifndef HTTP_ROUTES_H
#define HTTP_ROUTES_H

#include <event2/bufferevent.h>
#include "http_parse.h"

/** 路由处理函数类型定义 */
typedef void (*route_handler_t)(struct bufferevent* bev, const http_request_t* req);

/** 初始化路由系统，注册所有API路由 */
void http_routes_init(void);

/** 分发HTTP请求到对应的处理函数 */
void http_dispatch_request(struct bufferevent* bev, const http_request_t* req);

#endif /* HTTP_ROUTES_H */