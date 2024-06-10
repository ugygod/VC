#include <iostream>
#include <string>
#include <chrono>
#include <opencv2\opencv.hpp>
#include <opencv2\core.hpp>
#include <opencv2\highgui.hpp>
#include <opencv2\videoio.hpp>

extern "C" {
#include "vc.h"
}


void vc_timer(void) {
	static bool running = false;
	static std::chrono::steady_clock::time_point previousTime = std::chrono::steady_clock::now();

	if (!running) {
		running = true;
	}
	else {
		std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
		std::chrono::steady_clock::duration elapsedTime = currentTime - previousTime;

		// Tempo em segundos.
		std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(elapsedTime);
		double nseconds = time_span.count();

		std::cout << "Tempo decorrido: " << nseconds << "segundos" << std::endl;
		std::cout << "Pressione qualquer tecla para continuar...\n";
		std::cin.get();
	}
}

int main(void) {

	int resistor_value = 0;
	int cont = 0;

	// Vídeo
	char videofile[20] = "video.mp4";
	cv::VideoCapture capture;
	struct
	{
		int width, height;
		int ntotalframes;
		int fps;
		int nframe;
	} video;
	// Outros
	std::string str;
	int key = 0;

	/* Leitura de vídeo de um ficheiro */
	/* NOTA IMPORTANTE:
	O ficheiro video.avi deverá estar localizado no mesmo directório que o ficheiro de código fonte.
	*/
	capture.open(videofile);

	/* Em alternativa, abrir captura de vídeo pela Webcam #0 */
	//capture.open(0, cv::CAP_DSHOW); // Pode-se utilizar apenas capture.open(0);

	/* Verifica se foi possível abrir o ficheiro de vídeo */
	if (!capture.isOpened())
	{
		std::cerr << "Erro ao abrir o ficheiro de vídeo!\n";
		return 1;
	}

	/* Número total de frames no vídeo */
	video.ntotalframes = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
	/* Frame rate do vídeo */
	video.fps = (int)capture.get(cv::CAP_PROP_FPS);
	/* Resolução do vídeo */
	video.width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
	video.height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);

	/* Cria uma janela para exibir o vídeo */
	cv::namedWindow("VC - VIDEO", cv::WINDOW_AUTOSIZE);

	// Cria uma nova imagem IVC
	IVC* imagem = vc_image_new(video.width, video.height, 3, 255);
	IVC* imagem2 = vc_image_new(video.width, video.height, 3, 255);
	IVC* imagem3 = vc_image_new(video.width, video.height, 1, 255);
	IVC* imagemSegmentada = vc_image_new(video.width, video.height, 1, 255);


	/* Inicia o timer */
	vc_timer();

	cv::Mat frame;

	while (key != 'q') {
		/* Leitura de uma frame do vídeo */
		capture.read(frame);

		/* Verifica se conseguiu ler a frame */
		if (frame.empty()) break;



		/* Número da frame a processar */
		video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);

		// Copia dados de imagem da estrutura cv::Mat para uma estrutura IVC
		memcpy(imagem->data, frame.data, video.width * video.height * 3);
		memcpy(imagem2->data, frame.data, video.width * video.height * 3);

		vc_rgb_to_hsv(imagem, imagem);

		vc_hsv_segmentation(imagem, imagemSegmentada, 0, 200, 40, 50, 32, 80);

		// Converte IVC para cv::Mat
		cv::Mat matImage(video.height, video.width, CV_8UC1, imagemSegmentada->data);
		cv::Mat matDilated;

		// Cria um elemento estruturante (kernel)
		int kernelSize = 50;
		cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(kernelSize, kernelSize));

		cv::dilate(matImage, matDilated, kernel);

		// Converte de volta para IVC
		memcpy(imagem3->data, matDilated.data, video.width * video.height);

		OVC* blobs;
		int nblobs = 0;
		int labels = 0;

		blobs = vc_binary_blob_labelling(imagem3, imagemSegmentada, &nblobs);
		vc_binary_blob_info(imagemSegmentada, blobs, nblobs);

		vc_desenha_box(imagem2, blobs, nblobs, &cont);

		vc_desenha_centroide(imagem2, blobs, nblobs);

		memcpy(frame.data, imagem2->data, video.width * video.height * 3);

		for (int i = 0; i < nblobs; i++)
		{
			if (blobs[i].width > 150 && blobs[i].area > 11000 && blobs[i].area < 21000 && blobs[i].height > 80 && blobs[i].height <= 115)
			{
				// Detectar as cores das faixas
				int digits[5] = { -1, -1, -1, -1, -1 };
				int digit_index = 0;

				for (int x = blobs[i].x; x < blobs[i].x + blobs[i].width && digit_index < 5; x++)
				{
					int pixel_index = (blobs[i].y + blobs[i].height / 2) * imagem2->bytesperline + x * imagem2->channels;
					Color pixel = { imagem2->data[pixel_index], imagem2->data[pixel_index + 1], imagem2->data[pixel_index + 2] };
					int digit = detectColor(pixel);

					if (digit != -1 && (digit_index == 0 || digits[digit_index - 1] != digit))
					{
						digits[digit_index] = digit;
						digit_index++;
					}
				}

				// Calcular o valor da resistência
				if (digit_index >= 4)
				{
					int resistanceValue = calculateResistance(digits, digit_index);
					std::string valor = "Valor: " + std::to_string(resistanceValue) + " ohms";

					// Define a posição base para o texto (acima da bounding box do blob)
					int baseY = blobs[i].y - 20;

					// Insere o texto na imagem
					cv::putText(frame, valor, cv::Point(blobs[i].x, baseY), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 1);
				}
			}
		}
		/*str = std::string("Resistencias: ").append(std::to_string(cont / 10));
		cv::putText(frame, str, cv::Point(15, 25), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);*/

		/* Exibe a frame */
		cv::imshow("VC - VIDEO", frame);

		/* Sai da aplica��o, se o utilizador premir a tecla 'q' */
		key = cv::waitKey(1);
	}

	/* Fecha a janela */
	cv::destroyWindow("VC - VIDEO");

	/* Fecha o ficheiro de v�deo */
	capture.release();

	return 0;
}