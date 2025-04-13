#ifndef VC_H
#define VC_H

#define VC_DEBUG

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DE UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
typedef struct {
    unsigned char* data;
    int width, height;
    int channels;       // Binário/Cinzentos=1; RGB=3
    int levels;         // Binário=1; Cinzentos [1,255]; RGB [1,255]
    int bytesperline;   // width * channels
} IVC;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                    PROTÓTIPOS DE FUNÇÕES
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// FUNÇÕES: ALOCAR E LIBERTAR UMA IMAGEM
IVC* vc_image_new(int width, int height, int channels, int levels);
IVC* vc_image_free(IVC* image);

// FUNÇÕES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
IVC* vc_read_image(char* filename);
int vc_write_image(char* filename, IVC* image);

// FUNÇÕES: TRANSFORMAÇÕES DE IMAGENS
int vc_gray_negative(IVC* srcdst);
int vc_rgb_to_gray(IVC* src, IVC* dst);
int vc_rgb_to_hsv(IVC* src, IVC* dst);
int vc_hsv_segmentation(IVC* src, IVC* dst, int hmin, int hmax, int smin, int smax, int vmin, int vmax);

// FUNÇÕES: OPERAÇÕES SOBRE CANAIS DE COR
int vc_rgb_get_red_channel(IVC* src, IVC* dst);
int vc_rgb_get_green_channel(IVC* src, IVC* dst);
int vc_rgb_get_blue_channel(IVC* src, IVC* dst);

// FUNÇÕES: BINARIZAÇÃO
int vc_gray_to_binary(IVC* src, IVC* dst, int threshold);
int vc_gray_to_binary_global_mean(IVC* src, IVC* dst);

// FUNÇÕES: OPERAÇÕES MORFOLÓGICAS
int vc_binary_dilate(IVC* src, IVC* dst, int kernel_size);
int vc_binary_erode(IVC* src, IVC* dst, int kernel_size);

#endif
