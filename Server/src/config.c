#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * 加载服务器配置文件
 * 
 * @param config 服务器配置结构体指针
 * @return 0 表示成功，-1 表示失败
 */
int load_config(server_config_t* config)
{
    if (!config) {
        return -1;
    }
    
    // 初始化默认配置
    config->server_port = 8080;
    strcpy(config->db_host, "127.0.0.1");
    strcpy(config->db_user, "root");
    strcpy(config->db_password, "123456");
    strcpy(config->db_name, "iot_monitoring");

    const char* filename = "../config/server.conf";
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        return -1;
    }
    
    char line[256];
    
    while (fgets(line, sizeof(line), fp)) {
        char* newline = strchr(line, '\n');
        if (newline) *newline = '\0';
        
        // 忽略空行和注释
        if (strlen(line) == 0 || line[0] == '#') {
            continue;
        }
        
        // 解析配置项
        char* equals = strchr(line, '=');
        if (!equals) {
            continue;
        }
        
        *equals = '\0';
        char* key = line;
        char* value = equals + 1;
        
        if (strcmp(key, "server_port") == 0) {
            config->server_port = atoi(value);
            if (config->server_port < 1 || config->server_port > 65535) {
                config->server_port = 8000;
            }
        }
        else if (strcmp(key, "db_host") == 0) {
            strncpy(config->db_host, value, sizeof(config->db_host) - 1);
            config->db_host[sizeof(config->db_host) - 1] = '\0';
        }
        else if (strcmp(key, "db_user") == 0) {
            strncpy(config->db_user, value, sizeof(config->db_user) - 1);
            config->db_user[sizeof(config->db_user) - 1] = '\0';
        }
        else if (strcmp(key, "db_password") == 0) {
            strncpy(config->db_password, value, sizeof(config->db_password) - 1);
            config->db_password[sizeof(config->db_password) - 1] = '\0';
        }
        else if (strcmp(key, "db_name") == 0) {
            strncpy(config->db_name, value, sizeof(config->db_name) - 1);
            config->db_name[sizeof(config->db_name) - 1] = '\0';
        }
    }
    
    fclose(fp);
    return 0;
}