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
#include "touch.h"

#include "player.h"

#define PWM_CHANNEL 1
#define PWM_LOC PWM_LOC4 // PWM location for TIMER0 channel 1
#define TIMER TIMER0
#define TOUCH_PERIOD 100

#define USE_DAC 0
#define USE_PWM 1

#if USE_DAC && USE_PWM
#error "Cannot use both DAC and PWM at the same time"
#endif

#if (!USE_DAC) && (!USE_PWM)
#error "Must use either DAC or PWM"
#endif

const int TickDivisor = 22050; // Frequency of SysTick. 44.1 kHz equal to audio sample rate
Player_t *player;
char current_bpm[4] = "000";


void init_hardware_output()
{
#if USE_DAC
    // Configure DAC
    unsigned conf = DAC_VREF_VDD
                   |DAC_SINGLE_ENDED_OUTPUT;
    DAC_Init(conf,500000,DAC_CHN_LOC_0,DAC_CHN_LOC_0);
#endif
#if USE_PWM
    // Configure PWM output
    PWM_Init(TIMER, PWM_LOC, PWM_PARAMS_CH1_ENABLEPIN);
#endif
}

void output_audio_sample(int16_t sample)
{
#if USE_DAC
    uint32_t shifted = (uint32_t)(sample + 32768);
    DAC_SetOutput(0, shifted >> 4); // Convert to 12-bit value
#endif
#if USE_PWM
    uint32_t shifted = (uint32_t)(sample + 32768);
    PWM_Write(TIMER, 1, (unsigned int) (shifted >> 9));
#endif
}

void output_audio_sample_uint7_t(uint8_t sample)
{
#if USE_DAC
    uint32_t shifted = (uint32_t)(sample) << 5; // Convert 7-bit to 12-bit
    DAC_SetCombOutput(0, shifted);
#endif
#if USE_PWM
    PWM_Write(TIMER, PWM_CHANNEL, sample);
#endif
}

void set_rythm_display(char *rythm)
{
    LCD_WriteAlphanumericDisplay(rythm);
}

void show_bpm_display(uint8_t bpm)
{
    current_bpm[0] = '0' + (bpm / 100);
    current_bpm[1] = '0' + ((bpm / 10) % 10);
    current_bpm[2] = '0' + (bpm % 10);
    current_bpm[3] = '\0'; // Null-terminate the string
    LCD_WriteNumericDisplay(current_bpm);
}

void set_bpm_display(Player_t *player, uint8_t bpm)
{
    Player_SetBPM(player, bpm);
    show_bpm_display(bpm);
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

    output_audio_sample_uint7_t(sine_table[index]);

    index++;
    if (index >= (sizeof(sine_table) / sizeof(sine_table[0])))
    {
        index = 0; // reinicia ciclo da senóide
    }
}

void SysTick_Handler(void)
{
    static int16_t value = 0;
    static int touchcounter = 0;

    // /* Touch processing */
    if( touchcounter != 0 ) {
        touchcounter--;
    } else {
        touchcounter = TOUCH_PERIOD;
        Touch_PeriodicProcess();
        unsigned int v = Touch_Read();
        int touch_center = Touch_GetCenterOfTouch(v);
        if (touch_center > 0) {
            set_bpm_display(player, 60 + (7 - touch_center) * 15);
        }
    }

    value = Player_Tick(player);

    output_audio_sample(value);

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

    /* Configure hardware output */
    init_hardware_output();

    // Configure touch input
    Touch_Init();

    // Initialize player
    Player_Config_t config = {
        .sample_rate = TickDivisor,
        .bpm = 90,
        .beats_per_bar = 4
    };
    player = Player_GetInstance();
    Player_Init(player, config);
    show_bpm_display(config.bpm);
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
