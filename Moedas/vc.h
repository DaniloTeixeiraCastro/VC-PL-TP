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
//                   ESTRUTURA DE UMA IMAGEM MANUAL
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
typedef struct {
    unsigned char* data;
    int width, height;
    int channels;       // Binário/Cinzentos=1; RGB=3
    int levels;         // Binário=1; Cinzentos [1,255]; RGB [1,255]
    int bytesperline;   // width * channels
} IVC;

typedef struct {
    int x, y, width, height;  
    int area;                 
    int perimeter;            
    int xc, yc;               
    int label;                
    int tipo;                 
    double circularity;       
} OVC;

// FUNÇÕES: ALOCAR E LIBERTAR UMA IMAGEM
IVC* vc_image_new(int width, int height, int channels, int levels);
IVC* vc_image_free(IVC* image);

// FUNÇÕES: CONVERSÃO E SEGMENTAÇÃO
int vc_bgr_to_hsv(IVC* src, IVC* dst);
int vc_hsv_segmentation(IVC* src, IVC* dst, int hmin, int hmax, int smin, int smax, int vmin, int vmax);

// FUNÇÕES: ANÁLISE DE BLOBS
OVC* vc_binary_blob_labelling(cv::Mat src, cv::Mat dst, int* nlabels);
int vc_binary_blob_info(cv::Mat src, OVC* blobs, int nblobs);
OVC* vc_component_labelling(IVC* src, IVC* dst, int* nlabels);
int vc_binary_blob_info_ivc(IVC* src, OVC* blobs, int nblobs);

// FUNÇÕES: DESENHO E VISUALIZAÇÃO
int vc_desenha_bounding_box(cv::Mat src, OVC blobs);
int vc_draw_bounding_box(IVC* src, OVC blobs);
int vc_draw_line(IVC* src, int x1, int y1, int x2, int y2, int color[3]);
int vc_draw_circle(IVC* src, int xc, int yc, int radius, int color[3], int fill);
int vc_put_text(IVC* src, const char* text, int x, int y, int color[3], int fontsize);

// FUNÇÕES: MORFOLOGIA
int vc_dilate(IVC* src, IVC* dst, int kernel_size);
int vc_erode(IVC* src, IVC* dst, int kernel_size);

// FUNÇÕES: AUXILIARES
int desenha_linhaVermelha(cv::Mat frame);
int desenha_linhaVerde(cv::Mat frame);
int idBlobs(cv::Mat frameIn, cv::Mat frameOut, int hueMin, int hueMax, float satMin, float satMax, int valueMin, int valueMax);
int verificaPassouAntes(OVC* passou, OVC moedas, int cont);
int idMoeda(int area, int perimeter, float circularity, cv::Vec3b meanColor);
cv::Vec3b mediaCorROI(const cv::Mat& img, int x, int y, int width, int height);
void escreverInfo(FILE* fp, int cont, int mTotal, int m200, int m100, int m50, int m20, int m10, int m5, int m2, int m1, const char* videofile);

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   FUNÇÕES DE CONVERSÃO MAT-IVC
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
IVC* cv_mat_to_ivc(cv::Mat src);

