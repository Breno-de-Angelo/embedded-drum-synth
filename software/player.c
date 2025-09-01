#include "player.h"
#include "resampled_kick.h"
#include "resampled_snare.h"

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
    uint8_t         paused;                             // Playback paused state
    uint8_t         bpm;                                // Beats per minute
    uint8_t         beats_per_bar;                      // Number of beats in a bar
    CurrentSounds_t current_sounds[CURRENT_SOUNDS_MAX]; // Pointer to currently playing sounds
    const uint8_t   *rythm;                             // Pointer to the rythm pattern
    uint32_t        rythm_length;                       // Length of the rythm pattern
    uint8_t         rythm_index;                        // Em qual passo do ritmo estamos (substitui current_quarter_beat)
    uint32_t        samples_per_beat;                   // Valor pré-calculado de amostras por passo do ritmo
    uint32_t        samples_until_next_beat;            // Contador regressivo até a próxima batida};
};

static Player_t global_player; // memória estática

Player_t *Player_GetInstance(void) {
    return &global_player;
}

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
    player->paused = 0;
    
    for (int i = 0; i < CURRENT_SOUNDS_MAX; i++) {
        player->current_sounds[i].tick = 0;
        player->current_sounds[i].sound = 0;
        player->current_sounds[i].sound_length = 0;
    }

    player->rythm = rythms[0].rythm;
    player->rythm_length = rythms[0].length;
    player->rythm_index = 0;

    // OTMIZAÇÃO: Pré-calcula o número de amostras por batida do ritmo
    player->samples_per_beat = (player->sample_rate * 15) / player->bpm;
    
    // Inicia o contador para disparar a primeira batida imediatamente.
    player->samples_until_next_beat = 0;
}

char *Player_NextRythm(Player_t *player) {
    if (!player) return 0;
    static int current_rythm_index = 0;
    current_rythm_index = (current_rythm_index + 1) % (sizeof(rythms) / sizeof(rythms[0]));
    player->rythm = rythms[current_rythm_index].rythm;
    player->rythm_length = rythms[current_rythm_index].length;
    
    // OTMIZAÇÃO: Reseta o estado do ritmo
    player->rythm_index = 0; 
    player->samples_until_next_beat = 0;
    
    return rythms[current_rythm_index].name;
}

uint8_t get_free_sound_channel(Player_t *player) {
    for (int i = 0; i < CURRENT_SOUNDS_MAX; i++) {
        if (player->current_sounds[i].sound == 0) {
            return i;
        }
    }
    return CURRENT_SOUNDS_MAX;
}

int16_t Player_Tick(Player_t *player)
{
    if (!player || player->paused) return 0;

    if (player->samples_until_next_beat <= 0) {
        // Reinicia o contador para a próxima batida. Usar '+=' previne drift.
        player->samples_until_next_beat += player->samples_per_beat;

        // Pega a batida atual do padrão de ritmo
        uint8_t beat = player->rythm[player->rythm_index];
        
        if (beat & bKICK) {
            uint8_t channel = get_free_sound_channel(player);
            if (channel < CURRENT_SOUNDS_MAX) {
                CurrentSounds_t *sound = &player->current_sounds[channel];
                sound->tick = 0;
                sound->sound = KICK;
                sound->sound_length = sizeof(KICK) / sizeof(KICK[0]);
            }
        }
        if (beat & bSNARE) {
            uint8_t channel = get_free_sound_channel(player);
            if (channel < CURRENT_SOUNDS_MAX) {
                CurrentSounds_t *sound = &player->current_sounds[channel];
                sound->tick = 0;
                sound->sound = SNARE;
                sound->sound_length = sizeof(SNARE) / sizeof(SNARE[0]);
            }
        }
        if (beat & bHIHAT) {
            uint8_t channel = get_free_sound_channel(player);
            if (channel < CURRENT_SOUNDS_MAX) {
                CurrentSounds_t *sound = &player->current_sounds[channel];
                sound->tick = 0;
                sound->sound = SNARE; // Usando SNARE como placeholder para HIHAT
                sound->sound_length = sizeof(SNARE) / sizeof(SNARE[0]);
            }
        }

        // Avança para o próximo passo do ritmo
        player->rythm_index++;
        if (player->rythm_index >= player->rythm_length) {
            player->rythm_index = 0; // Volta para o começo
        }
    }

    player->samples_until_next_beat--;

    int32_t sound_to_play = 0;
    for (int i = 0; i < CURRENT_SOUNDS_MAX; i++) {
        CurrentSounds_t *sound = &player->current_sounds[i];
        if (sound->sound != 0) {
            sound_to_play += ((sound->sound[sound->tick] + sound->sound[sound->tick+1]) >> 1);
            
            sound->tick += 2;
            if (sound->tick >= sound->sound_length) {
                sound->sound = 0;
            }
        }
    }
    
    if (sound_to_play > INT16_MAX) {
        return INT16_MAX;
    } else if (sound_to_play < INT16_MIN) {
        return INT16_MIN;
    }
    
    return (int16_t)sound_to_play;
}

void Player_Stop(Player_t *player)
{
    if (!player) return;
    player->rythm_index = 0;
    player->samples_until_next_beat = 0;
    for (int i = 0; i < CURRENT_SOUNDS_MAX; i++) {
        player->current_sounds[i].sound = 0;
    }
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

void Player_SetBPM(Player_t *player, uint8_t bpm) {
    if (!player || player->bpm == bpm) return;

    // OTMIZAÇÃO: Recalcula o valor pré-calculado apenas quando o BPM muda.
    uint8_t old_bpm = player->bpm;
    player->bpm = bpm;
    player->samples_per_beat = (player->sample_rate * 15) / player->bpm;
    
    // OTMIZAÇÃO: Ajusta o contador atual para manter a fase do ritmo
    // Esta divisão só acontece raramente (quando o usuário muda o BPM).
    player->samples_until_next_beat = (player->samples_until_next_beat * old_bpm) / bpm;
}
