#include "player.h"
#include "kick.h"
#include "snare.h"


enum {
    bKICK  = 0x01,
    bSNARE = 0x02,
    bHIHAT = 0x04,
};

const uint8_t rock_rythm[] = {
    0, 0, 0, 0,
    bKICK|bSNARE, 0, bKICK, 0,
    0, 0, 0, 0,
    bKICK|bSNARE, 0, bKICK, 0 
};
const uint32_t rock_rythm_length = sizeof(rock_rythm) / sizeof(rock_rythm[0]);


uint8_t get_free_quarter_beat(Player_t *player) {
    for (int i = 0; i < CURRENT_SOUNDS_MAX; i++) {
        if (player->current_sounds[i].sound == 0) {
            return i;
        }
    }
    return CURRENT_SOUNDS_MAX; // No free quarter beat found
}

int16_t Player_Tick(Player_t *player)
{
    if (!player || player->paused) return 0;

    int8_t quarter_beat = (player->tick * player->bpm) / (player->sample_rate * 15);
    if (quarter_beat > player->current_quarter_beat) {
        if (quarter_beat >= player->rythm_length) {
            quarter_beat = 0; // Reset quarter beat if it exceeds rhythm length
        }
        player->current_quarter_beat = quarter_beat;
        uint8_t beat = player->rythm[quarter_beat];
        if (beat & bKICK) {
            uint8_t free_quarter_beat = get_free_quarter_beat(player);
            if (free_quarter_beat == CURRENT_SOUNDS_MAX) {
                // No free quarter beat available, cannot play sound
                return 0;
            }
            CurrentSounds_t *sound = &player->current_sounds[free_quarter_beat];
            sound->tick = 0;
            sound->sound = KICK;
            sound->sound_length = sizeof(KICK) / sizeof(KICK[0]);
        }
        if (beat & bSNARE) {
            uint8_t free_quarter_beat = get_free_quarter_beat(player);
            if (free_quarter_beat == CURRENT_SOUNDS_MAX) {
                return 0;
            }
            CurrentSounds_t *sound = &player->current_sounds[free_quarter_beat];
            sound->tick = 0;
            sound->sound = SNARE;
            sound->sound_length = sizeof(SNARE) / sizeof(SNARE[0]);
        }
        if (beat & bHIHAT) {
            // CurrentSounds_t *sound = &player->current_sounds[free_quarter_beat];
            // sound->tick = 0;
            // sound->sound = NULL; // Assuming HIHAT is not defined in this context
            // sound->sound_length = 0; // Assuming no sound data for HIHAT
        }
    }

    int32_t sound_to_play = 0;
    for (int i = 0; i < CURRENT_SOUNDS_MAX; i++) {
        CurrentSounds_t *sound = &player->current_sounds[i];
        if (sound->sound != 0) {
            sound_to_play += sound->sound[sound->tick];
            sound->tick++;
            if (sound->tick >= sound->sound_length) {
                sound->sound = 0; // Reset sound when it finishes
            }
        }
    }
    player->tick++;

    int sound_to_play_16bits;
    if (sound_to_play > INT16_MAX / 2) {
        sound_to_play_16bits = INT16_MAX / 2; // Clamp to max int16_t value
    } else if (sound_to_play < INT16_MIN / 2) {
        sound_to_play_16bits = INT16_MIN / 2; // Clamp to min int16_t value
    }
    else {
        sound_to_play_16bits = (int16_t)sound_to_play; // Cast to int16_t
    }
    return sound_to_play_16bits;
}

void Player_Stop(Player_t *player)
{
    if (!player) return;
    player->tick = 0;
    player->paused = 1;
}

void Player_Pause(Player_t *player)
{
    if (!player) return;
    player->paused = 1;
}

void Player_Resume(Player_t *player)
{
    if (!player) return;
    player->paused = 0;
}

void Player_TogglePause(Player_t *player)
{
    if (!player) return;
    player->paused = !player->paused;
}



