#include <stdio.h>

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h" 
#include "lwip/err.h"
#include "lwip/sys.h"

const char *SSID = "";
const char *PASSWORD = "";

int attempts = 0;

static const char* TAG = "WiFi";

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data){
    if(event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "Attempting connection to %s", SSID);
    } else if(event_id == WIFI_EVENT_STA_CONNECTED)
    {
        ESP_LOGI(TAG, "Connected to %s", SSID);
    } else if(event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGE(TAG, "Cannot connect or disconnected from %s", SSID);
        if(attempts < 5)
        {
            esp_wifi_connect(); attempts++; ESP_LOGI(TAG, "Attempting to re-connect to %s", SSID);
        }
    } else if(event_id == IP_EVENT_STA_GOT_IP){
        ESP_LOGI(TAG, "Obtained IP address from %s", SSID);
    }
}

void wifi_init(){
    // wifi init
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_initiation);

    // event handler registration
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = ""
        }
    };

    strcpy((char*)wifi_config.sta.ssid, SSID);
    strcpy((char*)wifi_config.sta.password, PASSWORD);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();
}

void app_main(void)
{
    nvs_flash_init();
    wifi_init();
}
