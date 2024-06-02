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

// Função para segmentar resistências por cor
cv::Mat segment_resistors(cv::Mat frame) {
    cv::Mat hsvFrame, mask;
    cv::cvtColor(frame, hsvFrame, cv::COLOR_BGR2HSV);

    // Faixa de cor para segmentar resistências (ajustar conforme necessário)
    cv::inRange(hsvFrame, cv::Scalar(20, 100, 100), cv::Scalar(30, 255, 255), mask);

    return mask;
}

// Função para encontrar e desenhar contornos
void find_and_draw_contours(cv::Mat frame, cv::Mat mask) {
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    for (size_t i = 0; i < contours.size(); ++i) {
        cv::Rect boundingBox = cv::boundingRect(contours[i]);
        cv::Moments m = cv::moments(contours[i]);
        cv::Point centroid(m.m10 / m.m00, m.m01 / m.m00);

        // desenha o contorno e o centro de massa (centroide)
        cv::rectangle(frame, boundingBox, cv::Scalar(0, 255, 0), 2);
        cv::circle(frame, centroid, 5, cv::Scalar(0, 0, 255), -1);

        // Exemplo de texto na imagem
        std::string text = "Resistor: " + std::to_string(i + 1);
        cv::putText(frame, text, cv::Point(boundingBox.x, boundingBox.y - 10), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
    }
}

int main(void) {
    char videofile[20] = "video.mp4";  // nome do arquivo de vídeo atualizado
    cv::VideoCapture capture;
    struct
    {
        int width, height;
        int ntotalframes;
        int fps;
        int nframe;
    } video;

    std::string str;
    int key = 0;

    capture.open(videofile);

    if (!capture.isOpened())
    {
        std::cerr << "Erro ao abrir o ficheiro de vídeo!\n";
        return 1;
    }

    video.ntotalframes = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
    video.fps = (int)capture.get(cv::CAP_PROP_FPS);
    video.width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
    video.height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);

    cv::namedWindow("VC - VIDEO", cv::WINDOW_AUTOSIZE);

    vc_timer();

    cv::Mat frame;
    while (key != 'q') {
        capture.read(frame);

        if (frame.empty()) break;

        video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);

        str = std::string("RESOLUCAO: ").append(std::to_string(video.width)).append("x").append(std::to_string(video.height));
        cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
        cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
        str = std::string("TOTAL DE FRAMES: ").append(std::to_string(video.ntotalframes));
        cv::putText(frame, str, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
        cv::putText(frame, str, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
        str = std::string("FRAME RATE: ").append(std::to_string(video.fps));
        cv::putText(frame, str, cv::Point(20, 75), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
        cv::putText(frame, str, cv::Point(20, 75), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
        str = std::string("N. DA FRAME: ").append(std::to_string(video.nframe));
        cv::putText(frame, str, cv::Point(20, 100), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
        cv::putText(frame, str, cv::Point(20, 100), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);

        // Processamento de imagem para segmentação de resistências
        cv::Mat mask = segment_resistors(frame);
        find_and_draw_contours(frame, mask);

        cv::imshow("VC - VIDEO", frame);
        key = cv::waitKey(1);
    }

    vc_timer();
    cv::destroyWindow("VC - VIDEO");
    capture.release();

    return 0;
}

//
//int main(void) {
//	// Vídeo
//	char videofile[20] = "video_resistors.mp4";
//	cv::VideoCapture capture;
//	struct
//	{
//		int width, height;
//		int ntotalframes;
//		int fps;
//		int nframe;
//	} video;
//	// Outros
//	std::string str;
//	int key = 0;
//
//	/* Leitura de vídeo de um ficheiro */
//	/* NOTA IMPORTANTE:
//	O ficheiro video.avi deverá estar localizado no mesmo directório que o ficheiro de código fonte.
//	*/
//	capture.open(videofile);
//
//	/* Em alternativa, abrir captura de vídeo pela Webcam #0 */
//	//capture.open(0, cv::CAP_DSHOW); // Pode-se utilizar apenas capture.open(0);
//
//	/* Verifica se foi possível abrir o ficheiro de vídeo */
//	if (!capture.isOpened())
//	{
//		std::cerr << "Erro ao abrir o ficheiro de vídeo!\n";
//		return 1;
//	}
//
//	/* Número total de frames no vídeo */
//	video.ntotalframes = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
//	/* Frame rate do vídeo */
//	video.fps = (int)capture.get(cv::CAP_PROP_FPS);
//	/* Resolução do vídeo */
//	video.width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
//	video.height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);
//
//	/* Cria uma janela para exibir o vídeo */
//	cv::namedWindow("VC - VIDEO", cv::WINDOW_AUTOSIZE);
//
//	/* Inicia o timer */
//	vc_timer();
//
//	cv::Mat frame;
//	while (key != 'q') {
//		/* Leitura de uma frame do vídeo */
//		capture.read(frame);
//
//		/* Verifica se conseguiu ler a frame */
//		if (frame.empty()) break;
//
//		/* Número da frame a processar */
//		video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);
//
//		/* Exemplo de inserção texto na frame */
//		str = std::string("RESOLUCAO: ").append(std::to_string(video.width)).append("x").append(std::to_string(video.height));
//		cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
//		cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
//		str = std::string("TOTAL DE FRAMES: ").append(std::to_string(video.ntotalframes));
//		cv::putText(frame, str, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
//		cv::putText(frame, str, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
//		str = std::string("FRAME RATE: ").append(std::to_string(video.fps));
//		cv::putText(frame, str, cv::Point(20, 75), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
//		cv::putText(frame, str, cv::Point(20, 75), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
//		str = std::string("N. DA FRAME: ").append(std::to_string(video.nframe));
//		cv::putText(frame, str, cv::Point(20, 100), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
//		cv::putText(frame, str, cv::Point(20, 100), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
//
//
//		// Faça o seu código aqui...
//		/*
//		// Cria uma nova imagem IVC
//		IVC *image = vc_image_new(video.width, video.height, 3, 255);
//		// Copia dados de imagem da estrutura cv::Mat para uma estrutura IVC
//		memcpy(image->data, frame.data, video.width * video.height * 3);
//		// Executa uma função da nossa biblioteca vc
//		vc_rgb_get_green(image);
//		// Copia dados de imagem da estrutura IVC para uma estrutura cv::Mat
//		memcpy(frame.data, image->data, video.width * video.height * 3);
//		// Liberta a memória da imagem IVC que havia sido criada
//		vc_image_free(image);
//		*/
//		// +++++++++++++++++++++++++
//
//		/* Exibe a frame */
//		cv::imshow("VC - VIDEO", frame);
//
//		/* Sai da aplicação, se o utilizador premir a tecla 'q' */
//		key = cv::waitKey(1);
//	}
//
//	/* Para o timer e exibe o tempo decorrido */
//	vc_timer();
//
//	/* Fecha a janela */
//	cv::destroyWindow("VC - VIDEO");
//
//	/* Fecha o ficheiro de vídeo */
//	capture.release();
//
//	return 0;
//}
