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

#include "console_ssh.h"

static const char *TAG = "MAIN";

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
  ESP_ERROR_CHECK(console_cmd_ssh_register());
  
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

}
