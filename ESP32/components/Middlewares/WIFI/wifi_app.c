#include "wifi_app.h"

static const char *TAG = "WIFI_APP";
static int s_retry_num = 0;

// 事件标志组：用于通知主任务 Wi-Fi 是连上了还是彻底失败了
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

//核心回调函数：处理 Wi-Fi 的各种突发事件
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if(event_base==WIFI_EVENT && event_id==WIFI_EVENT_STA_START)
	{
		esp_wifi_connect();
        ESP_LOGI(TAG, "Wi-Fi 启动，正在尝试连接...");
	}
	else if(event_base==WIFI_EVENT && event_id==WIFI_EVENT_STA_DISCONNECTED)
	{
		if(s_retry_num<MAXIMUM_RETRY)
		{
			esp_wifi_connect(); // 触发重连
            s_retry_num++;
            ESP_LOGW(TAG, "连接失败，正在进行第 %d 次重试...", s_retry_num);
		}
		else
		{
			xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT); // 重试耗尽，宣告失败
            ESP_LOGE(TAG, "Wi-Fi 彻底连接失败！");
		}
	}
	if(event_base==IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
	{
		ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
		ESP_LOGI(TAG, "🟢 联网成功！分配到的 IP 地址: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
	}
}

// Wi-Fi 初始化主函数
void wifi_init_sta(void)
{
    s_wifi_event_group=xEventGroupCreate();

	ESP_ERROR_CHECK(esp_netif_init());

	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_create_default_wifi_sta();

	wifi_init_config_t cfg=WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,ESP_EVENT_ANY_ID,&event_handler,NULL,&instance_any_id));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,IP_EVENT_STA_GOT_IP,&event_handler,NULL,&instance_got_ip));

	wifi_config_t wifi_config ={
		.sta={
			.ssid=WIFI_SSID,
			.password=WIFI_PASS,
			.threshold.authmode=WIFI_AUTH_WPA2_PSK,
		},
	};
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );

	ESP_ERROR_CHECK(esp_wifi_start() );
	ESP_LOGI(TAG, "Wi-Fi 初始化完成，等待连接结果...");

	EventBits_t bits=xEventGroupWaitBits(s_wifi_event_group,WIFI_CONNECTED_BIT|WIFI_FAIL_BIT,pdFALSE,pdFALSE,portMAX_DELAY);
	if(bits & WIFI_CONNECTED_BIT)
	{
		ESP_LOGI(TAG, "已连接到 SSID:%s", WIFI_SSID);
	}
	else if(bits &WIFI_FAIL_BIT)
	{
		ESP_LOGE(TAG, "无法连接到 SSID:%s", WIFI_SSID);
	}
	else
	{
		ESP_LOGE(TAG, "未知的严重错误");
	}
}