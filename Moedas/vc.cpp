#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <opencv2/core.hpp>
#include "vc.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// FUNÇÕES: ALOCAR E LIBERTAR UMA IMAGEM (ADAPTADAS DO VC.C ORIGINAL)
IVC* vc_image_new(int width, int height, int channels, int levels) {
    IVC* image = (IVC*)malloc(sizeof(IVC));
    if (image == NULL) return NULL;
    if ((width <= 0) || (height <= 0) || (channels <= 0) || (levels <= 0)) return NULL;

    image->width = width;
    image->height = height;
    image->channels = channels;
    image->levels = levels;
    image->bytesperline = width * channels;
    image->data = (unsigned char*)malloc(image->width * image->height * image->channels * sizeof(char));

    if (image->data == NULL) {
        free(image);
        return NULL;
    }
    return image;
}

IVC* vc_image_free(IVC* image) {
    if (image != NULL) {
        if (image->data != NULL) free(image->data);
        free(image);
    }
    return NULL;
}

// FUNÇÃO PARA CONVERTER DE BGR PARA HSV
int vc_bgr_to_hsv(IVC* src, IVC* dst) {
    unsigned char* datasrc = (unsigned char*)src->data;
    unsigned char* datadst = (unsigned char*)dst->data;
    int width = src->width;
    int height = src->height;
    int bytesperline_src = src->bytesperline;
    int bytesperline_dst = dst->bytesperline;
    int channels_src = src->channels;
    int channels_dst = dst->channels;
    float r, g, b, h, s, v;
    float rgb_max, rgb_min;
    int i, j;

    // Verificação de erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if ((src->width != dst->width) || (src->height != dst->height)) return 0;
    if ((src->channels != 3) || (dst->channels != 3)) return 0;

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            b = (float)datasrc[i * bytesperline_src + j * channels_src + 0];
            g = (float)datasrc[i * bytesperline_src + j * channels_src + 1];
            r = (float)datasrc[i * bytesperline_src + j * channels_src + 2];

            b /= 255.0f;
            g /= 255.0f;
            r /= 255.0f;

            // Valor (v)
            rgb_max = MAX(r, MAX(g, b));
            rgb_min = MIN(r, MIN(g, b));
            v = rgb_max;

            // Saturação (s)
            if (v == 0.0f) s = 0.0f;
            else s = (rgb_max - rgb_min) / rgb_max;

            // Matiz (h)
            if (s == 0.0f) h = 0.0f;
            else {
                if (rgb_max == r) h = 60.0f * (0.0f + (g - b) / (rgb_max - rgb_min));
                else if (rgb_max == g) h = 60.0f * (2.0f + (b - r) / (rgb_max - rgb_min));
                else h = 60.0f * (4.0f + (r - g) / (rgb_max - rgb_min));
            }

            if (h < 0.0f) h += 360.0f;
            
            // Normalizar para os intervalos do OpenCV (para compatibilidade)
            h /= 2.0f;    // [0..180]
            s *= 255.0f;  // [0..255]
            v *= 255.0f;  // [0..255]

            datadst[i * bytesperline_dst + j * channels_dst + 0] = (unsigned char)h;
            datadst[i * bytesperline_dst + j * channels_dst + 1] = (unsigned char)s;
            datadst[i * bytesperline_dst + j * channels_dst + 2] = (unsigned char)v;
        }
    }
    return 1;
}

int idBlobs(cv::Mat frameIn, cv::Mat frameOut, int hueMin, int hueMax, float satMin, float satMax, int valueMin, int valueMax) {
    if (frameIn.empty() || frameOut.empty() || frameIn.cols <= 0 || frameIn.rows <= 0 || frameIn.channels() != 3 ||
        frameIn.cols != frameOut.cols || frameIn.rows != frameOut.rows) return 0;
    
    // Criar imagens IVC a partir das matrizes OpenCV
    IVC* ivcIn = cv_mat_to_ivc(frameIn);
    IVC* ivcHsv = vc_image_new(frameIn.cols, frameIn.rows, 3, 255);  // Imagem HSV
    IVC* ivcOut = vc_image_new(frameIn.cols, frameIn.rows, 1, 255);  // Imagem binária
    
    if (ivcIn == NULL || ivcHsv == NULL || ivcOut == NULL) {
        if (ivcIn) vc_image_free(ivcIn);
        if (ivcHsv) vc_image_free(ivcHsv);
        if (ivcOut) vc_image_free(ivcOut);
        return 0;
    }
    
    // Converter para HSV
    if (!vc_bgr_to_hsv(ivcIn, ivcHsv)) {
        vc_image_free(ivcIn);
        vc_image_free(ivcHsv);
        vc_image_free(ivcOut);
        return 0;
    }
    
    // Segmentar baseado nos valores HSV
    if (!vc_hsv_segmentation(ivcHsv, ivcOut, hueMin, hueMax, (int)satMin, (int)satMax, valueMin, valueMax)) {
        vc_image_free(ivcIn);
        vc_image_free(ivcHsv);
        vc_image_free(ivcOut);
        return 0;
    }
    
    // Copiar resultado para a matriz de saída
    cv::Mat temp(frameOut.rows, frameOut.cols, CV_8UC1, ivcOut->data);
    temp.copyTo(frameOut);
    
    // Liberar memória
    vc_image_free(ivcIn);
    vc_image_free(ivcHsv);
    vc_image_free(ivcOut);
    
    return 1;
}

int verificaPassouAntes(OVC* passou, OVC moedas, int cont) {
    if (cont == 0) return 1;
    return (moedas.xc < passou[cont - 1].xc - 10 || moedas.xc > passou[cont - 1].xc + 10) ? 1 : 0;
}


int idMoeda(int area, int perimeter, float circularity, cv::Vec3b meanColor) {
    
    if (area > 35000 || area < 5000) return 0;
    if (circularity < 0.05) return 0;
 
    if (area >= 22000 && area < 29000 && perimeter >= 700 && perimeter < 1300) return 200;

    else if (area >= 10500 && area < 24000 && perimeter >= 660 && perimeter < 1500) return 100;

    else if (area >= 24000 && area < 27500 && perimeter >= 550 && perimeter < 700) return 50;

    else if (area >= 19000 && area < 22000 && perimeter >= 510 && perimeter < 670) return 20;
 
    else if (area >= 16000 && area < 18500 && perimeter >= 435 && perimeter < 620) return 10;

    else if (area >= 17500 && area < 20500 && perimeter >= 450 && perimeter < 600) return 5;

    else if (area >= 12500 && area < 15500 && perimeter >= 410 && perimeter < 570) return 2;
 
    else if (area >= 9600 && area < 12500 && perimeter >= 350 && perimeter < 600) return 1;
    
    else return 0;
}

void escreverInfo(FILE* fp, int cont, int mTotal, int m200, int m100, int m50, int m20, int m10, int m5, int m2, int m1, const char* videofile) {
    if (!fp) return;
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    fprintf(fp, "== VIDEO: %s | HORA: %d-%d-%d %d:%d:%d ==\n", videofile, tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
    fprintf(fp, "NUMERO DE MOEDAS: %d\n", mTotal);
    fprintf(fp, " -2 EUROS: %d\n", m200);
    fprintf(fp, " -1 EURO: %d\n", m100);
    fprintf(fp, " -50 CENT: %d\n", m50);
    fprintf(fp, " -20 CENT: %d\n", m20);
    fprintf(fp, " -10 CENT: %d\n", m10);
    fprintf(fp, " -5 CENT: %d\n", m5);
    fprintf(fp, " -2 CENT: %d\n", m2);
    fprintf(fp, " -1 CENT: %d\n\n", m1);
}

OVC* vc_binary_blob_labelling(cv::Mat src, cv::Mat dst, int* nlabels) {
    if (src.empty() || dst.empty() || src.cols <= 0 || src.rows <= 0 || src.channels() != 1 ||
        src.cols != dst.cols || src.rows != dst.rows || dst.channels() != 1) return NULL;
    
    // Converter as imagens para o formato IVC
    IVC* ivcSrc = vc_image_new(src.cols, src.rows, 1, 255);
    IVC* ivcDst = vc_image_new(dst.cols, dst.rows, 1, 255);
    
    if (ivcSrc == NULL || ivcDst == NULL) {
        if (ivcSrc) vc_image_free(ivcSrc);
        if (ivcDst) vc_image_free(ivcDst);
        return NULL;
    }
    
    // Copiar dados para as imagens IVC
    memcpy(ivcSrc->data, src.data, src.cols * src.rows);
    memcpy(ivcDst->data, dst.data, dst.cols * dst.rows);
    
    // Executar etiquetagem de componentes conectados
    OVC* blobs = vc_component_labelling(ivcSrc, ivcDst, nlabels);
    
    // Copiar resultado de volta para a matriz OpenCV
    cv::Mat temp(dst.rows, dst.cols, CV_8UC1, ivcDst->data);
    temp.copyTo(dst);
    
    // Liberar memória
    vc_image_free(ivcSrc);
    vc_image_free(ivcDst);
    
    return blobs;
}

int vc_binary_blob_info(cv::Mat src, OVC* blobs, int nblobs) {
    if (src.empty() || src.cols <= 0 || src.rows <= 0 || src.channels() != 1) return 0;
    
    // Converter a imagem para o formato IVC
    IVC* ivcSrc = vc_image_new(src.cols, src.rows, 1, 255);
    if (ivcSrc == NULL) return 0;
    
    // Copiar dados para a imagem IVC
    memcpy(ivcSrc->data, src.data, src.cols * src.rows);
    
    // Calcular propriedades dos blobs
    int result = vc_binary_blob_info_ivc(ivcSrc, blobs, nblobs);
    
    // Liberar memória
    vc_image_free(ivcSrc);
    
    return result;
}

int vc_desenha_bounding_box(cv::Mat src, OVC blobs) {
    if (src.empty() || src.cols <= 0 || src.rows <= 0 || src.channels() != 3) return 0;
    
    // Criar imagem IVC a partir da matriz OpenCV
    IVC* ivcSrc = cv_mat_to_ivc(src);
    if (ivcSrc == NULL) return 0;
    
    // Desenhar caixa delimitadora e centro usando funções manuais
    int result = vc_draw_bounding_box(ivcSrc, blobs);
    
    // Copiar resultado de volta para a matriz OpenCV
    cv::Mat temp(src.rows, src.cols, CV_8UC3, ivcSrc->data);
    temp.copyTo(src);
    
    // Liberar memória
    vc_image_free(ivcSrc);
    
    return result;
}

int desenha_linhaVerde(cv::Mat frame) {
    if (frame.empty() || frame.cols <= 0 || frame.rows <= 0 || frame.channels() != 3) return 0;
    
    // Criar imagem IVC a partir da matriz OpenCV
    IVC* ivcFrame = cv_mat_to_ivc(frame);
    if (ivcFrame == NULL) return 0;
    
    // Configurar cor verde
    int color[3] = { 0, 255, 0 };  // BGR
    
    // Desenhar linha verde
    int y = frame.rows / 4;
    int result = vc_draw_line(ivcFrame, 0, y, frame.cols - 1, y, color);
    
    // Copiar resultado de volta para a matriz OpenCV
    cv::Mat temp(frame.rows, frame.cols, CV_8UC3, ivcFrame->data);
    temp.copyTo(frame);
    
    // Liberar memória
    vc_image_free(ivcFrame);
    
    return result;
}

int desenha_linhaVermelha(cv::Mat frame) {
    if (frame.empty() || frame.cols <= 0 || frame.rows <= 0 || frame.channels() != 3) return 0;
    
    // Criar imagem IVC a partir da matriz OpenCV
    IVC* ivcFrame = cv_mat_to_ivc(frame);
    if (ivcFrame == NULL) return 0;
    
    // Configurar cor vermelha
    int color[3] = { 0, 0, 255 };  // BGR
    
    // Desenhar linha vermelha
    int y = frame.rows / 4;
    int result = vc_draw_line(ivcFrame, 0, y, frame.cols - 1, y, color);
    
    // Copiar resultado de volta para a matriz OpenCV
    cv::Mat temp(frame.rows, frame.cols, CV_8UC3, ivcFrame->data);
    temp.copyTo(frame);
    
    // Liberar memória
    vc_image_free(ivcFrame);
    
    return result;
}

// FUNÇÃO PARA SEGMENTAÇÃO HSV
int vc_hsv_segmentation(IVC* src, IVC* dst, int hmin, int hmax, int smin, int smax, int vmin, int vmax) {
    unsigned char* datasrc = (unsigned char*)src->data;
    unsigned char* datadst = (unsigned char*)dst->data;
    int width = src->width;
    int height = src->height;
    int bytesperline_src = src->bytesperline;
    int bytesperline_dst = dst->bytesperline;
    int channels_src = src->channels;
    int channels_dst = dst->channels;
    int h, s, v;
    int i, j;

    // Verificação de erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL) || (dst->data == NULL)) return 0;
    if ((src->width != dst->width) || (src->height != dst->height)) return 0;
    if ((src->channels != 3) || (dst->channels != 1)) return 0;

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            h = (int)datasrc[i * bytesperline_src + j * channels_src + 0];
            s = (int)datasrc[i * bytesperline_src + j * channels_src + 1];
            v = (int)datasrc[i * bytesperline_src + j * channels_src + 2];

            // Verificar se o pixel está dentro dos limites HSV especificados
            if ((h >= hmin && h <= hmax) && (s >= smin && s <= smax) && (v >= vmin && v <= vmax)) {
                datadst[i * bytesperline_dst + j] = 255;
            }
            else {
                datadst[i * bytesperline_dst + j] = 0;
            }
        }
    }
    return 1;
}


// FUNÇÕES DE MORFOLOGIA PARA DILATAÇÃO E EROSÃO

// Dilatação binária (kernel quadrado de tamanho kernel_size)
int vc_dilate(IVC* src, IVC* dst, int kernel_size)
{
    if (!src || !dst || src->channels != 1 || dst->channels != 1) return 0;
    int offset = kernel_size / 2;
    for (int y = 0; y < src->height; y++)
    {
        for (int x = 0; x < src->width; x++)
        {
            int found = 0;
            for (int ky = -offset; ky <= offset; ky++)
            {
                for (int kx = -offset; kx <= offset; kx++)
                {
                    int nx = x + kx;
                    int ny = y + ky;
                    if (nx >= 0 && nx < src->width && ny >= 0 && ny < src->height)
                    {
                        if (src->data[ny * src->bytesperline + nx] == 255)
                        {
                            found = 1;
                            break;
                        }
                    }
                }
                if (found) break;
            }
            dst->data[y * dst->bytesperline + x] = found ? 255 : 0;
        }
    }
    return 1;
}

// Erosão binária (kernel quadrado de tamanho kernel_size)
int vc_erode(IVC* src, IVC* dst, int kernel_size)
{
    if (!src || !dst || src->channels != 1 || dst->channels != 1) return 0;
    int offset = kernel_size / 2;
    for (int y = 0; y < src->height; y++)
    {
        for (int x = 0; x < src->width; x++)
        {
            int all = 1;
            for (int ky = -offset; ky <= offset; ky++)
            {
                for (int kx = -offset; kx <= offset; kx++)
                {
                    int nx = x + kx;
                    int ny = y + ky;
                    if (nx < 0 || nx >= src->width || ny < 0 || ny >= src->height ||
                        src->data[ny * src->bytesperline + nx] != 255)
                    {
                        all = 0;
                        break;
                    }
                }
                if (!all) break;
            }
            dst->data[y * dst->bytesperline + x] = all ? 255 : 0;
        }
    }
    return 1;
}


// FUNÇÃO PARA ETIQUETAGEM DE COMPONENTES CONECTADOS
OVC* vc_component_labelling(IVC* src, IVC* dst, int* nlabels) {
    unsigned char* datasrc = (unsigned char*)src->data;
    unsigned char* datadst = (unsigned char*)dst->data;
    int width = src->width;
    int height = src->height;
    int bytesperline = src->bytesperline;
    int i, j, pixelpos, containsp1;
    int labeltable[256] = { 0 };
    int labelarea[256] = { 0 };
    int label = 1; // Começa de 1, pois 0 é o fundo
    
    // Verificação de erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL) || (dst->data == NULL)) return NULL;
    if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != 1) || (dst->channels != 1)) return NULL;

    // Inicialização da tabela de equivalência
    for (i = 0; i < 256; i++) labeltable[i] = i;

    // Primeiro passo: etiquetagem inicial
    for (i = 1; i < height - 1; i++) {
        for (j = 1; j < width - 1; j++) {
            pixelpos = i * bytesperline + j;
            
            // Se o pixel for parte do objeto (255)
            if (datasrc[pixelpos] == 255) {
                // Verificar pixel acima e à esquerda
                int p1 = datadst[pixelpos - bytesperline]; // pixel acima
                int p2 = datadst[pixelpos - 1];           // pixel à esquerda
                
                // Se nenhum dos vizinhos tem etiqueta, criar nova etiqueta
                if (p1 == 0 && p2 == 0) {
                    if (label < 255) {
                        datadst[pixelpos] = label;
                        labelarea[label]++;
                        label++;
                    }
                }
                // Se apenas um dos vizinhos tem etiqueta, usar essa etiqueta
                else if (p1 != 0 && p2 == 0) {
                    datadst[pixelpos] = p1;
                    labelarea[p1]++;
                }
                else if (p1 == 0 && p2 != 0) {
                    datadst[pixelpos] = p2;
                    labelarea[p2]++;
                }
                // Se ambos os vizinhos têm etiquetas diferentes, usar a menor e registrar equivalência
                else {
                    int minlabel = MIN(p1, p2);
                    int maxlabel = MAX(p1, p2);
                    datadst[pixelpos] = minlabel;
                    labelarea[minlabel]++;
                    
                    if (minlabel != maxlabel) {
                        // Atualizar tabela de equivalência
                        for (int k = 1; k < label; k++) {
                            if (labeltable[k] == maxlabel)
                                labeltable[k] = minlabel;
                        }
                    }
                }
            }
            else {
                datadst[pixelpos] = 0; // fundo
            }
        }
    }

    // Segundo passo: atualizar etiquetas com base na tabela de equivalência
    for (i = 1; i < height - 1; i++) {
        for (j = 1; j < width - 1; j++) {
            pixelpos = i * bytesperline + j;
            if (datadst[pixelpos] != 0) {
                datadst[pixelpos] = labeltable[datadst[pixelpos]];
            }
        }
    }

    // Contar etiquetas únicas e criar nova tabela compacta
    int newlabel = 0;
    int newtable[256] = { 0 };
    
    for (i = 1; i < label; i++) {
        containsp1 = 0;
        for (j = 1; j < i; j++) {
            if (labeltable[i] == labeltable[j]) {
                containsp1 = 1;
                break;
            }
        }
        
        if (!containsp1) {
            newlabel++;
            newtable[labeltable[i]] = newlabel;
        }
    }

    // Terceiro passo: atualizar etiquetas com a nova tabela compacta
    for (i = 1; i < height - 1; i++) {
        for (j = 1; j < width - 1; j++) {
            pixelpos = i * bytesperline + j;
            if (datadst[pixelpos] != 0) {
                datadst[pixelpos] = newtable[datadst[pixelpos]];
            }
        }
    }

    *nlabels = newlabel;
    
    // Alocar estruturas para os blobs
    OVC* blobs = NULL;
    if (newlabel > 0) {
        blobs = (OVC*)calloc(newlabel, sizeof(OVC));
        if (blobs != NULL) {
            for (i = 0; i < newlabel; i++) {
                blobs[i].label = i + 1;
                // Inicializar min e max para encontrar caixa delimitadora
                blobs[i].x = width - 1;
                blobs[i].y = height - 1;
                blobs[i].width = 0;
                blobs[i].height = 0;
                blobs[i].area = 0;
            }
        }
        else return NULL;
    }
    else return NULL;
    
    return blobs;
}

// FUNÇÃO PARA CALCULAR PROPRIEDADES DOS BLOBS (ÁREA, PERÍMETRO, ETC.)
int vc_binary_blob_info_ivc(IVC* src, OVC* blobs, int nblobs) {
    unsigned char* data = (unsigned char*)src->data;
    int width = src->width;
    int height = src->height;
    int bytesperline = src->bytesperline;
    int i, j, pixelpos;
    int label;
    int perimeter;
    int xmin, ymin, xmax, ymax;
    
    // Verificação de erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL) || (blobs == NULL)) return 0;
    if (src->channels != 1) return 0;
    if (nblobs <= 0) return 0;
    
    // Inicialização das estruturas de blobs
    for (i = 0; i < nblobs; i++) {
        blobs[i].x = width - 1;
        blobs[i].y = height - 1;
        blobs[i].width = 0;
        blobs[i].height = 0;
        blobs[i].area = 0;
        blobs[i].perimeter = 0;
        blobs[i].xc = 0;
        blobs[i].yc = 0;
    }
    
    // Primeira passagem: cálculo da área e caixa delimitadora
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            pixelpos = i * bytesperline + j;
            label = data[pixelpos];
            
            if (label != 0) {
                // Índice no array de blobs (as etiquetas começam em 1)
                int idx = label - 1;
                
                if (idx < nblobs) {
                    // Atualizar área
                    blobs[idx].area++;
                    
                    // Atualizar limites da caixa delimitadora
                    if (j < blobs[idx].x) blobs[idx].x = j;
                    if (i < blobs[idx].y) blobs[idx].y = i;
                    if (j > blobs[idx].width) blobs[idx].width = j;
                    if (i > blobs[idx].height) blobs[idx].height = i;
                    
                    // Acumular coordenadas para cálculo do centro de massa
                    blobs[idx].xc += j;
                    blobs[idx].yc += i;
                }
            }
        }
    }
    
    // Calcular centro de massa e ajustar dimensões da caixa delimitadora
    for (i = 0; i < nblobs; i++) {
        if (blobs[i].area > 0) {
            blobs[i].xc /= blobs[i].area;
            blobs[i].yc /= blobs[i].area;
            blobs[i].width = blobs[i].width - blobs[i].x + 1;
            blobs[i].height = blobs[i].height - blobs[i].y + 1;
        }
    }
    
    // Segunda passagem: cálculo do perímetro
    for (i = 1; i < height - 1; i++) {
        for (j = 1; j < width - 1; j++) {
            pixelpos = i * bytesperline + j;
            label = data[pixelpos];
            
            if (label != 0) {
                // Índice no array de blobs
                int idx = label - 1;
                
                if (idx < nblobs) {
                    // Verificar se é um pixel de borda (tem pelo menos um vizinho com etiqueta diferente)
                    if (data[(i - 1) * bytesperline + j] != label ||
                        data[(i + 1) * bytesperline + j] != label ||
                        data[i * bytesperline + (j - 1)] != label ||
                        data[i * bytesperline + (j + 1)] != label) {
                        blobs[idx].perimeter++;
                    }
                }
            }
        }
    }
    
    // Calcular circularidade
    for (i = 0; i < nblobs; i++) {
        if (blobs[i].perimeter > 0) {
            blobs[i].circularity = (4.0 * M_PI * blobs[i].area) / (blobs[i].perimeter * blobs[i].perimeter);
        }
    }
    
    return 1;
}

// FUNÇÃO PARA DESENHAR CAIXA DELIMITADORA
int vc_draw_bounding_box(IVC* src, OVC blobs) {
    unsigned char* data = (unsigned char*)src->data;
    int width = src->width;
    int height = src->height;
    int bytesperline = src->bytesperline;
    int channels = src->channels;
    int i, j;
    
    // Verificação de erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if (channels != 3) return 0;
    
    // Desenhar caixa delimitadora
    int x1 = blobs.x;
    int y1 = blobs.y;
    int x2 = blobs.x + blobs.width - 1;
    int y2 = blobs.y + blobs.height - 1;
    
    // Garantir que estamos dentro dos limites da imagem
    x1 = MAX(0, MIN(width - 1, x1));
    y1 = MAX(0, MIN(height - 1, y1));
    x2 = MAX(0, MIN(width - 1, x2));
    y2 = MAX(0, MIN(height - 1, y2));
    
    // Cor vermelha
    int color[3] = { 0, 0, 255 };
    
    // Desenhar as quatro linhas da caixa
    vc_draw_line(src, x1, y1, x2, y1, color);  // Linha superior
    vc_draw_line(src, x1, y2, x2, y2, color);  // Linha inferior
    vc_draw_line(src, x1, y1, x1, y2, color);  // Linha esquerda
    vc_draw_line(src, x2, y1, x2, y2, color);  // Linha direita
    
    // Desenhar centro de massa
    int radius = (int)(sqrt(blobs.area / M_PI) * 0.1);  // 10% do raio estimado
    radius = MAX(2, MIN(10, radius));  // Entre 2 e 10 pixels
    
    int cx = blobs.xc;
    int cy = blobs.yc;
    
    // Desenhar círculo para representar o centro de massa
    vc_draw_circle(src, cx, cy, radius, color, 1);
    
    return 1;
}

// FUNÇÃO PARA DESENHAR LINHAS
int vc_draw_line(IVC* src, int x1, int y1, int x2, int y2, int color[3]) {
    unsigned char* data = (unsigned char*)src->data;
    int width = src->width;
    int height = src->height;
    int bytesperline = src->bytesperline;
    int channels = src->channels;
    int i, j;
    
    // Verificação de erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if (channels != 3) return 0;
    
    // Garantir que estamos dentro dos limites da imagem
    x1 = MAX(0, MIN(width - 1, x1));
    y1 = MAX(0, MIN(height - 1, y1));
    x2 = MAX(0, MIN(width - 1, x2));
    y2 = MAX(0, MIN(height - 1, y2));
    
    // Algoritmo de Bresenham para desenho de linhas
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    int e2;
    
    while (1) {
        // Desenhar pixel atual
        if (x1 >= 0 && x1 < width && y1 >= 0 && y1 < height) {
            data[y1 * bytesperline + x1 * channels + 0] = color[0];  // B
            data[y1 * bytesperline + x1 * channels + 1] = color[1];  // G
            data[y1 * bytesperline + x1 * channels + 2] = color[2];  // R
        }
        
        if (x1 == x2 && y1 == y2) break;
        
        e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
    
    return 1;
}

// FUNÇÃO PARA DESENHAR CÍRCULOS
int vc_draw_circle(IVC* src, int xc, int yc, int radius, int color[3], int fill) {
    unsigned char* data = (unsigned char*)src->data;
    int width = src->width;
    int height = src->height;
    int bytesperline = src->bytesperline;
    int channels = src->channels;
    int x, y, p;
    
    // Verificação de erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if (channels != 3) return 0;
    
    // Algoritmo de Bresenham para círculos
    x = 0;
    y = radius;
    p = 3 - 2 * radius;
    
    while (x <= y) {
        if (fill) {
            // Preencher círculo desenhando linhas horizontais
            for (int i = xc - x; i <= xc + x; i++) {
                if (i >= 0 && i < width) {
                    if (yc + y >= 0 && yc + y < height) {
                        data[(yc + y) * bytesperline + i * channels + 0] = color[0];
                        data[(yc + y) * bytesperline + i * channels + 1] = color[1];
                        data[(yc + y) * bytesperline + i * channels + 2] = color[2];
                    }
                    if (yc - y >= 0 && yc - y < height) {
                        data[(yc - y) * bytesperline + i * channels + 0] = color[0];
                        data[(yc - y) * bytesperline + i * channels + 1] = color[1];
                        data[(yc - y) * bytesperline + i * channels + 2] = color[2];
                    }
                }
            }
            
            for (int i = xc - y; i <= xc + y; i++) {
                if (i >= 0 && i < width) {
                    if (yc + x >= 0 && yc + x < height) {
                        data[(yc + x) * bytesperline + i * channels + 0] = color[0];
                        data[(yc + x) * bytesperline + i * channels + 1] = color[1];
                        data[(yc + x) * bytesperline + i * channels + 2] = color[2];
                    }
                    if (yc - x >= 0 && yc - x < height) {
                        data[(yc - x) * bytesperline + i * channels + 0] = color[0];
                        data[(yc - x) * bytesperline + i * channels + 1] = color[1];
                        data[(yc - x) * bytesperline + i * channels + 2] = color[2];
                    }
                }
            }
        }
        else {
            // Desenhar apenas os pontos do círculo
            if (xc + x >= 0 && xc + x < width && yc + y >= 0 && yc + y < height) {
                data[(yc + y) * bytesperline + (xc + x) * channels + 0] = color[0];
                data[(yc + y) * bytesperline + (xc + x) * channels + 1] = color[1];
                data[(yc + y) * bytesperline + (xc + x) * channels + 2] = color[2];
            }
            if (xc - x >= 0 && xc - x < width && yc + y >= 0 && yc + y < height) {
                data[(yc + y) * bytesperline + (xc - x) * channels + 0] = color[0];
                data[(yc + y) * bytesperline + (xc - x) * channels + 1] = color[1];
                data[(yc + y) * bytesperline + (xc - x) * channels + 2] = color[2];
            }
            if (xc + x >= 0 && xc + x < width && yc - y >= 0 && yc - y < height) {
                data[(yc - y) * bytesperline + (xc + x) * channels + 0] = color[0];
                data[(yc - y) * bytesperline + (xc + x) * channels + 1] = color[1];
                data[(yc - y) * bytesperline + (xc + x) * channels + 2] = color[2];
            }
            if (xc - x >= 0 && xc - x < width && yc - y >= 0 && yc - y < height) {
                data[(yc - y) * bytesperline + (xc - x) * channels + 0] = color[0];
                data[(yc - y) * bytesperline + (xc - x) * channels + 1] = color[1];
                data[(yc - y) * bytesperline + (xc - x) * channels + 2] = color[2];
            }
            if (xc + y >= 0 && xc + y < width && yc + x >= 0 && yc + x < height) {
                data[(yc + x) * bytesperline + (xc + y) * channels + 0] = color[0];
                data[(yc + x) * bytesperline + (xc + y) * channels + 1] = color[1];
                data[(yc + x) * bytesperline + (xc + y) * channels + 2] = color[2];
            }
            if (xc - y >= 0 && xc - y < width && yc + x >= 0 && yc + x < height) {
                data[(yc + x) * bytesperline + (xc - y) * channels + 0] = color[0];
                data[(yc + x) * bytesperline + (xc - y) * channels + 1] = color[1];
                data[(yc + x) * bytesperline + (xc - y) * channels + 2] = color[2];
            }
            if (xc + y >= 0 && xc + y < width && yc - x >= 0 && yc - x < height) {
                data[(yc - x) * bytesperline + (xc + y) * channels + 0] = color[0];
                data[(yc - x) * bytesperline + (xc + y) * channels + 1] = color[1];
                data[(yc - x) * bytesperline + (xc + y) * channels + 2] = color[2];
            }
            if (xc - y >= 0 && xc - y < width && yc - x >= 0 && yc - x < height) {
                data[(yc - x) * bytesperline + (xc - y) * channels + 0] = color[0];
                data[(yc - x) * bytesperline + (xc - y) * channels + 1] = color[1];
                data[(yc - x) * bytesperline + (xc - y) * channels + 2] = color[2];
            }
        }
        
        if (p < 0) {
            p += 4 * x + 6;
        }
        else {
            p += 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
    
    return 1;
}

// FUNÇÃO PARA RENDERIZAR TEXTO (IMPLEMENTAÇÃO MELHORADA)
int vc_put_text(IVC* src, const char* text, int x, int y, int color[3], int fontsize) {
    unsigned char* data = (unsigned char*)src->data;
    int width = src->width;
    int height = src->height;
    int bytesperline = src->bytesperline;
    int channels = src->channels;
    int i, j, k;
    
    // Verificação de erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if (channels != 3) return 0;
    if (text == NULL) return 0;
    
    // Calcular tamanho do texto
    int textlen = strlen(text);
    int charwidth = fontsize * 5;     // Tornar os caracteres mais largos
    int charheight = fontsize * 7;    // Tornar os caracteres mais altos
    int spacing = fontsize;           // Espaçamento entre caracteres
    
    // Arrays de representação de caracteres (baseados em grid 5x7)
    // Cada bit representa um pixel do caractere (1 = desenhar, 0 = não desenhar)
    // Implementamos apenas alguns caracteres básicos para demonstração
    const unsigned char chars[128][7] = {
        // Espaço (32)
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        // ! (33)
        {0x04, 0x04, 0x04, 0x04, 0x00, 0x04, 0x00},
        // " (34)
        {0x0A, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00},
        // # (35)
        {0x0A, 0x0A, 0x1F, 0x0A, 0x1F, 0x0A, 0x0A},
        // $ (36)
        {0x04, 0x0F, 0x14, 0x0E, 0x05, 0x1E, 0x04},
        // % (37)
        {0x18, 0x19, 0x02, 0x04, 0x08, 0x13, 0x03},
        // & (38)
        {0x0C, 0x12, 0x0C, 0x0D, 0x12, 0x0D, 0x00},
        // ' (39)
        {0x04, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00},
        // ( (40)
        {0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02},
        // ) (41)
        {0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08},
        // * (42)
        {0x00, 0x04, 0x15, 0x0E, 0x15, 0x04, 0x00},
        // + (43)
        {0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00},
        // , (44)
        {0x00, 0x00, 0x00, 0x00, 0x0C, 0x04, 0x08},
        // - (45)
        {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00},
        // . (46)
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C},
        // / (47)
        {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x00},
        // 0-9 (48-57)
        {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E},
        {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E},
        {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F},
        {0x0E, 0x11, 0x01, 0x06, 0x01, 0x11, 0x0E},
        {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02},
        {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E},
        {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E},
        {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08},
        {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E},
        {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C},
        // : (58)
        {0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00},
        // ; (59)
        {0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x04, 0x08},
        // < (60)
        {0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02},
        // = (61)
        {0x00, 0x00, 0x1F, 0x00, 0x1F, 0x00, 0x00},
        // > (62)
        {0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08},
        // ? (63)
        {0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04},
        // @ (64)
        {0x0E, 0x11, 0x01, 0x0D, 0x15, 0x15, 0x0E},
        // A-Z (65-90)
        {0x04, 0x0A, 0x11, 0x11, 0x1F, 0x11, 0x11},
        {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E},
        {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E},
        {0x1C, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1C},
        {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F},
        {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10},
        {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F},
        {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},
        {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E},
        {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C},
        {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11},
        {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F},
        {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11},
        {0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11},
        {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
        {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10},
        {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D},
        {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11},
        {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E},
        {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04},
        {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
        {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04},
        {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A},
        {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11},
        {0x11, 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04},
        {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F},
    };
    
    // Para cada caractere do texto
    for (k = 0; k < textlen; k++) {
        char c = text[k];
        // Calcular posição do início do caractere
        int x1 = x + k * (charwidth + spacing);
        int y1 = y;
        
        // Verificar se o caractere está dentro dos limites suportados
        if (c >= 32 && c < 123) {
            // Obter índice no array de chars (ajustado para ASCII)
            int idx = c - 32;
            
            // Desenhar o caractere
            for (j = 0; j < 7; j++) {
                for (i = 0; i < 5; i++) {
                    // Verificar se o pixel deve ser desenhado
                    if ((chars[idx][j] >> (4-i)) & 0x01) {
                        // Calcular posição real do pixel com escala
                        int py = y1 + j * fontsize;
                        int px = x1 + i * fontsize;
                        
                        // Desenhar um pixel quadrado com tamanho fontsize x fontsize
                        for (int sy = 0; sy < fontsize; sy++) {
                            for (int sx = 0; sx < fontsize; sx++) {
                                int fx = px + sx;
                                int fy = py + sy;
                                
                                if (fx >= 0 && fx < width && fy >= 0 && fy < height) {
                                    data[fy * bytesperline + fx * channels + 0] = color[0];
                                    data[fy * bytesperline + fx * channels + 1] = color[1];
                                    data[fy * bytesperline + fx * channels + 2] = color[2];
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    return 1;
}

// FUNÇÕES DE CONVERSÃO ENTRE CV::MAT E IVC
IVC* cv_mat_to_ivc(cv::Mat src) {
    IVC* ivc = NULL;
    
    if (!src.empty() && src.cols > 0 && src.rows > 0) {
        int channels = src.channels();
        ivc = vc_image_new(src.cols, src.rows, channels, 255);
        
        if (ivc) {
            memcpy(ivc->data, src.data, src.cols * src.rows * channels);
        }
    }
    
    return ivc;
}

