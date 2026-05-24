#ifndef UTILS_H
#define UTILS_H

#include <event2/bufferevent.h>
#include <stddef.h>

/** 查询参数键值对 */
typedef struct {
    char key[64];                   // 参数名
    char value[256];                // 参数值
} query_item_t;

/** 查询参数集合（最多16个参数） */
typedef struct {
    int count;                      // 参数个数
    query_item_t items[16];         // 参数数组
} query_params_t;

/** 解析URI中的查询参数 */
void parse_query(const char* uri, query_params_t* params);

/** 从查询参数中获取指定键的值 */
const char* query_get(query_params_t* params, const char* key);

/** 发送JSON格式的成功响应 */
void send_json_ok(struct bufferevent* bev, const char* json_body);

/** 发送JSON格式的错误响应 */
void send_json_error(struct bufferevent* bev, int code, const char* message);

/** 发送文本类型响应 */
void send_text_response(struct bufferevent* bev, const char* content, const char* content_type);

/** 去除字符串首尾空白字符 */
char* trim_string(char* str);

#endif /* UTILS_H */