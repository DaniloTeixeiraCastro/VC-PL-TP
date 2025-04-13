#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <opencv2/opencv.hpp>
#include <stack>
#include <filesystem>

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

// Funções auxiliares
void calcularCaracteristicas(InfoMoeda* moeda, IVC* imagem, IVC* imagemBinaria, int* visitado);
int detectarMoedas(IVC* imagem, IVC* imagemBinaria, InfoMoeda* moedas, int maxMoedas);
void floodFill(IVC* imagemBinaria, int* visitado, int x, int y, int width, int height, int bytesperline);
void classificarMoeda(InfoMoeda* moeda);
void segmentarImagem(IVC* imagemOriginal, IVC* imagemBinaria);
void desenharRetangulo(cv::Mat& frame, cv::Point p1, cv::Point p2, cv::Scalar cor, int espessura);
void desenharCirculo(cv::Mat& frame, cv::Point centro, int raio, cv::Scalar cor, int espessura);
void desenharTexto(cv::Mat& frame, const char* texto, cv::Point pos, cv::Scalar cor);

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
                if (moeda->area > 500 && moeda->circularidade > 0.75) {
                    classificarMoeda(moeda);
                    numMoedas++;
                }
            }
        }
    }
    
    // Limpar memória
    free(visitado);
    
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

// Funções para desenho
void desenharRetangulo(cv::Mat& frame, cv::Point p1, cv::Point p2, cv::Scalar cor, int espessura) {
    for(int y = p1.y; y <= p2.y; y++) {
        for(int x = p1.x; x <= p2.x; x++) {
            if(x == p1.x || x == p2.x || y == p1.y || y == p2.y) {
                cv::Vec3b& pixel = frame.at<cv::Vec3b>(y, x);
                pixel[0] = cor[0];
                pixel[1] = cor[1];
                pixel[2] = cor[2];
            }
        }
    }
}

void desenharCirculo(cv::Mat& frame, cv::Point centro, int raio, cv::Scalar cor, int espessura) {
    for(int y = centro.y - raio; y <= centro.y + raio; y++) {
        for(int x = centro.x - raio; x <= centro.x + raio; x++) {
            if(x >= 0 && x < frame.cols && y >= 0 && y < frame.rows) {
                double distancia = sqrt(pow(x - centro.x, 2) + pow(y - centro.y, 2));
                if(fabs(distancia - raio) <= espessura) {
                    cv::Vec3b& pixel = frame.at<cv::Vec3b>(y, x);
                    pixel[0] = cor[0];
                    pixel[1] = cor[1];
                    pixel[2] = cor[2];
                }
            }
        }
    }
}

void desenharTexto(cv::Mat& frame, const char* texto, cv::Point pos, cv::Scalar cor) {
    // Implementação simplificada do desenho de texto
    // Esta é uma implementação básica que desenha caracteres ASCII
    // Pode ser melhorada conforme necessário
    for(int i = 0; texto[i] != '\0'; i++) {
        // Aqui você implementaria o desenho de cada caractere
        // Para simplificar, vamos apenas desenhar um quadrado para cada caractere
        cv::Point p1(pos.x + i * 20, pos.y);
        cv::Point p2(p1.x + 10, p1.y + 10);
        desenharRetangulo(frame, p1, p2, cor, -1);
    }
}

// Função principal
int main(void) {
    // Vídeo
    char videofile[100] = "C:/TP-Project/VC-PL-TP/VC-PL-TP/video1.mp4";
    cv::VideoCapture capture;
    
    // Outros
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
    
    // Criar janela para exibir o vídeo
    cv::namedWindow("Detector de Moedas", cv::WINDOW_AUTOSIZE);
    
    cv::Mat frame;
    while (true) {
        // Ler um frame do vídeo
        capture.read(frame);
        
        // Verificar se conseguiu ler o frame
        if (frame.empty()) break;
        
        // Criar uma nova imagem IVC
        IVC* image = vc_image_new(frame.cols, frame.rows, 3, 255);
        if (image == NULL) {
            printf("Erro ao alocar memória para a imagem IVC\n");
            break;
        }
        
        // Copiar dados da imagem de cv::Mat para IVC
        memcpy(image->data, frame.data, frame.rows * frame.cols * 3);
        
        // Criar imagem binária para segmentação
        IVC* imagemBinaria = vc_image_new(frame.cols, frame.rows, 1, 255);
        if (imagemBinaria == NULL) {
            printf("Erro ao alocar memória para a imagem binária\n");
            vc_image_free(image);
            break;
        }
        
        // Segmentar a imagem para isolar as moedas
        segmentarImagem(image, imagemBinaria);
        
        // Detectar moedas na imagem binária
        numMoedas = detectarMoedas(image, imagemBinaria, moedas, 100);
        
        // Desenhar informações na imagem usando nossas funções próprias
        for(int i = 0; i < numMoedas; i++) {
            // Desenhar caixa delimitadora
            desenharRetangulo(frame, 
                cv::Point(moedas[i].x1, moedas[i].y1),
                cv::Point(moedas[i].x2, moedas[i].y2),
                cv::Scalar(0, 255, 0), 2);
            
            // Desenhar centro de gravidade
            desenharCirculo(frame, 
                cv::Point(moedas[i].x, moedas[i].y),
                3, cv::Scalar(0, 0, 255), -1);
            
            // Desenhar tipo da moeda
            char texto[20];
            sprintf(texto, "%d", moedas[i].valor);
            desenharTexto(frame, texto, 
                cv::Point(moedas[i].x - 15, moedas[i].y - 15),
                cv::Scalar(255, 255, 255));
        }
        
        // Liberar memória das imagens IVC
        vc_image_free(image);
        vc_image_free(imagemBinaria);
        
        // Exibir o frame
        cv::imshow("Detector de Moedas", frame);
        
        // Sair se o usuário pressionar 'q'
        if (cv::waitKey(1) == 'q') break;
    }
    
    // Fechar a janela
    cv::destroyWindow("Detector de Moedas");
    
    // Fechar o arquivo de vídeo
    capture.release();
    
    return 0;
}
