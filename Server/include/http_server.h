#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

/** HTTP服务器结构体（不透明类型） */
typedef struct http_server_s http_server_t;

/** 创建HTTP服务器实例 */
http_server_t* http_server_new(int port);

/** 启动HTTP服务器，开始监听端口 */
int http_server_start(http_server_t* server);

/** 停止HTTP服务器 */
void http_server_stop(http_server_t* server);

/** 释放HTTP服务器资源 */
void http_server_free(http_server_t* server);

#endif /* HTTP_SERVER_H */