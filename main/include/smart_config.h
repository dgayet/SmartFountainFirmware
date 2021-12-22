#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_smartconfig.h"

typedef enum{
    STATUS_OK = 0,
    STATUS_NOT_OK = -1
}smart_config_status_t;

void smartconfig_example_task(void * parm);
void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data);
void initialise_wifi(void);

smart_config_status_t get_config_status(void);

