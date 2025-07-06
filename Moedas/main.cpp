// main.cpp
// Programa principal para deteção e classificação de moedas em vídeo usando OpenCV e funções auxiliares personalizadas.
// Comentários detalhados em PT-PT.

#define _CRT_SECURE_NO_WARNINGS

#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include "vc.h"
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <chrono>

int main(int argc, const char* argv[]) {
    // Inicia o timer (apenas para mostrar tempo total no final)
    vc_timer();
    // Marca o tempo de início para overlay em tempo real
    auto start_time = std::chrono::steady_clock::now();

    // Variável para guardar o caminho do vídeo
    std::string videofile;

    // Seleção do vídeo a processar (Menu)
    if (argc == 2) {
        videofile = argv[1];
    }
    else {
        std::cout << "Escolha o video para processar:\n";
        std::cout << "1 - VIDEO 1\n";
        std::cout << "2 - VIDEO 2\n\n";
        std::cout << "Opcao: ";
        int opcao = 0;
        std::cin >> opcao;
        if (opcao == 1) {
            videofile = "C:/Projetos/Moedas/videos/video1.mp4";
        }
        else if (opcao == 2) {
            videofile = "C:/Projetos/Moedas/videos/video2.mp4";
        }
        else {
            std::cerr << "Opcao inválida!\n";
            return 1;
        }
    }

    // Abre o vídeo selecionado
    cv::VideoCapture capture(videofile);
    if (!capture.isOpened()) {
        std::cerr << "Erro ao abrir ficheiro!\n";
        return 1;
    }

    // Obtém propriedades do vídeo
    int totalFrames = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_COUNT)),
        fps = static_cast<int>(capture.get(cv::CAP_PROP_FPS)),
        width = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_WIDTH)),
        height = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_HEIGHT));

    // Cria janela principal
    cv::namedWindow("Detetor de moedas", cv::WINDOW_AUTOSIZE);
    //--- Cria janela Segmentação HSV ---
    //cv::namedWindow("Segmentacao HSV", cv::WINDOW_AUTOSIZE);
    
    // Variáveis para contagem e estatísticas
    std::vector<OVC> passou; // Guarda moedas já contadas
    int cont = 0, mTotal = 0;
    float soma = 0.0;
    int m200 = 0, m100 = 0, m50 = 0, m20 = 0, m10 = 0, m5 = 0, m2 = 0, m1 = 0;

    // Abre ficheiro para guardar resultados
    FILE* fp = fopen("Moedas.txt", "a");
    if (!fp) {
        std::cerr << "Erro ao abrir o ficheiro de guardar moedas!\n";
        return 1;
    }

    // Matriz para armazenar o frame original lido do vídeo
    cv::Mat frameorig;
    bool paused = false;
    std::cout << "Pressione 'q' para sair, 'p' para pausar.\n";

    // Ciclo principal de processamento de frames
    while (true) {
        if (!paused) {
            capture >> frameorig;
            if (frameorig.empty()) {
                std::cout << "Fim do vídeo ou erro na captura.\n";
                break;
            }
        }

        // Obtém o número do frame atual
        int currentFrame = static_cast<int>(capture.get(cv::CAP_PROP_POS_FRAMES));

        // Cria imagem binária para segmentação
        cv::Mat framethr(frameorig.size(), CV_8UC1);

		// Mostra o frame da imagem binária segmentada
        //cv::imshow("Segmentacao HSV", framethr);

        // Segmentação HSV (deteta regiões de interesse)
        if (!idBlobs(frameorig, framethr, 10, 75, 21, 255, 20, 150)) {
            std::cerr << "Erro na segmentação HSV!\n"; continue;
        }

        // Conversão para estrutura IVC e operações morfológicas
        IVC* ivcIn = cv_mat_to_ivc(framethr);
        IVC* ivcTemp1 = vc_image_new(ivcIn->width, ivcIn->height, 1, 255);
        IVC* ivcTemp2 = vc_image_new(ivcIn->width, ivcIn->height, 1, 255);
        IVC* ivcOut = vc_image_new(ivcIn->width, ivcIn->height, 1, 255);

        // Abertura: erosão seguida de dilatação (remove ruído)
        vc_erode(ivcIn, ivcTemp1, 5);
        vc_dilate(ivcTemp1, ivcTemp2, 5);

        // Fecho: dilatação seguida de erosão (fecha buracos)
        vc_dilate(ivcTemp2, ivcTemp1, 5);
        vc_erode(ivcTemp1, ivcOut, 5);

        // Copia resultado para Mat do OpenCV
        memcpy(framethr.data, ivcOut->data, ivcOut->width * ivcOut->height * ivcOut->channels);

        // Liberta memória temporária
        vc_image_free(ivcIn);
        vc_image_free(ivcTemp1);
        vc_image_free(ivcTemp2);
        vc_image_free(ivcOut);

        // Etiquetagem de blobs (moedas)
        int nMoedas = 0;
        OVC* moedas = vc_binary_blob_labelling(framethr, framethr, &nMoedas);
        if (!moedas && nMoedas > 0) { std::cerr << "Erro na etiquetagem!\n"; continue; }
        if (nMoedas > 0 && !vc_binary_blob_info(framethr, moedas, nMoedas)) {
            std::cerr << "Erro no cálculo de propriedades dos blobs!\n"; free(moedas); continue;
        }

        // Desenha linha de referência vermelha
        desenha_linhaVermelha(frameorig);
        // Processa cada moeda detetada
        for (int i = 0; i < nMoedas; i++) {
            if (moedas[i].area > 8000) {
                // Mostra coordenadas, área, perímetro e circularidade
                std::string text = "x: " + std::to_string(moedas[i].xc) + ", y: " + std::to_string(moedas[i].yc);
                cv::putText(frameorig, text, cv::Point(moedas[i].xc + 89, moedas[i].yc - 61), cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(255,8,0), 1, cv::LINE_AA);
                text = "AREA: " + std::to_string(moedas[i].area);
                cv::putText(frameorig, text, cv::Point(moedas[i].xc + 89, moedas[i].yc - 41), cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(255,8,0), 1, cv::LINE_AA);
                text = "PERIMETRO: " + std::to_string(moedas[i].perimeter);
                cv::putText(frameorig, text, cv::Point(moedas[i].xc + 89, moedas[i].yc - 21), cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(255,8,0), 1, cv::LINE_AA);
                text = "CIRCULARIDADE: " + std::to_string(moedas[i].circularity).substr(0, 5);
                cv::putText(frameorig, text, cv::Point(moedas[i].xc + 89, moedas[i].yc - 1), cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(255,8,0), 1, cv::LINE_AA);
                // Calcula cor média da moeda
                IVC* ivcFrameColor = cv_mat_to_ivc(frameorig);
                VEC3UC meanColor;
                mediaCorROI(ivcFrameColor, moedas[i].x, moedas[i].y, moedas[i].width, moedas[i].height, &meanColor);
                vc_image_free(ivcFrameColor);
                // Classifica moeda
                int tipo = idMoeda(moedas[i].area, moedas[i].perimeter, moedas[i].circularity, meanColor);
                std::string tipoText;
                if (tipo != 0 && moedas[i].circularity > 0.1) {
                    switch (tipo) {
                    case 200: tipoText = "2 EUR"; break;
                    case 100: tipoText = "1 EUR"; break;
                    case 50: tipoText = "50 CENT"; break;
                    case 20: tipoText = "20 CENT"; break;
                    case 10: tipoText = "10 CENT"; break;
                    case 5: tipoText = "5 CENT"; break;
                    case 2: tipoText = "2 CENT"; break;
                    case 1: tipoText = "1 CENT"; break;
                    default: tipoText = "DESCONHECIDO"; break;
                    }
                    text = "Tipo: " + tipoText;
                    cv::putText(frameorig, text, cv::Point(moedas[i].xc + 89, moedas[i].yc + 19), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0,0,0), 1, cv::LINE_AA);
                    // Desenha bounding box
                    vc_desenha_bounding_box(frameorig, moedas[i]);
                    // Verifica se moeda passou pela linha verde
                    if (height / 4 >= moedas[i].yc - 15 && height / 4 <= moedas[i].yc + 20) {
                        desenha_linhaVerde(frameorig);
                        // Evita contar a mesma moeda várias vezes
                        if (passou.size() == 0) {
                            passou.push_back(moedas[i]);
                            cont++; mTotal++;
                            switch (tipo) {
                            case 200: m200++; soma += 2.0f; break;
                            case 100: m100++; soma += 1.0f; break;
                            case 50: m50++; soma += 0.5f; break;
                            case 20: m20++; soma += 0.2f; break;
                            case 10: m10++; soma += 0.1f; break;
                            case 5: m5++; soma += 0.05f; break;
                            case 2: m2++; soma += 0.02f; break;
                            case 1: m1++; soma += 0.01f; break;
                            }
                        }
                        else {
                            int p = verificaPassouAntes(passou.data(), moedas[i], cont);
                            if (p == 1) {
                                passou.push_back(moedas[i]);
                                cont++; mTotal++;
                                switch (tipo) {
                                case 200: m200++; soma += 2.0f; break;
                                case 100: m100++; soma += 1.0f; break;
                                case 50: m50++; soma += 0.5f; break;
                                case 20: m20++; soma += 0.2f; break;
                                case 10: m10++; soma += 0.1f; break;
                                case 5: m5++; soma += 0.05f; break;
                                case 2: m2++; soma += 0.02f; break;
                                case 1: m1++; soma += 0.01f; break;
                                }
                            }
                        }
                    }
                }
            }
        }
        // Overlay de informações gerais do vídeo (tempo, resolução, fps, etc.)
        auto now = std::chrono::steady_clock::now();
        double tempo_decorrido = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count() / 1000.0;
        int info_y_offset = 20;
        int info_fontFace = cv::FONT_HERSHEY_SIMPLEX;
        double info_fontScale = 0.55;
        int info_thickness = 1;
        cv::Scalar info_color(0, 0, 0); // Preto (BGR)
        std::ostringstream info_oss;
        info_oss << std::fixed << std::setprecision(2);
        info_oss << "Tempo: " << tempo_decorrido << "s | Resolucao: " << width << "x" << height;
        cv::putText(frameorig, info_oss.str(), cv::Point(20, info_y_offset), info_fontFace, info_fontScale, info_color, info_thickness, cv::LINE_AA);
        info_y_offset += 18;
        info_oss.str(""); info_oss.clear();
        info_oss << "Frame rate: " << fps << " fps";
        cv::putText(frameorig, info_oss.str(), cv::Point(20, info_y_offset), info_fontFace, info_fontScale, info_color, info_thickness, cv::LINE_AA);
        info_y_offset += 18;
        info_oss.str(""); info_oss.clear();
        info_oss << "Total de frames: " << totalFrames;
        cv::putText(frameorig, info_oss.str(), cv::Point(20, info_y_offset), info_fontFace, info_fontScale, info_color, info_thickness, cv::LINE_AA);
        info_y_offset += 18;
        info_oss.str(""); info_oss.clear();
        info_oss << "Frame atual: " << currentFrame << "/" << totalFrames;
        cv::putText(frameorig, info_oss.str(), cv::Point(20, info_y_offset), info_fontFace, info_fontScale, info_color, info_thickness, cv::LINE_AA);
        info_y_offset += 18;

        // Overlay de estatísticas das moedas
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << soma;
        int y_offset = info_y_offset + 10;
        int fontFace = cv::FONT_HERSHEY_SIMPLEX;
        double fontScale = 0.6;
        int thickness = 1;
        cv::Scalar color(0, 0, 0); // Preto (BGR)
        std::string text = "TOTAL DE MOEDAS: " + std::to_string(mTotal);
        cv::putText(frameorig, text, cv::Point(20, y_offset), fontFace, fontScale, color, thickness, cv::LINE_AA);
        y_offset += 20;
        text = "VALOR ACUMULADO: " + oss.str();
        cv::putText(frameorig, text, cv::Point(20, y_offset), fontFace, fontScale, color, thickness, cv::LINE_AA);
        y_offset += 20;
        text = "2 EUR: " + std::to_string(m200);
        cv::putText(frameorig, text, cv::Point(20, y_offset), fontFace, fontScale, color, thickness, cv::LINE_AA);
        y_offset += 20;
        text = "1 EUR: " + std::to_string(m100);
        cv::putText(frameorig, text, cv::Point(20, y_offset), fontFace, fontScale, color, thickness, cv::LINE_AA);
        y_offset += 20;
        text = "50 CENT: " + std::to_string(m50);
        cv::putText(frameorig, text, cv::Point(20, y_offset), fontFace, fontScale, color, thickness, cv::LINE_AA);
        y_offset += 20;
        text = "20 CENT: " + std::to_string(m20);
        cv::putText(frameorig, text, cv::Point(20, y_offset), fontFace, fontScale, color, thickness, cv::LINE_AA);
        y_offset += 20;
        text = "10 CENT: " + std::to_string(m10);
        cv::putText(frameorig, text, cv::Point(20, y_offset), fontFace, fontScale, color, thickness, cv::LINE_AA);
        y_offset += 20;
        text = "5 CENT: " + std::to_string(m5);
        cv::putText(frameorig, text, cv::Point(20, y_offset), fontFace, fontScale, color, thickness, cv::LINE_AA);
        y_offset += 20;
        text = "2 CENT: " + std::to_string(m2);
        cv::putText(frameorig, text, cv::Point(20, y_offset), fontFace, fontScale, color, thickness, cv::LINE_AA);
        y_offset += 20;
        text = "1 CENT: " + std::to_string(m1);
        cv::putText(frameorig, text, cv::Point(20, y_offset), fontFace, fontScale, color, thickness, cv::LINE_AA);
        y_offset += 20;

        // Mostra o frame processado
        cv::imshow("Detetor de moedas", frameorig);
        cv::waitKey(1);
        int key = cv::waitKey(33);
        if (key == 'q') break;
        if (key == 'p') paused = !paused;

        if (moedas) { free(moedas); moedas = NULL; }
    }

    // Guarda estatísticas no ficheiro
    escreverInfo(fp, cont, mTotal, m200, m100, m50, m20, m10, m5, m2, m1, videofile.c_str());
    fclose(fp);
    
    // Liberta recursos e Fecha a janela de visualização do vídeo
    capture.release();
    cv::destroyWindow("Detetor de moedas");
    //cv::destroyWindow("Segmentacao HSV");
    std::cout << "Programa terminado.\n";
    
    // Para o timer e exibe o tempo decorrido
    vc_timer();
    return 0;
}