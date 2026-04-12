#ifndef HTTP_PARSE_H
#define HTTP_PARSE_H

#include <stddef.h>

/** HTTP头部节点链表结构 */
typedef struct header_node_s {
    char* name;                     // 头部字段名
    char* value;                    // 头部字段值
    struct header_node_s* next;     // 下一个节点
} header_node_t;

/** HTTP请求结构体 */
typedef struct http_request_s {
    char method[16];                // 请求方法（GET/POST等）
    char uri[256];                  // 完整URI
    char path[256];                 // 请求路径
    char query[256];                // 查询参数字符串
    char http_version[16];          // HTTP版本
    
    header_node_t* headers;         // 头部链表

    char* body;                     // 请求体
    size_t body_length;             // 请求体长度
} http_request_t;

/** 解析原始HTTP请求数据 */
http_request_t* http_parse_request(const char* raw, size_t len);

/** 释放HTTP请求结构 */
void http_free_request(http_request_t* req);

/** 获取指定名称的HTTP头部值 */
const char* http_get_header(const http_request_t* req, const char* key);

/** 从查询字符串中获取指定参数的值 */
char* http_get_query_param(const http_request_t* req, const char* key);

#endif /* HTTP_PARSE_H */