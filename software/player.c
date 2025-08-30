#include "player.h"
#include "kick.h"
#include "snare.h"

enum {
    bKICK  = 0x01,
    bSNARE = 0x02,
    bHIHAT = 0x04,
};

typedef struct {
    uint32_t       tick;         // Current tick in the playback
    const int16_t  *sound;       // Pointer to the sound data
    uint32_t       sound_length; // Length of the sound data in samples
} CurrentSounds_t;

struct Player
{
    uint32_t        sample_rate;                        // Sample rate in Hz
    uint32_t        tick;                               // Current tick in the playback
    uint8_t         paused;                             // Playback paused state
    uint8_t         bpm;                                // Beats per minute
    uint8_t         beats_per_bar;                      // Number of beats in a bar
    int8_t          current_quarter_beat;               // Current quarter beat in the bar
    CurrentSounds_t current_sounds[CURRENT_SOUNDS_MAX]; // Pointer to currently playing sounds
    uint8_t         current_sounds_mask;                // Mask for currently playing sounds
    const uint8_t   *rythm;                             // Pointer to the rythm pattern
    uint32_t        rythm_length;                       // Length of the rythm pattern
};

static Player_t global_player; // memória estática

Player_t *Player_GetInstance(void) {
    return &global_player;
}

// const uint8_t rock_rythm[] = {
//     0, 0, 0, 0,
//     bKICK|bSNARE, 0, bKICK, 0,
//     0, 0, 0, 0,
//     bKICK|bSNARE, 0, bKICK, 0 
// };
const uint8_t rock_rythm[] = {
    bKICK|bSNARE, bKICK, 0, 0,
    bKICK, 0, bKICK, 0,
    bKICK|bSNARE, bKICK, 0, 0,
    bKICK, 0, bKICK, 0 
};

const uint32_t rock_rythm_length = sizeof(rock_rythm) / sizeof(rock_rythm[0]);

const uint8_t funk_rythm[] = {
    0, bHIHAT, 0, bHIHAT,
    bKICK, bHIHAT, 0, bHIHAT,
    0, bHIHAT, bSNARE, bHIHAT,
    bKICK, bHIHAT, 0, bHIHAT 
};
const uint32_t funk_rythm_length = sizeof(funk_rythm) / sizeof(funk_rythm[0]);

typedef struct {
    char *name;
    const uint8_t *rythm;
    uint32_t length;
} Rythm_t;

const Rythm_t rythms[] = {
    {"ROCK", rock_rythm, rock_rythm_length},
    {"FUNK", funk_rythm, funk_rythm_length},
};

void Player_Init(Player_t *player, Player_Config_t config)
{
    if (!player) return;

    player->sample_rate = config.sample_rate;
    player->bpm = config.bpm;
    player->beats_per_bar = config.beats_per_bar;
    player->tick = 0;
    player->paused = 0;
    player->current_quarter_beat = -1; // Initialize to -1 to ensure the first beat is detected
    for (int i = 0; i < CURRENT_SOUNDS_MAX; i++) {
        player->current_sounds[i].tick = 0;
        player->current_sounds[i].sound = 0;
        player->current_sounds[i].sound_length = 0;
    }
    player->current_sounds_mask = 0;
    player->rythm = rythms[0].rythm;
    player->rythm_length = rythms[0].length;
}

char *Player_NextRythm(Player_t *player) {
    if (!player) return 0;
    static int current_rythm_index = 0;
    current_rythm_index = (current_rythm_index + 1) % (sizeof(rythms) / sizeof(rythms[0]));
    player->rythm = rythms[current_rythm_index].rythm;
    player->rythm_length = rythms[current_rythm_index].length;
    player->tick = 0; // Reset tick to start the new rhythm from the beginning
    player->current_quarter_beat = -1; // Reset quarter beat
    return rythms[current_rythm_index].name;
}

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
            player->tick = 0; // Reset tick to start the new bar from the beginning
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
            sound_to_play += ((sound->sound[sound->tick] + sound->sound[sound->tick+1]) / 2);
            sound->tick += 2; // Move to the next sample (stereo)
            if (sound->tick >= sound->sound_length) {
                sound->sound = 0; // Reset sound when it finishes
            }
        }
    }
    player->tick++;

    int sound_to_play_16bits;
    if (sound_to_play > INT16_MAX) {
        sound_to_play_16bits = INT16_MAX; // Clamp to max int16_t value
    } else if (sound_to_play < INT16_MIN) {
        sound_to_play_16bits = INT16_MIN; // Clamp to min int16_t value
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

char *Player_GetRythmName(Player_t *player) {
    if (!player) return 0;
    for (int i = 0; i < sizeof(rythms) / sizeof(rythms[0]); i++) {
        if (player->rythm == rythms[i].rythm) {
            return rythms[i].name;
        }
    }
    return "Unknown";
}
