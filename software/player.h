/** ***************************************************************************
 * @file    player.h
 * @brief   Player for high-level audio playback
 * @version 1.0
******************************************************************************/
#ifndef PLAYER_H
#define PLAYER_H
#include <stdint.h>

#define CURRENT_SOUNDS_MAX 20

extern const uint8_t rock_rythm[];
extern const uint32_t rock_rythm_length;

typedef struct Player Player_t;

typedef struct {
    uint32_t sample_rate;      // Sample rate in Hz
    uint8_t  bpm;              // Beats per minute
    uint8_t  beats_per_bar;    // Number of beats in a bar
} Player_Config_t;

// Functions to control playback
Player_t *Player_GetInstance(void);
void Player_Init(Player_t *player, Player_Config_t config);
int16_t Player_Tick(Player_t *player);
void Player_Stop(Player_t *player);
void Player_Pause(Player_t *player);
void Player_Resume(Player_t *player);
void Player_TogglePause(Player_t *player);
char *Player_NextRythm(Player_t *player);
char *Player_GetRythmName(Player_t *player);

#endif // PLAYER_H