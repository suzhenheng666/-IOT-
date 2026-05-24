#ifndef CONFIG_H
#define CONFIG_H

/** 服务器配置结构体 */
typedef struct {
    int server_port;          // 服务器端口
    char db_host[64];         // 数据库主机地址
    char db_user[32];         // 数据库用户名
    char db_password[32];     // 数据库密码
    char db_name[32];         // 数据库名称
} server_config_t;

/** 加载配置文件 */
int load_config(server_config_t* config);

#endif