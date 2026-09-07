#pragma once

#include <string>

#include <opencv2/core/mat.hpp>
#include <opencv2/videoio.hpp>

/**
 * 图像帧数据流接口。
 *
 * 用于封装视频、单张图像、GIF 等不同来源的多帧图像数据，对外提供一致的按帧读取方式。
 */
class ImageFramesOStream
{
public:
    // 帧数/帧索引的无效值，用于标识实时流或尚未读取任何帧
    static constexpr long long INVALID_INDEX = -1;

    ImageFramesOStream()          = default;
    virtual ~ImageFramesOStream() = default;

    ImageFramesOStream(const ImageFramesOStream&)            = delete;
    ImageFramesOStream& operator=(const ImageFramesOStream&) = delete;

    /** 获取总帧数，如果是实时流返回 #INVALID_INDEX */
    virtual long long frameCount() const = 0;

    /** 获取当前帧索引（从 0 开始计数），如果尚未读取任何帧返回 #INVALID_INDEX */
    virtual long long currentFrameIndex() const = 0;

    /** 读取并前进到下一帧图像，如果流已结束或读取失败返回空 #cv::Mat */
    virtual cv::Mat nextFrame() = 0;

    /** 获取帧尺寸，如果尚未知晓（如实时流尚未读取首帧）返回空 #cv::Size */
    virtual cv::Size frameSize() const = 0;

    /** 判断流是否已结束。 */
    virtual bool isEnd() const = 0;

    /** 判断流是否成功打开/处于可用状态。 */
    virtual bool isOpened() const = 0;
};

/** 单张图像的图像帧数据流。 */
class SingleImageFramesOStream : public ImageFramesOStream
{
public:
    explicit SingleImageFramesOStream(cv::Mat image);
    ~SingleImageFramesOStream() override = default;

    long long frameCount() const override;
    long long currentFrameIndex() const override;
    cv::Mat   nextFrame() override;
    cv::Size  frameSize() const override;
    bool      isEnd() const override;
    bool      isOpened() const override;

private:
    cv::Mat image_;
    bool    consumed_ = false;
};

/** 封装 #cv::VideoCapture 的图像帧数据流。 */
class VideoImageFramesOStream : public ImageFramesOStream
{
public:
    /** 以视频文件路径构造，失败时 #isOpened() 返回 false。 */
    explicit VideoImageFramesOStream(const std::string& videoFilepath);
    /** 以摄像头设备索引构造，失败时 #isOpened() 返回 false。 */
    explicit VideoImageFramesOStream(int cameraIndex);
    ~VideoImageFramesOStream() override = default;

    long long frameCount() const override;
    long long currentFrameIndex() const override;
    cv::Mat   nextFrame() override;
    cv::Size  frameSize() const override;
    bool      isEnd() const override;
    bool      isOpened() const override;

private:
    cv::VideoCapture capture_;
    long long        currentFrameIndex_ = INVALID_INDEX;
    bool             isEnd_             = false;
};
