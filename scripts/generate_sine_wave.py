import math

TABLE_SIZE = 100   # pontos na tabela (ajuste conforme necessário)
AMPLITUDE = 127    # máximo
OFFSET = AMPLITUDE // 2  # deslocamento para ficar centrado em 63~64

table = []
for n in range(TABLE_SIZE):
    theta = 2.0 * math.pi * n / TABLE_SIZE
    # senóide normalizada 0..127
    value = int((math.sin(theta) * 0.5 + 0.5) * AMPLITUDE)
    table.append(value)

# imprimir formato C
print("static const uint8_t sine_table[{}] = {{".format(TABLE_SIZE))
for i, v in enumerate(table):
    sep = "," if i < TABLE_SIZE - 1 else ""
    end = "\n" if (i+1) % 10 == 0 else ""
    print(f" {v:3d}{sep}", end=end)
print("};")
