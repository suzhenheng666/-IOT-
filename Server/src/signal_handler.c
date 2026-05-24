#include <stdio.h>
#include <stdlib.h>
#include "signal_handler.h"
#include "http_server.h"
#include "db_mysql.h"

static void* g_server_instance = NULL;

/**
 * 信号处理函数 - 优雅关闭服务器
 */
static void signal_handler(int sig)
{
    printf("\n[INFO] Received signal %d, shutting down...\n", sig);
    
    if (g_server_instance) {
        http_server_stop(g_server_instance);
    }
    
    db_mysql_close();
    
    printf("[INFO] Server shutdown completed.\n");
    exit(0);
}

/**
 * 初始化信号处理器
 */
void signal_handler_init(void* server_instance)
{
    g_server_instance = server_instance;
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGQUIT, signal_handler);
    
    signal(SIGPIPE, SIG_IGN);
}
