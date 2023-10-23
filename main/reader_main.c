/* AllFlex animal tag reader project

*/

/**

From "HDX Animal Identification protocol description" doc
LSB first !
*
00001111110000000000000000000000000000
38 bit (12 digit) National code.
eg. 000000001008 (decimal), 0b1111110000 (Binary)

6068 => 0b1011110110100 => 00101101111010000000000000000000000000
5736 => 0b1011001101000 => 00010110011010000000000000000000000000

Country code.
1110011111
10 bit (3 digit) Country code.
eg. 999 (decimal), 0b1111100111 (Binary)

France: 250
250 => 0b0011111010 => 0101111100

0111111110000101100111110110010101111000001

************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>

#include "esp_sleep.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#include "driver/ledc.h"

#include "fsk_decoder.h"

#define CLOCKGEN_MOD_OUTPUT_IO      (27) // Define the output GPIO
#define CLOCKGEN_OUTPUT_IO          (25) // Define the output GPIO
#define CLOCKGEN_FREQUENCY          (134200) // Frequency in Hertz. Set frequency at 134.2 kHz



#define STACK_SIZE 4096
TaskHandle_t ActivationTaskHandle;
TaskHandle_t FskDecoderLoopHandle;

extern volatile uint32_t Period;

esp_timer_handle_t activation_timer;
esp_timer_handle_t fskdecoding_timer;

/* Clock Generator using ledc driver

*/
static void clockgen_setup(void)
{

    /*
     * Prepare and set configuration of timer
     * that will be used by LED Controller
     * using required frequency in hertz and gpio from high speed channel group
     */
    ledc_timer_config_t ledc_timer;
    ledc_timer.speed_mode = LEDC_HIGH_SPEED_MODE;  // hi speed timer mode
    ledc_timer.duty_resolution = LEDC_TIMER_4_BIT; // resolution of PWM duty
    ledc_timer.timer_num = LEDC_TIMER_0;           // timer index
    ledc_timer.freq_hz   = CLOCKGEN_FREQUENCY;                // frequency of PWM signal
    ledc_timer.clk_cfg   = LEDC_AUTO_CLK;
    // Set configuration of timer0 for high speed channels
    ledc_timer_config(&ledc_timer);

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {0};
    ledc_channel.speed_mode     = LEDC_HIGH_SPEED_MODE;
    ledc_channel.channel        = LEDC_CHANNEL_0;
    ledc_channel.timer_sel      = LEDC_TIMER_0;
    ledc_channel.intr_type      = LEDC_INTR_DISABLE;
    ledc_channel.gpio_num       = CLOCKGEN_OUTPUT_IO;
    ledc_channel.duty           = 4, // Set duty cycle

    ledc_channel_config(&ledc_channel);


    gpio_set_direction(CLOCKGEN_MOD_OUTPUT_IO, GPIO_MODE_OUTPUT);

}

static void clockgen_on(int on)
{
    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {0};
    ledc_channel.speed_mode     = LEDC_HIGH_SPEED_MODE;
    ledc_channel.channel        = LEDC_CHANNEL_0;
    ledc_channel.timer_sel      = LEDC_TIMER_0;
    ledc_channel.intr_type      = LEDC_INTR_DISABLE;
    ledc_channel.gpio_num       = CLOCKGEN_OUTPUT_IO;
    if ( on )
        ledc_channel.duty           = 8; // Set duty cycle
    else
        ledc_channel.duty           = 0; // Set duty cycle

    ledc_channel_config(&ledc_channel);
}



void ActivationTask(void *p) {
    int64_t t0, t;

#if 1
    while(1){
        clockgen_on(1);
        esp_timer_start_once(activation_timer, 50000);

        vTaskDelay(80 / portTICK_PERIOD_MS);
        FSK_DumpBuffer();
    }

#else
    while(1) {
        clockgen_on(1);
        vTaskDelay(50 / portTICK_PERIOD_MS);
        //printf("Clock OFF(%ld)\n", portTICK_PERIOD_MS);
        clockgen_on(0);

        gpio_set_level(CLOCKGEN_MOD_OUTPUT_IO, 1);
        //gpio_set_level(FSK_PROBE_OUTPUT_IO, 1);
        FSK_DecodeEnable();

        vTaskDelay(20 / portTICK_PERIOD_MS);

        FSK_DecodeDisable();
        gpio_set_level(CLOCKGEN_MOD_OUTPUT_IO, 0);

        //gpio_set_level(FSK_PROBE_OUTPUT_IO, 0);
        //T = (Period+80) / 160;
        //if ( T != oldT ){
        //    printf("freq = %f KHz (%lu)\n", (160000./(double)Period), Period );
        //    oldT = T;
        //}
        //FSK_DumpBuffer();
    }
#endif
}

static void activation_timer_callback(void* arg)
{
    // Turn activation clock OFF
    clockgen_on(0);
    // Wait a while before starting decoder
    usleep( 2000 );
    gpio_set_level(CLOCKGEN_MOD_OUTPUT_IO, 1);
    FSK_DecodeEnable();

    esp_timer_start_once(fskdecoding_timer, 16000);
}


static void fskdecoding_timer_callback(void* arg)
{
    // Turn activation clock OFF
    FSK_DecodeDisable();
    gpio_set_level(CLOCKGEN_MOD_OUTPUT_IO, 0);

}


void TimerSetup( void ){
    esp_timer_create_args_t oneshot_timer_args = {
            .callback = &activation_timer_callback,
            /* argument specified here will be passed to timer callback function */
            .arg = NULL,
            .name = "one-shot"
    };

    esp_timer_create(&oneshot_timer_args, &activation_timer);

    oneshot_timer_args.callback = fskdecoding_timer_callback;
    esp_timer_create(&oneshot_timer_args, &fskdecoding_timer);
}


void app_main(void)
{
    // Set the LEDC peripheral configuration
    clockgen_setup();

    FSK_DecoderSetup();

    TimerSetup();

    xTaskCreatePinnedToCore(
                  ActivationTask,               // Function that implements the task.
                  "RTOS-1",                     // Text name for the task.
                  STACK_SIZE,                   // Stack size in bytes, not words.
                  ( void * )1,                  // Parameter passed into the task.
                  tskIDLE_PRIORITY+1,
                  &ActivationTaskHandle,        // Variable to hold the task's data structure.
                  0);                           // Core 0


    // Start the one and only task in core 1
    xTaskCreatePinnedToCore(
                  FskDecoderLoop,       // Function that implements the task.
                  "Core1",          // Text name for the task.
                  STACK_SIZE,      // Stack size in bytes, not words.
                  ( void * ) 1,    // Parameter passed into the task.
                  tskIDLE_PRIORITY+2,
                  &FskDecoderLoopHandle,
                  1);              // Core 1





}
