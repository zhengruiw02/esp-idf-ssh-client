/* SSH Client Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "esp_err.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sys.h"

// for console_wifi & console_ping
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_event.h"
// for console_wifi
#include "console_wifi.h"
// for console_ping
#include "console_ping.h"


static const char *TAG = "MAIN";


EventGroupHandle_t xEventGroup;
int TASK_FINISH_BIT	= BIT4;


static void SPIFFS_Directory(char * path) {
	DIR* dir = opendir(path);
	assert(dir != NULL);
	while (true) {
		struct dirent*pe = readdir(dir);
		if (!pe) break;
		ESP_LOGI(__FUNCTION__,"d_name=%s d_ino=%d d_type=%x", pe->d_name,pe->d_ino, pe->d_type);
	}
	closedir(dir);
}

static void netif_nvs_flash_init(void)
{
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_err_t ret = nvs_flash_init();   //Initialize NVS
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
}

static void console_init(void)
{
  ESP_ERROR_CHECK(console_cmd_init());     // Initialize console
  // Register all plugin command added to your project
  ESP_ERROR_CHECK(console_cmd_all_register());
  // To register only wifi command skip calling console_cmd_all_register()
  ESP_ERROR_CHECK(console_cmd_wifi_register());  
  // To register only ifconfig command skip calling console_cmd_all_register()
  ESP_ERROR_CHECK(console_cmd_ping_register());
  ESP_ERROR_CHECK(console_cmd_start());    // Start console
}


void ssh_task(void *pvParameters);

void app_main(void)
{
	esp_err_t ret;
	//Initialize NVS	
	netif_nvs_flash_init();

	//Initialize Wi-Fi & ping console
	console_init();

	ESP_LOGI(TAG, "Initializing SPIFFS");
	esp_vfs_spiffs_conf_t conf = {
		.base_path = "/spiffs",
		.partition_label = NULL,
		.max_files = 8,
		.format_if_mount_failed = true
	};

	// Use settings defined above toinitialize and mount SPIFFS filesystem.
	// Note: esp_vfs_spiffs_register is anall-in-one convenience function.
	ret = esp_vfs_spiffs_register(&conf);

	if (ret != ESP_OK) {
		if (ret == ESP_FAIL) {
			ESP_LOGE(TAG, "Failed to mount or format filesystem");
		} else if (ret == ESP_ERR_NOT_FOUND) {
			ESP_LOGE(TAG, "Failed to find SPIFFS partition");
		} else {
			ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)",esp_err_to_name(ret));
		}
		return;
	}
	SPIFFS_Directory("/spiffs/");

	// Create Eventgroup
	xEventGroup = xEventGroupCreate();
	configASSERT( xEventGroup );

	ESP_LOGI(TAG, "Opening file");
	FILE* fp = fopen("/spiffs/command.txt", "r");
	if (fp == NULL) {
		ESP_LOGE(TAG, "Failed to open file for reading");
		return;
	}

	char line[256];
	while(fgets(line, sizeof(line), fp) != NULL) {
		int lineLen = strlen(line);
		line[lineLen-1] = 0;
		ESP_LOGI(TAG, "line=[%s] lineLen=%d", line, lineLen);
		if (lineLen == 1) continue;
		if (line[0] == '#') continue;

		// Execute ssh command
		xEventGroupClearBits( xEventGroup, TASK_FINISH_BIT );
		xTaskCreate(&ssh_task, "SSH", 1024*8, (void *) &line, 2, NULL);

		// Wit for ssh finish.
		xEventGroupWaitBits( xEventGroup,
			TASK_FINISH_BIT,	/* The bits within the event group to wait for. */
			pdTRUE,				/* HTTP_CLOSE_BIT should be cleared before returning. */
			pdFALSE,			/* Don't wait for both bits, either bit will do. */
			portMAX_DELAY);		/* Wait forever. */
	}
	fclose(fp);

	ESP_LOGI(TAG, "SSH all finish");

}
