#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_check.h"
#include "soc/rtc.h"
#include "driver/mcpwm.h"

#define HC_SR04_SAMPLE_PERIOD_MS    300
_Static_assert(HC_SR04_SAMPLE_PERIOD_MS > 50, "Sample period too short!");
#define HC_SR04_PIN_ECHO    25
#define HC_SR04_PIN_TRIG    26

#define TRIGGER_THREAD_STACK_SIZE 512
#define TRIGGER_THREAD_PRIORITY 5
#define SOUND_SPEED_MPS 330
#define GPIO_OUTPUT_PIN_SEL  1ULL<<WATER_PUMP_PIN

void gen_trig_output(void *arg);
bool sr04_echo_isr_handler(mcpwm_unit_t mcpwm, mcpwm_capture_channel_id_t cap_sig, const cap_event_data_t *edata,
                                  void *arg);
