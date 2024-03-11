#include "esp_sntp.h"
#include "esp_log.h"
#include "time_util.h"

static const char *TAG = "TIME";

void sntp_config(void)
{
	static uint8_t is_sntp_init = pdFALSE;
	if(!is_sntp_init)
	{
		/*
		sntp_setoperatingmode(SNTP_OPMODE_POLL);
		sntp_setservername(0, "pool.ntp.org");
		sntp_set_sync_interval(10);
		*/
		esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
		esp_sntp_setservername(0, "pool.ntp.org");
		esp_sntp_init();
		is_sntp_init = pdTRUE;
	}
	setenv("TZ", "IST-5:30", 1);
	tzset();
	sync_time(0);
}

void sync_time(void *param)
{

	int retry = 0;
	const int retry_count = 15;
	time_t now = 0;
	struct tm time_info = {0};
	time(&now);
	localtime_r(&now,&time_info);
	while((esp_sntp_get_sync_status()==SNTP_SYNC_STATUS_RESET && ++retry < retry_count) && time_info.tm_year < 2000 )
	{
		time(&now);
		localtime_r(&now,&time_info);
		ESP_LOGI(TAG,"sntp sync... %d",retry);
		vTaskDelay(100);
	}
}

void print_time(void)
{
	time_t now = 0;
	struct tm time_info = {0};
	time(&now);
	localtime_r(&now,&time_info);
	char strftime_buf[64];
	strftime(strftime_buf, sizeof(strftime_buf), "%c", &time_info);
	ESP_LOGI(TAG, "%s\n",strftime_buf);

}

