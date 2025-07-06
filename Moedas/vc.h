#ifndef VC_DEBUG
#define VC_DEBUG
#endif

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#ifndef MAX
#define MAX(a,b) (a > b ? a : b)
#endif
#ifndef MIN
#define MIN(a,b) (a < b ? a : b)
#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DE UMA IMAGEM 
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Estrutura que representa uma imagem genérica (binária, cinzento ou cor)
typedef struct {
    unsigned char* data;   // Ponteiro para os dados da imagem
    int width, height;     // Largura e altura
    int channels;          // Nº de canais: Binário/Cinzento=1; RGB=3
    int levels;            // Nº de níveis: Binário=1; Cinzento [1,255]; RGB [1,255]
    int bytesperline;      // Nº de bytes por linha (width * channels)
} IVC;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DE UM OBJETO (BLOB)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Estrutura que representa um objeto detetado (moeda, etc.)
typedef struct {
    int x, y, width, height;  // Caixa delimitadora (bounding box)
    int area;                 // Área do objeto
    int perimeter;            // Perímetro do objeto
    int xc, yc;               // Centro de massa
    int label;                // Etiqueta do blob
    int tipo;                 // Tipo/classificação (ex: valor da moeda)
    double circularity;       // Circularidade
} OVC;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DE UM VECTOR
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Estrutura para guardar cor média (BGR)
typedef struct {
    unsigned char b, g, r;
} VEC3UC;

// Funções para alocar e libertar imagens IVC
IVC* vc_image_new(int width, int height, int channels, int levels);
IVC* vc_image_free(IVC* image);

// Funções de conversão e segmentação de cor
int vc_bgr_to_hsv(IVC* src, IVC* dst);
int vc_hsv_segmentation(IVC* src, IVC* dst, int hmin, int hmax, int smin, int smax, int vmin, int vmax);

// Funções de análise de blobs (etiquetagem e propriedades)
OVC* vc_binary_blob_labelling(cv::Mat src, cv::Mat dst, int* nlabels);
int vc_binary_blob_info(cv::Mat src, OVC* blobs, int nblobs);
OVC* vc_component_labelling(IVC* src, IVC* dst, int* nlabels);
int vc_binary_blob_info_ivc(IVC* src, OVC* blobs, int nblobs);

// Funções de desenho e visualização
int vc_desenha_bounding_box(cv::Mat src, OVC blobs); // Desenha bounding box e centro
int vc_draw_bounding_box(IVC* src, OVC blobs);       // Idem, mas sobre IVC
int vc_draw_line(IVC* src, int x1, int y1, int x2, int y2, int color[3]); // Linha
int vc_draw_circle(IVC* src, int xc, int yc, int radius, int color[3], int fill); // Círculo

// Funções de morfologia (dilatação e erosão)
int vc_dilate(IVC* src, IVC* dst, int kernel_size);
int vc_erode(IVC* src, IVC* dst, int kernel_size);

// Funções auxiliares para o projeto
int desenha_linhaVermelha(cv::Mat frame); // Linha de referência
int desenha_linhaVerde(cv::Mat frame);    // Linha de validação
int idBlobs(cv::Mat frameIn, cv::Mat frameOut, int hueMin, int hueMax, float satMin, float satMax, int valueMin, int valueMax); // Segmentação HSV
int verificaPassouAntes(OVC* passou, OVC moedas, int cont); // Evita contar moedas repetidas
int idMoeda(int area, int perimeter, float circularity, VEC3UC meanColor); // Classificação da moeda
void escreverInfo(FILE* fp, int cont, int mTotal, int m200, int m100, int m50, int m20, int m10, int m5, int m2, int m1, const char* videofile); // Guarda estatísticas
void mediaCorROI(const IVC* ivcImg, int x, int y, int width, int height, VEC3UC* meanColor); // Cor média de ROI
void vc_timer(void); // Cronómetro para tempo total

// Conversão entre cv::Mat (OpenCV) e IVC
IVC* cv_mat_to_ivc(cv::Mat src);

