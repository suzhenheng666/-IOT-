#include "http_parse.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * 去除字符串首尾空白字符
 */
static char* trim(char* s)
{
    while (*s == ' ' || *s == '\t') {
        s++;
    }
    
    if (*s == '\0') {
        return s;
    }
    
    char* end = s + strlen(s) - 1;
    while (end > s && (*end == ' ' || *end == '\t')) {
        *end = '\0';
        end--;
    }
    
    return s;
}

/**
 * 添加HTTP头部字段到链表
 */
static void add_header(header_node_t** head, const char* name, const char* value)
{
    header_node_t* node = malloc(sizeof(header_node_t));
    node->name = strdup(name);
    node->value = strdup(value);
    node->next = *head;
    *head = node;
}

/**
 * 解析HTTP请求行（方法、URI、版本）
 */
static int parse_request_line(http_request_t* req, char* line)
{
    char* method = strtok(line, " ");
    char* uri = strtok(NULL, " ");
    char* version = strtok(NULL, "\r\n");
    
    if (!method || !uri || !version) {
        return -1;
    }
    
    strncpy(req->method, method, sizeof(req->method) - 1);
    req->method[sizeof(req->method) - 1] = '\0';
    
    strncpy(req->uri, uri, sizeof(req->uri) - 1);
    req->uri[sizeof(req->uri) - 1] = '\0';
    
    strncpy(req->http_version, version, sizeof(req->http_version) - 1);
    req->http_version[sizeof(req->http_version) - 1] = '\0';
    
    // 分离路径和查询参数
    char* qmark = strchr(uri, '?');
    if (qmark) {
        *qmark = '\0';
        strncpy(req->path, uri, sizeof(req->path) - 1);
        strncpy(req->query, qmark + 1, sizeof(req->query) - 1);
        *qmark = '?';
    } else {
        strncpy(req->path, uri, sizeof(req->path) - 1);
        req->query[0] = '\0';
    }
    
    req->path[sizeof(req->path) - 1] = '\0';
    req->query[sizeof(req->query) - 1] = '\0';
    
    return 0;
}

/**
 * 解析原始HTTP请求数据
 */
http_request_t* http_parse_request(const char* raw, size_t len)
{
    http_request_t* req = calloc(1, sizeof(http_request_t));
    if (!req) {
        return NULL;
    }
    
    char* data = malloc(len + 1);
    if (!data) {
        free(req);
        return NULL;
    }
    memcpy(data, raw, len);
    data[len] = '\0';
    
    // 查找头部结束位置
    char* header_end = strstr(data, "\r\n\r\n");
    if (!header_end) {
        printf("[ERROR] HTTP头部不完整\n");
        free(data);
        free(req);
        return NULL;
    }
    
    *header_end = '\0';
    char* body_start = header_end + 4;
    
    // 解析请求行
    char* line = data;
    char* line_end = strstr(line, "\r\n");
    if (!line_end) {
        printf("[ERROR] 找不到请求行结束\n");
        free(data);
        free(req);
        return NULL;
    }
    
    *line_end = '\0';
    if (parse_request_line(req, line) < 0) {
        printf("[ERROR] 解析请求行失败\n");
        free(data);
        free(req);
        return NULL;
    }
    *line_end = '\r';
    
    // 解析头部字段
    char* next_line = line_end + 2;
    while (next_line < header_end) {
        char* current_line_end = strstr(next_line, "\r\n");
        
        if (!current_line_end || current_line_end >= header_end) {
            current_line_end = header_end;
        }
        
        size_t line_len = current_line_end - next_line;
        if (line_len == 0) {
            break;
        }
        
        char* header_line = malloc(line_len + 1);
        if (!header_line) {
            break;
        }
        memcpy(header_line, next_line, line_len);
        header_line[line_len] = '\0';
        
        char* colon = strchr(header_line, ':');
        if (colon) {
            *colon = '\0';
            char* name = trim(header_line);
            char* value = trim(colon + 1);
            
            if (*name && *value) {
                add_header(&req->headers, name, value);
            }
            
            *colon = ':';
        }
        
        free(header_line);

        if (current_line_end >= header_end) {
            break;
        }

        next_line = current_line_end + 2;
    }
    
    // 解析请求体
    const char* content_len = http_get_header(req, "Content-Length");
    if (content_len) {
        size_t body_len = atoi(content_len);
        size_t body_available = len - (body_start - data);
        
        printf("[DEBUG] HTTP 解析：Content-Length=%s, body_len=%zu, body_available=%zu\n", 
               content_len, body_len, body_available);
        
        if (body_len <= body_available) {
            req->body = malloc(body_len + 1);
            if (req->body) {
                memcpy(req->body, body_start, body_len);
                req->body[body_len] = '\0';
                req->body_length = body_len;
                printf("[DEBUG] HTTP 解析：成功读取 %zu 字节的请求体\n", req->body_length);
            }
        } else {
            printf("[WARN] HTTP 解析：请求体长度不匹配，body_len=%zu > body_available=%zu\n", 
                   body_len, body_available);
        }
    } else {
        printf("[DEBUG] HTTP 解析：未找到 Content-Length 头部\n");
    }
    
    free(data);
    return req;
}

/**
 * 释放HTTP请求结构
 */
void http_free_request(http_request_t* req)
{
    if (!req) return;
    
    // 释放头部链表
    header_node_t* current = req->headers;
    while (current) {
        header_node_t* next = current->next;
        free(current->name);
        free(current->value);
        free(current);
        current = next;
    }
    
    if (req->body) {
        free(req->body);
    }
    
    free(req);
}

/**
 * 获取指定名称的HTTP头部值
 */
const char* http_get_header(const http_request_t* req, const char* key)
{
    if (!req || !key) return NULL;
    
    for (header_node_t* p = req->headers; p; p = p->next) {
        if (strcmp(p->name, key) == 0) {
            return p->value;
        }
    }
    
    return NULL;
}

/**
 * 从查询字符串中获取指定参数的值
 */
char* http_get_query_param(const http_request_t* req, const char* key)
{
    if (!req || !key || req->query[0] == '\0') {
        return NULL;
    }
    
    char query_copy[256];
    strncpy(query_copy, req->query, sizeof(query_copy) - 1);
    query_copy[sizeof(query_copy) - 1] = '\0';
    
    char* token = strtok(query_copy, "&");
    while (token) {
        char* equals = strchr(token, '=');
        if (equals) {
            *equals = '\0';
            
            if (strcmp(token, key) == 0) {
                char* value = strdup(equals + 1);
                *equals = '=';
                return value;
            }
            
            *equals = '=';
        }
        token = strtok(NULL, "&");
    }
    
    return NULL;
}