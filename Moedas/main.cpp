#define _CRT_SECURE_NO_WARNINGS

#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include "vc.h"
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>

int main(int argc, const char* argv[]) {
    // Inicia o timer
    vc_timer();
    std::string videofile;
    if (argc == 2) {

        videofile = argv[1];
    }
    else {
        std::cout << "Escolha o video para processar:\n";
        std::cout << "1 - C:/Projetos/Moedas/videos/video1.mp4\n";
        std::cout << "2 - C:/Projetos/Moedas/videos/video2.mp4\n";
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

    cv::VideoCapture capture(videofile);
    if (!capture.isOpened()) {
        std::cerr << "Erro ao abrir ficheiro!\n";
        return 1;
    }

    int totalFrames = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_COUNT)),
        fps = static_cast<int>(capture.get(cv::CAP_PROP_FPS)),
        width = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_WIDTH)),
        height = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_HEIGHT));

    cv::namedWindow("Detetor de moedas", cv::WINDOW_AUTOSIZE);

    //--- Valores HSV fixos (removendo trackbars) ---
    //cv::namedWindow("Segmentacao HSV", cv::WINDOW_AUTOSIZE);
    //const int hmin = 10, hmax = 75, smin = 21, smax = 255, vmin = 20, vmax = 150;


    std::vector<OVC> passou;
    int cont = 0, mTotal = 0;
    float soma = 0.0;
    int m200 = 0, m100 = 0, m50 = 0, m20 = 0, m10 = 0, m5 = 0, m2 = 0, m1 = 0;

    FILE* fp = fopen("Moedas.txt", "a");
    if (!fp) {
        std::cerr << "Erro ao abrir o ficheiro de guardar moedas!\n";
        return 1;
    }

    cv::Mat frameorig;
    bool paused = false;
    std::cout << "Pressione 'q' para sair, 'p' para pausar.\n";

    while (true) {
        if (!paused) {
            capture >> frameorig;
            if (frameorig.empty()) {
                std::cout << "Fim do vídeo ou erro na captura.\n";
                break;
            }
        }

        int currentFrame = static_cast<int>(capture.get(cv::CAP_PROP_POS_FRAMES));

        cv::Mat framethr(frameorig.size(), CV_8UC1);

        // Passe os valores fixos para a função de segmentação
        //if (!idBlobs(frameorig, framethr, hmin, hmax, smin, smax, vmin, vmax)) {
        //    std::cerr << "Erro na segmentação HSV!\n"; continue;
        //}

        //cv::imshow("Segmentacao HSV", framethr);

        if (!idBlobs(frameorig, framethr, 10, 75, 21, 255, 20, 150)) {
            std::cerr << "Erro na segmentação HSV!\n"; continue;
        }

        // Após a segmentação HSV (framethr já contém a imagem binária)
        IVC* ivcIn = cv_mat_to_ivc(framethr);
        IVC* ivcTemp1 = vc_image_new(ivcIn->width, ivcIn->height, 1, 255);
        IVC* ivcTemp2 = vc_image_new(ivcIn->width, ivcIn->height, 1, 255);
        IVC* ivcOut = vc_image_new(ivcIn->width, ivcIn->height, 1, 255);

        // Abertura: erosão seguida de dilatação
        vc_erode(ivcIn, ivcTemp1, 5);
        vc_dilate(ivcTemp1, ivcTemp2, 5);

        // Fecho: dilatação seguida de erosão
        vc_dilate(ivcTemp2, ivcTemp1, 5);
        vc_erode(ivcTemp1, ivcOut, 5);

        // Copiar resultado de volta para o Mat do OpenCV usando memcpy
        memcpy(framethr.data, ivcOut->data, ivcOut->width * ivcOut->height * ivcOut->channels);

        // Liberar memória
        vc_image_free(ivcIn);
        vc_image_free(ivcTemp1);
        vc_image_free(ivcTemp2);
        vc_image_free(ivcOut);


        int nMoedas = 0;
        OVC* moedas = vc_binary_blob_labelling(framethr, framethr, &nMoedas);
        if (!moedas && nMoedas > 0) { std::cerr << "Erro na etiquetagem!\n"; continue; }
        if (nMoedas > 0 && !vc_binary_blob_info(framethr, moedas, nMoedas)) {
            std::cerr << "Erro no cálculo de propriedades dos blobs!\n"; free(moedas); continue;
        }

        desenha_linhaVermelha(frameorig);       
            for (int i = 0; i < nMoedas; i++) {
                if (moedas[i].area > 8000) {
                    // Coordenadas
                    std::string text = "x: " + std::to_string(moedas[i].xc) + ", y: " + std::to_string(moedas[i].yc);
                    cv::putText(frameorig, text, cv::Point(moedas[i].xc + 90, moedas[i].yc - 60), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0,0,0), 2, cv::LINE_AA);
                    cv::putText(frameorig, text, cv::Point(moedas[i].xc + 89, moedas[i].yc - 61), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255,8,0), 1, cv::LINE_AA);
                    // Área
                    text = "AREA: " + std::to_string(moedas[i].area);
                    cv::putText(frameorig, text, cv::Point(moedas[i].xc + 90, moedas[i].yc - 40), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0,0,0), 2, cv::LINE_AA);
                    cv::putText(frameorig, text, cv::Point(moedas[i].xc + 89, moedas[i].yc - 41), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255,8,0), 1, cv::LINE_AA);
                    // Perímetro
                    text = "PERIMETRO: " + std::to_string(moedas[i].perimeter);
                    cv::putText(frameorig, text, cv::Point(moedas[i].xc + 90, moedas[i].yc - 20), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0,0,0), 2, cv::LINE_AA);
                    cv::putText(frameorig, text, cv::Point(moedas[i].xc + 89, moedas[i].yc - 21), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255,8,0), 1, cv::LINE_AA);
                    // Circularidade
                    text = "CIRCULARIDADE: " + std::to_string(moedas[i].circularity).substr(0, 5);
                    cv::putText(frameorig, text, cv::Point(moedas[i].xc + 90, moedas[i].yc), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0,0,0), 2, cv::LINE_AA);
                    cv::putText(frameorig, text, cv::Point(moedas[i].xc + 89, moedas[i].yc - 1), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255,8,0), 1, cv::LINE_AA);
                    // Tipo
                    int tipo = idMoeda(moedas[i].area, moedas[i].perimeter, moedas[i].circularity, meanColor);
                    std::string tipoText;
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
                    cv::putText(frameorig, text, cv::Point(moedas[i].xc + 90, moedas[i].yc + 20), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0,0,0), 2, cv::LINE_AA);
                    cv::putText(frameorig, text, cv::Point(moedas[i].xc + 89, moedas[i].yc + 19), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0,0,0), 1, cv::LINE_AA);
                }
            }
        }        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << soma;
        
        // Substituir bloco de texto estatístico por OpenCV putText
        int y_offset = 30;
        int fontFace = cv::FONT_HERSHEY_SIMPLEX;
        double fontScale = 0.6;
        int thickness = 1;
        cv::Scalar color(0, 0, 0); // Preto (BGR)
        
        std::string text = "TOTAL DE MOEDAS: " + std::to_string(mTotal);
        cv::putText(frameorig, text, cv::Point(20, y_offset), fontFace, fontScale, color, thickness, cv::LINE_AA);
        y_offset += 20;
        text = "TOTAL: " + oss.str();
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

        cv::imshow("Detetor de moedas", frameorig);
        cv::waitKey(1);
        int key = cv::waitKey(33);
        if (key == 'q') break;
        if (key == 'p') paused = !paused;

        if (moedas) { free(moedas); moedas = NULL; }
    }

    escreverInfo(fp, cont, mTotal, m200, m100, m50, m20, m10, m5, m2, m1, videofile.c_str());
    fclose(fp);
    capture.release();
    cv::destroyWindow("Detetor de moedas");
    //cv::destroyWindow("Segmentacao HSV");
    std::cout << "Programa terminado.\n";
    // Para o timer e exibe o tempo decorrido
    vc_timer();
    return 0;
}