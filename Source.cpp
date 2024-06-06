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

        std::cout << "Tempo decorrido: " << nseconds << " segundos" << std::endl;
        std::cout << "Pressione qualquer tecla para continuar...\n";
        std::cin.get();
    }
}

int main(void) {
    // Vídeo
    char videofile[20] = "video_resistors.mp4";
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
    cv::namedWindow("VC - BINARY", cv::WINDOW_AUTOSIZE);

    // +++++++++++++++++++++++++
    // +++++++++++++++++++++++++

    // Cria uma nova imagem IVC
    IVC* imagem = vc_image_new(video.width, video.height, 3, 255);
    IVC* gray_image = vc_image_new(video.width, video.height, 1, 255);
    IVC* binary_image = vc_image_new(video.width, video.height, 1, 255);
    IVC* dilated_image = vc_image_new(video.width, video.height, 1, 255);
    IVC* labelled_image = vc_image_new(video.width, video.height, 1, 255);

    // Verifica se todas as imagens foram alocadas corretamente
    if (!imagem || !gray_image || !binary_image || !dilated_image || !labelled_image) {
        std::cerr << "Erro ao alocar memória para as imagens!\n";
        return 1;
    }

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

        // Converte a imagem de RGB para escala de cinza
        vc_rgb_to_gray(imagem, gray_image);

        // Aplica thresholding para binarizar a imagem
        int threshold = 105;  // Ajuste conforme necessário
        vc_gray_to_binary(gray_image, binary_image, threshold);

        // Aplica dilatação à imagem binária
        int kernel_size = 35;  // Ajuste conforme necessário
        vc_binary_dilate(binary_image, dilated_image, kernel_size);

        // Cria uma cv::Mat para exibir a imagem binarizada
        cv::Mat binary_frame(cv::Size(video.width, video.height), CV_8UC1, dilated_image->data);

        // Exibe a imagem binarizada
        cv::imshow("VC - BINARY", binary_frame);

        // Detecta blobs na imagem binarizada usando as funções de vc.c
        int nblobs;
        OVC* blobs = vc_binary_blob_labelling(dilated_image, labelled_image, &nblobs);

        if (blobs != NULL) {
            vc_binary_blob_info(labelled_image, blobs, nblobs);

            // Desenha bounding boxes ao redor das resistências detectadas na imagem original
            vc_desenha_box(imagem, blobs, nblobs);

            // Copia os dados da estrutura IVC de volta para cv::Mat
            memcpy(frame.data, imagem->data, video.width * video.height * 3);

            // Exibe a frame com bounding boxes
            cv::imshow("VC - VIDEO", frame);
        }

        // Liberta a memória dos blobs
        if (blobs != NULL) {
            free(blobs);
        }

        // +++++++++++++++++++++++++
        // +++++++++++++++++++++++++

        /* Sai da aplicação, se o utilizador premir a tecla 'q' */
        key = cv::waitKey(1);
    }

    /* Fecha as janelas */
    cv::destroyWindow("VC - VIDEO");
    cv::destroyWindow("VC - BINARY");

    /* Fecha o ficheiro de vídeo */
    capture.release();

    // Liberta a memória das imagens IVC que haviam sido criadas
    vc_image_free(imagem);
    vc_image_free(gray_image);
    vc_image_free(binary_image);
    vc_image_free(dilated_image);
    vc_image_free(labelled_image);

    return 0;
}
