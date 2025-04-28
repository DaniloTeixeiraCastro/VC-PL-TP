# Versão mínima do CMake necessária
cmake_minimum_required(VERSION 3.15)

# Nome do projeto e linguagem principal (C)
project(CMAKE_MODULE_LINKER_FLAGS_<CONFIG>)

# Definir o standard de C que vamos usar (C11)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Incluir o diretório de headers
include_directories(include)

# Ir buscar automaticamente todos os ficheiros .c dentro da pasta src
file(GLOB SOURCES src/*.c)

# Encontrar o pacote OpenCV (através do vcpkg, por exemplo)
find_package(OpenCV REQUIRED)

# Criar o executável com o nome "moedas" e associar-lhe os ficheiros fontes
add_executable(moedas main.cpp vc.c)

# Ligar o executável às bibliotecas do OpenCV
target_link_libraries(moedas PRIVATE ${OpenCV_LIBS})
