// FaceDetector.cpp
#pragma managed(push, off)
#include "FaceDetector.h"
#pragma managed(pop)
FaceDetector::FaceDetector(const std::string& proto, const std::string& caffe, const std::string& onnx) {
    faceNet = cv::dnn::readNetFromCaffe(proto, caffe);
    maskNet = cv::dnn::readNetFromONNX(onnx);
}

std::vector<FaceResult> FaceDetector::detect(const cv::Mat& frame) {
    std::vector<FaceResult> out;
    cv::Mat blob = cv::dnn::blobFromImage(frame, 1.0, cv::Size(300, 300), cv::Scalar(104, 177, 123));
    faceNet.setInput(blob);
    cv::Mat det = faceNet.forward();
    cv::Mat d(det.size[2], det.size[3], CV_32F, det.ptr<float>());

    for (int i = 0; i < d.rows; i++) {
        float conf = d.at<float>(i, 2);
        if (conf < 0.5f) continue;
        int x1 = (int)(d.at<float>(i, 3) * frame.cols), y1 = (int)(d.at<float>(i, 4) * frame.rows);
        int x2 = (int)(d.at<float>(i, 5) * frame.cols), y2 = (int)(d.at<float>(i, 6) * frame.rows);
        cv::Rect box = cv::Rect(x1, y1, x2 - x1, y2 - y1) & cv::Rect(0, 0, frame.cols, frame.rows);
        if (box.area() <= 0) continue;

        cv::Mat face;
        cv::cvtColor(frame(box), face, cv::COLOR_BGR2RGB);
        cv::resize(face, face, cv::Size(224, 224));
        face.convertTo(face, CV_32FC3, 1.0 / 127.5, -1.0);   // нормализация в диапазон [-1, 1]

        int sz[] = { 1, 224, 224, 3 };                         // NHWC
        cv::Mat fb(4, sz, CV_32F, face.data);
        maskNet.setInput(fb);
        cv::Mat pred = maskNet.forward();
        out.push_back({ box, pred.at<float>(0, 0) > pred.at<float>(0, 1), conf });
    }
    return out;
}