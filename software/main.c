/** ***************************************************************************
 * @file    main.c
 * @brief   Simple LED Blink Demo for EFM32GG_STK3700
 * @version 1.0
 ******************************************************************************/

#include <stdint.h>
/*
 * Including this file, it is possible to define which processor using command
 * line E.g. -DEFM32GG995F1024 The alternative is to include the processor
 * specific file directly #include "efm32gg995f1024.h"
 */

#include "clock_efm32gg_ext.h"
#include "em_device.h"

#include "button.h"
#include "daconverter.h"
#include "lcd.h"
#include "led.h"
#include "pwm.h"

#include "player.h"

#define PWM_CHANNEL 1
#define PWM_LOC PWM_LOC4 // PWM location for TIMER0 channel 1
#define TIMER TIMER0

const int TickDivisor = 44100; // Frequency of SysTick: 44.1 kHz equal to audio sample rate
Player_t *player;
char current_bpm[4] = "000";

void set_rythm_display(char *rythm)
{
    LCD_WriteAlphanumericDisplay(rythm);
}

void set_bpm_display(uint8_t bpm)
{
    current_bpm[0] = '0' + (bpm / 100);
    current_bpm[1] = '0' + ((bpm / 10) % 10);
    current_bpm[2] = '0' + (bpm % 10);
    current_bpm[3] = '\0'; // Null-terminate the string
    LCD_WriteNumericDisplay(current_bpm);
}

void buttoncallback(uint32_t v)
{
    uint32_t b = Button_ReadReleased();

    if (b & BUTTON1)
    {
        LED_Toggle(LED1);
        Player_TogglePause(player);
    }

    if (b & BUTTON2)
    {
        char *rythm = Player_NextRythm(player);
        set_rythm_display(rythm);
    }
}

static const uint8_t sine_table[100] = {
    63, 67, 71, 75, 79, 83, 86, 90, 94, 97,
    100, 103, 106, 109, 112, 114, 117, 119, 120, 122,
    123, 125, 125, 126, 126, 127, 126, 126, 125, 125,
    123, 122, 120, 119, 117, 114, 112, 109, 106, 103,
    100, 97, 94, 90, 86, 83, 79, 75, 71, 67,
    63, 59, 55, 51, 47, 43, 40, 36, 32, 29,
    26, 23, 20, 17, 14, 12, 9, 7, 6, 4,
    3, 1, 1, 0, 0, 0, 0, 0, 1, 1,
    3, 4, 6, 7, 9, 12, 14, 17, 20, 23,
    26, 29, 32, 36, 40, 43, 47, 51, 55, 59};

void play_tone()
{
    static int index = 0;

    PWM_Write(TIMER, PWM_CHANNEL, sine_table[index]);

    index++;
    if (index >= (sizeof(sine_table) / sizeof(sine_table[0])))
    {
        index = 0; // reinicia ciclo da senóide
    }
}

void SysTick_Handler(void)
{
    static int16_t value = 0;

    value = Player_Tick(player);

    uint32_t shifted = (uint32_t)(value + 32768);
    PWM_Write(TIMER, 1, (unsigned int) (shifted >> 9));

    // play_tone();
}

int main(void)
{
    // Set clock source to external crystal: 48 MHz
    (void)SystemCoreClockSet(CLOCK_HFXO, 1, 1);

    /* Configure LEDs */
    LED_Init(LED1);

    /* Configure buttons */
    Button_Init(BUTTON1 | BUTTON2);
    Button_SetCallback(buttoncallback);

    /* Configure LCD */
    LCD_Init();

    // /* Configure DAC */
    // unsigned conf = DA_VREF_VDD
    //             |DA_ENABLE_CH0
    //             |DA_SINGLE_ENDED_OUTPUT;
    // DA_Init(conf, 44100); // 44.1 kHz sample rate

    // Configure PWM output
    PWM_Init(TIMER, PWM_LOC, PWM_PARAMS_CH1_ENABLEPIN);

    // Initialize player
    Player_Config_t config = {
        .sample_rate = 44100,
        .bpm = 120,
        .beats_per_bar = 4};
    player = Player_GetInstance();
    Player_Init(player, config);
    set_bpm_display(config.bpm);
    set_rythm_display(Player_GetRythmName(player));

    /* Enable interrupts */
    __enable_irq();

    /* Configure SysTick */
    SysTick_Config(SystemCoreClock / TickDivisor);

    while (1)
    {
        __WFI(); // Enter low power state
    }
}
