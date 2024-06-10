#include <iostream>
#include <string>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <set> // For counting

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
    // Inicia contador, a ser utilizado na contagem das resistências
    int contador = 0;

    // Vídeo
    char videofile[65] = "video_resistors.mp4";
    cv::VideoCapture capture;
    struct {
        int width, height;
        int ntotalframes;
        int fps;
        int nframe;
    } video;
    // Outros
    std::string str;
    int key = 0;

    // Leitura de vídeo de um ficheiro
    capture.open(videofile);

    // Verifica se foi possível abrir o ficheiro de vídeo
    if (!capture.isOpened()) {
        std::cerr << "Erro ao abrir o ficheiro de vídeo!\n";
        return 1;
    }

    // Número total de frames no vídeo
    video.ntotalframes = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
    // Frame rate do vídeo
    video.fps = (int)capture.get(cv::CAP_PROP_FPS);
    // Resolução do vídeo
    video.width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
    video.height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);

    // Cria uma janela para exibir o vídeo
    cv::namedWindow("VC - VIDEO", cv::WINDOW_AUTOSIZE);

    // Cria uma nova imagem IVC
    IVC* image = vc_image_new(video.width, video.height, 3, 255);
    IVC* image2 = vc_image_new(video.width, video.height, 3, 255);
    IVC* imageLab = vc_image_new(video.width, video.height, 1, 255);
    IVC* imageLab2 = vc_image_new(video.width, video.height, 1, 255);

    // Inicia o timer
    vc_timer();

    cv::Mat frame;
    while (key != 'q') {
        // Leitura de uma frame do vídeo
        capture.read(frame);

        // Verifica se conseguiu ler a frame
        if (frame.empty()) break;

        // Número da frame a processar
        video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);

        // Copia dados de imagem da estrutura cv::Mat para uma estrutura IVC
        memcpy(image->data, frame.data, video.width * video.height * 3);
        memcpy(image2->data, frame.data, video.width * video.height * 3);

        // Processamento das imagens
        vc_rgb_to_hsv(image);

        vc_hsv_segmentation(image, imageLab2, 0, 200, 40, 65, 32, 110);

        // Converte IVC para cv::Mat
        cv::Mat matImageLab2(video.height, video.width, CV_8UC1, imageLab2->data);
        cv::Mat matDilated;

        // Cria um elemento estruturante (kernel)
        int kernelSize = 55; // Ajuste conforme necessário
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(kernelSize, kernelSize));

        // Aplica a função cv::dilate
        cv::dilate(matImageLab2, matDilated, kernel);

        // Converte de volta para IVC
        memcpy(imageLab->data, matDilated.data, video.width * video.height);

        // Processo de detecção de blobs
        int nblobs;
        OVC* blobs = vc_binary_blob_labelling(imageLab, imageLab2, &nblobs);

        // Extração de informações dos blobs
        vc_binary_blob_info(imageLab2, blobs, nblobs);

        // Desenho das bounding boxes e contagem de resistências
        vc_desenha_box(image2, blobs, nblobs, &contador);

        // Copia dados de imagem da estrutura IVC para uma estrutura cv::Mat
        memcpy(frame.data, image2->data, video.width * video.height * 3);

        // Loop pelos blobs para escrever valores
        for (int i = 0; i < nblobs; i++) {
            if (blobs[i].width > 150 &&
                blobs[i].area > 11000 && blobs[i].area < 21000 &&
                blobs[i].height > 80 && blobs[i].height <= 115) {

                std::string terceiro(blobs[i].terceiro, '0'); // Converter para 'zeros' o mesmo número de vezes que o terceiro tem.

                std::string valor = "Valor: " + std::to_string(blobs[i].primeiro - 1) + std::to_string(blobs[i].segundo - 1) + terceiro + " +/-5% ohms";

                // Define a posição base para o texto (acima da bounding box do blob)
                int baseY = blobs[i].y - 20;

                // Insere o texto na imagem
                cv::putText(frame, valor, cv::Point(blobs[i].x, baseY), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 1);
            }
        }

        // Escrever no vídeo contador de blobs
        str = std::string("Contador resistências: ").append(std::to_string(contador));
        cv::putText(frame, str, cv::Point(15, 25), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);

        // Exibe a frame
        cv::imshow("VC - VIDEO", frame);

        // Sai da aplicação, se o utilizador premir a tecla 'q'
        key = cv::waitKey(1);
    }

    // Para o timer e exibe o tempo decorrido
    vc_timer();

    // Liberta a memória da imagem IVC
    vc_image_free(image);
    vc_image_free(image2);
    vc_image_free(imageLab);
    vc_image_free(imageLab2);

    // Fecha a janela
    cv::destroyWindow("VC - VIDEO");

    // Fecha o ficheiro de vídeo
    capture.release();
    return 0;
}
