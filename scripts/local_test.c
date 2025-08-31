/**
 * @file    main_host.c
 * @brief   Test harness for the Player module on a host machine.
 * Generates a WAV file with the audio output.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// Inclua os headers do seu projeto que são independentes de hardware
#include "player.h"

// Função para escrever o cabeçalho de um arquivo WAV.
// Um arquivo WAV precisa dessas informações no início para ser tocável.
void write_wav_header(FILE *f, int32_t sample_rate, int32_t frame_count) {
    int32_t bits_per_sample = 16;
    int32_t channel_count = 1; // Mono
    int32_t byte_rate = sample_rate * channel_count * (bits_per_sample / 8);
    int32_t block_align = channel_count * (bits_per_sample / 8);
    int32_t subchunk2_size = frame_count * channel_count * (bits_per_sample / 8);
    int32_t chunk_size = 36 + subchunk2_size;

    // Cabeçalho RIFF
    fwrite("RIFF", 1, 4, f);
    fwrite(&chunk_size, 4, 1, f);
    fwrite("WAVE", 1, 4, f);

    // Sub-chunk 'fmt '
    fwrite("fmt ", 1, 4, f);
    int32_t subchunk1_size = 16;
    fwrite(&subchunk1_size, 4, 1, f);
    int16_t audio_format = 1; // 1 = PCM
    fwrite(&audio_format, 2, 1, f);
    fwrite(&channel_count, 2, 1, f);
    fwrite(&sample_rate, 4, 1, f);
    fwrite(&byte_rate, 4, 1, f);
    fwrite(&block_align, 2, 1, f);
    fwrite(&bits_per_sample, 2, 1, f);

    // Sub-chunk 'data'
    fwrite("data", 1, 4, f);
    fwrite(&subchunk2_size, 4, 1, f);
}

int main(void) {
    printf("Iniciando gerador de áudio para o host...\n");

    // Configurações do Player
    Player_Config_t config = {
      .sample_rate = 44100,
      .bpm = 120,
      .beats_per_bar = 4
    };
    Player_t *player = Player_GetInstance();
    Player_Init(player, config);

    // Configurações do arquivo de saída
    const int DURATION_SECONDS = 30;
    const int SAMPLE_RATE = 44100;
    const int FRAME_COUNT = DURATION_SECONDS * SAMPLE_RATE;

    FILE *output_file = fopen("output.wav", "wb");
    if (!output_file) {
        perror("Não foi possível criar o arquivo de saída");
        return 1;
    }

    // Escreve um cabeçalho WAV temporário (os tamanhos serão atualizados no final)
    write_wav_header(output_file, SAMPLE_RATE, FRAME_COUNT);

    printf("Gerando %d segundos de áudio...\n", DURATION_SECONDS);

    // Loop principal para gerar as amostras de áudio
    for (int i = 0; i < FRAME_COUNT; i++) {
        // Pega a próxima amostra do player
        int16_t sample = Player_Tick(player);
        // Escreve a amostra no arquivo
        fwrite(&sample, sizeof(int16_t), 1, output_file);
    }
    
    fclose(output_file);

    printf("Arquivo 'output.wav' gerado com sucesso!\n");
    printf("Você pode tocá-lo com um player de música como VLC ou Audacity.\n");

    return 0;
}
