#define _CRT_SECURE_NO_WARNINGS

#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include "vc.h"
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>

// Função usando apenas IVC para calcular média de cor
cv::Vec3b mediaCorROI_IVC(const cv::Mat& img, int x, int y, int width, int height) {
    // Converter Mat para IVC
    IVC* ivcImg = cv_mat_to_ivc(img);
    if (!ivcImg) return cv::Vec3b(0, 0, 0);
    
    long sumB = 0, sumG = 0, sumR = 0, count = 0;
    
    for (int j = y; j < y + height && j < ivcImg->height; ++j) {
        for (int i = x; i < x + width && i < ivcImg->width; ++i) {
            int pos = j * ivcImg->bytesperline + i * ivcImg->channels;
            sumB += ivcImg->data[pos];     // Blue
            sumG += ivcImg->data[pos + 1]; // Green  
            sumR += ivcImg->data[pos + 2]; // Red
            count++;
        }
    }
    
    vc_image_free(ivcImg);
    
    if (count == 0) return cv::Vec3b(0, 0, 0);
    return cv::Vec3b((uchar)(sumB / count), (uchar)(sumG / count), (uchar)(sumR / count));
}

int main(int argc, const char* argv[]) {
    std::string videofile;
    if (argc == 2) {

        videofile = argv[1];
    }
    else {
        std::cout << "Escolha o video para processar:\n";
        std::cout << "1 - C:/VC-PL-TP/videos/video1.mp4\n";
        std::cout << "2 - C:/VC-PL-TP/videos/video2.mp4\n";
        std::cout << "Opcao: ";
        int opcao = 0;
        std::cin >> opcao;
        if (opcao == 1) {
            videofile = "C:/VC-PL-TP/videos/video1.mp4";
        }
        else if (opcao == 2) {
            videofile = "C:/VC-PL-TP/videos/video2.mp4";
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

    //--- Valores HSV fixos (removendo trackbars OpenCV) ---
    cv::namedWindow("Segmentacao HSV", cv::WINDOW_AUTOSIZE);
    const int hmin = 10, hmax = 80, smin = 25, smax = 255, vmin = 20, vmax = 150;


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
        if (!idBlobs(frameorig, framethr, hmin, hmax, smin, smax, vmin, vmax)) {
            std::cerr << "Erro na segmentação HSV!\n"; continue;
        }

        cv::imshow("Segmentacao HSV", framethr);

        //if (!idBlobs(frameorig, framethr, 12, 150, 35, 255, 20, 150)) {
            //std::cerr << "Erro na segmentação HSV!\n"; continue;
        //}

        // Após a segmentação HSV (framethr já contém a imagem binária)
        IVC* ivcIn = cv_mat_to_ivc(framethr);
        IVC* ivcTemp1 = vc_image_new(ivcIn->width, ivcIn->height, 1, 255);
        IVC* ivcTemp2 = vc_image_new(ivcIn->width, ivcIn->height, 1, 255);
        IVC* ivcOut = vc_image_new(ivcIn->width, ivcIn->height, 1, 255);

        // Abertura: erosão seguida de dilatação
        vc_erode(ivcIn, ivcTemp1, 5);
        vc_dilate(ivcTemp1, ivcTemp2, 5);

        // Fechamento: dilatação seguida de erosão
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
            if (moedas[i].area > 8000) {                // Converter a imagem para o formato IVC para desenhar texto
                IVC* ivcFrame = cv_mat_to_ivc(frameorig);
                if (ivcFrame != NULL) {
                    int colorBlack[3] = { 0, 0, 0 };   // Cor preta para sombra
                    int colorBlue[3] = { 255, 8, 0 };  // Cor azul para texto (BGR)
                    
                    // Coordenadas
                    std::string text = "x: " + std::to_string(moedas[i].xc) + ", y: " + std::to_string(moedas[i].yc);
                    // Texto com sombra
                    vc_put_text(ivcFrame, text.c_str(), moedas[i].xc + 90, moedas[i].yc - 60, colorBlack, 1);
                    // Texto principal
                    vc_put_text(ivcFrame, text.c_str(), moedas[i].xc + 89, moedas[i].yc - 61, colorBlue, 1);
                    
                    // Área
                    text = "AREA: " + std::to_string(moedas[i].area);
                    // Texto com sombra
                    vc_put_text(ivcFrame, text.c_str(), moedas[i].xc + 90, moedas[i].yc - 40, colorBlack, 1);
                    // Texto principal
                    vc_put_text(ivcFrame, text.c_str(), moedas[i].xc + 89, moedas[i].yc - 41, colorBlue, 1);
                    
                    // Perímetro
                    text = "PERIMETRO: " + std::to_string(moedas[i].perimeter);
                    // Texto com sombra
                    vc_put_text(ivcFrame, text.c_str(), moedas[i].xc + 90, moedas[i].yc - 20, colorBlack, 1);
                    // Texto principal
                    vc_put_text(ivcFrame, text.c_str(), moedas[i].xc + 89, moedas[i].yc - 21, colorBlue, 1);
                    
                    // Circularidade
                    text = "CIRCULARIDADE: " + std::to_string(moedas[i].circularity).substr(0, 5);
                    // Texto com sombra
                    vc_put_text(ivcFrame, text.c_str(), moedas[i].xc + 90, moedas[i].yc, colorBlack, 1);
                    // Texto principal
                    vc_put_text(ivcFrame, text.c_str(), moedas[i].xc + 89, moedas[i].yc - 1, colorBlue, 1);
                    
                    // Copiar resultado de volta para a matriz OpenCV
                    cv::Mat temp(frameorig.rows, frameorig.cols, CV_8UC3, ivcFrame->data);
                    temp.copyTo(frameorig);
                    
                    // Liberar memória
                    vc_image_free(ivcFrame);
                }

                
                cv::Vec3b meanColor = mediaCorROI_IVC(frameorig, moedas[i].x, moedas[i].y, moedas[i].width, moedas[i].height);
                
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
                    std::string text = "Valor: " + tipoText;
                    // Converter a imagem para o formato IVC para desenhar texto
                    IVC* ivcFrame = cv_mat_to_ivc(frameorig);
                    if (ivcFrame != NULL) {
                        int colorBlack[3] = { 0, 0, 0 };  // Cor preta para texto
                        
                        // Texto com sombra
                        vc_put_text(ivcFrame, text.c_str(), moedas[i].xc + 90, moedas[i].yc + 20, colorBlack, 1);
                        // Texto principal (um pouco deslocado para criar efeito de shadow)
                        vc_put_text(ivcFrame, text.c_str(), moedas[i].xc + 89, moedas[i].yc + 19, colorBlack, 1);
                        
                        // Copiar resultado de volta para a matriz OpenCV
                        cv::Mat temp(frameorig.rows, frameorig.cols, CV_8UC3, ivcFrame->data);
                        temp.copyTo(frameorig);
                        
                        // Liberar memória
                        vc_image_free(ivcFrame);
                    }

                    vc_desenha_bounding_box(frameorig, moedas[i]);

                    if (height / 4 >= moedas[i].yc - 15 && height / 4 <= moedas[i].yc + 20) {
                        desenha_linhaVerde(frameorig);
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
        }        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << soma;
        
        // Converter a imagem para o formato IVC para desenhar texto de estatísticas
        IVC* ivcFrame = cv_mat_to_ivc(frameorig);
        if (ivcFrame != NULL) {
            int colorBlack[3] = { 0, 0, 0 };  // Cor preta
            int y_offset = 30;
              // Total de moedas
            std::string text = "TOTAL DE MOEDAS: " + std::to_string(mTotal);
            vc_put_text(ivcFrame, text.c_str(), 20, y_offset, colorBlack, 1);
            y_offset += 20;
            
            // Valor total
            text = "TOTAL: " + oss.str();
            vc_put_text(ivcFrame, text.c_str(), 20, y_offset, colorBlack, 1);
            y_offset += 20;
            
            // Contagens por tipo de moeda
            text = "2 EUR: " + std::to_string(m200);
            vc_put_text(ivcFrame, text.c_str(), 20, y_offset, colorBlack, 1);
            y_offset += 20;
            
            text = "1 EUR: " + std::to_string(m100);
            vc_put_text(ivcFrame, text.c_str(), 20, y_offset, colorBlack, 1);
            y_offset += 20;
            
            text = "50 CENT: " + std::to_string(m50);
            vc_put_text(ivcFrame, text.c_str(), 20, y_offset, colorBlack, 1);
            y_offset += 20;
            
            text = "20 CENT: " + std::to_string(m20);
            vc_put_text(ivcFrame, text.c_str(), 20, y_offset, colorBlack, 1);
            y_offset += 20;
            
            text = "10 CENT: " + std::to_string(m10);
            vc_put_text(ivcFrame, text.c_str(), 20, y_offset, colorBlack, 1);
            y_offset += 20;
            
            text = "5 CENT: " + std::to_string(m5);
            vc_put_text(ivcFrame, text.c_str(), 20, y_offset, colorBlack, 1);
            y_offset += 20;
            
            text = "2 CENT: " + std::to_string(m2);
            vc_put_text(ivcFrame, text.c_str(), 20, y_offset, colorBlack, 1);
            y_offset += 20;
            
            text = "1 CENT: " + std::to_string(m1);
            vc_put_text(ivcFrame, text.c_str(), 20, y_offset, colorBlack, 1);
            
            // Copiar resultado de volta para a matriz OpenCV
            cv::Mat temp(frameorig.rows, frameorig.cols, CV_8UC3, ivcFrame->data);
            temp.copyTo(frameorig);
            
            // Liberar memória
            vc_image_free(ivcFrame);
        }

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
    cv::destroyWindow("Segmentacao HSV");
    std::cout << "Programa terminado.\n";
    return 0;
}