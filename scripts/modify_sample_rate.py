# resample_header_v2.py
import sys
import re

def resample_data(data):
    """
    Reamostra dados de áudio de 44.1kHz para 22.05kHz (razão 2:1).
    Pega dois pares de amostras estéreo (L0, R0, L1, R1) e calcula a média
    para produzir um novo par (L_new, R_new). Isso reduz o aliasing.
    """
    resampled = []
    # Itera sobre os dados de 4 em 4 (dois pares estéreo de cada vez)
    for i in range(0, len(data) - 3, 4):
        l0, r0 = data[i], data[i+1]
        l1, r1 = data[i+2], data[i+3]

        # Calcula a média para cada canal
        new_l = (l0 + l1) // 2
        new_r = (r0 + r1) // 2
        
        resampled.extend([new_l, new_r])
        
    return resampled

def process_header_file(input_path, output_path):
    """
    Lê um arquivo .h, reamostra o array de áudio e escreve um novo arquivo .h
    usando uma máquina de estados para garantir a integridade do arquivo.
    """
    try:
        with open(input_path, 'r') as f_in:
            lines = f_in.readlines()
    except FileNotFoundError:
        print(f"Erro: Arquivo de entrada não encontrado em '{input_path}'")
        return

    # Estados: PREAMBLE (antes do array), DATA (dentro do array), POSTAMBLE (depois do array)
    state = "PREAMBLE"
    collected_data = []
    
    with open(output_path, 'w') as f_out:
        for line in lines:
            if state == "PREAMBLE":
                f_out.write(line)
                if 'const int16_t' in line and '[] = {' in line:
                    state = "DATA"
            
            elif state == "DATA":
                # Se encontrarmos o final do array
                if '};' in line:
                    # 1. Reamostra os dados coletados
                    print(f"Total de amostras originais (L/R) lidas: {len(collected_data)}")
                    resampled_values = resample_data(collected_data)
                    print(f"Total de amostras reamostradas (L/R): {len(resampled_values)}")

                    # 2. Escreve os novos dados formatados
                    values_per_line = 12 # 6 pares estéreo por linha
                    for i, value in enumerate(resampled_values):
                        if i % values_per_line == 0:
                            f_out.write('\n    ')
                        f_out.write(f"{value: >7},")
                    
                    f_out.write('\n')
                    f_out.write(line) # Escreve a linha original com "};"
                    state = "POSTAMBLE" # Muda para o estado final
                else:
                    # Se ainda estamos nos dados, apenas coleta os números
                    found_numbers = re.findall(r'-?\d+', line)
                    if found_numbers:
                        collected_data.extend(map(int, found_numbers))
            
            elif state == "POSTAMBLE":
                # Apenas copia o restante do arquivo (incluindo o #endif)
                f_out.write(line)
    
    print(f"Arquivo reamostrado com sucesso e salvo em '{output_path}'")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Uso: python resample_header_v2.py <arquivo_de_entrada.h> <arquivo_de_saida.h>")
        print("Exemplo: python resample_header_v2.py kick_44k.h resampled_kick.h")
        sys.exit(1)
        
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    
    process_header_file(input_file, output_file)
