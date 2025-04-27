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

📦 Requisitos
Windows 10/11
MinGW-w64
CMake
vcpkg
OpenCV 4 (instalado via vcpkg)

🛠 Instalação
Instala MinGW, CMake e vcpkg conforme o Guia de Instalação.

Instala OpenCV:

bash
Copiar
Editar
.\vcpkg install opencv4:x64-mingw-dynamic
Clona ou descarrega este repositório.

⚙️ Configuração e Build
No terminal:

bash
Copiar
Editar
cd TPProject
cmake -B cmakebuild -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build cmakebuild --clean-first
Antes de correr o executável:

bash
Copiar
Editar
$env:PATH="C:\vcpkg\installed\x64-mingw-dynamic\bin;$env:PATH"
Executar:

bash
Copiar
Editar
cd cmakebuild
.\moedas.exe

🎯 Funcionalidades
Detecção automática de moedas em vídeos.

Segmentação da imagem via filtro Gaussiano e binarização adaptativa.

Cálculo e exibição do número e valor total das moedas detectadas.

🐞 Problemas comuns

Problema	Solução
Imagem binária branca	Ajustar parâmetros de filtro ou binarização
Falta de DLLs	Verificar se o PATH inclui vcpkg/bin
Janela não abre	Verificar instalação do OpenCV e dependências

📄 Licença
Projeto académico. Uso livre para fins educativos.
