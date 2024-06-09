#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <math.h>

#define PI 3.14285714286

extern "C" {
#include "vc.h"
}

void process_combined_mask(cv::Mat& combined_mask, cv::Scalar color, cv::Mat& frame, const std::string& label, bool combine_blobs = false) {
    IVC* src_image = vc_image_new(combined_mask.cols, combined_mask.rows, 1, 255);
    memcpy(src_image->data, combined_mask.data, combined_mask.total());
    IVC* dst_image = vc_image_new(combined_mask.cols, combined_mask.rows, 1, 255);
    memset(dst_image->data, 0, combined_mask.total());

    vc_binary_dilate(src_image, dst_image, 9);
    cv::Mat dilatedMask(combined_mask.rows, combined_mask.cols, CV_8UC1, dst_image->data);

    IVC* image_blob = vc_image_new(dilatedMask.cols, dilatedMask.rows, 1, 255);
    memcpy(image_blob->data, dilatedMask.data, dilatedMask.total());

    // Detectar blobs na imagem binária
    OVC* blobs;
    int nblobs;
    blobs = vc_binary_blob_labelling(image_blob, image_blob, &nblobs);
    vc_get_blob_properties(image_blob, blobs, nblobs);

    if (combine_blobs && nblobs > 0) {
        // Combinar blobs
        int xmin = blobs[0].x, ymin = blobs[0].y;
        int xmax = blobs[0].x + blobs[0].width - 1, ymax = blobs[0].y + blobs[0].height - 1;
        for (int i = 1; i < nblobs; i++) {
            xmin = std::min(xmin, blobs[i].x);
            ymin = std::min(ymin, blobs[i].y);
            xmax = std::max(xmax, blobs[i].x + blobs[i].width - 1);
            ymax = std::max(ymax, blobs[i].y + blobs[i].height - 1);
        }

        // Desenhar as bordas da caixa na imagem colorida
        for (int x = xmin; x <= xmax; x++) {
            for (int c = 0; c < frame.channels(); c++) {
                frame.at<cv::Vec3b>(ymin, x)[c] = color[c];
                frame.at<cv::Vec3b>(ymax, x)[c] = color[c];
            }
        }
        for (int y = ymin; y <= ymax; y++) {
            for (int c = 0; c < frame.channels(); c++) {
                frame.at<cv::Vec3b>(y, xmin)[c] = color[c];
                frame.at<cv::Vec3b>(y, xmax)[c] = color[c];
            }
        }

        // Adicionar o texto da label acima da bounding box
        int fontFace = cv::FONT_HERSHEY_SIMPLEX;
        double fontScale = 0.5;
        int thickness = 1;
        int baseline = 0;
        cv::Size textSize = cv::getTextSize(label, fontFace, fontScale, thickness, &baseline);
        int textX = xmin;
        int textY = ymin - 5;

        // Verificar se o texto vai sair fora da imagem e ajustar
        if (textY - textSize.height < 0) {
            textY = ymin + textSize.height + 5;
        }

        cv::putText(frame, label, cv::Point(textX, textY), fontFace, fontScale, color, thickness);

        // Desenhar o centro de massa como uma cruz na imagem colorida
        int xc = (xmin + xmax) / 2;
        int yc = (ymin + ymax) / 2;
        int cross_size = 5;
        for (int i = -cross_size; i <= cross_size; i++) {
            if (yc + i >= 0 && yc + i < frame.rows) {
                for (int c = 0; c < frame.channels(); c++) {
                    frame.at<cv::Vec3b>(yc + i, xc)[c] = 0; // Preto
                }
            }
            if (xc + i >= 0 && xc + i < frame.cols) {
                for (int c = 0; c < frame.channels(); c++) {
                    frame.at<cv::Vec3b>(yc, xc + i)[c] = 0; // Preto
                }
            }
        }
    }
    else {
        // Desenhar caixas ao redor dos blobs detectados na imagem binária e colorida
        for (int i = 0; i < nblobs; i++) {
            // Também desenhar na imagem colorida
            int x1 = blobs[i].x, y1 = blobs[i].y;
            int x2 = blobs[i].x + blobs[i].width - 1, y2 = blobs[i].y + blobs[i].height - 1;
            int xc = blobs[i].xc, yc = blobs[i].yc;
            int cross_size = 5;

            // Desenhar as bordas da caixa na imagem colorida
            for (int x = x1; x <= x2; x++) {
                for (int c = 0; c < frame.channels(); c++) {
                    frame.at<cv::Vec3b>(y1, x)[c] = color[c];
                    frame.at<cv::Vec3b>(y2, x)[c] = color[c];
                }
            }
            for (int y = y1; y <= y2; y++) {
                for (int c = 0; c < frame.channels(); c++) {
                    frame.at<cv::Vec3b>(y, x1)[c] = color[c];
                    frame.at<cv::Vec3b>(y, x2)[c] = color[c];
                }
            }

            // Adicionar o texto da label acima da bounding box
            int fontFace = cv::FONT_HERSHEY_SIMPLEX;
            double fontScale = 0.5;
            int thickness = 1;
            int baseline = 0;
            cv::Size textSize = cv::getTextSize(label, fontFace, fontScale, thickness, &baseline);
            int textX = x1;
            int textY = y1 - 5;

            // Verificar se o texto vai sair fora da imagem e ajustar
            if (textY - textSize.height < 0) {
                textY = y1 + textSize.height + 5;
            }

            cv::putText(frame, label, cv::Point(textX, textY), fontFace, fontScale, color, thickness);

            // Desenhar o centro de massa como uma cruz na imagem colorida
            for (int i = -cross_size; i <= cross_size; i++) {
                if (yc + i >= 0 && yc + i < frame.rows) {
                    for (int c = 0; c < frame.channels(); c++) {
                        if (xc >= 0 && xc < frame.cols) {
                            frame.at<cv::Vec3b>(yc + i, xc)[c] = 0; // Preto
                        }
                    }
                }
                if (xc + i >= 0 && xc + i < frame.cols) {
                    for (int c = 0; c < frame.channels(); c++) {
                        if (yc >= 0 && yc < frame.rows) {
                            frame.at<cv::Vec3b>(yc, xc + i)[c] = 0; // Preto
                        }
                    }
                }
            }
        }
    }

    // Limpar memória
    vc_image_free(src_image);
    vc_image_free(dst_image);
    vc_image_free(image_blob);
}

bool is_non_zero(cv::Mat& mask) {
    for (int y = 0; y < mask.rows; ++y) {
        for (int x = 0; x < mask.cols; ++x) {
            if (mask.at<uchar>(y, x) > 0) {
                return true;
            }
        }
    }
    return false;
}

int main(void) {
    char videofile[20] = "video.mp4";
    cv::VideoCapture capture;
    struct {
        int width, height;
        int ntotalframes;
        int fps;
        int nframe;
    } video;

    std::string str;
    int key = 0;
    int resistores = 0;

    capture.open(videofile);
        
    if (!capture.isOpened()) {
        std::cerr << "Erro ao abrir o ficheiro de vídeo!\n";
        return 1;
    }

    video.ntotalframes = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
    video.fps = (int)capture.get(cv::CAP_PROP_FPS);
    video.width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
    video.height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);

    cv::namedWindow("VC - VIDEO", cv::WINDOW_AUTOSIZE);

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
        str = std::string("N. DE RESISTORES: ").append(std::to_string(resistores));
        cv::putText(frame, str, cv::Point(20, 125), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
        cv::putText(frame, str, cv::Point(20, 125), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);

        IVC* image = vc_image_new(video.width, video.height, 3, 255);
        IVC* image_dst = vc_image_new(image->width, image->height, 3, 255);
        IVC* image_dst1 = vc_image_new(image->width, image->height, 1, 255);
        IVC* image_dst2 = vc_image_new(image->width, image->height, 1, 255);
        IVC* image_dst3 = vc_image_new(image->width, image->height, 1, 255);
        IVC* image_dst4 = vc_image_new(image->width, image->height, 1, 255);
        IVC* image_dst5 = vc_image_new(image->width, image->height, 1, 255);
        IVC* image_dst6 = vc_image_new(image->width, image->height, 1, 255);
        IVC* image_dst7 = vc_image_new(image->width, image->height, 1, 255);
        IVC* image_dst8 = vc_image_new(image->width, image->height, 1, 255);

        memcpy(image->data, frame.data, video.width * video.height * 3);
        vc_bgr_to_rgb(image);
        vc_rgb_to_hsv(image, image_dst);

        vc_hsv_segmentation(image_dst, image_dst1, 0, 10, 50, 255, 50, 255); //Red
        vc_hsv_segmentation(image_dst, image_dst2, 170, 180, 70, 255, 50, 255); //Red
        vc_hsv_segmentation(image_dst, image_dst3, 70, 120, 40, 255, 30, 255); //Green
        vc_hsv_segmentation(image_dst, image_dst4, 120, 150, 200, 255, 230, 255); //Green
        vc_hsv_segmentation(image_dst, image_dst5, 180, 210, 20, 255, 20, 255); //Blue
        vc_hsv_segmentation(image_dst, image_dst6, 210, 270, 20, 255, 20, 255); //Blue
        vc_hsv_segmentation(image_dst, image_dst7, 30, 50, 40, 255, 50, 255); //Yellow
        vc_hsv_segmentation(image_dst, image_dst8, 10, 30, 200, 255, 150, 255); //Brown NÃO FUNCIONAL

        cv::Mat maskRed1(image_dst1->height, image_dst1->width, CV_8UC1, image_dst1->data);
        cv::Mat maskRed2(image_dst2->height, image_dst2->width, CV_8UC1, image_dst2->data);
        cv::Mat maskRed = maskRed1 | maskRed2;

        cv::Mat maskGreen1(image_dst3->height, image_dst3->width, CV_8UC1, image_dst3->data);
        cv::Mat maskGreen2(image_dst4->height, image_dst4->width, CV_8UC1, image_dst4->data);
        cv::Mat maskGreenTotal = maskGreen1 | maskGreen2;

        cv::Mat maskBlue1(image_dst5->height, image_dst5->width, CV_8UC1, image_dst5->data);
        cv::Mat maskBlue2(image_dst6->height, image_dst6->width, CV_8UC1, image_dst6->data);
        cv::Mat maskBlue = maskBlue1 | maskBlue2;

        cv::Mat maskYellow(image_dst7->height, image_dst7->width, CV_8UC1, image_dst7->data);

        cv::Mat maskBrown(image_dst8->height, image_dst8->width, CV_8UC1, image_dst8->data);

        // Combine all masks into a single mask
        cv::Mat combinedMask = maskRed | maskGreenTotal | maskBlue | maskYellow | maskBrown;

        // Process the combined mask to detect and draw boxes around resistors with labels
        if (is_non_zero(maskRed)) {
            process_combined_mask(maskRed, cv::Scalar(0, 0, 255), frame, "Red");
        }
        if (is_non_zero(maskGreenTotal)) {
            process_combined_mask(maskGreenTotal, cv::Scalar(0, 255, 0), frame, "Green");
        }
        if (is_non_zero(maskBlue)) {
            process_combined_mask(maskBlue, cv::Scalar(255, 0, 0), frame, "Blue");
        }
        if (is_non_zero(maskYellow)) {
            process_combined_mask(maskYellow, cv::Scalar(0, 0, 0), frame, "Resistor", true); // Combinar blobs amarelos
        }
        if (is_non_zero(maskBrown)) {
            process_combined_mask(maskBrown, cv::Scalar(42, 42, 165), frame, "Brown");
        }

        cv::imshow("VC - VIDEO", frame);
        cv::imshow("VC - VIDEO - MASK", combinedMask);

        key = cv::waitKey(10);

        vc_image_free(image);
        vc_image_free(image_dst);
        vc_image_free(image_dst1);
        vc_image_free(image_dst2);
        vc_image_free(image_dst3);
        vc_image_free(image_dst4);
        vc_image_free(image_dst5);
        vc_image_free(image_dst6);
        vc_image_free(image_dst7);
        vc_image_free(image_dst8);
    }

    cv::destroyWindow("VC - VIDEO");
    cv::destroyWindow("VC - VIDEO - MASK");
    capture.release();

    return 0;
}
