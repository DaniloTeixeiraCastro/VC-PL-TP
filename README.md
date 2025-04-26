# Detector de Moedas - Trabalho Prático de Visão por Computador

Este projeto implementa um sistema de detecção e quantificação de moedas em vídeos, desenvolvido em C com a biblioteca OpenCV.

## Funcionalidades

- Leitura de vídeos em formato .mp4
- Identificação e contagem de moedas
- Determinação do tipo de cada moeda (1, 2, 5, 10, 20, 50 cêntimos, 1 ou 2 euros)
- Cálculo da área e perímetro de cada moeda
- Exibição do somatório total em dinheiro
- Visualização gráfica das moedas detectadas com suas caixas delimitadoras e centroides

## Estrutura do Projeto

- `main.c`: Arquivo principal do programa
- `vc.h`: Cabeçalho com definições de estruturas e protótipos de funções
- `vc.c`: Implementação das funções de processamento de imagem

## Técnicas Implementadas

1. **Segmentação de Imagem**
   - Conversão para espaço de cor HSV
   - Segmentação por tonalidade e brilho

2. **Melhoramento de Imagem**
   - Remoção de ruído com operações morfológicas
   - Filtros para melhorar a detecção

3. **Análise de Imagem**
   - Cálculo de área
   - Determinação de caixa delimitadora
   - Cálculo de circularidade
   - Determinação do centro de massa (centroide)

4. **Classificação de Moedas**
   - Algoritmos para distinguir diferentes tipos de moedas com base em suas características

## Como Usar

1. Compile o programa:
   ```
   gcc -o detector_moedas main.c vc.c -lopencv_core -lopencv_highgui -lopencv_videoio -lopencv_imgproc
   ```

2. Execute o programa:
   ```
   ./detector_moedas
   ```

3. O programa irá processar o vídeo e exibir os resultados em tempo real.

## Observações para Implementação

- A função `segmentarImagem()` precisa ser ajustada para melhor detectar as moedas nos vídeos fornecidos.
- A função `detectarMoedas()` deve ser implementada para identificar corretamente os componentes conectados e calcular suas propriedades.
- Os limiares para classificação das moedas precisam ser calibrados com base nos vídeos fornecidos.
- Apenas 3 funções do OpenCV são permitidas além das fornecidas no exemplo.

## Restrições Técnicas

- Resolução dos vídeos: 1280x720 pixels
- Taxa de frames: 30 fps
- Linguagem: C
- Biblioteca permitida: OpenCV (máximo 3 funções extras)
