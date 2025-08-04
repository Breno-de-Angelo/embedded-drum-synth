/** ***************************************************************************
 * @file    player.h
 * @brief   Player for high-level audio playback
 * @version 1.0
******************************************************************************/
#ifndef PLAYER_H
#define PLAYER_H
#include <stdint.h>

#define CURRENT_SOUNDS_MAX 10

typedef struct {
    uint32_t       tick;         // Current tick in the playback
    const int16_t  *sound;       // Pointer to the sound data
    uint32_t       sound_length; // Length of the sound data in samples
} CurrentSounds_t;

typedef struct
{
    uint32_t        sample_rate;                        // Sample rate in Hz
    uint32_t        tick;                               // Current tick in the playback
    uint8_t         paused;                             // Playback paused state
    uint8_t         bpm;                                // Beats per minute
    uint8_t         beats_per_bar;                      // Number of beats in a bar
    int8_t          current_quarter_beat;               // Current quarter beat in the bar
    CurrentSounds_t current_sounds[CURRENT_SOUNDS_MAX]; // Pointer to currently playing sounds
    uint8_t         current_sounds_mask;                // Mask for currently playing sounds
    uint8_t         *rythm;                             // Pointer to the rythm pattern
    uint32_t        rythm_length;                       // Length of the rythm pattern
} Player_t;

// Functions to control playback
int16_t Player_Tick(Player_t *player);
void Player_Stop(Player_t *player);
void Player_Pause(Player_t *player);
void Player_Resume(Player_t *player);
void Player_TogglePause(Player_t *player);

#endif // PLAYER_H