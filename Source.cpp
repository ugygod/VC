#include <iostream>
#include <string>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>

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

        std::cout << "Elapsed time: " << nseconds << " seconds" << std::endl;
        std::cout << "Press any key to continue...\n";
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
    capture.open(videofile);

    /* Verifica se foi possível abrir o ficheiro de vídeo */
    if (!capture.isOpened()) {
        std::cerr << "Error opening video file!" << std::endl;
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
    cv::namedWindow("VC - HSV", cv::WINDOW_AUTOSIZE);
    cv::namedWindow("VC - SEGMENTED", cv::WINDOW_AUTOSIZE);
    cv::namedWindow("VC - BINARY", cv::WINDOW_AUTOSIZE);

    cv::Mat frame;
    IVC* image = vc_image_new(video.width, video.height, 3, 255);
    IVC* hsv_image = vc_image_new(video.width, video.height, 3, 255);
    IVC* segmented_image = vc_image_new(video.width, video.height, 1, 255);
    IVC* binary_image = vc_image_new(video.width, video.height, 1, 255);
    IVC* dilated_image = vc_image_new(video.width, video.height, 1, 255);
    IVC* final_image = vc_image_new(video.width, video.height, 3, 255);

    unsigned char green[3] = { 0, 255, 0 }; // Color for the bounding box

    while (key != 'q') {
        /* Leitura de uma frame do vídeo */
        capture.read(frame);

        /* Verifica se conseguiu ler a frame */
        if (frame.empty()) {
            std::cerr << "Error reading frame from video!" << std::endl;
            break;
        }

        // Copia dados de imagem da estrutura cv::Mat para uma estrutura IVC
        memcpy(image->data, frame.data, video.width * video.height * 3);
        memcpy(final_image->data, frame.data, video.width * video.height * 3);

        // Convert RGB to HSV
        vc_rgb_to_hsv(image);

        // Apply HSV segmentation
        vc_hsv_segmentation(image, 0, 50, 40, 80, 40, 75);

        // Exibe a imagem HSV
        cv::Mat hsv_frame(cv::Size(video.width, video.height), CV_8UC3, image->data);
        cv::imshow("VC - HSV", hsv_frame);

        // Convert 3-channel HSV image to binary
        vc_3_channels_to_binary(image, binary_image);
        cv::Mat binary_frame(cv::Size(video.width, video.height), CV_8UC1, binary_image->data);
        cv::imshow("VC - BINARY", binary_frame);

        // Dilate the binary image with a larger kernel size
        int kernel_size = 35;
        cv::Mat dilated_mat;
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(kernel_size, kernel_size));
        cv::dilate(binary_frame, dilated_mat, kernel);
        memcpy(dilated_image->data, dilated_mat.data, video.width * video.height);

        // Detect blobs in the dilated binary image
        int nlabels;
        OVC* blobs = vc_binary_blob_labelling(dilated_image, segmented_image, &nlabels);
        vc_binary_blob_info(segmented_image, blobs, nlabels);

        // Filter blobs by size
        for (int i = 0; i < nlabels; i++) {
            if (blobs[i].width < 70 || blobs[i].height < 70 || blobs[i].width > 190 || blobs[i].height > 190) {
                blobs[i].x = -1;
                blobs[i].y = -1;
                blobs[i].width = 0;
                blobs[i].height = 0;
            }
        }

        // Draw bounding boxes around the detected blobs on the original image
        vc_desenha_box(final_image, blobs, nlabels);

        // Copy image data from IVC structure to cv::Mat
        memcpy(frame.data, final_image->data, video.width * video.height * 3);

        // Display the final frame with bounding boxes
        cv::imshow("VC - VIDEO", frame);

        // Exit the application if the user presses the 'q' key
        if (cv::waitKey(1) == 'q') break;
    }

    // Release the video capture and destroy all windows
    capture.release();
    cv::destroyAllWindows();

    // Free the IVC images
    vc_image_free(image);
    vc_image_free(hsv_image);
    vc_image_free(segmented_image);
    vc_image_free(binary_image);
    vc_image_free(dilated_image);
    vc_image_free(final_image);

    return 0;
}






