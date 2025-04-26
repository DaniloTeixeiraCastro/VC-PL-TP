#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <stack>
#include <filesystem>
#include <iostream>

// Define M_PI if it's not already defined
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

extern "C" {
#include "vc.h"
}

// Definição da estrutura InfoMoeda
struct InfoMoeda {
    int tipo;           // 1, 2, 5, 10, 20, 50 cents or 1, 2 euros
    double valor;       // Valor monetário
    int x, y;           // Centro da moeda
    double area;        // Área em pixels
    double perimetro;   // Perímetro em pixels
    int x1, y1, x2, y2; // Caixa delimitadora
    double circularidade; // Medida de circularidade
};

// Declarações das funções
void floodFill(IVC* imagemBinaria, int* visitado, int x, int y, int width, int height, int bytesperline);
void calcularCaracteristicas(InfoMoeda* moeda, IVC* imagem, IVC* imagemBinaria, int* visitado);
void classificarMoeda(InfoMoeda* moeda);

// Função para calcular características da moeda
void calcularCaracteristicas(InfoMoeda* moeda, IVC* imagem, IVC* imagemBinaria, int* visitado) {
    int width = imagem->width;
    int height = imagem->height;
    int bytesperline = imagemBinaria->bytesperline;
    
    // Percorrer todos os pixels visitados
    for(int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {
            if(visitado[y * width + x]) {
                // Atualizar área
                moeda->area++;
                
                // Atualizar centroide
                moeda->x += x;
                moeda->y += y;
                
                // Atualizar caixa delimitadora
                if(x < moeda->x1) moeda->x1 = x;
                if(y < moeda->y1) moeda->y1 = y;
                if(x > moeda->x2) moeda->x2 = x;
                if(y > moeda->y2) moeda->y2 = y;
            }
        }
    }
    
    // Calcular centroide
    moeda->x /= moeda->area;
    moeda->y /= moeda->area;
    
    // Calcular perímetro (aproximado)
    moeda->perimetro = 2 * (moeda->x2 - moeda->x1 + moeda->y2 - moeda->y1);
    
    // Calcular circularidade
    double area = moeda->area;
    double perimetro = moeda->perimetro;
    moeda->circularidade = (4 * M_PI * area) / (perimetro * perimetro);
}

// Função para classificar moeda com base na área
void classificarMoeda(InfoMoeda* moeda) {
    // Valores aproximados de área para cada tipo de moeda
    // Estes valores precisarão ser ajustados com base nos dados reais
    if(moeda->area < 2000) {
        moeda->tipo = 1; // 1 cent
        moeda->valor = 0.01;
    } else if(moeda->area < 3000) {
        moeda->tipo = 2; // 2 cents
        moeda->valor = 0.02;
    } else if(moeda->area < 4000) {
        moeda->tipo = 5; // 5 cents
        moeda->valor = 0.05;
    } else if(moeda->area < 5000) {
        moeda->tipo = 10; // 10 cents
        moeda->valor = 0.10;
    } else if(moeda->area < 6000) {
        moeda->tipo = 20; // 20 cents
        moeda->valor = 0.20;
    } else if(moeda->area < 7000) {
        moeda->tipo = 50; // 50 cents
        moeda->valor = 0.50;
    } else if(moeda->area < 8000) {
        moeda->tipo = 100; // 1 euro
        moeda->valor = 1.0;
    } else {
        moeda->tipo = 200; // 2 euros
        moeda->valor = 2.0;
    }
}

// Substituir a função floodFill existente por esta versão iterativa
void floodFill(IVC* imagemBinaria, int* visitado, int x, int y, int width, int height, int bytesperline) {
    std::stack<std::pair<int, int>> pilha;
    pilha.push({x, y});

    while (!pilha.empty()) {
        auto [cx, cy] = pilha.top();
        pilha.pop();

        int pos = cy * bytesperline + cx;
        if (cx < 0 || cx >= width || cy < 0 || cy >= height || imagemBinaria->data[pos] != 255 || visitado[cy * width + cx]) {
            continue;
        }

        visitado[cy * width + cx] = 1;

        // Adicionar vizinhos à pilha (4-conectividade)
        pilha.push({cx + 1, cy});
        pilha.push({cx - 1, cy});
        pilha.push({cx, cy + 1});
        pilha.push({cx, cy - 1});
    }
}

// Função para processar a imagem binária e detectar moedas
int detectarMoedas(IVC* imagem, IVC* imagemBinaria, InfoMoeda* moedas, int maxMoedas) {
    int numMoedas = 0;
    int width = imagem->width;
    int height = imagem->height;
    int bytesperline = imagemBinaria->bytesperline;
    unsigned char* dataBin = imagemBinaria->data;
    
    // Matriz para marcar pixels já processados
    int* visitado = (int*)malloc(width * height * sizeof(int));
    memset(visitado, 0, width * height * sizeof(int));
    
    // Lista para armazenar pixels do contorno
    int* contornoX = (int*)malloc(10000 * sizeof(int));
    int* contornoY = (int*)malloc(10000 * sizeof(int));
    int contornoSize = 0;
    
    for(int y = 0; y < height && numMoedas < maxMoedas; y++) {
        for(int x = 0; x < width && numMoedas < maxMoedas; x++) {
            int pos = y * bytesperline + x;
            
            // Se encontrarmos um pixel branco não visitado
            if(dataBin[pos] == 255 && !visitado[y * width + x]) {
                // Inicializar moeda
                InfoMoeda* moeda = &moedas[numMoedas];
                moeda->area = 0;
                moeda->perimetro = 0;
                moeda->x = 0;
                moeda->y = 0;
                moeda->x1 = width;
                moeda->y1 = height;
                moeda->x2 = 0;
                moeda->y2 = 0;
                
                // Fazer flood fill para encontrar toda a região
                floodFill(imagemBinaria, visitado, x, y, width, height, bytesperline);
                
                // Calcular características da moeda
                calcularCaracteristicas(moeda, imagem, imagemBinaria, visitado);
                
                // Verificar se é uma moeda válida (baseado em área e circularidade)
                if (moeda->area > 300 && moeda->circularidade > 0.75) {
                    classificarMoeda(moeda);
                    numMoedas++;
                }
            }
        }
    }
    
    // Limpar memória
    free(visitado);
    free(contornoX);
    free(contornoY);
    
    return numMoedas;
}

// Função para segmentar a imagem e isolar as moedas
void segmentarImagem(IVC* imagemOriginal, IVC* imagemBinaria) {
    // Converter para escala de cinza
    IVC* imagemGray = vc_image_new(imagemOriginal->width, imagemOriginal->height, 1, 255);
    vc_rgb_to_gray(imagemOriginal, imagemGray);
    
    // Aplicar filtro de média para reduzir ruído
    int kernelSize = 3;
    IVC* imagemFiltrada = vc_image_new(imagemOriginal->width, imagemOriginal->height, 1, 255);
    
    // Calcular média local
    for(int y = kernelSize/2; y < imagemGray->height - kernelSize/2; y++) {
        for(int x = kernelSize/2; x < imagemGray->width - kernelSize/2; x++) {
            int soma = 0;
            for(int ky = -kernelSize/2; ky <= kernelSize/2; ky++) {
                for(int kx = -kernelSize/2; kx <= kernelSize/2; kx++) {
                    soma += imagemGray->data[(y+ky)*imagemGray->bytesperline + (x+kx)];
                }
            }
            imagemFiltrada->data[y*imagemGray->bytesperline + x] = soma / (kernelSize*kernelSize);
        }
    }
    
    // Binarização usando média global
    vc_gray_to_binary_global_mean(imagemFiltrada, imagemBinaria);
    
    // Aplicar operações morfológicas para melhorar a detecção
    vc_binary_dilate(imagemBinaria, imagemBinaria, 3);
    vc_binary_erode(imagemBinaria, imagemBinaria, 3);
    
    // Limpar memória
    vc_image_free(imagemGray);
    vc_image_free(imagemFiltrada);
}

// Função para desenhar informações na imagem
void desenharInformacoes(cv::Mat frame, InfoMoeda* moedas, int numMoedas) {
    int i;
    char texto[100];
    double valorTotal = 0.0;
    
    // Contar moedas por tipo
    int count_1c = 0, count_2c = 0, count_5c = 0, count_10c = 0;
    int count_20c = 0, count_50c = 0, count_1e = 0, count_2e = 0;
    
    for (i = 0; i < numMoedas; i++) {
        // Desenhar caixa delimitadora manualmente
        for (int j = moedas[i].x1; j <= moedas[i].x2; j++) {
            frame.at<cv::Vec3b>(moedas[i].y1, j) = cv::Vec3b(0, 255, 0); // Linha superior
            frame.at<cv::Vec3b>(moedas[i].y2, j) = cv::Vec3b(0, 255, 0); // Linha inferior
        }
        for (int j = moedas[i].y1; j <= moedas[i].y2; j++) {
            frame.at<cv::Vec3b>(j, moedas[i].x1) = cv::Vec3b(0, 255, 0); // Linha esquerda
            frame.at<cv::Vec3b>(j, moedas[i].x2) = cv::Vec3b(0, 255, 0); // Linha direita
        }
        
        // Desenhar ponto central
        frame.at<cv::Vec3b>(moedas[i].y, moedas[i].x) = cv::Vec3b(0, 0, 255);
        
        // Exibir tipo de moeda
        if (moedas[i].tipo < 100) {
            sprintf(texto, "%d cents", moedas[i].tipo);
        } else {
            sprintf(texto, "%d euros", moedas[i].tipo / 100);
        }
        
        cv::putText(frame, texto, cv::Point(moedas[i].x - 30, moedas[i].y - 20), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
        
        // Acumular valor total
        valorTotal += moedas[i].valor;
        
        // Contar por tipo
        switch (moedas[i].tipo) {
            case 1: count_1c++; break;
            case 2: count_2c++; break;
            case 5: count_5c++; break;
            case 10: count_10c++; break;
            case 20: count_20c++; break;
            case 50: count_50c++; break;
            case 100: count_1e++; break;
            case 200: count_2e++; break;
        }
    }
    
    // Exibir estatísticas
    int y_pos = 150;
    
    // Total de moedas
    sprintf(texto, "Total moedas: %d", numMoedas);
    cv::rectangle(frame, cv::Point(20, y_pos - 20), cv::Point(300, y_pos + 5), cv::Scalar(0, 0, 0), cv::FILLED);
    cv::putText(frame, texto, cv::Point(20, y_pos), 
               cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 1);
    y_pos += 25;
    
    // Valor total
    sprintf(texto, "Valor total: %.2f euros", valorTotal);
    cv::rectangle(frame, cv::Point(20, y_pos - 20), cv::Point(300, y_pos + 5), cv::Scalar(0, 0, 0), cv::FILLED);
    cv::putText(frame, texto, cv::Point(20, y_pos), 
               cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 1);
    y_pos += 25;
    
    // Exibir contagem por tipo
    if (count_1c > 0) {
        sprintf(texto, "1 cent: %d", count_1c);
        cv::rectangle(frame, cv::Point(20, y_pos - 20), cv::Point(300, y_pos + 5), cv::Scalar(0, 0, 0), cv::FILLED);
        cv::putText(frame, texto, cv::Point(20, y_pos), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 1);
        y_pos += 25;
    }
    
    if (count_2c > 0) {
        sprintf(texto, "2 cents: %d", count_2c);
        cv::rectangle(frame, cv::Point(20, y_pos - 20), cv::Point(300, y_pos + 5), cv::Scalar(0, 0, 0), cv::FILLED);
        cv::putText(frame, texto, cv::Point(20, y_pos), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 1);
        y_pos += 25;
    }
    
    if (count_5c > 0) {
        sprintf(texto, "5 cents: %d", count_5c);
        cv::rectangle(frame, cv::Point(20, y_pos - 20), cv::Point(300, y_pos + 5), cv::Scalar(0, 0, 0), cv::FILLED);
        cv::putText(frame, texto, cv::Point(20, y_pos), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 1);
        y_pos += 25;
    }
    
    if (count_10c > 0) {
        sprintf(texto, "10 cents: %d", count_10c);
        cv::rectangle(frame, cv::Point(20, y_pos - 20), cv::Point(300, y_pos + 5), cv::Scalar(0, 0, 0), cv::FILLED);
        cv::putText(frame, texto, cv::Point(20, y_pos), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 1);
        y_pos += 25;
    }
    
    if (count_20c > 0) {
        sprintf(texto, "20 cents: %d", count_20c);
        cv::rectangle(frame, cv::Point(20, y_pos - 20), cv::Point(300, y_pos + 5), cv::Scalar(0, 0, 0), cv::FILLED);
        cv::putText(frame, texto, cv::Point(20, y_pos), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 1);
        y_pos += 25;
    }
    
    if (count_50c > 0) {
        sprintf(texto, "50 cents: %d", count_50c);
        cv::rectangle(frame, cv::Point(20, y_pos - 20), cv::Point(300, y_pos + 5), cv::Scalar(0, 0, 0), cv::FILLED);
        cv::putText(frame, texto, cv::Point(20, y_pos), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 1);
        y_pos += 25;
    }
    
    if (count_1e > 0) {
        sprintf(texto, "1 euro: %d", count_1e);
        cv::rectangle(frame, cv::Point(20, y_pos - 20), cv::Point(300, y_pos + 5), cv::Scalar(0, 0, 0), cv::FILLED);
        cv::putText(frame, texto, cv::Point(20, y_pos), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 1);
        y_pos += 25;
    }
    
    if (count_2e > 0) {
        sprintf(texto, "2 euros: %d", count_2e);
        cv::rectangle(frame, cv::Point(20, y_pos - 20), cv::Point(300, y_pos + 5), cv::Scalar(0, 0, 0), cv::FILLED);
        cv::putText(frame, texto, cv::Point(20, y_pos), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 1);
    }
}

int main(void) {
    // Vídeo
    char videofile[100] = "C:/Users/fferreira/Projetos/TPProject/video1.mp4";
    cv::VideoCapture capture;
    struct {
        int width, height;
        int ntotalframes;
        int fps;
        int nframe;
    } video;
    
    // Outros
    char str[100];
    int key = 0;
    InfoMoeda moedas[100]; // Array para armazenar informações das moedas
    int numMoedas = 0;
    
    // Verificar se o arquivo de vídeo existe antes de abrir
    if (!std::filesystem::exists(videofile)) {
        printf("Erro: O arquivo de vídeo '%s' não foi encontrado!\n", videofile);
        return 1;
    }
    
    // Abrir o arquivo de vídeo
    capture.open(videofile);
    
    // Verificar se foi possível abrir o arquivo de vídeo
    if (!capture.isOpened()) {
        printf("Erro ao abrir o arquivo de vídeo!\n");
        return 1;
    }
    
    // Obter propriedades do vídeo
    video.ntotalframes = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
    video.fps = (int)capture.get(cv::CAP_PROP_FPS);
    video.width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
    video.height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);
    
    // Criar janela para exibir o vídeo
    cv::namedWindow("Detector de Moedas", cv::WINDOW_AUTOSIZE);
    
    // Iniciar o timer
    // vc_timer();
    
    cv::Mat frame;
    while (key != 'q') {
        // Ler um frame do vídeo
        capture.read(frame);
        
        // Verificar se conseguiu ler o frame
        if (frame.empty()) break;
        
        // Número do frame atual
        video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);
        
        // Exibir informações do vídeo
        sprintf(str, "RESOLUCAO: %dx%d", video.width, video.height);
        cv::putText(frame, str, cv::Point(20, 25), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 1);
        
        sprintf(str, "FRAME: %d/%d", video.nframe, video.ntotalframes);
        cv::putText(frame, str, cv::Point(20, 50), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 1);
        
        sprintf(str, "FPS: %d", video.fps);
        cv::putText(frame, str, cv::Point(20, 75), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 1);
        
        printf("Processando frame %d de %d...\n", video.nframe, video.ntotalframes);
        
        // Criar uma nova imagem IVC
        IVC* image = vc_image_new(video.width, video.height, 3, 255);
        if (image == NULL) {
            printf("Erro ao alocar memória para a imagem IVC\n");
            break;
        }
        
        // Copiar dados da imagem de cv::Mat para IVC
        memcpy(image->data, frame.data, video.width * video.height * 3);
        
        // Criar imagem binária para segmentação
        IVC* imagemBinaria = vc_image_new(video.width, video.height, 1, 255);
        if (imagemBinaria == NULL) {
            printf("Erro ao alocar memória para a imagem binária\n");
            vc_image_free(image);
            break;
        }
        
        // Segmentar a imagem para isolar as moedas
        segmentarImagem(image, imagemBinaria);
        
        // Detectar moedas na imagem binária
        cv::imwrite("C:/Projetos/TPProject/CMakeBuild/debug_binaria.png", cv::Mat(video.height, video.width, CV_8UC1, imagemBinaria->data));
        numMoedas = detectarMoedas(image, imagemBinaria, moedas, 100);
        
        // Desenhar informações na imagem
        desenharInformacoes(frame, moedas, numMoedas);
        
        // Liberar memória das imagens IVC
        vc_image_free(image);
        vc_image_free(imagemBinaria);
        
        // Exibir o frame
        cv::imshow("Detector de Moedas", frame);
        
        // Sair se o usuário pressionar 'q'
        key = cv::waitKey(1);
    }
    
    // Parar o timer e exibir o tempo decorrido
    // vc_timer();
    
    // Fechar a janela
    cv::destroyWindow("Detector de Moedas");
    
    // Fechar o arquivo de vídeo
    
    std::cout << "Pressione Enter para sair..." << std::endl;
    std::cin.get(); // Aguarda o utilizador pressionar Enter

    capture.release();
    return 0;
}
