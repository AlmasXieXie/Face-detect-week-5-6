#include "FrameProcessor.h"
#include "FaceDetector.h"
#include <algorithm>

struct FrameProcessorImpl {
    FaceDetector det;
    cv::VideoCapture cap;
    FrameProcessorImpl(const std::string& a, const std::string& b, const std::string& c) : det(a, b, c) {}
};

// скрывает область лица пикселизацией
static void pixelate(cv::Mat& img, const cv::Rect& box, int blocks = 12) {
    cv::Mat roi = img(box);
    cv::Mat small;
    cv::resize(roi, small, cv::Size(blocks, blocks), 0, 0, cv::INTER_LINEAR);
    cv::resize(small, roi, roi.size(), 0, 0, cv::INTER_NEAREST);
}

FrameProcessor::FrameProcessor(const std::string& proto, const std::string& caffe, const std::string& onnx)
    : p(new FrameProcessorImpl(proto, caffe, onnx)) {
}
FrameProcessor::~FrameProcessor() {}

bool FrameProcessor::openCamera(int index) {
    p->cap.open(index, cv::CAP_DSHOW);
    if (p->cap.isOpened()) {
        p->cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
        p->cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    }
    return p->cap.isOpened();
}
bool FrameProcessor::isOpen() const { return p->cap.isOpened(); }
void FrameProcessor::close() { p->cap.release(); }

bool FrameProcessor::next(int& w, int& h, std::vector<unsigned char>& bgr, int& withMask, int& noMask) {
    cv::Mat frame;
    if (!p->cap.isOpened() || !p->cap.read(frame) || frame.empty()) return false;
    if (!frame.isContinuous()) frame = frame.clone();

    withMask = noMask = 0;
    for (const auto& r : p->det.detect(frame)) {
        (r.mask ? withMask : noMask)++;
        pixelate(frame, r.box);   // лицо скрыто и с маской, и без маски
        cv::Scalar c = r.mask ? cv::Scalar(0, 200, 0) : cv::Scalar(0, 0, 255);
        cv::rectangle(frame, r.box, c, 2);
        cv::putText(frame, r.mask ? "Mask" : "No mask", cv::Point(r.box.x, std::max(0, r.box.y - 6)),
            cv::FONT_HERSHEY_SIMPLEX, 0.6, c, 2);
    }
    w = frame.cols; h = frame.rows;
    bgr.assign(frame.data, frame.data + frame.total() * 3);
    return true;
}