# Embedded Drum Synth (Bateria Eletrônica Embarcada)

**Disciplina:** Sistemas Embarcados 2 – 2025/1  

**Autor:** Breno Uliana de Angelo

**Kit de Desenvolvimento:** EFM32 Giant Gecko STK3700

---

## 1. Sobre o Projeto

Este projeto consiste no desenvolvimento de uma bateria eletrônica utilizando o kit de desenvolvimento EFM32STK3700. O objetivo é criar um sistema embarcado capaz de reproduzir diversos ritmos musicais, combinando três sons de percussão diferentes em velocidades variadas.

Os sons utilizados são gerados a partir de arquivos de áudio no formato `.wav` e convertidos para arrays em C, que são então compilados e armazenados na memória do microcontrolador.

## 2. Funcionalidades

* **Reprodução de Sons:** O sistema utiliza amostras de áudio (`.wav`) para gerar os sons de percussão.
* **Mixagem de Ritmos:** Capacidade de misturar até três sons diferentes para criar ritmos complexos.
* **Controle de Velocidade:** Permite a alteração da velocidade (BPM - Batidas Por Minuto) dos ritmos.
* **Saída de Áudio:** O áudio é gerado através de uma das seguintes abordagens de hardware:
    1.  **DAC (Conversor Digital-Analógico):** Utiliza o DAC integrado ao EFM32 para gerar um sinal de áudio analógico.
    2.  **PWM (Modulação por Largura de Pulso):** Usa um temporizador para gerar um sinal PWM, que é então filtrado (filtro passa-baixas) para se obter o sinal analógico.
    3.  **I²S (Inter-IC Sound):** Emprega o barramento I²S para enviar o áudio digital para um amplificador ou codec externo compatível.

> **⚠️ Atenção com o Hardware:** A placa de desenvolvimento EFM32STK3700 **não tolera tensões de entrada superiores a 3.3V**. Todo hardware externo conectado deve respeitar este limite para evitar danos permanentes ao microcontrolador. Certifique-se de que as conexões de terra (`GND`) estão corretas.

## 3. Estrutura do Projeto

O projeto está organizado nos seguintes diretórios:

```
.
├── Doxyfile              # Arquivo de configuração do Doxygen
├── Makefile              # Makefile principal do projeto
├── README.md             # Este arquivo
├── firmware/             # Drivers e código de baixo nível (ex: lcd.c, led.c)
├── main.c                # Arquivo principal da aplicação
├── scripts/              # Scripts para o computador host (ex: Wave2C)
├── sounds/               # Arquivos de som (.wav) e os arrays em C gerados
└── startup/              # Arquivos de inicialização do microcontrolador (CMSIS)
```

## 4. Como Compilar e Usar

Este projeto utiliza `make` para automatizar todas as tarefas de compilação, geração de arquivos e gravação no hardware.

### Comandos Principais

* **Compilar o Firmware:**
    ```bash
    make build
    ```
    *Para gerar o binário final (`.bin`), use `make all`.*

* **Converter Arquivos de Som:**
    Este comando utiliza o script `Wave2C` para converter todos os arquivos `.wav` do diretório `sounds/` para arquivos `.c`.
    ```bash
    make sounds
    ```

* **Gerar a Documentação:**
    Gera a documentação do código-fonte usando Doxygen. O resultado fica no diretório `html/`.
    ```bash
    make docs
    ```

* **Gravar na Placa:**
    Compila o projeto e grava o binário na memória flash do EFM32.
    ```bash
    make flash
    ```

* **Limpar o Projeto:**
    Remove todos os arquivos gerados durante a compilação (arquivos objeto, binários, documentação, etc.).
    ```bash
    make clean
    ```

## 5. Ferramentas

* **Wave2C:** Um programa utilitário que converte arquivos de áudio `.wav` em arrays C. Isso permite que os sons sejam facilmente incorporados ao firmware do microcontrolador. O código-fonte e o executável para o host estão no diretório `scripts/`.

## 6. Referências

* **Visualização de Ritmos:**
    * [A different way to visualize rhythm - John Varney](https://youtu.be/2UphAzryVpY?si=VngcVZno8oOKhMd7)
    * [18 Rhythms you should know](https://youtu.be/ZROR_E5bFEI?si=uzjE3WsRIYgIKPKj)
* **Exemplos de Ritmos:**
    * [Bossa Nova (Hi-hat) - Drum Groove](https://youtu.be/mZ_mEmaJu98?si=K3nY5y-9Rqi2kRlX)
* **Repositórios e Amostras de Som:**
    * [Repositório Original do Wave2C](https://github.com/hans-jorg/Wave2C)
    * [Free Wave Samples (Sons de Bateria)](https://freewavesamples.com/sample-type/drums)
