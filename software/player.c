#include "player.h"
#include "kick.h"
#include "snare.h"

#define ORIGINAL_SAMPLE_RATE 44100
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

typedef struct {
    char *name;
    const uint8_t *rythm;
    uint32_t length;
} Rythm_t;

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

const Rythm_t rythms[] = {
    {"ROCK", rock_rythm, rock_rythm_length},
    {"FUNK", funk_rythm, funk_rythm_length},
};

static int16_t resampled_kick[sizeof(KICK)/sizeof(KICK[0]) / 2];
static uint32_t resampled_kick_len = 0;

static int16_t resampled_snare[sizeof(SNARE)/sizeof(SNARE[0]) / 2];
static uint32_t resampled_snare_len = 0;

static int16_t* kick_ptr = 0;
static uint32_t kick_len = 0;
static int16_t* snare_ptr = 0;
static uint32_t snare_len = 0;

static Player_t global_player; // memória estática
Player_t *Player_GetInstance(void) {
    return &global_player;
}

void Resample_Linear_Stereo_to_Mono(int16_t* dest, uint32_t* dest_len, const int16_t* const src, uint32_t src_len, uint32_t src_rate, uint32_t dest_rate)
{
    const uint32_t src_len_mono = src_len / 2;
    // Garante que não tentaremos interpolar além da penúltima amostra.
    if (src_len_mono < 2) {
        *dest_len = 0;
        return;
    }
    
    // Calcula o novo comprimento do som. A multiplicação usa 64 bits para evitar overflow.
    *dest_len = (uint32_t)(((uint64_t)(src_len_mono - 1) * dest_rate) / src_rate);
    
    // Usa matemática de ponto fixo Q16.16 para a posição na amostra de origem.
    const uint32_t step_fixed = ((uint64_t)src_rate << 16) / dest_rate;
    uint32_t current_pos_fixed = 0;
    
    for (uint32_t i = 0; i < *dest_len; i++) {
        const uint32_t index = current_pos_fixed >> 16;
        const uint16_t fraction = current_pos_fixed & 0xFFFF;
        
        // Converte as duas amostras estéreo adjacentes para mono (média simples)
        const int16_t y1 = (src[index * 2] + src[index * 2 + 1]) >> 1;
        const int16_t y2 = (src[(index + 1) * 2] + src[(index + 1) * 2 + 1]) >> 1;
        
        // Interpolação Linear: y = y1 + (y2 - y1) * fração
        // O cálculo intermediário usa int32_t para evitar overflow.
        const int32_t interpolated_sample = (int32_t)y1 + ((((int32_t)y2 - y1) * fraction) >> 16);
        dest[i] = (int16_t)interpolated_sample;
        
        current_pos_fixed += step_fixed;
    }
}

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

    Resample_Linear_Stereo_to_Mono(
        resampled_kick, &resampled_kick_len,
        KICK, sizeof(KICK)/sizeof(KICK[0]),
        ORIGINAL_SAMPLE_RATE, config.sample_rate
    );
    kick_ptr = resampled_kick;
    kick_len = resampled_kick_len;

    Resample_Linear_Stereo_to_Mono(
        resampled_snare, &resampled_snare_len,
        SNARE, sizeof(SNARE)/sizeof(SNARE[0]),
        ORIGINAL_SAMPLE_RATE, config.sample_rate
    );
    snare_ptr = resampled_snare;
    snare_len = resampled_snare_len;

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

    // Dispara uma nova batida se o contador expirar
    if (player->samples_until_next_beat <= 0) {
        player->samples_until_next_beat += player->samples_per_beat;
        const uint8_t beat = player->rythm[player->rythm_index];
        
        if (beat & bKICK) {
            uint8_t channel = get_free_sound_channel(player);
            if (channel < CURRENT_SOUNDS_MAX) {
                CurrentSounds_t *sound = &player->current_sounds[channel];
                sound->tick = 0;
                sound->sound = kick_ptr;
                sound->sound_length = kick_len;
            }
        }
        if (beat & bSNARE) {
            uint8_t channel = get_free_sound_channel(player);
            if (channel < CURRENT_SOUNDS_MAX) {
                CurrentSounds_t *sound = &player->current_sounds[channel];
                sound->tick = 0;
                sound->sound = snare_ptr;
                sound->sound_length = snare_len;
            }
        }
        
        player->rythm_index = (player->rythm_index + 1) % player->rythm_length;
    }

    player->samples_until_next_beat--;

    // Lógica de mixagem simplificada para os sons MONO pré-processados
    int32_t sound_to_play = 0;
    for (int i = 0; i < CURRENT_SOUNDS_MAX; i++) {
        CurrentSounds_t *sound = &player->current_sounds[i];
        if (sound->sound != 0) {
            sound_to_play += sound->sound[sound->tick];
            sound->tick++; // Incrementa de 1 em 1 para mono
            if (sound->tick >= sound->sound_length) {
                sound->sound = 0;
            }
        }
    }
    
    // Clamping para evitar estouro do áudio
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
