/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
// C libraries
// #include <stdio.h>

// // FreeRTOS libraries
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/event_groups.h"

// // ESP libraries
// #include "esp_system.h"
// #include "esp_spi_flash.h"
// #include "esp_event.h"
// #include "esp_log.h"
// #include "nvs_flash.h"
// #include "esp_netif.h"

// // wifi libraries
// #include "esp_wifi.h"
// #include "esp_wpa2.h"
// #include "esp_smartconfig.h"
// #include "mqtt_client.h"

// external libraries
#include "include/smart_config.h"
#include "include/mqqt_config.h"
//#include "include/gpio_config.h"
#include "include/mcpwm_capture_hc_sr04.h"
#include "include/HX711.h"
#include "include/esp_idf_lib_helpers.h"


#define WATER_PUMP_PIN  23
#define DISTANCE_THR_CM 10
#define DEBOUNCE_THR 4 // times to measure until turning on/off water pump

#define LOW_THR_WEIGHT -139000 //threshold for minimum weight
#define HI_THR_WEIGHT -136000 //threshold for maximum weight

const static char *TAG = "hc-sr04";

// MQTT Cilent 
const esp_mqtt_client_config_t mqtt_cfg = {
    .host = "192.168.0.21",
    .port = 1883,
    .transport = MQTT_TRANSPORT_OVER_TCP,
    .username = "amy",
    .password = "pate"
    // .user_context = (void *)your_context
};


// Queues de data:
static xQueueHandle cap_queue = NULL;
static xQueueHandle mqtt_data_queue = NULL;
static xQueueHandle volume_queue = NULL;


// esto tiene que ir a HX711.c










void app_main(void)
{
    /* INITIALIZE WIFI */
    ESP_ERROR_CHECK( nvs_flash_init() );
    initialise_wifi();
    //vTaskDelay(50000 / portTICK_PERIOD_MS);
    while(get_config_status() != STATUS_OK){
        vTaskDelay(10);
    }

    /* INITIALIZE MQTT */
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    mqtt_data_queue = xQueueCreate(1, sizeof(int));
    if (mqtt_data_queue == NULL) {
        ESP_LOGE(TAG, "failed to alloc MQTT DATA QUEUE");
        return;
    }
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, (void *) mqtt_data_queue);
    esp_mqtt_client_start(client);
    esp_mqtt_client_publish(client, WATER_PUMP_STATUS_TOPIC, "INICIO:", 0, 1, false);
    esp_mqtt_client_subscribe(client, WATER_PUMP_STATUS_TOPIC, 1);
    esp_mqtt_client_publish(client, WATER_PUMP_MODE_TOPIC, "-1", 0, 1, false); // INICIO EN AUTOMATICO
    esp_mqtt_client_subscribe(client, WATER_PUMP_MODE_TOPIC, 1);


    /* INITIALIZE CAPTURE PINS */
    // queue for data
    cap_queue = xQueueCreate(1, sizeof(uint32_t));
    if (cap_queue == NULL) {
        ESP_LOGE(TAG, "failed to alloc cap_queue");
        return;
    }
    /* configure Echo pin */
    // set CAP_0 on GPIO
    ESP_ERROR_CHECK(mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM_CAP_0, HC_SR04_PIN_ECHO));
    // enable pull down CAP0, to reduce noise
    ESP_ERROR_CHECK(gpio_pulldown_en(HC_SR04_PIN_ECHO));
    // enable both edge capture on CAP0
    mcpwm_capture_config_t conf = {
        .cap_edge = MCPWM_BOTH_EDGE,
        .cap_prescale = 1,
        .capture_cb = sr04_echo_isr_handler,
        .user_data = (void *) cap_queue // estos son los argumentos, va a ser otra estructura con la queue y con edata?
    };
    ESP_ERROR_CHECK(mcpwm_capture_enable_channel(MCPWM_UNIT_0, MCPWM_SELECT_CAP0, &conf));
    ESP_LOGI(TAG, "Echo pin configured");
    /* configure Trig Pin */
    gpio_config_t trig_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pin_bit_mask = BIT64(HC_SR04_PIN_TRIG),
    };
    ESP_ERROR_CHECK(gpio_config(&trig_conf));
    ESP_ERROR_CHECK(gpio_set_level(HC_SR04_PIN_TRIG, 0)); // drive low by default
    ESP_LOGI(TAG, "Trig pin configured");

    /* configure WATER PUMP Pin */
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = GPIO_OUTPUT_PIN_SEL,
        .pull_down_en = 0,
        .pull_up_en = 0
    };
    gpio_config(&io_conf);
    gpio_set_level(WATER_PUMP_PIN, 0); // water pump turned-off initially

    // create task and queue for scale
    volume_queue = xQueueCreate(1, sizeof(int32_t));
    xTaskCreate(read_weight, "test", configMINIMAL_STACK_SIZE * 5, (void *) volume_queue, 4, NULL);


    // create task for trig signal generation
    xTaskCreate(gen_trig_output, "gen_trig_output", TRIGGER_THREAD_STACK_SIZE, NULL, TRIGGER_THREAD_PRIORITY, NULL);
    ESP_LOGI(TAG, "trig task started");

    int debounce_us_dist_on = 0;
    int debounce_us_dist_off = 0;
    int received_data = 0;
    int raw_weight;
    int HI_FLAG = 0;
    int OK_FLAG = 0;
    int LOW_FLAG = 0;
    while(1) {
        uint32_t pulse_count;
        // block and wait for new measurement
        xQueueReceive(cap_queue, &pulse_count, portMAX_DELAY);
        uint32_t pulse_width_us = pulse_count * (1000000.0 / rtc_clk_apb_freq_get());
        if (pulse_width_us > 35000) {
            // out of range
            continue;
        }
        float distance = (float) pulse_width_us / 58;
        ESP_LOGI(TAG, "Pulse width: %uus, Measured distance: %.2fcm", pulse_width_us, distance);

        // DATA to change MODE 
        xQueueReceive(mqtt_data_queue, &received_data, 10);
        printf("MQTT Data from /home/water_pump/mode: %d\n", received_data);

        xQueueReceive(volume_queue, &raw_weight, 10);
        printf("Uncalibrated weight: %d\n", raw_weight);

        if (raw_weight < LOW_THR_WEIGHT){
            HI_FLAG = 0;
            OK_FLAG = 0;
            if (LOW_FLAG == 0){
                esp_mqtt_client_publish(client, WATER_PUMP_STATUS_TOPIC, "LOW", 0, 1, false);
                LOW_FLAG = 1;
            }
            
            gpio_set_level(WATER_PUMP_PIN, 0);
        }
        else if(raw_weight < HI_THR_WEIGHT){
            HI_FLAG = 0;
            LOW_FLAG = 0;
            if (OK_FLAG == 0){
                esp_mqtt_client_publish(client, WATER_PUMP_STATUS_TOPIC, "OK", 0, 1, false);
                OK_FLAG = 1;
            }
            switch(received_data){
                case WATER_PUMP_MODE_MANUAL:
                    gpio_set_level(WATER_PUMP_PIN, 1);
                    break;
                case WATER_PUMP_MODE_BLOCKED:
                    gpio_set_level(WATER_PUMP_PIN, 0);
                    break;
                case WATER_PUMP_MODE_AUTO:
                    if (distance < DISTANCE_THR_CM){
                        debounce_us_dist_on++;
                        debounce_us_dist_off = 0;
                        if (debounce_us_dist_on >= DEBOUNCE_THR){
                            // encender bebedero
                            gpio_set_level(WATER_PUMP_PIN, 1);
                            //printf("GPIO[%d]: ")

                            if (debounce_us_dist_on == DEBOUNCE_THR){
                            //esp_mqtt_client_publish(client, WATER_PUMP_STATUS_TOPIC, "WATER PUMP ON", 0, 1, false);
                            }
                        }
                    }
                    else{
                        debounce_us_dist_off++;
                        debounce_us_dist_on = 0;
                        if (debounce_us_dist_off >= DEBOUNCE_THR){
                            // apagar bebedero
                            gpio_set_level(WATER_PUMP_PIN, 0);
                            if (debounce_us_dist_off == DEBOUNCE_THR){
                            //esp_mqtt_client_publish(client, WATER_PUMP_STATUS_TOPIC, "WATER PUMP OFF", 0, 1, false);
                            }
                        }
                    
                    }
                    break;
                default:
                    break;
            }
        }
        else{
            LOW_FLAG = 0;
            OK_FLAG = 0;
            if (HI_FLAG == 0){
                esp_mqtt_client_publish(client, WATER_PUMP_STATUS_TOPIC, "MAX", 0, 1, false);
                HI_FLAG = 1;
            }
            gpio_set_level(WATER_PUMP_PIN, 0);
        }


    }
}
