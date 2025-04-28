//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLITÉCNICO DO CÁVADO E DO AVE
//                          2023/2024
//             ENGENHARIA DE SISTEMAS INFORMÁTICOS
//                    VISÃO POR COMPUTADOR
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "vc.h"

// Funções auxiliares para leitura de imagens NetPBM
char *netpbm_get_token(FILE *file, char *tok, int len)
{
    char *t;
    int c;
    
    for (;;)
    {
        while (isspace(c = getc(file)));
        
        if (c != '#') break;
        
        do c = getc(file);
        while ((c != '\n') && (c != EOF));
        
        if (c == EOF) break;
    }
    
    t = tok;
    
    if (c != EOF)
    {
        *t++ = c;
        while (!isspace(c = getc(file)) && (c != '#') && (c != EOF) && (t - tok < len - 1))
            *t++ = c;
        
        if (c == '#') ungetc(c, file);
    }
    
    *t = 0;
    
    return tok;
}

// Função para alocar memória para uma nova imagem
IVC* vc_image_new(int width, int height, int channels, int levels)
{
    IVC *image = (IVC *) malloc(sizeof(IVC));
    
    if (image == NULL) return NULL;
    if ((width <= 0) || (height <= 0) || (channels <= 0) || (levels <= 0)) return NULL;
    
    image->width = width;
    image->height = height;
    image->channels = channels;
    image->levels = levels;
    image->bytesperline = width * channels;
    image->data = (unsigned char *) malloc(image->bytesperline * height);
    
    if (image->data == NULL)
    {
        free(image);
        return NULL;
    }
    
    return image;
}

// Função para liberar a memória de uma imagem
IVC* vc_image_free(IVC* image)
{
    if (image != NULL)
    {
        if (image->data != NULL) free(image->data);
        free(image);
    }
    
    return NULL;
}

// Função para ler uma imagem nos formatos PBM, PGM ou PPM
IVC* vc_read_image(char* filename)
{
    FILE *file = NULL;
    IVC *image = NULL;
    unsigned char *tmp;
    char tok[20];
    int channels, levels;
    int width, height;
    int i, v;
    
    // Abre o arquivo para leitura em modo binário
    if ((file = fopen(filename, "rb")) != NULL)
    {
        // Lê o tipo de arquivo
        netpbm_get_token(file, tok, sizeof(tok));
        
        if (strcmp(tok, "P4") == 0) { channels = 1; levels = 1; } // PBM (binário)
        else if (strcmp(tok, "P5") == 0) { channels = 1; levels = 255; } // PGM (grayscale)
        else if (strcmp(tok, "P6") == 0) { channels = 3; levels = 255; } // PPM (RGB)
        else
        {
#ifdef VC_DEBUG
            printf("ERROR -> vc_read_image():\n\tFile format is not supported!\n");
#endif
            fclose(file);
            return NULL;
        }
        
        // Lê a largura da imagem
        if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1)
        {
#ifdef VC_DEBUG
            printf("ERROR -> vc_read_image():\n\tInvalid width!\n");
#endif
            fclose(file);
            return NULL;
        }
        
        // Lê a altura da imagem
        if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1)
        {
#ifdef VC_DEBUG
            printf("ERROR -> vc_read_image():\n\tInvalid height!\n");
#endif
            fclose(file);
            return NULL;
        }
        
        // Lê o valor máximo de intensidade (no caso de PGM e PPM)
        if (levels == 255)
        {
            if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &levels) != 1)
            {
#ifdef VC_DEBUG
                printf("ERROR -> vc_read_image():\n\tInvalid maximum intensity level!\n");
#endif
                fclose(file);
                return NULL;
            }
        }
        
        // Aloca memória para a imagem
        image = vc_image_new(width, height, channels, levels);
        if (image == NULL)
        {
#ifdef VC_DEBUG
            printf("ERROR -> vc_read_image():\n\tOut of memory!\n");
#endif
            fclose(file);
            return NULL;
        }
        
        // Lê os dados da imagem
        if (levels == 1)
        {
            // Imagem binária (PBM)
            int bytesperline = (image->width + 7) / 8;
            tmp = (unsigned char *) malloc(bytesperline);
            if (tmp == NULL)
            {
#ifdef VC_DEBUG
                printf("ERROR -> vc_read_image():\n\tOut of memory!\n");
#endif
                fclose(file);
                vc_image_free(image);
                return NULL;
            }
            
            for (i = 0; i < image->height; i++)
            {
                if (fread(tmp, 1, bytesperline, file) != bytesperline)
                {
#ifdef VC_DEBUG
                    printf("ERROR -> vc_read_image():\n\tError reading PBM file!\n");
#endif
                    fclose(file);
                    free(tmp);
                    vc_image_free(image);
                    return NULL;
                }
                
                for (v = 0; v < image->width; v++)
                {
                    if (tmp[v / 8] & (1 << (7 - v % 8)))
                        image->data[v + i * image->bytesperline] = 0;
                    else
                        image->data[v + i * image->bytesperline] = 1;
                }
            }
            
            free(tmp);
        }
        else
        {
            // Imagem grayscale (PGM) ou RGB (PPM)
            if (fread(image->data, image->bytesperline, image->height, file) != image->height)
            {
#ifdef VC_DEBUG
                printf("ERROR -> vc_read_image():\n\tError reading PGM/PPM file!\n");
#endif
                fclose(file);
                vc_image_free(image);
                return NULL;
            }
        }
        
        fclose(file);
        return image;
    }
    
#ifdef VC_DEBUG
    printf("ERROR -> vc_read_image():\n\tFile not found!\n");
#endif
    return NULL;
}

// Função para escrever uma imagem nos formatos PBM, PGM ou PPM
int vc_write_image(char* filename, IVC* image)
{
    FILE *file = NULL;
    unsigned char *tmp;
    int i, v;
    int bytesperline;
    int sizeofbinarydata;
    
    // Verifica se a imagem é válida
    if (image == NULL) return 0;
    
    // Abre o arquivo para escrita em modo binário
    if ((file = fopen(filename, "wb")) != NULL)
    {
        // Escreve o cabeçalho do arquivo
        if (image->levels == 1)
        {
            // Imagem binária (PBM)
            fprintf(file, "P4\n%d %d\n", image->width, image->height);
            
            sizeofbinarydata = (image->width + 7) / 8 * image->height;
            tmp = (unsigned char *) malloc(sizeofbinarydata);
            if (tmp == NULL)
            {
#ifdef VC_DEBUG
                printf("ERROR -> vc_write_image():\n\tOut of memory!\n");
#endif
                fclose(file);
                return 0;
            }
            
            // Converte os dados para o formato PBM
            bytesperline = (image->width + 7) / 8;
            for (i = 0; i < image->height; i++)
            {
                for (v = 0; v < image->width; v++)
                {
                    if (image->data[v + i * image->bytesperline] == 0)
                        tmp[v / 8 + i * bytesperline] |= (1 << (7 - v % 8));
                    else
                        tmp[v / 8 + i * bytesperline] &= ~(1 << (7 - v % 8));
                }
            }
            
            // Escreve os dados no arquivo
            if (fwrite(tmp, bytesperline, image->height, file) != image->height)
            {
#ifdef VC_DEBUG
                printf("ERROR -> vc_write_image():\n\tError writing PBM file!\n");
#endif
                fclose(file);
                free(tmp);
                return 0;
            }
            
            free(tmp);
        }
        else
        {
            // Imagem grayscale (PGM) ou RGB (PPM)
            if (image->channels == 1)
                fprintf(file, "P5\n%d %d\n%d\n", image->width, image->height, image->levels);
            else
                fprintf(file, "P6\n%d %d\n%d\n", image->width, image->height, image->levels);
            
            // Escreve os dados no arquivo
            if (fwrite(image->data, image->bytesperline, image->height, file) != image->height)
            {
#ifdef VC_DEBUG
                printf("ERROR -> vc_write_image():\n\tError writing PGM/PPM file!\n");
#endif
                fclose(file);
                return 0;
            }
        }
        
        fclose(file);
        return 1;
    }
    
#ifdef VC_DEBUG
    printf("ERROR -> vc_write_image():\n\tFile not opened for writing!\n");
#endif
    return 0;
}




int vc_rgb_to_gray(IVC *src, IVC *dst)
{
    if ((src == NULL) || (dst == NULL)) return 0;
    if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != 3) || (dst->channels != 1)) return 0;

    for (int y = 0; y < src->height; y++)
    {
        for (int x = 0; x < src->width; x++)
        {
            int pos_src = y * src->bytesperline + x * src->channels;
            int pos_dst = y * dst->bytesperline + x;

            unsigned char r = src->data[pos_src];
            unsigned char g = src->data[pos_src + 1];
            unsigned char b = src->data[pos_src + 2];

            dst->data[pos_dst] = (unsigned char)((r * 0.299) + (g * 0.587) + (b * 0.114));
        }
    }

    return 1;
}

int vc_gray_to_binary_global_mean(IVC *src, IVC *dst)
{
    if ((src == NULL) || (dst == NULL)) return 0;
    if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != 1) || (dst->channels != 1)) return 0;

    long sum = 0;
    int total = src->width * src->height;

    for (int i = 0; i < total; i++)
        sum += src->data[i];

    /* int threshold = sum / total; */
    int threshold = (sum / total) - 30;

    for (int i = 0; i < total; i++)
        dst->data[i] = (src->data[i] >= threshold) ? 255 : 0;

    return 1;
}

int vc_binary_dilate(IVC *src, IVC *dst, int kernel_size)
{
    if (!src || !dst || kernel_size <= 0) return 0;

    int offset = kernel_size / 2;

    for (int y = 0; y < src->height; y++)
    {
        for (int x = 0; x < src->width; x++)
        {
            int found_white = 0;

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
                            found_white = 1;
                            break;
                        }
                    }
                }
                if (found_white) break;
            }

            dst->data[y * dst->bytesperline + x] = found_white ? 255 : 0;
        }
    }

    return 1;
}

int vc_binary_erode(IVC *src, IVC *dst, int kernel_size)
{
    if (!src || !dst || kernel_size <= 0) return 0;

    int offset = kernel_size / 2;

    for (int y = 0; y < src->height; y++)
    {
        for (int x = 0; x < src->width; x++)
        {
            int all_white = 1;

            for (int ky = -offset; ky <= offset; ky++)
            {
                for (int kx = -offset; kx <= offset; kx++)
                {
                    int nx = x + kx;
                    int ny = y + ky;

                    if (nx >= 0 && nx < src->width && ny >= 0 && ny < src->height)
                    {
                        if (src->data[ny * src->bytesperline + nx] != 255)
                        {
                            all_white = 0;
                            break;
                        }
                    }
                }
                if (!all_white) break;
            }

            dst->data[y * dst->bytesperline + x] = all_white ? 255 : 0;
        }
    }

    return 1;
}

int vc_gray_to_binary_adaptive_mean(IVC *src, IVC *dst, int windowSize, int offset)
{
    int halfWindow = windowSize / 2;

    if ((src == NULL) || (dst == NULL)) return 0;
    if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != 1) || (dst->channels != 1)) return 0;

    for (int y = halfWindow; y < src->height - halfWindow; y++)
    {
        for (int x = halfWindow; x < src->width - halfWindow; x++)
        {
            int sum = 0;
            for (int wy = -halfWindow; wy <= halfWindow; wy++)
            {
                for (int wx = -halfWindow; wx <= halfWindow; wx++)
                {
                    sum += src->data[(y + wy) * src->bytesperline + (x + wx)];
                }
            }
            int area = windowSize * windowSize;
            int localMean = sum / area;

            int pixel = src->data[y * src->bytesperline + x];
            dst->data[y * dst->bytesperline + x] = (pixel < (localMean - offset)) ? 0 : 255;
        }
    }

    return 1;
}


int vc_gray_gaussian_blur(IVC *src, IVC *dst)
{
    int kernelSize = 5;
    int halfKernel = kernelSize / 2;
    float gaussianKernel[5][5] = {
        {1, 4, 7, 4, 1},
        {4, 16, 26, 16, 4},
        {7, 26, 41, 26, 7},
        {4, 16, 26, 16, 4},
        {1, 4, 7, 4, 1}
    };
    float sumKernel = 273.0f;

    if ((src == NULL) || (dst == NULL)) return 0;
    if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != 1) || (dst->channels != 1)) return 0;

    for (int y = halfKernel; y < src->height - halfKernel; y++)
    {
        for (int x = halfKernel; x < src->width - halfKernel; x++)
        {
            float sum = 0.0f;
            for (int ky = -halfKernel; ky <= halfKernel; ky++)
            {
                for (int kx = -halfKernel; kx <= halfKernel; kx++)
                {
                    int pixel = src->data[(y + ky) * src->bytesperline + (x + kx)];
                    sum += pixel * gaussianKernel[ky + halfKernel][kx + halfKernel];
                }
            }
            dst->data[y * dst->bytesperline + x] = (unsigned char)(sum / sumKernel);
        }
    }

    return 1;
}
