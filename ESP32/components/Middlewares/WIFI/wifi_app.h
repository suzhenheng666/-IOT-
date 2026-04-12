#ifndef __WIFI_APP_H__
#define __WIFI_APP_H__

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "wifi_app.h"

// 替换为你家里的 Wi-Fi 名称和密码
#define WIFI_SSID      "666"
#define WIFI_PASS      "202885202"
#define MAXIMUM_RETRY  5  // 最大重连次数

void wifi_init_sta(void);

#endif