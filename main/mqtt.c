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

#include "mqtt_client.h"
//#include "protocol_examples_common.h"
#include "freertos/task.h"

const char *SSID = "";
const char *PASSWORD = "";

int attempts = 0;

static const char* TAG = "wifi";
static const char* TAG2 = "mqtt";

const char* rpi_broadcast_topic = "/rpi/broadcast";
const char* esp32_broadcast_topic = "/esp32/broadcast";

static void mqtt_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    int msg_id;

    if (event_id == MQTT_EVENT_CONNECTED){
        ESP_LOGI(TAG2, "connected to broker");
        msg_id = esp_mqtt_client_publish(client, esp32_broadcast_topic, "esp32_broadcast000", 0, 1, 0);
        ESP_LOGI(TAG2, "sent publish successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, rpi_broadcast_topic, 0);
        ESP_LOGI(TAG2, "sent subscribe successful, msg_id=%d", msg_id);   
    } else if (event_id == MQTT_EVENT_DISCONNECTED){
        ESP_LOGE(TAG2, "disconnected or cannot connect to broker");
    } else if (event_id == MQTT_EVENT_SUBSCRIBED){
        ESP_LOGI(TAG2, "subscribed to topic, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, rpi_broadcast_topic, "esp32_broadcast000_to_rpi_topic", 0, 0 ,0);
        ESP_LOGI(TAG2, "sent publish successful, msg_id%d", msg_id);
    } else if (event_id == MQTT_EVENT_UNSUBSCRIBED){
        ESP_LOGI(TAG2, "unbsubscribed, msg_id=%d", event->msg_id);
    } else if (event_id == MQTT_EVENT_PUBLISHED){
        ESP_LOGI(TAG2, "published to topic, msg_id=%d", event->msg_id);
    } else if (event_id == MQTT_EVENT_DATA){
        ESP_LOGI(TAG2, "data recieved");
        printf("topic=%.*s\r\n", event->topic_len, event->topic);
        printf("data=%.*s\r\n", event->data_len, event->data);
        // reply over /esp32/broadcast topic if data is equal to any number divisible by 5
        if (event->data_len > 0)
        {
            char data_str[32];

            strncpy(data_str, event->data, event->data_len);
            data_str[event->data_len] = '\0';

            if (strcmp(data_str, "15") == 0 || 
                strcmp(data_str, "10") == 0 || 
                strcmp(data_str, "20") == 0 || 
                strcmp(data_str, "5") == 0) {
                msg_id = esp_mqtt_client_publish(client, esp32_broadcast_topic, "reply: esp32_broadcast000_to_rpi_topic", 0, 0, 0);
                ESP_LOGI(TAG2, "sent publish successful, msg_id%d", msg_id);
            }
        }
    } else if (event_id == MQTT_EVENT_ERROR){
        ESP_LOGE(TAG2, "error");
    } else {
        ESP_LOGI(TAG2, "other event id:%d", event->event_id);
    }
}

static void mqtt_init()
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri= "",

    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if(event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "attempting connection to %s", SSID);
    } else if(event_id == WIFI_EVENT_STA_CONNECTED)
    {
        ESP_LOGI(TAG, "connected to %s", SSID);
    } else if(event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGE(TAG, "cannot connect or disconnected from %s", SSID);
        if(attempts < 5)
        {
            esp_wifi_connect(); attempts++; ESP_LOGI(TAG, "Attempting to re-connect to %s", SSID);
        }
    } else if(event_id == IP_EVENT_STA_GOT_IP){
        ESP_LOGI(TAG, "obtained IP address from %s", SSID);
    }
}

void wifi_init()
{
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
    vTaskDelay(10000 /portTICK_PERIOD_MS);
    mqtt_init();
}
