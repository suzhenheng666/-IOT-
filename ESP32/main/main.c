#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "uart.h"
#include <string.h>
#include "esp_log.h"
#include "Protocol.h"
#include "wifi_app.h"
#include "cJSON.h"
#include "esp_http_client.h"

QueueHandle_t sensor_data_queue;

static const char *TAG = "UART_INTR_DEMO";
// 1. 准备一个 512 字节的“桶”来装云端发来的 JSON 指令
#define MAX_HTTP_RECV_BUFFER 512
static char receive_buffer[MAX_HTTP_RECV_BUFFER] = {0};
static int receive_len = 0;

// 2. HTTP 事件拦截器
esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    switch(evt->event_id) {
        case HTTP_EVENT_ON_DATA: // 🟢 核心：底层收到了数据报文
            // 如果数据没有被分块传输（通常我们简单的 JSON 都不是 chunked）
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // 防止溢出保护
                if (receive_len + evt->data_len < MAX_HTTP_RECV_BUFFER) {
                    // 把收到的字节流拼接到我们的桶里
                    memcpy(receive_buffer + receive_len, evt->data, evt->data_len);
                    receive_len += evt->data_len;
                }
            }
            break;
            
        case HTTP_EVENT_ON_FINISH: // 🏁 核心：整个 HTTP 请求彻底结束
            // 在末尾补上字符串结束符 '\0'，把它变成一个标准的 C 字符串
            receive_buffer[receive_len] = '\0'; 
            break;
            
        default:
            break;
    }
    return ESP_OK;
}

static void http_upload_task(void *pvParameters)
{
    SensorPayload_t sensor_data;
    
    // 真实 API 地址
    char api_url[] = "http://10.176.101.126:8080/device/report"; // ⚠️ 请替换为你电脑的局域网 IP
    
    for(;;) {
        // 1. 死等串口任务发来的传感器数据
        if (xQueueReceive(sensor_data_queue, &sensor_data, portMAX_DELAY)) {
            
            // 2. 数据转换：把单片机的整数/小数，还原成浮点数
            double temperature = sensor_data.temp_int + (sensor_data.temp_dec / 100.0);
            double humidity = sensor_data.humi_int + (sensor_data.humi_dec / 10.0);
            
            ESP_LOGI("HTTP_TASK", "拿到队列数据，准备打包 JSON...");

            // 3. 使用 cJSON 打包成后端需要的格式
            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "device_id", "dev001");
            cJSON_AddNumberToObject(root, "longitude", 121.112345); // 假经度
            cJSON_AddNumberToObject(root, "latitude", 31.245678);   // 假纬度
            cJSON_AddNumberToObject(root, "temperature", temperature);
            cJSON_AddNumberToObject(root, "humidity", humidity);
            cJSON_AddNumberToObject(root, "distance", 1.2);         // 假距离
            
            // 压缩为字符串格式 (无换行，节省带宽)
            char *json_string = cJSON_PrintUnformatted(root);
            
            // 4. 配置 HTTP 客户端
            esp_http_client_config_t config = {
                .url = api_url,
                .method = HTTP_METHOD_POST,
                .timeout_ms = 5000,
				.event_handler = _http_event_handler,
            };
            esp_http_client_handle_t client = esp_http_client_init(&config);
            
            // 设置 HTTP 请求头 (告诉服务器我发的是 JSON)
            esp_http_client_set_header(client, "Content-Type", "application/json");
            esp_http_client_set_post_field(client, json_string, strlen(json_string));
            
            // 5. 正式发起网络请求！
            // ⚠️ 极其重要：在每次 perform 发送请求前，先把上次的“桶”清空！
			memset(receive_buffer, 0, sizeof(receive_buffer));
			receive_len = 0;

			esp_err_t err = esp_http_client_perform(client);
			if (err == ESP_OK) {
				int status_code = esp_http_client_get_status_code(client);
				ESP_LOGI("HTTP_TASK", "🌐 云端响应状态码 = %d", status_code);
				
				// 💥 见证奇迹的时刻：打印接收到的完整包！
				if (receive_len > 0) {
				ESP_LOGI("HTTP_TASK", "📦 完整指令包: %s", receive_buffer);
				
				// ==========================================
				// 🧠 接下来直接把 receive_buffer 喂给 cJSON！
				// ⚠️ 注意：这里变量名改成了 rx_json，防止和上面的 root 冲突！
				// ==========================================
				cJSON *rx_json = cJSON_Parse(receive_buffer);
				if (rx_json != NULL) {
					cJSON *cmd_item = cJSON_GetObjectItem(rx_json, "command");
					if (cmd_item && cJSON_IsString(cmd_item)) {
						ESP_LOGW("HTTP_TASK", "⚡ 准备执行硬件控制: %s", cmd_item->valuestring);
						
						uint8_t cmd_frame[4] = {0x55, 0xAA, 0x00, 0xFF}; 

						// 2. 根据云端 JSON 判断动作
						if (strcmp(cmd_item->valuestring, "turn_on_fan") == 0) {
							ESP_LOGW("HTTP_TASK", "⚡ 准备执行硬件控制: 开启风扇");
							cmd_frame[2] = 0x01; // 装载“开”指令
							
						} else if (strcmp(cmd_item->valuestring, "turn_off_fan") == 0) {
							ESP_LOGW("HTTP_TASK", "⚡ 准备执行硬件控制: 关闭风扇");
							cmd_frame[2] = 0x00; // 装载“关”指令
						}

						// 3. 通过串口把这 4 个字节顺着杜邦线轰给 STM32！
						// 🚨 务必确认你传入的串口号（如 UART_NUM_1）是你初始化好的那个！
						uart_write_bytes(UART_NUM_1, (const char*)cmd_frame, 4);
						ESP_LOGI("HTTP_TASK", "📤 控制帧已通过串口下发给 STM32!");
					}
					cJSON_Delete(rx_json); // 解析完千万别忘了释放新的 rx_json！
				}
			}
				
			} else {
				ESP_LOGE("HTTP_TASK", "❌ HTTP 请求失败: %s", esp_err_to_name(err));
			}
            
            // 6. 史诗级关键：释放内存！(否则几分钟后 219KB 内存就会被吃光死机)
            esp_http_client_cleanup(client);
            cJSON_Delete(root);
            free(json_string);
        }
    }
}

static void uart0_event_task(void *pvParameters)
{
    uart_event_t event;
    uint8_t* dtmp = (uint8_t*) malloc(RX_BUF_SIZE);
    for(;;) {
        if(xQueueReceive(uart0_queue, (void * )&event, (TickType_t)portMAX_DELAY)) {
            bzero(dtmp, RX_BUF_SIZE);
            if(event.type == UART_DATA) {
                uart_read_bytes(USART_UX, dtmp, event.size, portMAX_DELAY);
                ESP_LOGI(TAG, "[HOST -> ESP32]: %s", dtmp);
                // 这里可以写：如果电脑发送特定指令，ESP32 再通过 UART1 转发给 STM32
            }
            // 忽略了其他错误处理分支以保持代码简洁，实际项目中建议加上溢出处理
        }
    }
    free(dtmp);
    vTaskDelete(NULL);
}

uint16_t find_head(uint8_t *rx_buffer,uint16_t rx_len)
{
	while(rx_len>=sizeof(DataFrame_t))
	{
		int header_idx = -1;

		for(uint8_t i=0;i<rx_len-1;i++)
		{
			if(rx_buffer[i]==0xA5 && rx_buffer[i+1]==0x5A)
			{
				header_idx=i;
				break;
			}
		}

		if(header_idx==-1)
		{
			if(rx_buffer[rx_len-1]==0xA5)
			{
				header_idx=rx_len-1;
				rx_buffer[0] = 0xA5;
				rx_len = 1;
			}
			else
			{
				rx_len=0;
			}
			return rx_len;
		}

		if(header_idx)
		{
			rx_len-=header_idx;
			memmove(rx_buffer,&rx_buffer[header_idx],rx_len);
		}

		if(rx_len>=sizeof(DataFrame_t))
		{
			DataFrame_t *frame=(DataFrame_t *)rx_buffer;
			uint8_t *calc_start_ptr=(uint8_t*)&frame->cmd;
			uint8_t len=1+1+sizeof(frame->payload);
			uint8_t cal_sum =Calc_Checksum(calc_start_ptr,len);
			
			if (cal_sum == frame->checksum) {
				ESP_LOGI(TAG, "🟢 收到有效数据帧！温度: %d.%d ℃ | 湿度: %d.%d %% | 风扇状态: %d", 
							frame->payload.temp_int, frame->payload.temp_dec,
							frame->payload.humi_int, frame->payload.humi_dec,
							frame->payload.fan_state);
				
				SensorPayload_t payload_copy = frame->payload;
				// 将解析出的真实数据，发送到网络任务的队列中（不阻塞）
				xQueueSend(sensor_data_queue, &payload_copy, 0);

				// 剩下的数据往前挪
				rx_len -= sizeof(DataFrame_t);
				memmove(rx_buffer, rx_buffer + sizeof(DataFrame_t), rx_len);
				
			} else {
				//校验失败 
				ESP_LOGE(TAG, "🔴 校验和错误! 收到: %02X, 计算应为: %02X", frame->checksum, cal_sum);
				// 破坏包头，丢弃第一个字节 A5，让循环重新去寻找下一个合法的 A5 5A
				rx_len -= 1;
				memmove(rx_buffer, rx_buffer + 1, rx_len);
			}
		}
	}
	return rx_len;
}

static void uart1_event_task(void *pvParameters)
{
	uart_event_t event;

	static uint8_t rx_buffer[256];
	static uint16_t rx_len = 0; 
    for(;;)
	{
		if(xQueueReceive(uart1_queue,(void*)&event,(TickType_t)portMAX_DELAY))
		{
			if(event.type==UART_DATA)
			{
				ESP_LOGW(TAG, "==== 底层串口被触发！收到了 %d 个字节 ====", event.size);
				int read_len = uart_read_bytes(STM_UART_NUM,rx_buffer+rx_len,event.size,(TickType_t)portMAX_DELAY);
				rx_len+=read_len;

				rx_len=find_head(rx_buffer,rx_len);
			}
			else if(event.type == UART_FIFO_OVF || event.type == UART_BUFFER_FULL)
			{
				ESP_LOGW(TAG, "UART1 缓冲区溢出，已重置！");
                uart_flush_input(STM_UART_NUM);
                xQueueReset(uart1_queue);
                rx_len = 0; // 发生溢出
			}
		}
	}
	vTaskDelete(NULL);
}


void app_main(void)
{
	esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
	led_init();
	usart_init();
	wifi_init_sta();

	sensor_data_queue = xQueueCreate(5, sizeof(SensorPayload_t));

	xTaskCreate(uart0_event_task, "uart0_event_task", 4096, NULL, 12, NULL);
    xTaskCreate(uart1_event_task, "uart1_event_task", 4096, NULL, 12, NULL);
	xTaskCreate(http_upload_task, "http_upload_task", 4096, NULL, 5, NULL);
	

	ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "🚀 双串口 IoT 网关 + Wi-Fi 启动成功！");
    ESP_LOGI(TAG, "===========================================");
}
