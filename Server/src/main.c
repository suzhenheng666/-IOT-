#include<stdio.h>
#include "config.h"
#include "db_mysql.h"
#include "http_server.h"
#include "http_routes.h"
#include "signal_handler.h"

/**
 * 主函数 - 初始化并启动HTTP服务器
 */
int main(void) 
{
    server_config_t config;
    
    printf("========================================\n");
    printf("      MyTinyHttpd Server\n");
    printf("      Version: 1.0.0\n");
    printf("========================================\n");
    
    // 加载配置文件
    if (load_config( &config) != 0) {
        printf("[INFO] 使用默认配置\n");
    }
    else {
        printf("[INFO] 配置文件加载成功\n");
    }
    
    // 初始化数据库连接
    if (db_mysql_init(config.db_host, 
                                        config.db_user, 
                                        config.db_password, 
                                        config.db_name) != 0) {
        fprintf(stderr, "[ERROR] 数据库初始化失败\n");
        return 1;
    }
    
    // 注册路由
    http_routes_init();
    
    // 创建HTTP服务器实例
    http_server_t* server = http_server_new(config.server_port);
    if (!server) {
        fprintf(stderr, "[ERROR] 创建HTTP服务器失败\n");
        db_mysql_close();
        return 1;
    }

    // 注册信号处理
    signal_handler_init(server);
    
    // 启动服务器
    if (http_server_start(server) != 0) {
        fprintf(stderr, "[ERROR] 启动HTTP服务器失败\n");
        http_server_free(server);
        db_mysql_close();
        return 1;
    }
    
    // 清理资源
    http_server_free(server);
    db_mysql_close();
    
    return 0;
}