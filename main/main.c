/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "sdkconfig.h"
#include "aws_iot.h"
#include "time_util.h"
static const char *TAG = "example";

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define WIFI_SSID       "MyPiXtreme"
#define WIFI_PASSWORD   "Devd$2101"

#define WIFI_STA_CONNECTED_EVENT_BIT            BIT0
#define WIFI_STA_DISCONNECT_EVENT_BIT         BIT1
#define WIFI_STA_GOT_IP_BIT                   BIT2

#define BLINK_GPIO  GPIO_NUM_2
#define CONFIG_BLINK_PERIOD   1000
bool  s_led_state = 0;
uint8_t s_retry_num = 0;

uint8_t g_mqtt_connected =  false;
EventGroupHandle_t g_event_group;
esp_mqtt_client_handle_t mqtt_client;

static void blink_led(void)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(BLINK_GPIO, s_led_state);
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");

    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT )
    {
    	switch(event_id){
		case WIFI_EVENT_STA_START:
			esp_wifi_connect();
			break;
		case WIFI_EVENT_STA_CONNECTED:
			ESP_LOGI(TAG, "Connected to %s", WIFI_SSID);
			break;

		case WIFI_EVENT_STA_DISCONNECTED:
			if (s_retry_num < 5) {
				esp_wifi_connect();
				s_retry_num++;
				ESP_LOGI(TAG, "retry to connect to the AP");
			} else {

					ESP_LOGI(TAG,"connect to the AP fail");
			}
			break;
    	}
    }
	else if (event_base == IP_EVENT){
    	switch(event_id){
    		case IP_EVENT_STA_GOT_IP:
    			ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
    			ESP_LOGI(TAG, "GOT IP: " IPSTR, IP2STR(&event->ip_info.ip));
    			xEventGroupSetBits(g_event_group, WIFI_STA_GOT_IP_BIT);
    		    break;

		}
	}
}



void wifi_config()
{
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	// register event handler
	esp_event_handler_instance_t instance_any_id, instance_got_ip;

	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
														 ESP_EVENT_ANY_ID,
														 &event_handler,
														 NULL,
														 &instance_any_id));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
													 ESP_EVENT_ANY_ID,
													 &event_handler,
													 NULL,
													 &instance_got_ip));
	// tcp stack init
	ESP_ERROR_CHECK(esp_netif_init());

	 // init wifi
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	// cretae default interface for station
	esp_netif_t *netif_sta = esp_netif_create_default_wifi_sta();
	ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
	wifi_config_t wifi_config = {
		.sta = {
			.ssid = WIFI_SSID,
			.password = WIFI_PASSWORD,
			/* Setting a password implies station will connect to all security modes including WEP/WPA.
			 * However these modes are deprecated and not advisable to be used. Incase your Access point
			 * doesn't support WPA2, these mode can be enabled by commenting below line */
		 .threshold.authmode = WIFI_AUTH_WPA2_PSK,

		},
	};

	    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
	    ESP_ERROR_CHECK(esp_wifi_start() );

	    ESP_LOGI(TAG, "wifi_init_sta finished.");

}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
	esp_mqtt_event_handle_t event = event_data;
	int msg_id;
	switch ((esp_mqtt_event_id_t)event_id)
	{
		case MQTT_EVENT_CONNECTED:
			g_mqtt_connected = true;
			msg_id = esp_mqtt_client_subscribe(mqtt_client, "/topic/temp", 0);
			ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
		break;

		case MQTT_EVENT_DISCONNECTED:
			g_mqtt_connected = false;
			ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
			break;

		case MQTT_EVENT_DATA:
			ESP_LOGI(TAG, "MQTT_EVENT_DATA");
			printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
			printf("DATA=%.*s\r\n", event->data_len, event->data);
			break;
		default:
			break;
	}

}

void mqtt_app_start(void)
{
	if(!g_mqtt_connected)
	{
		const esp_mqtt_client_config_t mqtt_cfg = {
		    .broker = {
		    		.address.uri = "mqtt://broker.hivemq.com",
		    		.address.port = 1883
		    }

		};
		mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
		esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, mqtt_client);
		esp_mqtt_client_start(mqtt_client);
	}
}

void vLedTask()
{
	configure_led();
	for(;;)
	{
		ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
		blink_led();
		/* Toggle the LED state */
		s_led_state = !s_led_state;
		vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
		print_time();
	}
}

void vEventTaskFunction()
{
	g_event_group = xEventGroupCreate();
	uint32_t event_group_bits = 0;
	for(;;)
	{
		event_group_bits = xEventGroupWaitBits(g_event_group, WIFI_STA_GOT_IP_BIT, pdTRUE, pdFALSE, portMAX_DELAY );
		if(event_group_bits & WIFI_STA_GOT_IP_BIT)
		{
			xEventGroupClearBits(g_event_group, WIFI_STA_GOT_IP_BIT);
			s_retry_num = 0;
			sntp_config();
			sync_time(0);
			//mqtt_app_start();
			aws_iot_start();
		}
	}
}


void vPublishMsgTask()
{
	char jsonBuff[256];
	for(;;)
	{
		if(g_mqtt_connected)
		{
			sprintf(jsonBuff,"{\"time\": %s, \"device-d\": %d}", __TIME__, 123);
			esp_mqtt_client_publish(mqtt_client, "/topic/temp2", jsonBuff, strlen(jsonBuff), 1, 0);
		}
		vTaskDelay(2000/portTICK_PERIOD_MS);
	}
}


void app_main(void)
{
    /* initialize nvs flash */
	if(nvs_flash_init()!= ESP_OK)
	{
		ESP_LOGI(TAG,"nvs init faild");
		ESP_ERROR_CHECK(nvs_flash_init());
	}
    wifi_config();

    xTaskCreate(vEventTaskFunction, "event group task", 512*4, NULL , 5, NULL);
    xTaskCreate(vLedTask, "led blink task", 512*4, NULL , 5, NULL);
    xTaskCreate(vPublishMsgTask, "mqtt_publish", 512*4, NULL , 5, NULL);

}
