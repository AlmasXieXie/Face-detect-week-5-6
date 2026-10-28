// FaceDetector.h
#pragma once
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <string>
#include <vector>

struct FaceResult { cv::Rect box; bool mask; float conf; };

class FaceDetector {
public:
    FaceDetector(const std::string& proto, const std::string& caffe, const std::string& onnx);
    std::vector<FaceResult> detect(const cv::Mat& frameBgr);
private:
    cv::dnn::Net faceNet, maskNet;
};