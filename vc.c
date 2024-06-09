//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLITECNICO DO CAVADO E DO AVE
//                          2023/2024
//             ENGENHARIA DE SISTEMAS INFORMATICOS
//                    VISAO POR COMPUTADOR
//
//             [  Rodrigo, Ruben, Thiago  ]
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Desabilita (no MSVC++) warnings de fun��es n�o seguras (fopen, sscanf, etc...)
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include "vc.h"

#pragma region Funcoes de leitura e escrita de uma imagem
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUN��ES: ALOCAR E LIBERTAR UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//Alocar memória para uma imagem
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

#pragma endregion

#pragma region Funcoes de manipulacao de imagem

// Gerar negativo da imagem Gray
int vc_gray_negative(IVC* srcdst)
{
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width; int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	// Verifica��o de erros
	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0; if (channels != 1) return 0;

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

//Gerar negativo de imagem RGB
int vc_rgb_negative(IVC* srcdst)
{
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width; int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	// Verifica��o de erros
	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0; if (channels != 3) return 0;

	//Inverte a imagem RGB
	for (y = 0; y < width; y++)
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

//Extrair vermelho
int vc_rgb_get_red(IVC* srcdst)
{
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	// Verifica��o de erros
	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0; if (channels != 3) return 0;
	if (channels != 3) return 0;

	//Extrair a cor vermelha
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			data[pos + 1] = 0;
			data[pos + 2] = 0;
		}
	}
	return 1;
}

//Extrair verde
int vc_rgb_get_green(IVC* srcdst)
{
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width; int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	// Verifica��o de erros
	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0; if (channels != 3) return 0;

	//Extrair a cor verde
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			data[pos] = 0;
			data[pos + 2] = 0;
		}
	}
	return 1;
}

//Extrair azul
int vc_rgb_get_blue(IVC* srcdst)
{
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width; int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	// Verifica��o de erros
	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0; if (channels != 3) return 0;
	if (channels != 3) return 0;

	//Extrair a cor azul
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			data[pos] = 0;
			data[pos + 1] = 0;
		}
	}
	return 1;
}


//Extrair vermelho e mudar para cinza
int vc_rgb_get_red_gray(IVC* srcdst)
{
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width; int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	// Verifica��o de erros
	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0; if (channels != 3) return 0;
	if (channels != 3) return 0;

	//Extrair a cor vermelha
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			data[pos + 1] = data[pos];
			data[pos + 2] = data[pos];
		}
	}
	return 1;
}

//Extrair verde e mudar para cinza
int vc_rgb_get_green_gray(IVC* srcdst)
{
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width; int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	// Verifica��o de erros
	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0; if (channels != 3) return 0;
	if (channels != 3) return 0;

	//Extrair a cor vermelha
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			data[pos] = data[pos + 1];
			data[pos + 2] = data[pos + 1];
		}
	}
	return 1;
}

//Extrair azul e mudar para cinza
int vc_rgb_get_blue_gray(IVC* srcdst)
{
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width; int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	// Verifica��o de erros
	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0; if (channels != 3) return 0;
	if (channels != 3) return 0;

	//Extrair a cor vermelha
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			data[pos] = data[pos + 2];
			data[pos + 1] = data[pos + 2];
		}
	}
	return 1;
}

//RGB -> Cinza
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

	// Verifica��o de erros
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

//BGR -> RGB
int vc_bgr_to_rgb(IVC* srcdst)
{
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->bytesperline;
	int channels = srcdst->channels;
	int pos;
	int x, y;

	//Verificação de erros
	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL))
		return 0;
	if (channels != 3)
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			unsigned char temp = data[pos];

			data[pos] = data[pos + 2];
			data[pos + 2] = temp;
		}
	}
	return 1;

}

//RGB -> HSV
int vc_rgb_to_hsv(IVC* src, IVC* dst)
{
	unsigned char* data = (unsigned char*)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;

	unsigned char* data_dst = (unsigned char*)dst->data;
	int width_dst = dst->width;
	int height_dst = dst->height;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;

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
		data_dst[i] = (unsigned char)(hue / 360.0f * 255.0f);
		data_dst[i + 1] = (unsigned char)(saturation);
		data_dst[i + 2] = (unsigned char)(value);

	}

	return 1;
}

//Segmentação de HSV, esta função recebe uma imagem e cria 2 com base nos parâmetros inseridos para cada imagem de saida
int vc_hsv_segmentation(IVC* src, IVC* dst, int hmin, int hmax, int smin,int smax, int vmin, int vmax)
{
	unsigned char* data = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int channels = src->channels;
	int hue, saturation, value;
	long int pos_src, pos_dst;
	int x, y;
	int bytesperline_src = src->width * src->channels;
	int bytesperline_dst = dst->width * dst->channels;

	// Verifica??o de erros
	if ((width <= 0) || (height <= 0) || (data == NULL) || datadst == NULL) return 0;
	if (width != dst->width || height != dst->height) return 0;
	if (channels != 3 || dst->channels != 1) return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels;
			pos_dst = y * bytesperline_dst + x;

			hue = (int)((float)data[pos_src] / 255.0f * 360.0f);
			saturation = (int)((float)data[pos_src + 1] / 255.0f * 100.0f);
			value = (int)((float)data[pos_src + 2] / 255.0f * 100.0f);

			if (hue >= hmin && hue <= hmax &&
				saturation >= smin && saturation <= smax &&
				value >= vmin && value <= vmax)
				datadst[pos_dst] = (unsigned char)255;
			else datadst[pos_dst] = (unsigned char)0;
		}
	}

	return 1;
}

//int vc_hsv_segmentation(IVC* src, IVC* dst1, IVC* dst2, int hmin1, int hmax1, int smin1, int smax1, int vmin1, int vmax1, int hmin2, int hmax2, int smin2, int smax2, int vmin2, int vmax2)
//{
//	unsigned char* data = (unsigned char*)src->data;
//	int width = src->width;
//	int height = src->height;
//	int bytesperline = src->bytesperline;
//	int channels = src->channels;
//
//	unsigned char* data_dst1 = (unsigned char*)dst1->data;
//	int width_dst1 = dst1->width;
//	int height_dst1 = dst1->height;
//	int bytesperline_dst1 = dst1->width * dst1->channels;
//	int channels_dst1 = dst1->channels;
//
//	unsigned char* data_dst2 = (unsigned char*)dst2->data;
//	int width_dst2 = dst2->width;
//	int height_dst2 = dst2->height;
//	int bytesperline_dst2 = dst2->width * dst2->channels;
//	int channels_dst2 = dst2->channels;
//
//	int h, s, v; // h=[0, 360] s=[0, 100] v=[0, 100]
//	int i, size;
//
//	// Verificação de erros
//	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
//	if (channels != 3) return 0;
//
//	size = width * height * channels;
//
//	for (i = 0; i < size; i = i + channels)
//	{
//		h = (int)(((float)data[i]) / 255.0f * 360.0f);
//		s = (int)(((float)data[i + 1]) / 255.0f * 100.0f);
//		v = (int)(((float)data[i + 2]) / 255.0f * 100.0f);
//
//		if ((h > hmin1) && (h <= hmax1) && (s >= smin1) && (s <= smax1) && (v >= vmin1) && (v <= vmax1))
//		{
//			data_dst1[i] = 255;
//			data_dst1[i + 1] = 255;
//			data_dst1[i + 2] = 255;
//		}
//		else
//		{
//			data_dst1[i] = 0;
//			data_dst1[i + 1] = 0;
//			data_dst1[i + 2] = 0;
//		}
//
//		if ((h > hmin2) && (h <= hmax2) && (s >= smin2) && (s <= smax2) && (v >= vmin2) && (v <= vmax2))
//		{
//			data_dst2[i] = 255;
//			data_dst2[i + 1] = 255;
//			data_dst2[i + 2] = 255;
//		}
//		else
//		{
//			data_dst2[i] = 0;
//			data_dst2[i + 1] = 0;
//			data_dst2[i + 2] = 0;
//		}
//	}
//
//	return 1;
//}

//Conta o número de pixeis a branco de uma imagem
int vc_count_pixels(IVC* src)
{
	unsigned char* data = (unsigned char*)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;

	int x, y, pos, count = 0;

	// Confere erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if (src->channels != 3)
		return 0;

	// Conta os píxeis
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			if (data[pos] != 0)
			{
				count++;
			}
		}
	}
	return count;
}


//Threshold manual
int vc_gray_to_binary(IVC* src, IVC* dst, int threshold)
{
	unsigned char* data = (unsigned char*)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;

	unsigned char* data_dst = (unsigned char*)dst->data;
	int width_dst = dst->width;
	int height_dst = dst->height;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;

	int pos_src, pos_dst, x, y;

	// Cofere erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((dst->width <= 0) || (dst->height <= 0))
		return 0;
	if ((src->channels != 1) || (dst->channels != 1))
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline + x * channels;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			if (data[pos_src] > threshold)
			{
				data_dst[pos_dst] = 255;
			}
			else if (data[pos_src] <= threshold)
			{
				data_dst[pos_dst] = 0;
			}
		}
	}
	return 1;
}

//Threshold média
int vc_gray_to_binary_global_mean(IVC* src, IVC* dst)
{
	unsigned char* data = (unsigned char*)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;

	unsigned char* data_dst = (unsigned char*)dst->data;
	int width_dst = dst->width;
	int height_dst = dst->height;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;

	int pos_src, pos_dst, x, y, z;
	float media;
	int soma = 0;

	// Cofere erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((dst->width <= 0) || (dst->height <= 0))
		return 0;
	if ((src->channels != 1) || (dst->channels != 1))
		return 0;

	for (z = 0; z < 2; z++)
	{
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				pos_src = y * bytesperline + x * channels;
				pos_dst = y * bytesperline_dst + x * channels_dst;

				if (z == 0)
				{
					soma += data[pos_src];
				}
				else if (z == 1)
				{
					if (data[pos_src] > (int)media)
					{
						data_dst[pos_dst] = 255;
					}
					else if (data[pos_src] <= (int)media)
					{
						data_dst[pos_dst] = 0;
					}
				}
			}
		}
		media = soma / (width * height);
		//printf("media %f, soma %d\n", media, soma);
	}
	return 1;
}

//Threshold por vizinhaça do píxel
int vc_gray_to_binary_kernel_8(IVC* src, IVC* dst)
{
	unsigned char* data = (unsigned char*)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;

	unsigned char* data_dst = (unsigned char*)dst->data;
	int width_dst = dst->width;
	int height_dst = dst->height;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;

	int pos_src, pos_dst, x, y, z;
	int soma = 0, media;

	// Confere erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((dst->width <= 0) || (dst->height <= 0))
		return 0;
	if ((src->channels != 1) || (dst->channels != 1))
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline + x * channels;
			pos_dst = y * bytesperline_dst + x * channels_dst;
			soma = 0;

			soma += (((pos_src - (width * y)) == 0) || (y == 0)) ? 0 : data[pos_src - 1 - width];

			soma += y == 0 ? 0 : data[pos_src - width];

			soma += ((width * (y + 1)) % ((pos_src + 1)) == 0 || (y == 0)) ? 0 : data[pos_src + 1 - width];

			soma += ((pos_src - (width * y)) == 0) ? 0 : data[pos_src - 1];

			soma += data[pos_src];

			soma += (width * (y + 1)) % ((pos_src + 1)) == 0 ? 0 : data[pos_src + 1];

			soma += ((pos_src - (width * y)) == 0) || (y == (width - 1)) ? 0 : data[pos_src - 1 + width];

			soma += (y == (width - 1)) ? 0 : data[pos_src];

			soma += ((width * (y + 1)) % ((pos_src + 1)) == 0 || (y == (width - 1))) ? 0 : data[pos_src + 1 + width];

			media = soma / 9;

			if (data[pos_src] > media)
			{
				data_dst[pos_dst] = 255;
			}
			else if (data[pos_src] <= media)
			{
				data_dst[pos_dst] = 0;
			}
		}
	}
	return 1;
}


// Etiquetagem de blobs
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
	int label = 1; // Etiqueta inicial.
	int num, tmplabel;
	OVC* blobs; // Apontador para lista de blobs (objectos) que ser� retornada desta fun��o.

	// Verifica��o de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return NULL;
	if (channels != 1) return NULL;

	// Copia dados da imagem bin�ria para imagem grayscale
	memcpy(datadst, datasrc, bytesperline * height);

	// Todos os pix�is de plano de fundo devem obrigat�riamente ter valor 0
	// Todos os pix�is de primeiro plano devem obrigat�riamente ter valor 255
	// Ser�o atribu�das etiquetas no intervalo [1,254]
	// Este algoritmo est� assim limitado a 254 labels
	for (i = 0, size = bytesperline * height; i < size; i++)
	{
		if (datadst[i] != 0) datadst[i] = 255;
	}

	// Limpa os rebordos da imagem bin�ria
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

					// Se A est� marcado
					if (datadst[posA] != 0) num = labeltable[datadst[posA]];
					// Se B est� marcado, e � menor que a etiqueta "num"
					if ((datadst[posB] != 0) && (labeltable[datadst[posB]] < num)) num = labeltable[datadst[posB]];
					// Se C est� marcado, e � menor que a etiqueta "num"
					if ((datadst[posC] != 0) && (labeltable[datadst[posC]] < num)) num = labeltable[datadst[posC]];
					// Se D est� marcado, e � menor que a etiqueta "num"
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
							for (tmplabel = labeltable[datadst[posD]], a = 1; a < label; a++)
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


	// Contagem do n�mero de blobs
	// Passo 1: Eliminar, da tabela, etiquetas repetidas
	for (a = 1; a < label - 1; a++)
	{
		for (b = a + 1; b < label; b++)
		{
			if (labeltable[a] == labeltable[b]) labeltable[b] = 0;
		}
	}
	// Passo 2: Conta etiquetas e organiza a tabela de etiquetas, para que n�o hajam valores vazios (zero) entre etiquetas
	*nlabels = 0;
	for (a = 1; a < label; a++)
	{
		if (labeltable[a] != 0)
		{
			labeltable[*nlabels] = labeltable[a]; // Organiza tabela de etiquetas
			(*nlabels)++; // Conta etiquetas
		}
	}

	// Se n�o h� blobs
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

//Obter propriedades de um blob
int vc_get_blob_properties(IVC* src, OVC* blobs, int nlabels)
{
	unsigned char* datadst = (unsigned char*)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;
	int x, y, i;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if (channels != 1)
		return 0;

	for (i = 0; i < (nlabels); i++)
	{
		int label;
		int xmin = width - 1;
		int ymin = height - 1;
		int xmax = 0;
		int ymax = 0;
		int area = 0;
		int perimeter = 0;
		int sum_x = 0;
		int sum_y = 0;
		int pixel_count = 0;
		label = blobs[i].label;
		label = nlabels; //Esta linha foi acrescentada para ultrapassar um obstáculo que não conseguíamos resolver. Gera problemas quando a imagem tem mais de um blob.

		// Percorre a imagem para encontrar os pontos extremos do blob
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				//printf("%hhu %d", datadst[y * bytesperline + x * channels], label);
				if (datadst[y * bytesperline + x * channels] == label)
				{
					if (x < xmin)
						xmin = x;
					if (y < ymin)
						ymin = y;
					if (x > xmax)
						xmax = x;
					if (y > ymax)
						ymax = y;
					area++;
					perimeter += 4 - ((datadst[y * bytesperline + (x - 1) * channels] == label) +
						(datadst[y * bytesperline + (x + 1) * channels] == label) +
						(datadst[(y - 1) * bytesperline + x * channels] == label) +
						(datadst[(y + 1) * bytesperline + x * channels] == label));

					// Calcula o centro de gravidade
					sum_x += x;
					sum_y += y;
					pixel_count++;
				}
			}
		}

		// Atribui os valores encontrados ao blob correspondente
		blobs[i].x = xmin;
		blobs[i].y = ymin;
		blobs[i].width = xmax - xmin + 1;
		blobs[i].height = ymax - ymin + 1;
		blobs[i].area = area;
		blobs[i].perimeter = perimeter;

		// Calcula as coordenadas do centro de gravidade
		if (pixel_count > 0)
		{
			blobs[i].xc = sum_x / pixel_count;
			blobs[i].yc = sum_y / pixel_count;
		}
		else
		{
			// Se o blob não contém pixels, define as coordenadas do centro de gravidade como -1
			blobs[i].xc = -1;
			blobs[i].yc = -1;
		}
	}
	return 1;
}

// Desenha a caixa delimitadora de um objeto
int vc_draw_boundingbox(IVC* srcdst, OVC* blob)
{
	int c;
	int x, y;
	int espessura = 5;

	for (y = blob->y; y < blob->y + blob->height; y++)
	{
		for (c = 0; c < srcdst->channels; c++)
		{
			for (int i = 0; i < espessura; i++)
			{
				srcdst->data[y * srcdst->bytesperline + (blob->x + i) * srcdst->channels + 1] = 255;
				srcdst->data[y * srcdst->bytesperline + (blob->x + blob->width - 1 - i) * srcdst->channels + 1] = 255;
			}
		}
	}

	for (x = blob->x; x < blob->x + blob->width; x++)
	{
		for (c = 0; c < srcdst->channels; c++)
		{
			for (int i = 0; i < espessura; i++)
			{
				srcdst->data[(blob->y + i) * srcdst->bytesperline + x * srcdst->channels + 1] = 255;
				srcdst->data[(blob->y + blob->height - 1 - i) * srcdst->bytesperline + x * srcdst->channels + 1] = 255;
			}
		}
	}

	return 1;
}

// Desenha o centro de gravidade de um objecto
int vc_draw_centerofgravity(IVC* srcdst, OVC* blob)
{
	int c;
	int x, y;
	int xmin, xmax, ymin, ymax;
	int s = 30;

	xmin = blob->xc - s;
	ymin = blob->yc - s;
	xmax = blob->xc + s;
	ymax = blob->yc + s;

	if (xmin < blob->x) xmin = blob->x;
	if (ymin < blob->y) ymin = blob->y;
	if (xmax > blob->x + blob->width - 1) xmax = blob->x + blob->width - 1;
	if (ymax > blob->y + blob->height - 1) ymax = blob->y + blob->height - 1;

	for (y = ymin; y <= ymax; y++)
	{
		for (c = 0; c < srcdst->channels; c++)
		{
			srcdst->data[y * srcdst->bytesperline + blob->xc * srcdst->channels + 1] = 255;
		}
	}

	for (x = xmin; x <= xmax; x++)
	{
		for (c = 0; c < srcdst->channels; c++)
		{
			srcdst->data[blob->yc * srcdst->bytesperline + x * srcdst->channels + 1] = 255;
		}
	}

	return 1;
}

// Dilatação
int vc_binary_dilate(IVC* src, IVC* dst, int kernel)
{
	unsigned char* data = (unsigned char*)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;

	unsigned char* data_dst = (unsigned char*)dst->data;
	int width_dst = dst->width;
	int height_dst = dst->height;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;

	int pos_src, pos_dst, pos_x, pos_k, x, y, z, xx, yy;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((dst->width <= 0) || (dst->height <= 0) || (dst->data == NULL))
		return 0;
	if ((src->channels != 1) || (dst->channels != 1))
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline + x * channels;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			data_dst[pos_dst] = 0;

			for (yy = -(kernel - 1) / 2; yy <= (kernel - 1) / 2; yy++)
			{
				for (xx = -(kernel - 1) / 2; xx <= (kernel - 1) / 2; xx++)
				{
					pos_k = ((yy + y) * bytesperline) + ((xx + x) * channels);
					if (((yy + y) >= 0) && ((xx + x) >= 0) && ((yy + y) < height) && ((xx + x) < width))
					{
						if (data[pos_k] != 0)
						{
							data_dst[pos_dst] = 1;
							break;
						}
					}
				}
				if (data_dst[pos_dst] == 1)
					break;
			}
		}
	}
	return 1;
}

//Erosão
int vc_binary_erode(IVC* src, IVC* dst, int kernel)
{
	unsigned char* data = (unsigned char*)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;

	unsigned char* data_dst = (unsigned char*)dst->data;
	int width_dst = dst->width;
	int height_dst = dst->height;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;

	int pos_src, pos_dst, pos_k, x, y, xx, yy;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((dst->width <= 0) || (dst->height <= 0) || (dst->data == NULL))
		return 0;
	if ((src->channels != 1) || (dst->channels != 1))
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline + x * channels;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			data_dst[pos_dst] = 255; // Define o pixel na imagem de destino como branco (255)

			for (yy = -(kernel - 1) / 2; yy <= (kernel - 1) / 2; yy++)
			{
				for (xx = -(kernel - 1) / 2; xx <= (kernel - 1) / 2; xx++)
				{
					pos_k = ((y + yy) * bytesperline) + ((x + xx) * channels);
					if (((y + yy) >= 0) && ((x + xx) >= 0) && ((y + yy) < height) && ((x + xx) < width))
					{
						if (data[pos_k] != 255)
						{ // Se encontrar um pixel preto (0), define o pixel na imagem de destino como preto (0)
							data_dst[pos_dst] = 0;
							break;
						}
					}
				}
				if (data_dst[pos_dst] == 0)
					break;
			}
		}
	}
	return 1;
}

//Abertura
int vc_binary_open(IVC* src, IVC* dst, IVC* aux, int kernel)
{
	// Realiza a erosão seguida da dilatação
	vc_binary_erode(src, aux, kernel);
	vc_binary_dilate(aux, dst, kernel);
	return 1;
}

//Fecho
int vc_binary_close(IVC* src, IVC* dst, IVC* aux, int kernel)
{
	// Realiza a dilatação seguida da erosão
	vc_binary_dilate(src, aux, kernel);
	vc_binary_erode(aux, dst, kernel);
	return 1;
}

//Binário -> Grayscale
int vc_binary_to_gray(IVC* src, IVC* dst)
{
	unsigned char* data_src = (unsigned char*)src->data;
	unsigned char* data_dst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline_src = src->width * src->channels;
	int bytesperline_dst = dst->width * dst->channels;
	int channels = src->channels;
	int pos_src, pos_dst, x, y;

	// Verificar erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((dst->width <= 0) || (dst->height <= 0) || (dst->data == NULL))
		return 0;
	if ((src->channels != 1) || (dst->channels != 1))
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels;
			pos_dst = y * bytesperline_dst + x;

			// Se o pixel for 0, atribui valor 0 na imagem de destino
			if (data_src[pos_src] == 0)
			{
				data_dst[pos_dst] = 0;
			}
			// Se o pixel for 1, atribui valor 255 na imagem de destino
			else if (data_src[pos_src] == 1)
			{
				data_dst[pos_dst] = 255;
			}
		}
	}

	return 1;
}

//Inverter binária
int vc_binary_invert(IVC* src, IVC* dst)
{
	unsigned char* data_src = (unsigned char*)src->data;
	unsigned char* data_dst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline_src = src->width * src->channels;
	int bytesperline_dst = dst->width * dst->channels;
	int channels = src->channels;
	int pos_src, pos_dst, x, y;

	// Verificar erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((dst->width <= 0) || (dst->height <= 0) || (dst->data == NULL))
		return 0;
	if ((src->channels != 1) || (dst->channels != 1))
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels;
			pos_dst = y * bytesperline_dst + x;

			// Se o pixel for 0, atribui valor 1 na imagem de destino
			if (data_src[pos_src] == 0)
			{
				data_dst[pos_dst] = 255;
			}
			// Se o pixel for 1, atribui valor 0 na imagem de destino
			else
			{
				data_dst[pos_dst] = 0;
			}
		}
	}

	return 1;
}

//HSV -> Grayscale
int vc_hsv_to_grayscale(IVC* src, IVC* dst)
{
	unsigned char* data_src = (unsigned char*)src->data;
	unsigned char* data_dst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline_src = src->width * src->channels;
	int bytesperline_dst = dst->width * dst->channels;
	int channels = src->channels;
	int pos_src, pos_dst, x, y;

	// Verifica erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return;
	if ((dst->width <= 0) || (dst->height <= 0) || (dst->data == NULL))
		return;
	if ((src->channels != 3) || (dst->channels != 1))
		return;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels;
			pos_dst = y * bytesperline_dst + x;

			// Componente de valor (Value)
			unsigned char value = data_src[pos_src + 2];

			// Atribui o valor do componente de valor ao pixel na imagem de destino
			data_dst[pos_dst] = value;
		}
	}
	return 1;
}

//Grayscale -> HSV
int vc_grayscale_to_hsv(IVC* src, IVC* dst)
{
	unsigned char* data_src = (unsigned char*)src->data;
	unsigned char* data_dst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline_src = src->width * src->channels;
	int bytesperline_dst = dst->width * dst->channels;
	int channels = dst->channels;
	int pos_src, pos_dst, x, y;

	// Verifica erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((dst->width <= 0) || (dst->height <= 0) || (dst->data == NULL))
		return 0;
	if ((src->channels != 1) || (dst->channels != 3))
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x;
			pos_dst = y * bytesperline_dst + x * channels;

			// Componente de valor (Value)
			unsigned char value = data_src[pos_src];

			//
			if (value == 255) {
				data_dst[pos_dst + 2] = 255;
			}
			else {
				data_dst[pos_dst + 2] = 0;
			}
		}
	}
	return 1;
}

// Transforma uma imagem gray em red
int vc_scale_gray_to_red(IVC* src, IVC* dst)
{
	unsigned char* data = (unsigned char*)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;

	unsigned char* data_dst = (unsigned char*)dst->data;
	int width_dst = dst->width;
	int height_dst = dst->height;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;

	int x, y, pos_src, pos_dst;
	float r, g, b;

	// Cofere erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height))
		return 0;
	if ((src->channels != 1) || (dst->channels != 3))
		return 0;

	// Faz a segmentaÃ§Ã£o
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline + x * channels;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			/*if (data[pos_src] >= 0 && data[pos_src] <= 64)
			{
				data_dst[pos_dst] = 0;
				data_dst[pos_dst + 1] = data[pos_src] * 4;
				data_dst[pos_dst + 2] = 255;
			}
			else if (data[pos_src] > 64 && data[pos_src] <= 128)
			{
				data_dst[pos_dst] = 0;
				data_dst[pos_dst + 1] = 255;
				data_dst[pos_dst + 2] = data[pos_src] / 4;
			}*/

			data_dst[pos_dst + 2] = data[pos_src];
		}
	}
	return 1;
}

int vc_gray_histogram_show(IVC* src, IVC* dst)
{
	int x, y, i;
	unsigned char* data_src = (unsigned char*)src->data;
	unsigned char* data_dst = (unsigned char*)dst->data;
	int height = src->height;
	int width = src->width;
	long int histogram[256] = { 0 }; // array que vai armazenar o histograma
	int maximo = 0;
	int max_height = dst->height;

	// Verificações básicas
	if (src == NULL || src->channels != 1 || dst == NULL || dst->channels != 1)
	{
		printf("Erro: imagens de entrada e saída devem ser em tons de cinza.\n");
		return 0;
	}

	// Cálculo do histograma
	for (y = 0; y < height * width; y++)
	{
		histogram[data_src[y]]++;
	}

	// Buscar Máximo do Histograma
	for (i = 0; i < 256; i++)
	{
		if (histogram[i] > maximo)
		{
			maximo = histogram[i];
		}
	}

	// Normalização do histograma para uma imagem binária
	for (x = 0; x < 256; x++)
	{
		histogram[x] = (int)((histogram[x] * max_height) / maximo);
	}

	// Preenchimento da imagem de destino com o histograma
	for (x = 0; x < 256; x++)
	{
		for (y = height - 1; y >= 0; y--)
		{
			if (height - y <= histogram[x])
			{
				data_dst[y * dst->width + x] = 255; // Branco
			}
			else
			{
				data_dst[y * dst->width + x] = 0; // Preto
			}
		}
	}

	return 1;
}

int vc_hsv_to_binary(IVC* src, IVC* dst)
{
	unsigned char* data_src = (unsigned char*)src->data;
	unsigned char* data_dst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;
	int x, y;
	long int pos;

	if ((src->width) <= 0 || (src->height <= 0) || (src->data == NULL))
	{
		printf("(vc_3chanels_to_1) Tamanhos inválidos\n");
		return 0;
	}
	if (src->channels != 3)
	{
		printf("(vc_3chanels_to_1) Imagem tem de ter 3 canais\n");
		return 0;
	}
	if ((dst->width) <= 0 || (dst->height <= 0) || (dst->data == NULL))
	{
		printf("(vc_3chanels_to_1) Tamanhos inválidos\n");
		return 0;
	}
	if (dst->channels != 1)
	{
		printf("(vc_3chanels_to_1) Imagem tem de ter 1 canal\n");
		return 0;
	}
	if (src->width != dst->width || src->height != dst->height)
	{
		printf("(vc_3chanels_to_1) Imagens têm tamanhos diferentes\n");
		return 0;
	}

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;
			data_dst[y * width + x] = data_src[pos];
		}
	}

	return 1;
}
#pragma endregion