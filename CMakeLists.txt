cmake_minimum_required(VERSION 3.10)
project(Moedas)

# Define o padrão C++
set(CMAKE_CXX_STANDARD 17)

# Caminho para o vcpkg
set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

# Adiciona o caminho do FindFFMPEG.cmake fornecido pelo vcpkg
list(APPEND CMAKE_MODULE_PATH "C:/vcpkg/installed/x64-mingw-dynamic/share/ffmpeg")

# Força o vc.c a ser tratado como código C
set_source_files_properties(vc.c PROPERTIES LANGUAGE C)

# Encontrar o OpenCV
find_package(OpenCV REQUIRED)

# Encontrar o FFMPEG usando o FindFFMPEG.cmake
find_package(FFMPEG REQUIRED)

# Adicionar includes do OpenCV e FFMPEG
include_directories(${OpenCV_INCLUDE_DIRS} ${FFMPEG_INCLUDE_DIRS})

# Adicionar os arquivos do projeto
add_executable(moedas main.cpp vc.c)

# Linkar as bibliotecas do OpenCV e FFMPEG
target_link_libraries(moedas PRIVATE ${OpenCV_LIBS} ${FFMPEG_LIBRARIES})
