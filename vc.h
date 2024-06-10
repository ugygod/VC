//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLITECNICO DO CAVADO E DO AVE
//                          2023/2024
//             ENGENHARIA DE SISTEMAS INFORMATICOS
//                    VISAO POR COMPUTADOR
//
//             [  Rodrigo, Ruben, Thiago  ]
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#define VC_DEBUG

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   Macros
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#define MAX(a,b) (a>b ? a:b)
#define MIN(a,b) (a<b ? a:b)

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DE UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

typedef struct {
    unsigned char* data;
    int width, height;
    int channels;            // Binário/Cinzentos=1; RGB=3
    int levels;                // Binário=1; Cinzentos [1,255]; RGB [1,255]
    int bytesperline;        // width * channels
} IVC;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DE UM BLOB (OBJECTO)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

typedef struct {
	int x, y, width, height;	// Caixa Delimitadora (Bounding Box)
	int area;					// �rea
	int xc, yc;					// Centro-de-massa
	int perimeter;				// Per�metro
	int label;					// Etiqueta
	int valor;
	int digito1;
	int digito2;
	int digito3;
	int multiplicador;
} OVC;

//Estrutura para representar uma cor em RGB
typedef struct {
	unsigned char r, g, b;
} Color;

// Estrutura para mapear cores
typedef struct {
	Color color;
	int value;
} ColorMapping;

typedef struct {
	int x;
	int y;
	int width;
	int height;
} BoundingBox;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                    PROTÓTIPOS DE FUNÇÕES
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// FUNÇÕES: ALOCAR E LIBERTAR UMA IMAGEM
IVC* vc_image_new(int width, int height, int channels, int levels);
IVC* vc_image_free(IVC* image);
IVC* vc_read_image(char* filename);
int vc_write_image(char* filename, IVC* image);

// Gerar negativo da imagem Gray
int vc_gray_negative(IVC* srcdst);

//Gerar negativo de imagem RGB
int vc_rgb_negative(IVC* srcdst);

//Extrair RGB
int vc_rgb_get_red(IVC* srcdst);
int vc_rgb_get_green(IVC* srcdst);
int vc_rgb_get_blue(IVC* srcdst);

//Extrair RGB e mudar para cinza
int vc_rgb_get_red_gray(IVC* srcdst);
int vc_rgb_get_green_gray(IVC* srcdst);
int vc_rgb_get_blue_gray(IVC* srcdst);

// RGB to Gray
int vc_rgb_to_gray(IVC* src, IVC* dst);

// BGR to RGB
int vc_bgr_to_rgb(IVC* srcdst);

// RGB to HSV
int vc_rgb_to_hsv(IVC* src, IVC* dst);

//HSV to Grayscale
int vc_hsv_to_grayscale(IVC* src, IVC* dst);

//Grayscale -> HSV
int vc_grayscale_to_hsv(IVC* src, IVC* dst);

//Gray to RGB
int vc_scale_gray_to_red(IVC* src, IVC* dst);

//Segmentação
int vc_hsv_segmentation(IVC* src, IVC* dst, int hmin, int hmax, int smin, int smax, int vmin, int vmax);
//int vc_hsv_segmentation(IVC* src, IVC* dst1, IVC* dst2, int hmin1, int hmax1, int smin1, int smax1, int vmin1, int vmax1, int hmin2, int hmax2, int smin2, int smax2, int vmin2, int vmax2);

int vc_scale_gray_to_rgb(IVC* src, IVC* dst);

int vc_gray_to_rgb(IVC* srcdst, int threshold);

//Threshold manual
int vc_gray_to_binary(IVC* src, IVC* dst, int threshold);

//Threshold média
int vc_gray_to_binary_global_mean(IVC* src, IVC* dst);

//Threshold por vizinhaça do píxel
int vc_gray_to_binary_kernel_8(IVC* src, IVC* dst);

int vc_gray_to_binary_midpoint(IVC* src, IVC* dst, int kernel);
int vc_gray_to_binary_bernsen(IVC* src, IVC* dst, int kernel, int cmin);
int vc_gray_to_binary_niblack(IVC* src, IVC* dst, int kernel, float k);

//Dilatação
int vc_binary_dilate(IVC* src, IVC* dst, int kernel);

//Erosão
int vc_binary_erode(IVC* src, IVC* dst, int kernel);

//Abertura
int vc_binary_open(IVC* src, IVC* dst, IVC* aux, int kernel);

//Fecho
int vc_binary_close(IVC* src, IVC* dst, IVC* aux, int kernel);

//imagem binária para tons de cinza
int vc_binary_to_gray(IVC* src, IVC* dst);

//Inverter binária
int vc_binary_invert(IVC* src, IVC* dst);

//Etiquetagem de blobs
OVC* vc_binary_blob_labelling(IVC* src, IVC* dst, int* nlabels);

//Obter propriedades do blob
int vc_get_blob_properties(IVC* src, OVC* blobs, int nlabels);

//Desenhar bounding box
int vc_draw_boundingbox(IVC* srcdst, OVC* blob);

//Desenhar centro de gravidade
int vc_draw_centerofgravity(IVC* srcdst, OVC* blob);

////Contagem de número de píxeis a branco
int vc_count_pixels(IVC* src);

// Desenha a caixa delimitadora de um objeto
int vc_draw_boundingbox(IVC* src, OVC* blob);

//
int vc_gray_histogram_show(IVC* src, IVC* dst);

// 
int vc_hsv_to_binary(IVC* src, IVC* dst);

int vc_binary_blob_info(IVC* src, OVC* blobs, int nblobs);

int vc_desenha_box(IVC* imagem2, OVC* blobs, int nblobs, int* frame);

int vc_desenha_centroide(IVC* src, OVC* blobs, int nblobs);

int detectColor(Color pixel);

int calculateResistance(int* digits, int size);

void detectResistors(IVC* frame, IVC* binary_frame);

//int vc_draw_center_of_mass(IVC* src, OVC* blobs, int nblobs, int tamanho_alvo, int cor);
//int vc_normalizar_imagem_labelling(IVC* src, IVC* dst, int nblobs);
//int vc_gray_histogram_equalization(IVC* src, IVC* dst);
//int vc_gray_edge_prewitt(IVC* src, IVC* dst, float th);
//int vc_rgb_to_binary(IVC* srcdst);
//int vc_gray_dilate(IVC* src, IVC* dst, int kernel);
//int vc_gray_lowpass_mean_filter(IVC* src, IVC* dst, int kernel);
//int vc_gray_lowpass_median_filter(IVC* src, IVC* dst, int kernel);
//int vc_gray_lowpass_gaussian_filter(IVC* src, IVC* dst);
//int vc_hsv_to_binary(IVC* src, IVC* dst);
//int vc_3chanels_to_1_binary(IVC* src, IVC* dst);