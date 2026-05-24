#ifndef DB_MYSQL_H
#define DB_MYSQL_H

#include <mysql/mysql.h>

/** 设备数据结构体 */
typedef struct {
    char device_id[32];             // 设备ID
    double longitude;               // 经度
    double latitude;                // 纬度
    double temperature;             // 温度
    double humidity;                // 湿度
    double obstacle_distance;       // 障碍物距离
    char timestamp[20];             // 时间戳
} device_data_t;

/** 时序数据结构体（用于历史记录） */
typedef struct {
    char time[8];                   // 时间点
    char value[16];                 // 数值
} time_series_data_t;

/** 初始化MySQL数据库连接 */
int db_mysql_init(const char* host, 
    const char* user, 
    const char* password, 
    const char* database);

/** 关闭MySQL数据库连接 */
void db_mysql_close(void);

/** 插入设备数据（同时更新最新数据和历史记录） */
int db_mysql_insert_device_data(const char* device_id,
                                double longitude,
                                double latitude,
                                double temperature,
                                double humidity,
                                double distance);

/** 获取最新固件版本信息 */
int db_mysql_get_latest_firmware(char* version, int version_len,
                                 char* download_url,
                                  int url_len,
                                 long* file_size, 
                                 char* md5_hash, 
                                 int md5_len);

/** 获取所有设备的最新数据 */
device_data_t* db_mysql_get_all_devices_latest(int* count);

/** 获取设备温度历史记录（最近10分钟） */
time_series_data_t* db_mysql_get_device_temperature_history(const char* device_id, int* count);

/** 获取设备湿度历史记录（最近10分钟） */
time_series_data_t* db_mysql_get_device_humidity_history(const char* device_id, int* count);

#endif /* DB_MYSQL_H */