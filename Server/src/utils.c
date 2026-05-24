#include "utils.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * 解析URL查询参数字符串
 */
void parse_query(const char* uri, query_params_t* params)
{
    params->count = 0;
    
    const char* question_mark = strchr(uri, '?');
    if (!question_mark) {
        return;
    }
    
    const char* query_start = question_mark + 1;
    
    char query_str[512];
    strncpy(query_str, query_start, sizeof(query_str) - 1);
    query_str[sizeof(query_str) - 1] = '\0';
    
    char* token = strtok(query_str, "&");
    while (token && params->count < 16) {
        char* equals = strchr(token, '=');
        if (equals) {
            *equals = '\0';
            
            strncpy(params->items[params->count].key, 
                   token, 
                   sizeof(params->items[0].key) - 1);
            params->items[params->count].key[sizeof(params->items[0].key) - 1] = '\0';
            
            strncpy(params->items[params->count].value, 
                   equals + 1, 
                   sizeof(params->items[0].value) - 1);
            params->items[params->count].value[sizeof(params->items[0].value) - 1] = '\0';
            
            *equals = '=';
            
            params->count++;
        }
        
        token = strtok(NULL, "&");
    }
}

/**
 * 从查询参数中获取指定键的值
 */
const char* query_get(query_params_t* params, const char* key)
{
    if (!params || !key) {
        return NULL;
    }
    
    for (int i = 0; i < params->count; i++) {
        if (strcmp(params->items[i].key, key) == 0) {
            return params->items[i].value;
        }
    }
    
    return NULL;
}

/**
 * 发送JSON格式的成功响应
 */
void send_json_ok(struct bufferevent* bev, const char* json_body)
{
    if (!bev || !json_body) {
        return;
    }
    
    size_t body_len = strlen(json_body);
    
    char header[256];
    snprintf(header, sizeof(header),
        	"HTTP/1.1 200 OK\r\n"
			"Content-Type: application/json\r\n"
			"Content-Length: %zu\r\n"
			"\r\n"
			,body_len);
    
    bufferevent_write(bev, header, strlen(header));
    bufferevent_write(bev, json_body, body_len);
    
    printf("[DEBUG] 发送JSON响应，长度: %zu\n", body_len);
}

/**
 * 发送JSON格式的错误响应
 */
void send_json_error(struct bufferevent* bev, int code, const char* message)
{
    if (!bev || !message) {
        return;
    }
    
    char json_body[512];
    snprintf(json_body, sizeof(json_body),
        "{\"error\":%d,\"message\":\"%s\"}",
        code, message);
    
    size_t body_len = strlen(json_body);
    
    char header[256];
    snprintf(header, sizeof(header),
        "HTTP/1.1 %d Error\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %zu\r\n"
        "\r\n",
        code, body_len);
    
    bufferevent_write(bev, header, strlen(header));
    bufferevent_write(bev, json_body, body_len);
    
    printf("[DEBUG] 发送错误响应: %d - %s\n", code, message);
}

/**
 * 发送文本类型响应
 */
void send_text_response(struct bufferevent* bev, const char* content, const char* content_type)
{
    if (!bev || !content || !content_type) {
        return;
    }
    
    size_t content_len = strlen(content);
    
    char header[256];
    snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "\r\n",
        content_type, content_len);
    
    bufferevent_write(bev, header, strlen(header));
    bufferevent_write(bev, content, content_len);
    
    printf("[DEBUG] 发送文本响应，类型: %s, 长度: %zu\n", content_type, content_len);
}

/**
 * 去除字符串首尾空白字符
 */
char* trim_string(char* str)
{
    if (!str) {
        return NULL;
    }
    
    while (*str == ' ' || *str == '\t') {
        str++;
    }
    
    if (*str == '\0') {
        return str;
    }
    
    char* end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t')) {
        *end = '\0';
        end--;
    }
    
    return str;
}

