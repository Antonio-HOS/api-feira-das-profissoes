import numpy as np
from pathlib import Path

# Pasta onde estão os arquivos .dat
PASTA = Path(r"")

# Percorre todos os arquivos .dat
for arquivo in PASTA.glob("*.dat"):
    try:
        # Lê a imagem
        imagem = np.fromfile(arquivo, dtype=np.uint16)

        # Ajuste as dimensões da imagem
        imagem = imagem.reshape(1200, 1400)

        # Espelha verticalmente (cima <-> baixo)
        imagem_espelhada = np.flipud(imagem)

        # Sobrescreve o arquivo original
        imagem_espelhada.tofile(arquivo)

        print(f"Processado: {arquivo.name}")

    except Exception as e:
        print(f"Erro em {arquivo.name}: {e}")

print("Concluído!")
