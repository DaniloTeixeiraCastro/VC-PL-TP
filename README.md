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

1. Instala MinGW, CMake e vcpkg conforme o Guia de Instalação.

2. Instala OpenCV:

.\vcpkg install opencv4:x64-mingw-dynamic

3. Clona ou descarrega este repositório.



   📚 Guia de Instalação

Pré-requisitos
Sistema Operativo: Windows 10 ou superior

Compilador: MinGW-w64 (64 bits)

CMake: para configurar e gerar build

vcpkg: para gerir bibliotecas C/C++ (como OpenCV)

OpenCV: biblioteca principal para processamento de imagem

Terminal: PowerShell ou Command Prompt



🛠 Instalação Passo a Passo

1. Instalar o MinGW-w64
Acede a: https://winlibs.com/

Baixa a versão mais recente do MinGW-w64 (x86_64-posix-seh).

Extrai o zip num diretório simples, como C:\mingw64.

Adiciona o bin ao Path:

Ex: C:\mingw64\bin

2. Instalar o CMake
Vai a: https://cmake.org/download/

Instala o CMake (marca a opção "Add CMake to system PATH").

3. Instalar o vcpkg

git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat


Instala bibliotecas com:

.\vcpkg install opencv4:x64-mingw-dynamic

Nota: usa sempre o :x64-mingw-dynamic para ser compatível com o teu MinGW.




⚙️ Configuração e Build

1. No terminal:

cmake -S . -B cmakebuild -G "MinGW Makefiles" `
>>   -DCMAKE_C_COMPILER=C:/msys64/mingw64/bin/gcc.exe `
>>   -DCMAKE_CXX_COMPILER=C:/msys64/mingw64/bin/g++.exe `
>>   -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake `
>>   -DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic

cmake --build cmakebuild --clean-first

2. Antes de correr o executável:

$env:PATH="C:\vcpkg\installed\x64-mingw-dynamic\bin;$env:PATH"

3. Executar:

cd cmakebuild
.\moedas.exe


🐞 Problemas comuns

Problema	- Solução
Imagem binária branca	- Ajustar parâmetros de filtro ou binarização
Falta de DLLs	- Verificar se o PATH inclui vcpkg/bin
Janela não abre	- Verificar instalação do OpenCV e dependências


📄 Licença
Projeto académico. Uso livre para fins educativos.
