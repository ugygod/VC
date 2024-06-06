//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLIT�CNICO DO C�VADO E DO AVE
//                          2022/2023
//             ENGENHARIA DE SISTEMAS INFORM�TICOS
//                    VIS�O POR COMPUTADOR
//
//             [  DUARTE DUQUE - dduque@ipca.pt  ]
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Desabilita (no MSVC++) warnings de fun��es n�o seguras (fopen, sscanf, etc...)
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include "vc.h"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUN��ES: ALOCAR E LIBERTAR UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


// Alocar mem�ria para uma imagem
IVC *vc_image_new(int width, int height, int channels, int levels)
{
	IVC *image = (IVC *) malloc(sizeof(IVC));

	if(image == NULL) return NULL;
	if((levels <= 0) || (levels > 255)) return NULL;

	image->width = width;
	image->height = height;
	image->channels = channels;
	image->levels = levels;
	image->bytesperline = image->width * image->channels;
	image->data = (unsigned char *) malloc(image->width * image->height * image->channels * sizeof(char));

	if(image->data == NULL)
	{
		return vc_image_free(image);
	}

	return image;
}


// Libertar mem�ria de uma imagem
IVC *vc_image_free(IVC *image)
{
	if(image != NULL)
	{
		if(image->data != NULL)
		{
			free(image->data);
			image->data = NULL;
		}

		free(image);
		image = NULL;
	}

	return image;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//    FUN��ES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


char *netpbm_get_token(FILE *file, char *tok, int len)
{
	char *t;
	int c;
	
	for(;;)
	{
		while(isspace(c = getc(file)));
		if(c != '#') break;
		do c = getc(file);
		while((c != '\n') && (c != EOF));
		if(c == EOF) break;
	}
	
	t = tok;
	
	if(c != EOF)
	{
		do
		{
			*t++ = c;
			c = getc(file);
		} while((!isspace(c)) && (c != '#') && (c != EOF) && (t - tok < len - 1));
		
		if(c == '#') ungetc(c, file);
	}
	
	*t = 0;
	
	return tok;
}


long int unsigned_char_to_bit(unsigned char *datauchar, unsigned char *databit, int width, int height)
{
	int x, y;
	int countbits;
	long int pos, counttotalbytes;
	unsigned char *p = databit;

	*p = 0;
	countbits = 1;
	counttotalbytes = 0;

	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos = width * y + x;

			if(countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//*p |= (datauchar[pos] != 0) << (8 - countbits);
				
				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				*p |= (datauchar[pos] == 0) << (8 - countbits);

				countbits++;
			}
			if((countbits > 8) || (x == width - 1))
			{
				p++;
				*p = 0;
				countbits = 1;
				counttotalbytes++;
			}
		}
	}

	return counttotalbytes;
}


void bit_to_unsigned_char(unsigned char *databit, unsigned char *datauchar, int width, int height)
{
	int x, y;
	int countbits;
	long int pos;
	unsigned char *p = databit;

	countbits = 1;

	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos = width * y + x;

			if(countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//datauchar[pos] = (*p & (1 << (8 - countbits))) ? 1 : 0;

				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				datauchar[pos] = (*p & (1 << (8 - countbits))) ? 0 : 1;
				
				countbits++;
			}
			if((countbits > 8) || (x == width - 1))
			{
				p++;
				countbits = 1;
			}
		}
	}
}


IVC *vc_read_image(char *filename)
{
	FILE *file = NULL;
	IVC *image = NULL;
	unsigned char *tmp;
	char tok[20];
	long int size, sizeofbinarydata;
	int width, height, channels;
	int levels = 255;
	int v;
	
	// Abre o ficheiro
	if((file = fopen(filename, "rb")) != NULL)
	{
		// Efectua a leitura do header
		netpbm_get_token(file, tok, sizeof(tok));

		if(strcmp(tok, "P4") == 0) { channels = 1; levels = 1; }	// Se PBM (Binary [0,1])
		else if(strcmp(tok, "P5") == 0) channels = 1;				// Se PGM (Gray [0,MAX(level,255)])
		else if(strcmp(tok, "P6") == 0) channels = 3;				// Se PPM (RGB [0,MAX(level,255)])
		else
		{
			#ifdef VC_DEBUG
			printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM, PGM or PPM file.\n\tBad magic number!\n");
			#endif

			fclose(file);
			return NULL;
		}
		
		if(levels == 1) // PBM
		{
			if(sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 || 
			   sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1)
			{
				#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM file.\n\tBad size!\n");
				#endif

				fclose(file);
				return NULL;
			}

			// Aloca mem�ria para imagem
			image = vc_image_new(width, height, channels, levels);
			if(image == NULL) return NULL;

			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height;
			tmp = (unsigned char *) malloc(sizeofbinarydata);
			if(tmp == NULL) return 0;

			#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
			#endif

			if((v = fread(tmp, sizeof(unsigned char), sizeofbinarydata, file)) != sizeofbinarydata)
			{
				#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
				#endif

				vc_image_free(image);
				fclose(file);
				free(tmp);
				return NULL;
			}

			bit_to_unsigned_char(tmp, image->data, image->width, image->height);

			free(tmp);
		}
		else // PGM ou PPM
		{
			if(sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 || 
			   sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1 || 
			   sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &levels) != 1 || levels <= 0 || levels > 255)
			{
				#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PGM or PPM file.\n\tBad size!\n");
				#endif

				fclose(file);
				return NULL;
			}

			// Aloca mem�ria para imagem
			image = vc_image_new(width, height, channels, levels);
			if(image == NULL) return NULL;

			#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
			#endif

			size = image->width * image->height * image->channels;

			if((v = fread(image->data, sizeof(unsigned char), size, file)) != size)
			{
				#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
				#endif

				vc_image_free(image);
				fclose(file);
				return NULL;
			}
		}
		
		fclose(file);
	}
	else
	{
		#ifdef VC_DEBUG
		printf("ERROR -> vc_read_image():\n\tFile not found.\n");
		#endif
	}
	
	return image;
}


int vc_write_image(char *filename, IVC *image)
{
	FILE *file = NULL;
	unsigned char *tmp;
	long int totalbytes, sizeofbinarydata;
	
	if(image == NULL) return 0;

	if((file = fopen(filename, "wb")) != NULL)
	{
		if(image->levels == 1)
		{
			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height + 1;
			tmp = (unsigned char *) malloc(sizeofbinarydata);
			if(tmp == NULL) return 0;
			
			fprintf(file, "%s %d %d\n", "P4", image->width, image->height);
			
			totalbytes = unsigned_char_to_bit(image->data, tmp, image->width, image->height);
			printf("Total = %ld\n", totalbytes);
			if(fwrite(tmp, sizeof(unsigned char), totalbytes, file) != totalbytes)
			{
				#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
				#endif

				fclose(file);
				free(tmp);
				return 0;
			}

			free(tmp);
		}
		else
		{
			fprintf(file, "%s %d %d 255\n", (image->channels == 1) ? "P5" : "P6", image->width, image->height);
		
			if(fwrite(image->data, image->bytesperline, image->height, file) != image->height)
			{
				#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
				#endif

				fclose(file);
				return 0;
			}
		}
		
		fclose(file);

		return 1;
	}
	
	return 0;
}

// Gerar negativo da imagem Gray
int vc_gray_negative(IVC *srcdst)
{
    unsigned char *data = (unsigned char *) srcdst->data;
    int width = srcdst->width;
    int height = srcdst->height;
    int bytesperline = srcdst->width * srcdst->channels;
    int channels = srcdst->channels;
    int x, y;
    long int pos;

    // Verificação de erros
    if((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
    if(channels != 1) return 0;

    // Inverte a imagem Gray
    for(y=0; y<height; y++)
    {
        for(x=0; x<width; x++)
        {
            pos = y * bytesperline + x * channels;

            data[pos] = 255 - data[pos];
        }
    }
	return 1;
}

int vc_rgb_negative(IVC *srcdst)
{
    unsigned char *data = (unsigned char *) srcdst->data;
    int width = srcdst->width;
    int height = srcdst->height;
    int bytesperline = srcdst->width * srcdst->channels;
    int channels = srcdst->channels;
    int x, y;
    long int pos;

    // Verificação de erros
    if((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
    if(channels != 3) return 0;

    // Inverte a imagem RGB
    for(y=0; y<height; y++)
    {
        for(x=0; x<width; x++)
        {
            pos = y * bytesperline + x * channels;

            data[pos] = 255 - data[pos];
            data[pos + 1] = 255 - data[pos + 1];
            data[pos + 2] = 255 - data[pos + 2];
        }
    }

return 1;
}

// Extrair componente Red da imagem RGB para Gray 
int vc_rgb_get_red_gray(IVC *srcdst) 
{
unsigned char *data = (unsigned char *) srcdst->data;
int width = srcdst->width;
int height = srcdst->height;
int bytesperline = srcdst->width* srcdst->channels;
int channels = srcdst->channels;
int x, y;
long int pos;
// Verificação de erros 
if((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0; if(channels != 3) return 0;
// Extrai a componente Red for(y=0; y<height; y++)
{
for(x=0; x<width; x++)
{
pos = y * bytesperline + x * channels;
data [pos + 1] = data[pos]; // Green data [pos + 2] = data[pos]; // Blue
}
}
return 1;
}

// Converter de RGB para Gray 
int vc_rgb_to_gray(IVC *src, IVC *dst)
{
	unsigned char *datasrc = (unsigned char *) src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels; 
	unsigned char *datadst = (unsigned char *) dst->data;
	int bytesperline_dst = dst->width* dst->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y;
	long int pos_src, pos_dst;
	float rf, gf, bf;
	// Verificação de erros
	if((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0; 
	if((src->width != dst->width) || (src->height != dst->height)) return 0; 
	if((src->channels != 3) || (dst->channels != 1)) return 0;

	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos_src = y *  bytesperline_src + x * channels_src; 
			pos_dst = y * bytesperline_dst + x * channels_dst;

			rf = (float) datasrc[pos_src];
			gf = (float) datasrc[pos_src + 1];
			bf = (float) datasrc[pos_src + 2];
			datadst [pos_dst] = (unsigned char) ((rf * 0.299) + (gf * 0.587) + (bf * 0.114));
		}	
	}

	return 1;
}

int vc_rgb_to_hsv(IVC *srcdst)
{
	unsigned char *data = (unsigned char *)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->bytesperline;
	int channels = srcdst->channels;
	float r, g, b, hue, saturation, value;
	float rgb_max, rgb_min;
	int i, size;

	// Verificação de erros
	if ((width <= 0) || (height <= 0) || (data == NULL)) return 0;
	if (channels != 3) return 0;

	size = width * height * channels;

	for (i = 0; i<size; i = i + channels)
	{
		r = (float)data[i];
		g = (float)data[i + 1];
		b = (float)data[i + 2];

		// Calcula valores máximo e mínimo dos canais de cor R, G e B
		rgb_max = (r > g ? (r > b ? r : b) : (g > b ? g : b));
		rgb_min = (r < g ? (r < b ? r : b) : (g < b ? g : b));

		// Value toma valores entre [0,255]
		value = rgb_max;
		if (value == 0.0f)
		{
			hue = 0.0f;
			saturation = 0.0f;
		}
		else
		{
			// Saturation toma valores entre [0,255]
			saturation = ((rgb_max - rgb_min) / rgb_max) * 255.0f;

			if (saturation == 0.0f)
			{
				hue = 0.0f;
			}
			else
			{
				// Hue toma valores entre [0,360]
				if ((rgb_max == r) && (g >= b))
				{
					hue = 60.0f * (g - b) / (rgb_max - rgb_min);
				}
				else if ((rgb_max == r) && (b > g))
				{
					hue = 360.0f + 60.0f * (g - b) / (rgb_max - rgb_min);
				}
				else if (rgb_max == g)
				{
					hue = 120.0f + 60.0f * (b - r) / (rgb_max - rgb_min);
				}
				else
				{
					hue = 240.0f + 60.0f * (r - g) / (rgb_max - rgb_min);
				}
			}
		}

		// Atribui valores entre [0,255]
		data[i] = (unsigned char) (hue / 360.0f * 255.0f);
		data[i + 1] = (unsigned char) (saturation);
		data[i + 2] = (unsigned char) (value);

	}

	return 1;
}

// hmin,hmax = [0, 360]; smin,smax = [0, 100]; vmin,vmax = [0, 100]
int vc_hsv_segmentation(IVC *srcdst, int hmin, int hmax, int smin, int smax, int vmin, int vmax)
{
	unsigned char *data = (unsigned char *)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->bytesperline;
	int channels = srcdst->channels;
	int h, s, v; // h=[0, 360] s=[0, 100] v=[0, 100]
	int i, size;

	// Verificação de erros
	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
	if (channels != 3) return 0;

	size = width * height * channels;

	for (i = 0; i < size; i = i + channels)
	{
		h = (int) (((float)data[i]) / 255.0f * 360.0f);
		s = (int) (((float)data[i + 1]) / 255.0f * 100.0f);
		v = (int) (((float)data[i + 2]) / 255.0f * 100.0f);

		if ((h > hmin) && (h <= hmax) && (s >= smin) && (s <= smax) && (v >= vmin) && (v <= vmax))
		{
			data[i] = 255;
			data[i + 1] = 255;
			data[i + 2] = 255;
		}
		else
		{
			data[i] = 0;
			data[i + 1] = 0;
			data[i + 2] = 0;
		}
	}


	return 1;
}
	
	
int vc_scale_gray_to_rgb(IVC *src, IVC *dst) {
    unsigned char *datasrc = (unsigned char *)src->data;
    int bytesperline_src = src->width * src->channels;
    int channels_src = src->channels;
    unsigned char *datadst = (unsigned char *)dst->data;
    int bytesperline_dst = dst->width * dst->channels;
    int channels_dst = dst->channels;
    int width = src->width;
    int height = src->height;
    int x, y;
    long int pos_src, pos_dst;

    // Verificação de erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if ((src->width != dst->width) || (src->height != dst->height)) return 0;
    if ((src->channels != 1) || (dst->channels != 3)) return 0;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            pos_src = y * bytesperline_src + x * channels_src;
            pos_dst = y * bytesperline_dst + x * channels_dst;

            // Copia o valor de cinza para os canais R, G e B
            datadst[pos_dst] = datasrc[pos_src]; // R
            datadst[pos_dst + 1] = datasrc[pos_src]; // G
            datadst[pos_dst + 2] = datasrc[pos_src]; // B
        }
    }

    return 1;
}



int vc_gray_to_rgb(IVC *src, IVC *dst) {
    int x, y;
    long int pos;

    // Verifica se a imagem fonte é válida
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if (src->channels != 1) return 0; // Garante que é uma imagem de um canal (escala de cinzas)

    // Cria a imagem destino com 3 canais (RGB)
    dst->width = src->width;
    dst->height = src->height;
    dst->channels = 3; // RGB
    dst->bytesperline = dst->width * dst->channels;
    dst->data = (unsigned char*)malloc(dst->width * dst->height * dst->channels * sizeof(unsigned char));

    if (dst->data == NULL) return 0;

    for (y = 0; y < src->height; y++) {
        for (x = 0; x < src->width; x++) {
            pos = y * src->bytesperline + x * src->channels;
            unsigned char gray_value = src->data[pos];

            // Verifica se a intensidade de cinza é alta (cor clara) e atribui a cor verde
            if (gray_value > 200) {
                dst->data[pos] = 0;       // Canal R (vermelho)
                dst->data[pos + 1] = 255; // Canal G (verde)
                dst->data[pos + 2] = 0;   // Canal B (azul)
            } else {
                // Se não for uma cor clara, mantém a intensidade de cinza
                dst->data[pos] = gray_value;
                dst->data[pos + 1] = gray_value;
                dst->data[pos + 2] = gray_value;
            }
        }
    }

    return 1;
}


int vc_gray_to_binary(IVC* src, IVC* dst, int threshold) {
	unsigned char* data = (unsigned char*)src->data;
	unsigned char* data_out = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int bytesperline_out = dst->bytesperline;
	int channels_out = dst->channels;
	int x, y;
	long int pos, pos_out;

	// Verificação de erros
	if ((width <= 0) || (height <= 0) || (data == NULL)) return 0;
	if ((channels != 1) || (channels_out != 1)) return 0;
	if ((width != dst->width) || (height != dst->height)) return 0;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pos = y * bytesperline + x * channels;
			pos_out = y * bytesperline_out + x * channels_out;

			if (data[pos] < threshold) {
				data_out[pos_out] = 255;  // Background
			}
			else {
				data_out[pos_out] = 0;    // Resistor
			}
		}
	}

	return 1;
}


int vc_gray_to_binary_midpoint(IVC *srcdst, int kernel) {
	unsigned char *data = srcdst->data;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int width = srcdst->width;
	int height = srcdst->height;
	int x, y, x2, y2;
	long int pos;
	float threshold = 0;
	int max, min;

	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
	if (srcdst->channels != 1) return 0;
	kernel *= 0.5;
	for (y = kernel; y < height - kernel; y++) {
		for (x = kernel; x < width - kernel; x++) {
			max = 0;
			min = 255;

			for (y2 = y - kernel; y2 <= y + kernel; y2++) {
				for (x2 = x - kernel; x2 <= x + kernel; x2++) {
					pos = y2 * bytesperline + x2 * channels;
					if (data[pos] > max) { 
						max = data[pos]; 
					} else if (data[pos] < min) { 
						min = data[pos]; 
					}
				}
			}
			threshold = 0.5 * (max + min);

			pos = y * bytesperline + x * channels;
			if (data[pos] > threshold) {
				data[pos] = 255;
			} else {
				data[pos] = 0;
			}
		}
	}
	return 1;
}


int vc_gray_to_binary_bernsen(IVC *src, IVC *dst, int kernel, int cmin) {
    unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;
    int bytesperline_src = src->width * src->channels;
    int bytesperline_dst = dst->width * dst->channels;
    int channels_src = src->channels;
    int channels_dst = dst->channels;
    int width = src->width;
    int height = src->height;
    int x, y, x2, y2;
    long int pos_src, pos_dst;
    int max, min;
    float threshold;

    // Validar entrada
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if ((src->width != dst->width) || (src->height != dst->height)) return 0;
    if ((src->channels != 1) || (dst->channels != 1)) return 0;

    kernel /= 2;
    for (y = kernel; y < height - kernel; y++) {
        for (x = kernel; x < width - kernel; x++) {
            pos_dst = y * bytesperline_dst + x * channels_dst;
            max = 0;
            min = 255;

            for (y2 = y - kernel; y2 <= y + kernel; y2++) {
                for (x2 = x - kernel; x2 <= x + kernel; x2++) {
                    pos_src = y2 * bytesperline_src + x2 * channels_src;
                    if (datasrc[pos_src] > max) max = datasrc[pos_src];
                    if (datasrc[pos_src] < min) min = datasrc[pos_src];
                }
            }

            if (max - min < cmin) {
                threshold = 127.5; // Usar L/2 onde L=255 (escala de cinza máxima)
            } else {
                threshold = 0.5 * (max + min);
            }

            pos_src = y * bytesperline_src + x * channels_src;
            datadst[pos_dst] = (datasrc[pos_src] < threshold) ? 255 : 0;
        }
    }

    return 1;
}


int vc_gray_to_binary_niblack(IVC *src, IVC *dst, int kernel, float k) {
    unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;
    int bytesperline_src = src->width * src->channels;
    int bytesperline_dst = dst->width * dst->channels;
    int width = src->width;
    int height = src->height;
    int channels_src = src->channels;
    int channels_dst = dst->channels;
    int x, y, x2, y2;
    long int pos_src, pos_dst;
    float sum, sumsq, mean, std, threshold;
    int pixel_count;

    // Validar entrada
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if ((src->width != dst->width) || (src->height != dst->height)) return 0;
    if ((src->channels != 1) || (dst->channels != 1)) return 0;

    kernel /= 2;
    for (y = kernel; y < height - kernel; y++) {
        for (x = kernel; x < width - kernel; x++) {
            sum = 0;
            sumsq = 0;
            pixel_count = 0;

            for (y2 = y - kernel; y2 <= y + kernel; y2++) {
                for (x2 = x - kernel; x2 <= x + kernel; x2++) {
                    pos_src = y2 * bytesperline_src + x2 * channels_src;
                    sum += datasrc[pos_src];
                    sumsq += datasrc[pos_src] * datasrc[pos_src];
                    pixel_count++;
                }
            }

            mean = sum / pixel_count;
            std = sqrt((sumsq - (sum * sum) / pixel_count) / pixel_count);
            threshold = mean + k * std;

            pos_dst = y * bytesperline_dst + x * channels_dst;
            pos_src = y * bytesperline_src + x * channels_src;
            datadst[pos_dst] = (datasrc[pos_src] < threshold) ? 255 : 0;
        }
    }

    return 1;
}


// Etiquetagem de blobs
// src		: Imagem binária de entrada
// dst		: Imagem grayscale (irá conter as etiquetas)
// nlabels	: Endereço de memória de uma variável, onde será armazenado o número de etiquetas encontradas.
// OVC*		: Retorna um array de estruturas de blobs (objectos), com respectivas etiquetas. É necessário libertar posteriormente esta memória.
OVC* vc_binary_blob_labelling(IVC *src, IVC *dst, int *nlabels)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, a, b;
	long int i, size;
	long int posX, posA, posB, posC, posD;
	int labeltable[256] = { 0 };
	int labelarea[256] = { 0 };
	int label = 1; // Etiqueta inicial.
	int num, tmplabel;
	OVC *blobs; // Apontador para array de blobs (objectos) que será retornado desta função.

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return NULL;
	if (channels != 1) return NULL;

	// Copia dados da imagem binária para imagem grayscale
	memcpy(datadst, datasrc, bytesperline * height);

	// Todos os pixéis de plano de fundo devem obrigatóriamente ter valor 0
	// Todos os pixéis de primeiro plano devem obrigatóriamente ter valor 255
	// Serão atribuídas etiquetas no intervalo [1,254]
	// Este algoritmo está assim limitado a 254 labels
	for (i = 0, size = bytesperline * height; i<size; i++)
	{
		if (datadst[i] != 0) datadst[i] = 255;
	}

	// Limpa os rebordos da imagem binária
	for (y = 0; y<height; y++)
	{
		datadst[y * bytesperline + 0 * channels] = 0;
		datadst[y * bytesperline + (width - 1) * channels] = 0;
	}
	for (x = 0; x<width; x++)
	{
		datadst[0 * bytesperline + x * channels] = 0;
		datadst[(height - 1) * bytesperline + x * channels] = 0;
	}

	// Efectua a etiquetagem
	for (y = 1; y<height - 1; y++)
	{
		for (x = 1; x<width - 1; x++)
		{
			// Kernel:
			// A B C
			// D X

			posA = (y - 1) * bytesperline + (x - 1) * channels; // A
			posB = (y - 1) * bytesperline + x * channels; // B
			posC = (y - 1) * bytesperline + (x + 1) * channels; // C
			posD = y * bytesperline + (x - 1) * channels; // D
			posX = y * bytesperline + x * channels; // X

			// Se o pixel foi marcado
			if (datadst[posX] != 0)
			{
				if ((datadst[posA] == 0) && (datadst[posB] == 0) && (datadst[posC] == 0) && (datadst[posD] == 0))
				{
					datadst[posX] = label;
					labeltable[label] = label;
					label++;
				}
				else
				{
					num = 255;

					// Se A está marcado
					if (datadst[posA] != 0) num = labeltable[datadst[posA]];
					// Se B está marcado, e é menor que a etiqueta "num"
					if ((datadst[posB] != 0) && (labeltable[datadst[posB]] < num)) num = labeltable[datadst[posB]];
					// Se C está marcado, e é menor que a etiqueta "num"
					if ((datadst[posC] != 0) && (labeltable[datadst[posC]] < num)) num = labeltable[datadst[posC]];
					// Se D está marcado, e é menor que a etiqueta "num"
					if ((datadst[posD] != 0) && (labeltable[datadst[posD]] < num)) num = labeltable[datadst[posD]];

					// Atribui a etiqueta ao pixel
					datadst[posX] = num;
					labeltable[num] = num;

					// Actualiza a tabela de etiquetas
					if (datadst[posA] != 0)
					{
						if (labeltable[datadst[posA]] != num)
						{
							for (tmplabel = labeltable[datadst[posA]], a = 1; a<label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posB] != 0)
					{
						if (labeltable[datadst[posB]] != num)
						{
							for (tmplabel = labeltable[datadst[posB]], a = 1; a<label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posC] != 0)
					{
						if (labeltable[datadst[posC]] != num)
						{
							for (tmplabel = labeltable[datadst[posC]], a = 1; a<label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posD] != 0)
					{
						if (labeltable[datadst[posD]] != num)
						{
							for (tmplabel = labeltable[datadst[posD]], a = 1; a<label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
				}
			}
		}
	}

	// Volta a etiquetar a imagem
	for (y = 1; y<height - 1; y++)
	{
		for (x = 1; x<width - 1; x++)
		{
			posX = y * bytesperline + x * channels; // X

			if (datadst[posX] != 0)
			{
				datadst[posX] = labeltable[datadst[posX]];
			}
		}
	}

	//printf("\nMax Label = %d\n", label);

	// Contagem do número de blobs
	// Passo 1: Eliminar, da tabela, etiquetas repetidas
	for (a = 1; a<label - 1; a++)
	{
		for (b = a + 1; b<label; b++)
		{
			if (labeltable[a] == labeltable[b]) labeltable[b] = 0;
		}
	}
	// Passo 2: Conta etiquetas e organiza a tabela de etiquetas, para que não hajam valores vazios (zero) entre etiquetas
	*nlabels = 0;
	for (a = 1; a<label; a++)
	{
		if (labeltable[a] != 0)
		{
			labeltable[*nlabels] = labeltable[a]; // Organiza tabela de etiquetas
			(*nlabels)++; // Conta etiquetas
		}
	}

	// Se não há blobs
	if (*nlabels == 0) return NULL;

	// Cria lista de blobs (objectos) e preenche a etiqueta
	blobs = (OVC *)calloc((*nlabels), sizeof(OVC));
	if (blobs != NULL)
	{
		for (a = 0; a<(*nlabels); a++) blobs[a].label = labeltable[a];
	}
	else return NULL;

	return blobs;
}


int vc_binary_blob_info(IVC *src, OVC *blobs, int nblobs)
{
	unsigned char *data = (unsigned char *)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, i;
	long int pos;
	int xmin, ymin, xmax, ymax;
	long int sumx, sumy;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if (channels != 1) return 0;

	// Conta área de cada blob
	for (i = 0; i<nblobs; i++)
	{
		xmin = width - 1;
		ymin = height - 1;
		xmax = 0;
		ymax = 0;

		sumx = 0;
		sumy = 0;

		blobs[i].area = 0;

		for (y = 1; y<height - 1; y++)
		{
			for (x = 1; x<width - 1; x++)
			{
				pos = y * bytesperline + x * channels;

				if (data[pos] == blobs[i].label)
				{
					// Área
					blobs[i].area++;

					// Centro de Gravidade
					sumx += x;
					sumy += y;

					// Bounding Box
					if (xmin > x) xmin = x;
					if (ymin > y) ymin = y;
					if (xmax < x) xmax = x;
					if (ymax < y) ymax = y;

					// Perímetro
					// Se pelo menos um dos quatro vizinhos não pertence ao mesmo label, então é um pixel de contorno
					if ((data[pos - 1] != blobs[i].label) || (data[pos + 1] != blobs[i].label) || (data[pos - bytesperline] != blobs[i].label) || (data[pos + bytesperline] != blobs[i].label))
					{
						blobs[i].perimeter++;
					}
				}
			}
		}

		// Bounding Box
		blobs[i].x = xmin;
		blobs[i].y = ymin;
		blobs[i].width = (xmax - xmin) + 1;
		blobs[i].height = (ymax - ymin) + 1;

		// Centro de Gravidade
		//blobs[i].xc = (xmax - xmin) / 2;
		//blobs[i].yc = (ymax - ymin) / 2;
		blobs[i].xc = sumx / MAX(blobs[i].area, 1);
		blobs[i].yc = sumy / MAX(blobs[i].area, 1);
	}

	return 1;
}

int vc_binary_dilate(IVC *src, IVC *dst, int kernel) {
	unsigned char *datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width*src->channels;
	int channels_src = src->channels;
	unsigned char *datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline_dst = dst->width*dst->channels;
	int channels_dst = dst->channels;
	int x, y, x2, y2;
	long int pos_src, pos_dst;
	int verifica;
	kernel *= 0.5;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))return 0;
	if ((src->width != dst->width) || (src->height != dst->height))return 0;
	if ((src->channels != 1) || (dst->channels != 1))return 0;


	for (y = kernel; y < height-kernel; y++)
	{
		for (x = kernel; x < width-kernel; x++)
		{
			pos_dst = y*bytesperline_dst + x*channels_dst;

			verifica = 0;

			for (y2 = y-kernel; y2 <= y+kernel; y2++)
			{
				for (x2 = x-kernel; x2 <= x+kernel; x2++)
				{
					pos_src = y2*bytesperline_src + x2*channels_src;
					if (datasrc[pos_src] == 255) { verifica = 1; }
				}
			}

			if (verifica == 1) { datadst[pos_dst] = 255; }
			else { datadst[pos_dst] = 0; }
		}
	}

	return 1;
}

int vc_binary_erode(IVC* src, IVC* dst, int kernel)
{
	unsigned char* data = (unsigned char*)src->data;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;

	int width = src->width;
	int height = src->height;

	unsigned char* dataaux = (unsigned char*)dst->data;
	int bytesperaux = dst->width * dst->channels;
	int channelsaux = dst->channels;

	int x, y;
	long int pos, posaux;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height))
		return 0;
	if (channels != 1 || kernel < 1)
		return 0;

	int fill;

	int kernelFloor = floor((double)kernel / 2);

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			fill = 255;
			for (int yy = -kernelFloor; yy <= kernelFloor; yy++)
			{
				for (int xx = -kernelFloor; xx <= kernelFloor; xx++)
				{
					if (y + yy < 0 || y + yy >= height || x + xx < 0 || x + xx >= width)
						continue;
					if ((xx == yy || xx == -yy) && xx != 0)
						continue;
					pos = (y + yy) * bytesperline + (x + xx) * channels;

					if (data[pos] != 255)
						fill = 0;
				}
			}

			posaux = y * bytesperaux + x * channelsaux;

			dataaux[posaux] = fill;
		}
	}

	return 1;
}


void draw_bounding_box(IVC *image, int x, int y, int width, int height, unsigned char *color) {
    int bytesperline = image->bytesperline;
    unsigned char *data = image->data;
    int channels = image->channels;

    // Linhas horizontais
    for (int i = y; i < y + height; i++) {
        if (i >= 0 && i < image->height) {
            int pos = i * bytesperline + x * channels;
            for (int j = 0; j < channels; j++) {
                data[pos + j] = color[j];
                data[pos + width * channels + j] = color[j];
            }
        }
    }

    // Linhas verticais
    for (int i = x; i < x + width; i++) {
        if (i >= 0 && i < image->width) {
            int pos = y * bytesperline + i * channels;
            for (int j = 0; j < channels; j++) {
                data[pos + j] = color[j];
                data[pos + height * bytesperline + j] = color[j];
            }
        }
    }
}

void draw_centroid(IVC *image, int x, int y, unsigned char *color) {

    int bytesperline = image->bytesperline;
    unsigned char *data = image->data;
    int channels = image->channels;

    if (x >= 0 && x < image->width && y >= 0 && y < image->height) {
        int pos = y * bytesperline + x * channels;
        for (int i = 0; i < channels; i++) {
            data[pos + i] = color[i];
        }
    }
}


int vc_gray_histogram_show(IVC *src, IVC *dst) {
    if (src == NULL || dst == NULL || src->width <= 0 || src->height <= 0 || src->channels != 1) {
        printf("Error: invalid parameters.\n");
        return 0;
    }

    int x, y;
    int h[256] = {0}; // Histogram array with 256 bins initialized to 0

    // Calculate histogram
    for (y = 0; y < src->height; y++) {
        for (x = 0; x < src->width; x++) {
            unsigned char *data = (unsigned char *)src->data;
            int value = data[y * src->bytesperline + x * src->channels];
            h[value]++;
        }
    }

    // Find maximum histogram value
    int max_hist = 0;
    for (int i = 0; i < 256; i++) {
        if (h[i] > max_hist) {
            max_hist = h[i];
        }
    }

    // Normalize histogram values to fit in the destination image
    float scale = (float)(dst->height - 1) / max_hist;

    // Clear destination image
    for (y = 0; y < dst->height; y++) {
        for (x = 0; x < dst->width; x++) {
            unsigned char *data = (unsigned char *)dst->data;
            data[y * dst->bytesperline + x * dst->channels] = 255; // White background
        }
    }

    // Draw histogram
    for (x = 0; x < 256; x++) {
        int hist_value = (int)(h[x] * scale);
        for (y = 0; y < hist_value; y++) {
            unsigned char *data = (unsigned char *)dst->data;
            data[(dst->height - 1 - y) * dst->bytesperline + x * dst->channels] = 0; // Black color
        }
    }

    return 1;
}


int vc_gray_histogram_equalization(IVC *src, IVC *dst) {
    unsigned char *datasrc = (unsigned char *)src->data;
    int bytesperline_src = src->bytesperline;
    int channels_src = src->channels;
    unsigned char *datadst = (unsigned char *)dst->data;
    int width = src->width;
    int height = src->height;
    int bytesperline_dst = dst->bytesperline;
    int channels_dst = dst->channels;
    int x, y;
    float min;
    long int pos_src, pos_dst;
    int posicoes[256] = {0};
    float pdf[256] = { 0 }, cdf[256] = { 0 };
    float size = width * height;

    // Calcular histograma
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            pos_src = y * bytesperline_src + x * channels_src;
            posicoes[datasrc[pos_src]]++;
        }
    }

    // Calcular PDF
    for (x = 0; x <= 255; x++) {
        pdf[x] = (((float)posicoes[x]) / size);
    }

    // Calcular CDF
    for (cdf[0] = pdf[0], x = 1; x <= 255; x++) {
        cdf[x] = cdf[x - 1] + pdf[x];
    }

    // Encontrar valor mínimo não zero de CDF
    for (min = 0, x = 0; x <= 255; x++) {
        if (cdf[x] != 0) { 
            min = pdf[x]; 
            break; 
        }
    }
    
    // Equalizar imagem
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            pos_src = y * bytesperline_src + x * channels_src;
            pos_dst = y * bytesperline_dst + x * channels_dst;
            datadst[pos_dst] = ((cdf[datasrc[pos_src]] - min) / (1.0 - min)) * 255.0;
        }
    }

    return 1;
}


int vc_gray_edge_prewitt(IVC *src, IVC *dst, float th) {
	unsigned char *data = (unsigned char *)src->data;
	int width = src->width;
	int height = src->height;
	int byteperline = src->width*src->channels;
	int channels = src->channels;
	int x, y;
	long int pos;
	long int posA, posB, posC, posD, posE, posF, posG, posH;
	double mag, mx, my;

	if ((width <= 0) || (height <= 0) || (src->data == NULL)) return 0;
	if (channels != 1) return 0;

	for (y = 1; y < height; y++)
	{
		for (x = 1; x < width; x++)
		{
			pos = y * byteperline + x * channels;

			posA = (y - 1)* byteperline + (x - 1) * channels;
			posB = (y - 1)* byteperline + (x)* channels;
			posC = (y - 1)* byteperline + (x + 1)* channels;
			posD = (y)* byteperline + (x - 1)* channels;
			posE = (y)* byteperline + (x + 1)* channels;
			posF = (y + 1)* byteperline + (x - 1)* channels;
			posG = (y + 1)* byteperline + (x)* channels;
			posH = (y + 1)* byteperline + (x + 1)* channels;
			mx = ((-1 * data[posA]) + (1 * data[posC]) + (-1 * data[posD]) + (1 * data[posE]) + (-1 * data[posF]) + (1 * data[posH])) / 3; //?
			my = ((-1 * data[posA]) + (1 * data[posF]) + (-1 * data[posB]) + (1 * data[posG]) + (-1 * data[posC]) + (1 * data[posH])) / 3;

			mag = sqrt((mx*mx) + (my * my));

			if (mag > th)
				dst->data[pos] = 255;
			else
				dst->data[pos] = 0;
		}
	}
	return 1;
}

int vc_gray_edge_sobel(IVC *src, IVC *dst, float th) {
	unsigned char *data = (unsigned char *)src->data;
	int width = src->width;
	int height = src->height;
	int byteperline = src->width*src->channels;
	int channels = src->channels;
	int x, y;
	long int pos;
	long int posA, posB, posC, posD, posE, posF, posG, posH;
	double mag, mx, my;

	if ((width <= 0) || (height <= 0) || (src->data == NULL)) return 0;
	if (channels != 1) return 0;

	for (y = 1; y < height; y++)
	{
		for (x = 1; x < width; x++)
		{
			pos = y * byteperline + x * channels;

			posA = (y - 1)* byteperline + (x - 1) * channels;
			posB = (y - 1)* byteperline + (x)* channels;
			posC = (y - 1)* byteperline + (x + 1)* channels;
			posD = (y)* byteperline + (x - 1)* channels;
			posE = (y)* byteperline + (x + 1)* channels;
			posF = (y + 1)* byteperline + (x - 1)* channels;
			posG = (y + 1)* byteperline + (x)* channels;
			posH = (y + 1)* byteperline + (x + 1)* channels;

			mx = ((-1 * data[posA]) + (1 * data[posC]) + (-2 * data[posD]) + (2 * data[posE]) + (-1 * data[posF]) + (1 * data[posH])) / 3;
			my = ((-1 * data[posA]) + (1 * data[posF]) + (-2 * data[posB]) + (2 * data[posG]) + (-1 * data[posC]) + (1 * data[posH])) / 3;

			mag = sqrt((mx*mx) + (my * my));

			if (mag > th)
				dst->data[pos] = 255;
			else
				dst->data[pos] = 0;
		}
	}
	return 1;
}


//Verificar funções
int midpoint_threshold(IVC* src, IVC* dst, int kernel) {
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_src = src->width * src->channels;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_src = src->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y, kx, ky;
	int max = 0, min = 255;
	int offset = (kernel - 1) / 2;
	int counter;
	int threshold;
	long int pos_src, pos_dst, soma, posk;

	soma = 0;
	if (src == NULL || dst == NULL)
	{
		printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
		getchar();
		return 1;
	}

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;

			for (ky = -offset; ky <= offset; ky++) {

				for (kx = -offset; kx <= offset++; kx++) {
					if (x + kx > 0 && y + ky > 0 && x + kx < width && y + ky < height) {
						posk = (y + ky) * bytesperline_src + (x + kx) * channels_src;

						if (datasrc[posk] > max) {
							max = datasrc[posk];
						}
						if (datasrc[posk] < min) {
							min = datasrc[posk];
						}

					}
				}
			}
			threshold = (max + min) / 2;

			if (datasrc[pos_src] > threshold) {
				datadst[pos_src] = 255;
			}
			else {
				datadst[pos_src] = 0;
			}

		}
	}
	vc_write_image("segmented_imagewithmidpoint.pgm", dst);
	//printf("Press any key to exit...\n");
	//getchar();

	return 0;
}

int average_threshold(IVC* src, IVC* dst) {
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_src = src->width * src->channels;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_src = src->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y;
	long int pos_src, pos_dst, soma;

	soma = 0;
	if (src == NULL || dst == NULL)
	{
		printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
		getchar();
		return 1;
	}

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;

			soma += datasrc[pos_src];
		}
	}

	int threshold = soma / (width * height);

	printf("threshold = %d", threshold);

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;

			if (datasrc[pos_src] > threshold) {
				datadst[pos_src] = 255;
			}
			else {
				datadst[pos_src] = 0;
			}

		}
	}
	vc_write_image("segmented_imagewithglobal.pgm", dst);
	//printf("Press any key to exit...\n");
	//getchar();

	return 0;
}

int global_threshold(IVC* src, IVC* dst, int threshold) {
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_src = src->width * src->channels;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_src = src->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y;
	long int pos_src, pos_dst;

	if (src == NULL || dst == NULL)
	{
		printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
		getchar();
		return 1;
	}

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;

			if (datasrc[pos_src] >= threshold) {
				datadst[pos_src] = 255;
			}
			else {
				datadst[pos_src] = 0;
			}

		}
	}

	printf("Press any key to exit...\n");
	getchar();

	return 0;
}

int vc_binary_open(IVC* src, IVC* dst, int kernel)
{
	IVC* temp;
	temp = vc_image_new(src->width, src->height, 1, 1);
	vc_binary_erode(src, temp, kernel);
	vc_binary_dilate(temp, dst, kernel);
	vc_image_free(temp);
	return 1;
}


int vc_desenha_box(IVC* src, OVC* blobs, int nblobs)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;
	int i, xx, yy, pos;

	for (i = 0; i < nblobs; i++)
	{
		// Desenha as bordas horizontais
		for (xx = blobs[i].x; xx <= blobs[i].x + blobs[i].width; xx++) {
			pos = blobs[i].y * bytesperline_src + xx * channels_src;
			datasrc[pos] = 0;       // R
			datasrc[pos + 1] = 255; // G
			datasrc[pos + 2] = 0;   // B

			pos = (blobs[i].y + blobs[i].height) * bytesperline_src + xx * channels_src;
			datasrc[pos] = 0;       // R
			datasrc[pos + 1] = 255; // G
			datasrc[pos + 2] = 0;   // B
		}

		// Desenha as bordas verticais
		for (yy = blobs[i].y; yy <= blobs[i].y + blobs[i].height; yy++) {
			pos = yy * bytesperline_src + blobs[i].x * channels_src;
			datasrc[pos] = 0;       // R
			datasrc[pos + 1] = 255; // G
			datasrc[pos + 2] = 0;   // B

			pos = yy * bytesperline_src + (blobs[i].x + blobs[i].width) * channels_src;
			datasrc[pos] = 0;       // R
			datasrc[pos + 1] = 255; // G
			datasrc[pos + 2] = 0;   // B
		}
	}
	return 1;
}





