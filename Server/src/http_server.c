#include "http_server.h"
#include "http_parse.h"
#include "http_routes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <event2/listener.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <arpa/inet.h>
#include<unistd.h>

typedef struct client_ctx_s {
    struct bufferevent* bev;
    char read_buf[8192];
    size_t read_len;
    char client_ip[INET_ADDRSTRLEN];
    int should_close;
} client_ctx_t;

struct http_server_s {
    struct event_base* base;
    struct evconnlistener* listener;
    int port;
    int running;
};

/**
 * 处理完整的HTTP请求 - 解析并分发到对应路由
 */
static void process_full_http_request(client_ctx_t* ctx)
{
    ctx->read_buf[ctx->read_len] = '\0';
    
    // 查找头部结束标记
    char* header_end = strstr(ctx->read_buf, "\r\n\r\n");
    if (!header_end) {
        return;
    }
    
    size_t header_len = (header_end - ctx->read_buf) + 4;
    
    // 提取Content-Length
    size_t content_length = 0;
    char* content_len_str = strstr(ctx->read_buf, "Content-Length:");
    if (content_len_str) {
        content_len_str += 15;
        while (*content_len_str == ' ' || *content_len_str == '\t') {
            content_len_str++;
        }
        content_length = atoi(content_len_str);
        printf("[DEBUG] 检测到 Content-Length: %zu\n", content_length);
    }
    
    // 检查是否接收完整请求体
    size_t total_received = ctx->read_len;
    size_t expected_total = header_len + content_length;
    
    if (total_received < expected_total) {
        printf("[DEBUG] 等待请求体：已接收 %zu 字节，需要 %zu 字节\n", 
               total_received, expected_total);
        return;
    }
    
    printf("[DEBUG] 接收到完整 HTTP 请求：%zu 字节\n", ctx->read_len);
    
    // 解析HTTP请求
    http_request_t* req = http_parse_request(ctx->read_buf, ctx->read_len);
    if (!req) {
        printf("[WARN] 解析 HTTP 请求失败 (客户端：%s)\n", ctx->client_ip);
        
        const char* bad_request = 
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Length: 11\r\n"
            "\r\n"
            "Bad Request";
        bufferevent_write(ctx->bev, bad_request, strlen(bad_request));
        
        ctx->should_close = 1;
        return;
    }
    
    printf("[INFO] %s %s 来自 %s\n", req->method, req->uri, ctx->client_ip);
    
    // 分发请求到对应路由
    http_dispatch_request(ctx->bev, req);
    
    http_free_request(req);
    
    ctx->should_close = 1;
    printf("[INFO] 请求处理完成，等待数据发送后关闭连接 (客户端：%s)\n", ctx->client_ip);
}

/**
 * 读事件回调 - 接收客户端数据
 */
static void on_read(struct bufferevent* bev, void* arg)
{
    client_ctx_t* ctx = (client_ctx_t*)arg;

    struct evbuffer* input = bufferevent_get_input(bev);
    
    size_t to_read = sizeof(ctx->read_buf) - ctx->read_len - 1;
    if (to_read == 0) {
        printf("[WARN] 缓冲区满，关闭连接 (客户端: %s)\n", ctx->client_ip);
        bufferevent_free(bev);
        free(ctx);
        return;
    }
    
    int n = evbuffer_remove(input, ctx->read_buf + ctx->read_len, to_read);
    if (n <= 0) {
        return;
    }
    
    ctx->read_len += n;
    
    // 尝试处理完整请求
    if (ctx->read_len > 0) {
        process_full_http_request(ctx);
    }
}

/**
 * 写事件回调 - 数据发送完成后关闭连接
 */
static void on_write(struct bufferevent* bev, void* arg)
{
    client_ctx_t* ctx = (client_ctx_t*)arg;
    
    if (ctx->should_close) {
        struct evbuffer* output = bufferevent_get_output(bev);
        size_t pending = evbuffer_get_length(output);
        
        if (pending > 0) {
            printf("[DEBUG] 还有 %zu 字节待发送，等待下一次回调\n", pending);
            return;
        }
        
        printf("[INFO] 数据发送完成，关闭连接 (客户端: %s)\n", ctx->client_ip);

        bufferevent_free(bev);
        free(ctx);
    }
}

/**
 * 事件回调 - 处理连接错误和断开
 */
static void on_event(struct bufferevent* bev, short events, void* arg)
{
    client_ctx_t* ctx = (client_ctx_t*)arg;
    
    if (events & BEV_EVENT_ERROR) {
        printf("[ERROR] 连接错误 (客户端: %s)\n", ctx->client_ip);
    }
    
    if (events & BEV_EVENT_EOF) {
        printf("[INFO] 客户端关闭连接 (客户端: %s)\n", ctx->client_ip);
    }
    
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        bufferevent_free(bev);
        free(ctx);
    }
}

/**
 * 接受连接回调 - 创建新的客户端上下文
 */
static void on_accept(struct evconnlistener* listener,
                      evutil_socket_t fd,
                      struct sockaddr* addr,
                      int socklen,
                      void* arg)
{
    http_server_t* server = (http_server_t*)arg;
    
    struct bufferevent* bev = bufferevent_socket_new(
        server->base,
        fd,
        BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS
    );
    
    if (!bev) {
        fprintf(stderr, "[ERROR] 创建bufferevent失败\n");
        evutil_closesocket(fd);
        return;
    }
    
    client_ctx_t* ctx = (client_ctx_t*)calloc(1, sizeof(client_ctx_t));
    if (!ctx) {
        fprintf(stderr, "[ERROR] 分配客户端上下文失败\n");
        bufferevent_free(bev);
        return;
    }
    
    ctx->bev = bev;
    ctx->read_len = 0;
    ctx->should_close = 0;
    
    // 获取客户端IP
    if (addr->sa_family == AF_INET) {
        struct sockaddr_in* sin = (struct sockaddr_in*)addr;
        inet_ntop(AF_INET, &sin->sin_addr, ctx->client_ip, sizeof(ctx->client_ip));
    } else {
        strcpy(ctx->client_ip, "Unknown");
    }
    
    printf("[INFO] 新连接来自 %s\n", ctx->client_ip);
    
    // 设置回调函数
    bufferevent_setcb(bev, on_read, on_write, on_event, ctx);

    bufferevent_enable(bev, EV_READ | EV_WRITE);
}

/**
 * 创建HTTP服务器实例
 */
http_server_t* http_server_new(int port)
{
    http_server_t* server = (http_server_t*)calloc(1, sizeof(http_server_t));
    if (!server) {
        fprintf(stderr, "[ERROR] 分配服务器结构失败\n");
        return NULL;
    }
    
    server->port = port;
    server->running = 0;
    
    server->base = event_base_new();
    if (!server->base) {
        fprintf(stderr, "[ERROR] 创建event_base失败\n");
        free(server);
        return NULL;
    }
    
    return server;
}

/**
 * 启动HTTP服务器 - 开始监听端口
 */
int http_server_start(http_server_t* server)
{
    if (!server) {
        fprintf(stderr, "[ERROR] 无效的服务器实例\n");
        return -1;
    }
    
    if (server->running) {
        fprintf(stderr, "[ERROR] 服务器已在运行\n");
        return -1;
    }
    
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(server->port);
    
    server->listener = evconnlistener_new_bind(
        server->base,
        on_accept,
        server,
        LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,
        -1,
        (struct sockaddr*)&sin,
        sizeof(sin)
    );
    
    if (!server->listener) {
        fprintf(stderr, "[ERROR] 创建监听器失败\n");
        return -1;
    }
    
    server->running = 1;
    
    printf("========================================\n");
    printf("  HTTP服务器已启动\n");
    printf("  监听端口: %d\n", server->port);
    printf("  按 Ctrl+C 停止服务器\n");
    printf("========================================\n");
    
    event_base_dispatch(server->base);
    
    return 0;
}

/**
 * 停止HTTP服务器
 */
void http_server_stop(http_server_t* server)
{
    if (!server || !server->running) {
        return;
    }
    
    printf("\n[INFO] 正在停止服务器...\n");
    
    if (server->base) {
        event_base_loopbreak(server->base);
    }
    
    server->running = 0;
}

/**
 * 释放HTTP服务器资源
 */
void http_server_free(http_server_t* server)
{
    if (!server) return;
    
    if (server->listener) {
        evconnlistener_free(server->listener);
    }
    
    if (server->base) {
        event_base_free(server->base);
    }
    
    free(server);
}