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
#include "vc.h"
#include <math.h>

#define MAX


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUN��ES: ALOCAR E LIBERTAR UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


// Alocar mem�ria para uma imagem
IVC* vc_image_new(int width, int height, int channels, int levels)
{
	IVC* image = (IVC*)malloc(sizeof(IVC));

	if (image == NULL) return NULL;
	if ((levels <= 0) || (levels > 255)) return NULL;

	image->width = width;
	image->height = height;
	image->channels = channels;
	image->levels = levels;
	image->bytesperline = image->width * image->channels;
	image->data = (unsigned char*)malloc(image->width * image->height * image->channels * sizeof(char));

	if (image->data == NULL)
	{
		return vc_image_free(image);
	}

	return image;
}


// Libertar mem�ria de uma imagem
IVC* vc_image_free(IVC* image)
{
	if (image != NULL)
	{
		if (image->data != NULL)
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


char* netpbm_get_token(FILE* file, char* tok, int len)
{
	char* t;
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
		do
		{
			*t++ = c;
			c = getc(file);
		} while ((!isspace(c)) && (c != '#') && (c != EOF) && (t - tok < len - 1));

		if (c == '#') ungetc(c, file);
	}

	*t = 0;

	return tok;
}


long int unsigned_char_to_bit(unsigned char* datauchar, unsigned char* databit, int width, int height)
{
	int x, y;
	int countbits;
	long int pos, counttotalbytes;
	unsigned char* p = databit;

	*p = 0;
	countbits = 1;
	counttotalbytes = 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = width * y + x;

			if (countbits <= 8)
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
			if ((countbits > 8) || (x == width - 1))
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


void bit_to_unsigned_char(unsigned char* databit, unsigned char* datauchar, int width, int height)
{
	int x, y;
	int countbits;
	long int pos;
	unsigned char* p = databit;

	countbits = 1;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = width * y + x;

			if (countbits <= 8)
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
			if ((countbits > 8) || (x == width - 1))
			{
				p++;
				countbits = 1;
			}
		}
	}
}


IVC* vc_read_image(char* filename)
{
	FILE* file = NULL;
	IVC* image = NULL;
	unsigned char* tmp;
	char tok[20];
	long int size, sizeofbinarydata;
	int width, height, channels;
	int levels = 255;
	int v;

	// Abre o ficheiro
	if ((file = fopen(filename, "rb")) != NULL)
	{
		// Efectua a leitura do header
		netpbm_get_token(file, tok, sizeof(tok));

		if (strcmp(tok, "P4") == 0) { channels = 1; levels = 1; }	// Se PBM (Binary [0,1])
		else if (strcmp(tok, "P5") == 0) channels = 1;				// Se PGM (Gray [0,MAX(level,255)])
		else if (strcmp(tok, "P6") == 0) channels = 3;				// Se PPM (RGB [0,MAX(level,255)])
		else
		{
#ifdef VC_DEBUG
			printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM, PGM or PPM file.\n\tBad magic number!\n");
#endif

			fclose(file);
			return NULL;
		}

		if (levels == 1) // PBM
		{
			if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 ||
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
			if (image == NULL) return NULL;

			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height;
			tmp = (unsigned char*)malloc(sizeofbinarydata);
			if (tmp == NULL) return 0;

#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
#endif

			if ((v = fread(tmp, sizeof(unsigned char), sizeofbinarydata, file)) != sizeofbinarydata)
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
			if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 ||
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
			if (image == NULL) return NULL;

#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
#endif

			size = image->width * image->height * image->channels;

			if ((v = fread(image->data, sizeof(unsigned char), size, file)) != size)
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


int vc_write_image(char* filename, IVC* image)
{
	FILE* file = NULL;
	unsigned char* tmp;
	long int totalbytes, sizeofbinarydata;

	if (image == NULL) return 0;

	if ((file = fopen(filename, "wb")) != NULL)
	{
		if (image->levels == 1)
		{
			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height + 1;
			tmp = (unsigned char*)malloc(sizeofbinarydata);
			if (tmp == NULL) return 0;

			fprintf(file, "%s %d %d\n", "P4", image->width, image->height);

			totalbytes = unsigned_char_to_bit(image->data, tmp, image->width, image->height);
			printf("Total = %ld\n", totalbytes);
			if (fwrite(tmp, sizeof(unsigned char), totalbytes, file) != totalbytes)
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

			if (fwrite(image->data, image->bytesperline, image->height, file) != image->height)
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
int vc_gray_negative(IVC* srcdst)
{
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	// Verificação de erros
	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
	if (channels != 1) return 0;

	// Inverte a imagem Gray
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			data[pos] = 255 - data[pos];
		}
	}
	return 1;
}

int vc_rgb_negative(IVC* srcdst)
{
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	// Verificação de erros
	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
	if (channels != 3) return 0;

	// Inverte a imagem RGB
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
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
int vc_rgb_get_red_gray(IVC* srcdst)
{
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;
	// Verificação de erros 
	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0; if (channels != 3) return 0;
	// Extrai a componente Red for(y=0; y<height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;
			data[pos + 1] = data[pos]; // Green data [pos + 2] = data[pos]; // Blue
		}
	}
	return 1;
}

// Converter de RGB para Gray 
int vc_rgb_to_gray(IVC* src, IVC* dst)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;
	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y;
	long int pos_src, pos_dst;
	float rf, gf, bf;
	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->channels != 3) || (dst->channels != 1)) return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			rf = (float)datasrc[pos_src];
			gf = (float)datasrc[pos_src + 1];
			bf = (float)datasrc[pos_src + 2];
			datadst[pos_dst] = (unsigned char)((rf * 0.299) + (gf * 0.587) + (bf * 0.114));
		}
	}

	return 1;
}

/*int vc_rgb_to_hsv(IVC* srcdst)
{
	unsigned char* data = (unsigned char*)srcdst->data;
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

	for (i = 0; i < size; i = i + channels)
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
		data[i] = (unsigned char)(hue / 360.0f * 255.0f);
		data[i + 1] = (unsigned char)(saturation);
		data[i + 2] = (unsigned char)(value);

	}

	return 1;
}
*/

// hmin,hmax = [0, 360]; smin,smax = [0, 100]; vmin,vmax = [0, 100]
int vc_hsv_segmentation(IVC* src, IVC* dst, int hmin, int hmax, int smin, int smax, int vmin, int vmax)
{
	unsigned char* data_src = (unsigned char*)src->data;
	unsigned char* data_dst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int channels_src = src->channels;
	int channels_dst = dst->channels;
	float h, s, v;
	int i, j, size_src, size_dst;

	if ((src->width) <= 0 || (src->height) <= 0 || (src->data == NULL) || (dst->data == NULL))
		return 0;
	if (src->channels != 3 || dst->channels != 1)
		return 0;

	size_src = width * height * channels_src;
	size_dst = width * height * channels_dst;

	if (size_dst != (width * height))
		return 0;

	for (i = 0, j = 0; i < size_src; i += channels_src, j += channels_dst)
	{
		h = (float)data_src[i] * 360.0f / 255.0f;
		s = (float)data_src[i + 1] * 100.0f / 255.0f;
		v = (float)data_src[i + 2] * 100.0f / 255.0f;

		if (h >= hmin && h <= hmax && s >= smin && s <= smax && v >= vmin && v <= vmax)
		{
			data_dst[j] = 255;
		}
		else
		{
			data_dst[j] = 0;
		}
	}
	return 1;
}



// Função para converter escala de cinzas para RGB
int vc_scale_gray_to_rgb(IVC* src, IVC* dst) {
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
			pos = y * dst->width * dst->channels + x * dst->channels;

			// Obtenha o valor de pixel original
			unsigned char pixel_value = src->data[y * src->width + x];

			// Atribuir o valor de pixel aos canais R, G e B
			dst->data[pos] = pixel_value; // Canal R
			dst->data[pos + 1] = pixel_value; // Canal G
			dst->data[pos + 2] = pixel_value; // Canal B
		}
	}

	return 1;
}

// Gerar binário da imagem Gray
int vc_gray_to_binary(IVC* srcdst, int threshold)
{
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	// Verificação de erros
	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
	if (channels != 1) return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			if (data[pos] > threshold) {
				data[pos] = 255;
			}
			else
			{
				data[pos] = 0;
			}

		}
	}
	return 1;
}

int vc_gray_to_binary_global_mean(IVC* srcdst) {
	unsigned char* data = (unsigned char*)srcdst->data;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int width = srcdst->width;
	int height = srcdst->height;;
	int x, y;
	long int pos;
	float threshold = 0;

	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL))return 0;
	if ((srcdst->channels != 1))return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;
			threshold += data[pos];
		}
	}

	threshold = threshold / (width * height);

	printf("%f", threshold);

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			if (data[pos] > threshold) {
				data[pos] = 255;
			}
			else
			{
				data[pos] = 0;
			}

		}
	}
}

int vc_gray_to_binary_midpoint(IVC* srcdst, int kernel) {
	unsigned char* data = (unsigned char*)srcdst->data;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int width = srcdst->width;
	int height = srcdst->height;
	int x, y, x2, y2;
	long int pos;
	float threshold = 0;
	int max, min;

	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL))return 0;
	if ((srcdst->channels != 1)) return 0;
	kernel *= 0.5;
	for (y = kernel; y < height - kernel; y++)
	{
		for (x = kernel; x < width - kernel; x++)
		{
			max = 0;
			min = 255;

			for (y2 = y - kernel; y2 <= y + kernel; y2++)
			{
				for (x2 = x - kernel; x2 <= x + kernel; x2++)
				{
					pos = y2 * bytesperline + x2 * channels;
					if (data[pos] > max) { max = data[pos]; }
					else if (data[pos] < min) { min = data[pos]; }
				}
			}
			threshold = 0.5 * (max + min);

			pos = y * bytesperline + x * channels;
			if (data[pos] < threshold) {
				data[pos] = 255;
			}
			else
			{
				data[pos] = 0;
			}
		}
	}
	return 1;
}

int vc_gray_to_binary_bernsen(IVC* src, IVC* dst, int kernel, int cmin) {
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_src = src->width * src->channels;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_src = src->channels;
	int channels_dst = dst->channels;
	int levels = src->levels;
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

			if ((max - min) < cmin) {
				threshold = levels / 2;
			}
			else {
				threshold = 0.5 * (max + min);
			}

			pos_src = y * bytesperline_src + x * channels_src;
			datadst[pos_dst] = (datasrc[pos_src] < threshold) ? 255 : 0;
		}
	}

	return 1;
}

int vc_gray_to_binary_niblack(IVC* src, IVC* dst, int kernel, float k)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
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

int vc_binary_dilate(IVC* src, IVC* dst, int kernel)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;
	int x, y, x2, y2;
	long int pos_src, pos_dst;
	int verifica;
	kernel *= 0.5;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))return 0;
	if ((src->width != dst->width) || (src->height != dst->height))return 0;
	if ((src->channels != 1) || (dst->channels != 1))return 0;


	for (y = kernel; y < height - kernel; y++)
	{
		for (x = kernel; x < width - kernel; x++)
		{
			pos_dst = y * bytesperline_dst + x * channels_dst;

			verifica = 0;

			for (y2 = y - kernel; y2 <= y + kernel; y2++)
			{
				for (x2 = x - kernel; x2 <= x + kernel; x2++)
				{
					pos_src = y2 * bytesperline_src + x2 * channels_src;
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
	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;
	int x, y, x2, y2;
	long int pos_src, pos_dst;
	int verifica;
	kernel *= 0.5;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))return 0;
	if ((src->width != dst->width) || (src->height != dst->height))return 0;
	if ((src->channels != 1) || (dst->channels != 1))return 0;


	for (y = kernel; y < height - kernel; y++)
	{
		for (x = kernel; x < width - kernel; x++)
		{
			pos_dst = y * bytesperline_dst + x * channels_dst;

			verifica = 0;

			for (y2 = y - kernel; y2 <= y + kernel; y2++)
			{
				for (x2 = x - kernel; x2 <= x + kernel; x2++)
				{
					pos_src = y2 * bytesperline_src + x2 * channels_src;
					if (datasrc[pos_src] == 0) { verifica = 1; }
				}
			}

			if (verifica == 1) { datadst[pos_dst] = 0; }
			else { datadst[pos_dst] = 255; }

		}
	}


	return 1;
}

int vc_binary_open(IVC* src, IVC* dst, int kernel)
{
	int verifica = 1;
	IVC* dstTemp = vc_image_new(src->width, src->height, src->channels, src->levels);

	verifica &= vc_binary_erode(src, dstTemp, kernel);
	verifica &= vc_binary_dilate(dstTemp, dst, kernel);

	vc_image_free(dstTemp);

	return verifica;
}

int vc_binary_close(IVC* src, IVC* dst, int kernel)
{
	int verifica = 1;
	IVC* dstTemp = vc_image_new(src->width, src->height, src->channels, src->levels);

	verifica &= vc_binary_dilate(src, dstTemp, kernel);
	verifica &= vc_binary_erode(dstTemp, dst, kernel);

	vc_image_free(dstTemp);

	return verifica;
}

// Gerar binário da imagem Gray
int vc_subtrair_imagem(IVC* imagem1, IVC* imagem2)
{
	unsigned char* data1 = (unsigned char*)imagem1->data;
	int width = imagem1->width;
	int height = imagem1->height;
	int bytesperline = imagem1->width * imagem1->channels;
	int channels = imagem1->channels;

	unsigned char* data2 = (unsigned char*)imagem2->data;
	int x, y;
	long int pos;

	// Verificação de erros
	if ((imagem1->width <= 0) || (imagem1->height <= 0) || (imagem1->data == NULL)) return 0;
	if (channels != 1) return 0;


	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			if (data1[pos] == 255)
			{
				data2[pos] = 0;
			}
			else
			{
				data2[pos] = 255;
			}
		}
	}
	return 1;
}

/*OVC* vc_binary_blob_labelling(IVC* src, IVC* dst, int* nlabels)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
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
	OVC* blobs; // Apontador para array de blobs (objectos) que será retornado desta função.

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return NULL;
	if (channels != 1) return NULL;

	// Copia dados da imagem binária para imagem grayscale
	memcpy(datadst, datasrc, bytesperline * height);

	// Todos os pixéis de plano de fundo devem obrigatóriamente ter valor 0
	// Todos os pixéis de primeiro plano devem obrigatóriamente ter valor 255
	// Serão atribuídas etiquetas no intervalo [1,254]
	// Este algoritmo está assim limitado a 255 labels
	for (i = 0, size = bytesperline * height; i < size; i++)
	{
		if (datadst[i] != 0) datadst[i] = 255;
	}

	// Limpa os rebordos da imagem binária
	for (y = 0; y < height; y++)
	{
		datadst[y * bytesperline + 0 * channels] = 0;
		datadst[y * bytesperline + (width - 1) * channels] = 0;
	}
	for (x = 0; x < width; x++)
	{
		datadst[0 * bytesperline + x * channels] = 0;
		datadst[(height - 1) * bytesperline + x * channels] = 0;
	}

	// Efectua a etiquetagem
	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
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
							for (tmplabel = labeltable[datadst[posA]], a = 1; a < label; a++)
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
							for (tmplabel = labeltable[datadst[posB]], a = 1; a < label; a++)
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
							for (tmplabel = labeltable[datadst[posC]], a = 1; a < label; a++)
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
							for (tmplabel = labeltable[datadst[posC]], a = 1; a < label; a++)
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
	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
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
	for (a = 1; a < label - 1; a++)
	{
		for (b = a + 1; b < label; b++)
		{
			if (labeltable[a] == labeltable[b]) labeltable[b] = 0;
		}
	}
	// Passo 2: Conta etiquetas e organiza a tabela de etiquetas, para que não hajam valores vazios (zero) entre etiquetas
	*nlabels = 0;
	for (a = 1; a < label; a++)
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
	blobs = (OVC*)calloc((*nlabels), sizeof(OVC));
	if (blobs != NULL)
	{
		for (a = 0; a < (*nlabels); a++) blobs[a].label = labeltable[a];
	}
	else return NULL;


	return blobs;
}
*/

int vc_binary_blob_info(IVC* src, OVC* blobs, int nblobs)
{
	unsigned char* data = (unsigned char*)src->data;
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
	for (i = 0; i < nblobs; i++)
	{
		xmin = width - 1;
		ymin = height - 1;
		xmax = 0;
		ymax = 0;

		sumx = 0;
		sumy = 0;

		blobs[i].area = 0;

		for (y = 1; y < height - 1; y++)
		{
			for (x = 1; x < width - 1; x++)
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

int vc_gray_histogram_show(IVC* src, IVC* dst) {
	if (src == NULL || dst == NULL) return -1;
	if (src->channels != 1 || dst->channels != 1) return -1;  // Verificar se ambas são imagens em escala de cinza
	if (src->width * src->height == 0 || dst->width * dst->height == 0) return -1;
	if (dst->width != 256 || dst->height != 256) return -1;  // Garantindo que a imagem de destino tem as dimensões corretas

	int ni[256] = { 0 };
	unsigned char* datasrc = src->data;
	unsigned char* datadst = dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int n = width * height;

	// Contar o número de pixels para cada valor de brilho
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			unsigned char value = datasrc[y * bytesperline + x];
			ni[value]++;
		}
	}

	// Calcular a função densidade de probabilidade (pdf)
	float pdf[256];
	for (int i = 0; i < 256; i++) {
		pdf[i] = (float)ni[i] / (float)n;
	}

	// Encontrar o pdf máximo
	float pdfmax = 0;
	for (int i = 0; i < 256; i++) {
		if (pdf[i] > pdfmax) {
			pdfmax = pdf[i];
		}
	}

	// Normalizar o pdf
	float pdfnorm[256];
	for (int i = 0; i < 256; i++) {
		pdfnorm[i] = pdf[i] / pdfmax;
	}

	// Limpar a imagem de destino
	for (int i = 0; i < 256 * 256; i++) {
		datadst[i] = 0;
	}

	// Desenhar o histograma na imagem de destino
	for (int x = 0; x < 256; x++) {
		int height = (int)(pdfnorm[x] * 255.0);  // Altura da coluna do histograma
		for (int y = 255; y >= 256 - height; y--) {
			datadst[y * 256 + x] = 255;  // Pintando de branco
		}
	}

	return 0;  // Sucesso
}

int vc_gray_histogram_equalization(IVC* src, IVC* dst) {
	if (src == NULL || dst == NULL) return -1;
	if (src->channels != 1 || dst->channels != 1) return -1;  // Apenas para imagens em escala de cinza
	if (src->width != dst->width || src->height != dst->height) return -1;  // As imagens devem ter o mesmo tamanho

	int width = src->width;
	int height = src->height;
	int n = width * height; // Total de pixels na imagem
	int ni[256] = { 0 };
	float pdf[256] = { 0 };
	float cdf[256] = { 0 };

	// Contar o número de pixels para cada intensidade de cinza
	for (int i = 0; i < n; i++) {
		ni[src->data[i]]++;
	}

	// Calcular PDF
	for (int i = 0; i < 256; i++) {
		pdf[i] = (float)ni[i] / n;
	}

	// Calcular CDF
	cdf[0] = pdf[0];
	for (int i = 1; i < 256; i++) {
		cdf[i] = cdf[i - 1] + pdf[i];
	}

	// Encontrar o valor mínimo do CDF que não é zero
	float cdfmin = 0;
	for (int i = 0; i < 256; i++) {
		if (cdf[i] != 0) {
			cdfmin = cdf[i];
			break;
		}
	}

	// Aplicar a equalização do histograma
	for (int i = 0; i < n; i++) {
		unsigned char pixel = src->data[i];
		float value = (cdf[pixel] - cdfmin) / (1 - cdfmin) * 255;
		dst->data[i] = (unsigned char)(value + 0.5f); // Arredondar para o valor mais próximo
	}

	return 0;  // Sucesso
}

int vc_gray_edge_prewitt(IVC* src, IVC* dst, float th) {
	unsigned char* data = (unsigned char*)src->data;
	int width = src->width;
	int height = src->height;
	int byteperline = src->width * src->channels;
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

			posA = (y - 1) * byteperline + (x - 1) * channels;
			posB = (y - 1) * byteperline + (x)*channels;
			posC = (y - 1) * byteperline + (x + 1) * channels;
			posD = (y)*byteperline + (x - 1) * channels;
			posE = (y)*byteperline + (x + 1) * channels;
			posF = (y + 1) * byteperline + (x - 1) * channels;
			posG = (y + 1) * byteperline + (x)*channels;
			posH = (y + 1) * byteperline + (x + 1) * channels;
			mx = ((-1 * data[posA]) + (1 * data[posC]) + (-1 * data[posD]) + (1 * data[posE]) + (-1 * data[posF]) + (1 * data[posH])) / 3; //?
			my = ((-1 * data[posA]) + (1 * data[posF]) + (-1 * data[posB]) + (1 * data[posG]) + (-1 * data[posC]) + (1 * data[posH])) / 3;

			mag = sqrt((mx * mx) + (my * my));

			if (mag > th)
				dst->data[pos] = 255;
			else
				dst->data[pos] = 0;
		}
	}
	return 1;
}

int vc_gray_edge_sobel(IVC* src, IVC* dst, float th) {
	unsigned char* data = (unsigned char*)src->data;
	int width = src->width;
	int height = src->height;
	int byteperline = src->width * src->channels;
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

			posA = (y - 1) * byteperline + (x - 1) * channels;
			posB = (y - 1) * byteperline + (x)*channels;
			posC = (y - 1) * byteperline + (x + 1) * channels;
			posD = (y)*byteperline + (x - 1) * channels;
			posE = (y)*byteperline + (x + 1) * channels;
			posF = (y + 1) * byteperline + (x - 1) * channels;
			posG = (y + 1) * byteperline + (x)*channels;
			posH = (y + 1) * byteperline + (x + 1) * channels;

			mx = ((-1 * data[posA]) + (1 * data[posC]) + (-2 * data[posD]) + (2 * data[posE]) + (-1 * data[posF]) + (1 * data[posH])) / 3;
			my = ((-1 * data[posA]) + (1 * data[posF]) + (-2 * data[posB]) + (2 * data[posG]) + (-1 * data[posC]) + (1 * data[posH])) / 3;

			mag = sqrt((mx * mx) + (my * my));

			if (mag > th)
				dst->data[pos] = 255;
			else
				dst->data[pos] = 0;
		}
	}
	return 1;
}

int vc_desenha_box(IVC* src, OVC* blobs, int nblobs, int* counter)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;
	int i, xx, yy, pos;
	int middle_y = src->height / 2;
	int middle_x = src->width / 2;

	for (i = 0; i < nblobs; i++)
	{
		if (blobs[i].width > 150 && blobs[i].area > 11000 && blobs[i].area < 21000 && blobs[i].height > 80 && blobs[i].height <= 115)
		{
			// Verificar se o blob cruza a linha do meio do ecrã em ambos os eixos x e y
			if ((blobs[i].y <= middle_y && blobs[i].y + blobs[i].height >= middle_y) &&
				(blobs[i].x <= middle_x && blobs[i].x + blobs[i].width >= middle_x))
			{
				(*counter)++;
			}

			for (yy = blobs[i].y; yy <= blobs[i].y + blobs[i].height; yy++)
			{
				for (xx = blobs[i].x; xx <= blobs[i].x + blobs[i].width; xx++)
				{
					pos = yy * bytesperline_src + xx * channels_src;

					if (yy == blobs[i].y || yy == blobs[i].y + blobs[i].height || xx == blobs[i].x || xx == blobs[i].x + blobs[i].width)
					{
						datasrc[pos] = 255;
						datasrc[pos + 1] = 0;
						datasrc[pos + 2] = 0;
					}
				}
			}
		}
	}
	return 1;
}

int vc_convert_bgr_to_rgb(IVC* src, IVC* dst)
{
	unsigned char* data = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;
	int channels_dst = dst->channels;
	int x, y, i, j;
	long int pos;

	//Verificacao de erros
	if ((width <= 0) || (height <= 0) || (data == NULL)) return 0;
	if (channels != 3 || channels_dst != 3) return 0;
	//Verifica se existe blobs

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * dst->bytesperline + x * dst->channels;
			int* aux = datadst[pos];
			datadst[pos] = data[pos + 2];
			//datadst[pos + 1] = data[pos + 1];
			datadst[pos + 2] = aux;
		}
	}

	return 1;
}

void vc_rgb_value_to_hsv(unsigned char r, unsigned char g, unsigned char b, float* h, float* s, float* v)
{
	float red, green, blue;
	float max, min, delta;

	// Normalize the RGB values to the range [0, 1]
	red = r / 255.0;
	green = g / 255.0;
	blue = b / 255.0;

	// Find the maximum and minimum values among R, G, B
	max = (red > green) ? (red > blue ? red : blue) : (green > blue ? green : blue);
	min = (red < green) ? (red < blue ? red : blue) : (green < blue ? green : blue);

	// Compute the difference (delta) between max and min
	delta = max - min;

	// Calculate the Value (V)
	*v = max;

	// Calculate the Saturation (S)
	if (max == 0) {
		*s = 0;
	}
	else {
		*s = delta / max;
	}

	// Calculate the Hue (H)
	if (delta == 0) {
		*h = 0; // Undefined hue, set to 0
	}
	else {
		if (max == red) {
			*h = 60 * (fmod(((green - blue) / delta), 6));
		}
		else if (max == green) {
			*h = 60 * (((blue - red) / delta) + 2);
		}
		else if (max == blue) {
			*h = 60 * (((red - green) / delta) + 4);
		}
	}

	if (*h < 0) {
		*h += 360;
	}
}

int vc_cor_resistencia(OVC* blobs, IVC* src, int pos)
{
	unsigned char* data_src = src->data;
	int width = src->width;
	int channels = src->channels;
	int bytesperline = width * channels;

	// Tabela de cores e seus valores correspondentes
	int color_values[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }; // Preto a Branco
	float multiplier_values[10] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 0.1, 0.01 }; // Preto a Roxo, Dourado, Prata
	float tolerance_values[4] = { 0.01, 0.02, 0.05, 0.1 }; // Castanho (1%), Vermelho (2%), Dourado (5%), Prata (10%)

	// Identificação das faixas de cores
	int faixa1 = -1, faixa2 = -1, faixa3 = -1, faixa4 = -1;
	int counts[12] = { 0 }; // Contador de ocorrências de cada cor

	unsigned char blue = data_src[pos];
	unsigned char green = data_src[pos + 1];
	unsigned char red = data_src[pos + 2];


	float h, s, v;
	vc_rgb_value_to_hsv(red, green, blue, &h, &s, &v);

	s = (int)(s * 100);
	v = (int)(v * 100);


	if (s > 0 && s <= 20 && v < 25)
	{
		//Preto
		counts[0]++;
		//printf("Preto encontrado\n");
	}
	if ((h >= 80 && h <= 120) && (s > 0 && s < 100) && (v > 27 && v < 100))
	{
		//Verde
		counts[5]++;
		//printf("Verde encontrado\n");
	}
	if ((h >= 170 && h <= 220) && (v > 30 && v < 100))
	{
		//Azul
		counts[6]++;
		//printf("Azul encontrado\n");
	}
	if (((h > 0 && h < 20) || (h >= 300 && h <= 360)) && (s > 20))
	{
		if (s > 55 && v > 55)
		{
			//Vermelho
			counts[2]++;
			//printf("Vermelho encontrado\n");
		}
		else if (s < 55 && v < 55)
		{
			//Castanho
			counts[1]++;
			//printf("Castanho encontrado\n");
		}
	}



	// Determinar as cores predominantes (as três faixas com maior contagem)
	for (int i = 0; i < 3; i++)
	{
		int max_count = 0;
		int max_index = -1;
		for (int j = 0; j < 10; j++)
		{
			if (counts[j] > max_count)
			{
				max_count = counts[j];
				max_index = j;
			}
		}
		if (i == 0)
		{
			faixa1 = max_index;
		}
		else if (i == 1)
		{
			faixa2 = max_index;
		}
		else if (i == 2)
		{
			faixa3 = max_index;
		}
		counts[max_index] = 0; // Reseta para evitar contar a mesma faixa novamente
	}

	// Determinar a quarta faixa (tolerância)
	for (int j = 0; j < 12; j++)
	{
		if (counts[j] > 0)
		{
			faixa4 = j;
			break;
		}
	}

	// Calcular o valor da resistência
	int resistor_value = (color_values[faixa1] * 10 + color_values[faixa2]) * multiplier_values[faixa3];
	//printf("\ncolor_values[faixa1] = %d\n", color_values[faixa1]);
	//printf("\ncolor_values[faixa2] = %d\n", color_values[faixa2]);
	//printf("\nmultiplier_values[faixa3] = %.1f\n", multiplier_values[faixa3]);
	//float tolerance = tolerance_values[faixa4];
	//printf("\nresistor_value = %d\n", resistor_value);
	// Exibir valor da resistência (somente valor, ignorando tolerância para simplificação)
	return resistor_value;
}

int vc_desenha_centroide(IVC* src, OVC* blobs, int nblobs)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;
	int i, cx, cy, pos;
	int cross_size = 5; // Define o tamanho da cruz (11x11 pixels)

	for (i = 0; i < nblobs; i++)
	{
		// Verifica se o blob atende aos critérios especificados
		if (blobs[i].width > 150 && blobs[i].area > 11000 && blobs[i].area < 21000 && blobs[i].height > 80 && blobs[i].height <= 115)
		{
			// Calcula as coordenadas do centro de massa
			cx = blobs[i].x + blobs[i].width / 2;
			cy = blobs[i].y + blobs[i].height / 2;

			// Desenha uma cruz maior no centro de massa
			for (int dy = -cross_size; dy <= cross_size; dy++)
			{
				if ((cy + dy) >= 0 && (cy + dy) < src->height)
				{
					for (int c = 0; c < channels_src; c++)
					{
						pos = (cy + dy) * bytesperline_src + cx * channels_src;
						datasrc[pos + c] = 0; // Preto
					}
				}
			}

			for (int dx = -cross_size; dx <= cross_size; dx++)
			{
				if ((cx + dx) >= 0 && (cx + dx) < src->width)
				{
					for (int c = 0; c < channels_src; c++)
					{
						pos = cy * bytesperline_src + (cx + dx) * channels_src;
						datasrc[pos + c] = 0; // Preto
					}
				}
			}
		}
	}
	return 1;
}

// Função para detectar e mapear a cor
int detectColor(Color pixel) {
	// Definir as cores possíveis e seus valores
	ColorMapping colorMap[] = {
		{{0, 0, 0}, 0},       // Preto
		{{128, 0, 0}, 1},     // Marrom
		{{255, 0, 0}, 2},     // Vermelho
		{{255, 165, 0}, 3},   // Laranja
		{{255, 255, 0}, 4},   // Amarelo
		{{0, 255, 0}, 5},     // Verde
		{{0, 0, 255}, 6},     // Azul
		{{128, 0, 128}, 7},   // Violeta
		{{128, 128, 128}, 8}, // Cinza
		{{255, 255, 255}, 9}  // Branco
	};

	// Verificar a cor mais próxima
	for (int i = 0; i < 10; i++) {
		if (abs(pixel.r - colorMap[i].color.r) < 50 &&
			abs(pixel.g - colorMap[i].color.g) < 50 &&
			abs(pixel.b - colorMap[i].color.b) < 50) {
			return colorMap[i].value;
		}
	}

	return -1; // Cor não encontrada
}

// Função para calcular o valor da resistência
int calculateResistance(int* digits, int size) {
	int value = 0;
	for (int i = 0; i < size - 1; ++i) {
		value = value * 10 + digits[i];
	}
	value *= pow(10, digits[size - 1]);
	return value;
}

// Função para detectar resistências em uma imagem
void detectResistors(IVC* frame, IVC* binary_frame)
{
	int x, y;
	int num_resistors = 0;

	// Supondo que temos uma função para detectar bounding boxes
	OVC* blobs = vc_binary_blob_labelling(binary_frame, binary_frame, &num_resistors);

	for (int i = 0; i < num_resistors; i++) {
		int x_min = blobs[i].x;
		int x_max = blobs[i].x + blobs[i].width;
		int y_min = blobs[i].y;
		int y_max = blobs[i].y + blobs[i].height;

		// Detectar as faixas de cores na região da resistência
		int digits[5] = { -1, -1, -1, -1, -1 };
		int digit_index = 0;

		for (x = x_min; x < x_max && digit_index < 5; x++) {
			int pixel_index = (y_min + (y_max - y_min) / 2) * frame->bytesperline + x * frame->channels;
			Color pixel = { frame->data[pixel_index], frame->data[pixel_index + 1], frame->data[pixel_index + 2] };
			int digit = detectColor(pixel);

			if (digit != -1 && (digit_index == 0 || digits[digit_index - 1] != digit)) {
				digits[digit_index] = digit;
				digit_index++;
			}
		}

		// Calcular o valor da resistência
		if (digit_index >= 4) {
			int resistanceValue = calculateResistance(digits, digit_index);
			//printf("Valor da resistência: %d ohms\n", resistanceValue);
		}
	}

	free(blobs);
}

int vc_hsv_segmentation_retornaImag(IVC* src, IVC* dst, int hmin, int hmax, int smin, int smax, int vmin, int vmax)
{
	unsigned char* data_src = (unsigned char*)src->data;
	unsigned char* data_dst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int channels_src = src->channels;
	int channels_dst = dst->channels;
	float h, s, v;
	int i, j, size_src, size_dst;

	if ((src->width) <= 0 || (src->height) <= 0 || (src->data == NULL) || (dst->data == NULL))
		return 0;
	if (src->channels != 3 || dst->channels != 1)
		return 0;

	size_src = width * height * channels_src;
	size_dst = width * height * channels_dst;

	if (size_dst != (width * height))
		return 0;

	for (i = 0, j = 0; i < size_src; i += channels_src, j += channels_dst)
	{
		h = (float)data_src[i] * 360.0f / 255.0f;
		s = (float)data_src[i + 1] * 100.0f / 255.0f;
		v = (float)data_src[i + 2] * 100.0f / 255.0f;

		if (h >= hmin && h <= hmax && s >= smin && s <= smax && v >= vmin && v <= vmax)
		{
			data_dst[j] = 255;
		}
		else
		{
			data_dst[j] = 0;
		}
	}
	return 1;
}


OVC* vc_binary_blob_labelling(IVC* src, IVC* dst, int* nlabels)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, a, b;
	long int i, size;
	long int posX, posA, posB, posC, posD;
	int labeltable[256] = { 0 };
	int labelarea[256] = { 0 };
	int label = 1;
	int num, tmplabel;
	OVC* blobs;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return NULL;
	if (channels != 1) return NULL;

	memcpy(datadst, datasrc, bytesperline * height);

	for (i = 0, size = bytesperline * height; i < size; i++)
	{
		if (datadst[i] != 0) datadst[i] = 255;
	}

	for (y = 0; y < height; y++)
	{
		datadst[y * bytesperline + 0 * channels] = 0;
		datadst[y * bytesperline + (width - 1) * channels] = 0;
	}
	for (x = 0; x < width; x++)
	{
		datadst[0 * bytesperline + x * channels] = 0;
		datadst[(height - 1) * bytesperline + x * channels] = 0;
	}

	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			posA = (y - 1) * bytesperline + (x - 1) * channels;
			posB = (y - 1) * bytesperline + x * channels;
			posC = (y - 1) * bytesperline + (x + 1) * channels;
			posD = y * bytesperline + (x - 1) * channels;
			posX = y * bytesperline + x * channels;

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

					if (datadst[posA] != 0) num = labeltable[datadst[posA]];
					if ((datadst[posB] != 0) && (labeltable[datadst[posB]] < num)) num = labeltable[datadst[posB]];
					if ((datadst[posC] != 0) && (labeltable[datadst[posC]] < num)) num = labeltable[datadst[posC]];
					if ((datadst[posD] != 0) && (labeltable[datadst[posD]] < num)) num = labeltable[datadst[posD]];

					datadst[posX] = num;
					labeltable[num] = num;

					if (datadst[posA] != 0)
					{
						if (labeltable[datadst[posA]] != num)
						{
							for (tmplabel = labeltable[datadst[posA]], a = 1; a < label; a++)
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
							for (tmplabel = labeltable[datadst[posB]], a = 1; a < label; a++)
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
							for (tmplabel = labeltable[datadst[posC]], a = 1; a < label; a++)
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
							for (tmplabel = labeltable[datadst[posC]], a = 1; a < label; a++)
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

	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			posX = y * bytesperline + x * channels;

			if (datadst[posX] != 0)
			{
				datadst[posX] = labeltable[datadst[posX]];
			}
		}
	}

	for (a = 1; a < label - 1; a++)
	{
		for (b = a + 1; b < label; b++)
		{
			if (labeltable[a] == labeltable[b]) labeltable[b] = 0;
		}
	}

	*nlabels = 0;
	for (a = 1; a < label; a++)
	{
		if (labeltable[a] != 0)
		{
			labeltable[*nlabels] = labeltable[a];
			(*nlabels)++;
		}
	}

	if (*nlabels == 0) return NULL;

	blobs = (OVC*)calloc((*nlabels), sizeof(OVC));
	if (blobs != NULL)
	{
		for (a = 0; a < (*nlabels); a++) blobs[a].label = labeltable[a];
	}
	else return NULL;

	return blobs;
}


int vc_binary_blob_infoTeste(IVC* src, OVC* blobs, int nblobs)
{
	unsigned char* data = (unsigned char*)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, i;
	long int pos;
	int xmin, ymin, xmax, ymax;
	long int sumx, sumy;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if (channels != 1) return 0;

	for (i = 0; i < nblobs; i++)
	{
		xmin = width - 1;
		ymin = height - 1;
		xmax = 0;
		ymax = 0;

		sumx = 0;
		sumy = 0;

		blobs[i].area = 0;

		for (y = 1; y < height - 1; y++)
		{
			for (x = 1; x < width - 1; x++)
			{
				pos = y * bytesperline + x * channels;

				if (data[pos] == blobs[i].label)
				{
					blobs[i].area++;

					sumx += x;
					sumy += y;

					if (xmin > x) xmin = x;
					if (ymin > y) ymin = y;
					if (xmax < x) xmax = x;
					if (ymax < y) ymax = y;

					if ((data[pos - 1] != blobs[i].label) || (data[pos + 1] != blobs[i].label) || (data[pos - bytesperline] != blobs[i].label) || (data[pos + bytesperline] != blobs[i].label))
					{
						blobs[i].perimeter++;
					}
				}
			}
		}

		blobs[i].x = xmin;
		blobs[i].y = ymin;
		blobs[i].width = (xmax - xmin) + 1;
		blobs[i].height = (ymax - ymin) + 1;

		blobs[i].xc = sumx / MAX(blobs[i].area, 1);
		blobs[i].yc = sumy / MAX(blobs[i].area, 1);
	}

	return 1;
}

int vc_draw_bounding_box(IVC* src, OVC* blobs, int nblobs, int xoff, int yoff, int frame, int* counter)
{
	unsigned char* data = (unsigned char*)src->data;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int i, xx, yy;
	int pos;
	int middle_y = src->height / 2;
	int middle_x = src->width / 2;

	for (i = 0; i < nblobs; i++)
	{
		if (blobs[i].width > 150 && blobs[i].area > 11000 && blobs[i].area < 21000 && blobs[i].height > 80 && blobs[i].height <= 115)
		{
			if (frame > 100 && (blobs[i].y <= middle_y && blobs[i].y + blobs[i].height >= middle_y) &&
				(blobs[i].x <= middle_x && blobs[i].x + blobs[i].width >= middle_x))
			{
				(*counter)++;
			}

			for (yy = blobs[i].y; yy <= blobs[i].y + blobs[i].height; yy++)
			{
				for (xx = blobs[i].x; xx <= blobs[i].x + blobs[i].width; xx++)
				{
					pos = yy * bytesperline + xx * channels;

					if (yy == blobs[i].y || yy == blobs[i].y + blobs[i].height || xx == blobs[i].x || xx == blobs[i].x + blobs[i].width)
					{
						data[pos] = 255;
						data[pos + 1] = 0;
						data[pos + 2] = 0;
					}
				}
			}
		}
	}
	return 1;
}

int vc_rgb_to_hsv(IVC* srcdst)
{
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->bytesperline;
	int channels = srcdst->channels;
	float r, g, b, hue, saturation, value;
	float rgb_max, rgb_min;
	int i, size;

	if ((width <= 0) || (height <= 0) || (data == NULL)) return 0;
	if (channels != 3) return 0;

	size = width * height * channels;

	for (i = 0; i < size; i = i + channels)
	{
		r = (float)data[i];
		g = (float)data[i + 1];
		b = (float)data[i + 2];

		rgb_max = (r > g ? (r > b ? r : b) : (g > b ? g : b));
		rgb_min = (r < g ? (r < b ? r : b) : (g < b ? g : b));

		value = rgb_max;
		if (value == 0.0f)
		{
			hue = 0.0f;
			saturation = 0.0f;
		}
		else
		{
			saturation = ((rgb_max - rgb_min) / rgb_max) * 255.0f;

			if (saturation == 0.0f)
			{
				hue = 0.0f;
			}
			else
			{
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

		data[i] = (unsigned char)(hue / 360.0f * 255.0f);
		data[i + 1] = (unsigned char)(saturation);
		data[i + 2] = (unsigned char)(value);
	}

	return 1;
}
