// C libraries
#include <stdio.h>
#include <stdlib.h>

// FreeRTOS libraries
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// ESP libraries
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"

// wifi libraries
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "mqtt_client.h"

// external
//#include "include/mqtt_macros.h"


// const esp_mqtt_client_config_t mqtt_cfg = {
//     .host = "192.168.0.16",
//     .port = 1883,
//     .transport = MQTT_TRANSPORT_OVER_TCP,
//     .username = "amy",
//     .password = "pate"
//     // .user_context = (void *)your_context
// };
#define WATER_PUMP_STATUS_TOPIC "/home/water_pump/status"
#define WATER_PUMP_MODE_TOPIC "/home/water_pump/mode"
#define WATER_PUMP_MODE_MANUAL 1
#define WATER_PUMP_MODE_AUTO -1
#define WATER_PUMP_MODE_BLOCKED 2


void log_error_if_nonzero(const char *message, int error_code);
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
int data_str_to_num(char * data);