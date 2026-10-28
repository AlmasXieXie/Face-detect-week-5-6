#pragma once
#include <memory>
#include <string>
#include <vector>

struct FrameProcessorImpl;

class FrameProcessor {
public:
    FrameProcessor(const std::string& proto, const std::string& caffe, const std::string& onnx);
    ~FrameProcessor();
    bool openCamera(int index = 0);
    bool isOpen() const;
    void close();
    // читает кадр, находит лица, рисует рамки; возвращает BGR-байты
    bool next(int& w, int& h, std::vector<unsigned char>& bgr, int& withMask, int& noMask);
private:
    std::unique_ptr<FrameProcessorImpl> p;
};