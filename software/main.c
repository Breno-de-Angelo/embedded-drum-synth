/** ***************************************************************************
 * @file    main.c
 * @brief   Simple LED Blink Demo for EFM32GG_STK3700
 * @version 1.0
******************************************************************************/

#include <stdint.h>
/*
 * Including this file, it is possible to define which processor using command line
 * E.g. -DEFM32GG995F1024
 * The alternative is to include the processor specific file directly
 * #include "efm32gg995f1024.h"
 */

#include "em_device.h"
#include "clock_efm32gg_ext.h"

#include "led.h"
#include "lcd.h"
#include "button.h"
#include "daconverter.h"
#include "pwm.h"

#include "player.h"

#define PWM_CHANNEL 1
#define PWM_LOC PWM_LOC4 // PWM location for TIMER0 channel 1

const int TickDivisor = 44100; // Frequency of SysTick: 44.1 kHz equal to audio sample rate
Player_t player = {
    .sample_rate = 44100,
    .tick = 0,
    .paused = 0,
    .bpm = 120,
    .beats_per_bar = 4,
    .current_quarter_beat = -1
};

/************************************************************************//**
 * @brief  Button callback
 *
 * @note   runs with disabled interrupts
 */

void buttoncallback(uint32_t v) {
    uint32_t b = Button_ReadReleased();

    if(b & BUTTON1) {
        Player_TogglePause(&player);
    }

}

/*************************************************************************//**
 * @brief  Sys Tick Handler
 */

void SysTick_Handler (void) {
    static uint32_t tick_count = 0;
    static int16_t value = 0;

    // value = Player_Tick(&player);
    // PWM_Write(TIMER0, 1, value + (0xFFFF>>1)); // Write value to PWM channel 1

    // Triangular wave at 440 Hz for testing
    tick_count++;
    value += 65; // Increment value for sine wave
    if (tick_count >= 500) { // Change frequency every 100 ticks
        tick_count = 0;
        value = 0;
    }
    PWM_Write(TIMER0, PWM_CHANNEL, value / 70); // Write value to PWM channel 1
    PWM_Start(TIMER0);
}

/*************************************************************************//**
 * @brief  Main function
 *
 * @note   Using default clock configuration
 *         HFCLK = HFRCO
 *         HFCORECLK = HFCLK
 *         HFPERCLK  = HFCLK
 */

int main(void) {
    // Set clock source to external crystal: 48 MHz
    (void) SystemCoreClockSet(CLOCK_HFXO,1,1);
    
    /* Configure LEDs */
    LED_Init(LED1|LED2);

    /* Configure buttons */
    Button_Init(BUTTON1|BUTTON2);
    Button_SetCallback(buttoncallback);

    // /* Configure LCD */
    // LCD_Init();

    // /* Configure DAC */
    // unsigned conf = DA_VREF_VDD
    //             |DA_ENABLE_CH0
    //             |DA_SINGLE_ENDED_OUTPUT;
    // DA_Init(conf, 44100); // 44.1 kHz sample rate

    // Configure PWM output 
    PWM_Init(TIMER0, PWM_LOC, PWM_PARAMS_CH1_ENABLEPIN);
    PWM_Write(TIMER0, PWM_CHANNEL, (0xFFFF>>1)); // Set initial value to max
    PWM_Start(TIMER0);


    /* Enable interrupts */
    __enable_irq();

    /* Configure SysTick */
    SysTick_Config(SystemCoreClock/TickDivisor);

    /*
     * Read button loop: ATTENTION: No debounce
     */
    while (1) {
        __WFI();        // Enter low power state
    }
    
}
