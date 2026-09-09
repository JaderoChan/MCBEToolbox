#pragma once

#include <string>

#include <opencv2/core/mat.hpp>
#include <opencv2/videoio.hpp>  // cv::VideoCapture

/**
 * 图像帧数据流接口。
 *
 * 用于封装单张图像、视频、GIF 等不同来源的多帧图像数据，对外提供一致的按帧读取方式。
 *
 * 支持通过 maxWidth/maxHeight 限制输出帧的尺寸（等比例缩放），
 * 该限制对 #nextFrame() 与 #frameSize() 的返回值均生效，具体缩放规则参见 #limitSize()。
 */
class FramesOStream
{
public:
    // 帧数/帧索引的无效值，用于标识实时流（总帧数未知）或尚未读取任何帧
    static constexpr long long INVALID_INDEX = -1;

    /**
     * 带参构造函数。
     *
     * @param maxw 输出帧宽度上限，-1 表示不限制
     * @param maxh 输出帧高度上限，-1 表示不限制
     */
    explicit FramesOStream(int maxw = -1, int maxh = -1) noexcept : maxw_(maxw), maxh_(maxh) {}
    virtual ~FramesOStream() = default;

    FramesOStream(const FramesOStream&)            = delete;
    FramesOStream& operator=(const FramesOStream&) = delete;

    int  maxWidth() const               { return maxw_; }
    int  maxHeight() const              { return maxh_; }
    void setMaxWidth(int maxw)          { maxw_ = maxw; cachedSize_ = cv::Size(); }
    void setMaxHeight(int maxh)         { maxh_ = maxh; cachedSize_ = cv::Size(); }
    void setMaxSize(int maxw, int maxh) { maxw_ = maxw; maxh_ = maxh; cachedSize_ = cv::Size(); }

    /** 获取总帧数，如果是实时流返回 #INVALID_INDEX。 */
    virtual long long frameCount() const = 0;

    /** 获取当前帧索引（从 0 开始计数），如果尚未读取任何帧返回 #INVALID_INDEX。 */
    virtual long long currentFrameIndex() const = 0;

    /**
     * 读取并前进到下一帧图像，返回的图像已根据 maxWidth/maxHeight 等比例缩放。
     *
     * @return 如果流已结束或读取失败返回空 #cv::Mat。
     */
    cv::Mat nextFrame();

    /**
     * 获取帧尺寸（已根据 maxWidth/maxHeight 等比例缩放）。
     *
     * @return 如果尚未知晓（如实时流尚未读取首帧）返回空 #cv::Size。
     */
    cv::Size frameSize() const;

    /** 判断流是否已结束。 */
    virtual bool isEnd() const = 0;

    /** 判断流是否成功打开/处于可用状态。 */
    virtual bool isOpened() const = 0;

protected:
    /** 由派生类实现：读取原始的下一帧图像（未经尺寸限制）。 */
    virtual cv::Mat readNextFrame() = 0;

    /** 由派生类实现：获取原始帧尺寸（未经尺寸限制），尚未知晓时返回空。 */
    virtual cv::Size readFrameSize() const = 0;

private:
    int maxw_;
    int maxh_;
    // 缓存经尺寸限制后的帧尺寸，避免重复计算
    mutable cv::Size cachedSize_;
};

/** 单张图像的图像帧数据流（总帧数固定为 1）。 */
class SingleFramesOStream : public FramesOStream
{
public:
    /** 以图像进行构造。 */
    explicit SingleFramesOStream(cv::Mat image, int maxw = -1, int maxh = -1) noexcept
        : FramesOStream(maxw, maxh), image_(std::move(image)) {}
    ~SingleFramesOStream() override = default;

    long long frameCount() const override        { return isOpened() ? 1 : INVALID_INDEX; }
    long long currentFrameIndex() const override { return consumed_ ? 0 : INVALID_INDEX; }
    bool      isEnd() const override             { return consumed_ || image_.empty(); }
    bool      isOpened() const override          { return !image_.empty(); }

protected:
    cv::Mat  readNextFrame() override;
    cv::Size readFrameSize() const override;

private:
    cv::Mat image_;
    bool    consumed_ = false;
};

/** 视频文件的图像帧数据流。 */
class VideoFramesOStream : public FramesOStream
{
public:
    /**
     * 以视频文件路径构造。
     *
     * @param maxFrameCount 读取的最大帧数上限，-1 表示不限制
     */
    explicit VideoFramesOStream(
        const std::string& videoFilePath,
        long long maxFrameCount = -1, int maxw = -1, int maxh = -1) noexcept
        : FramesOStream(maxw, maxh), capture_(videoFilePath, cv::CAP_FFMPEG), maxFrameCount_(maxFrameCount)
    {}
    ~VideoFramesOStream() override = default;

    /** 获取总帧数（已受 maxFrameCount 限制），如果无法获知（如损坏的视频文件）返回 #INVALID_INDEX。 */
    long long frameCount() const override;
    long long currentFrameIndex() const override;
    bool      isEnd() const override    { return isEnd_ || !capture_.isOpened(); }
    bool      isOpened() const override { return capture_.isOpened(); }

protected:
    /** 供派生的实时流类使用，直接接管一个已打开（或待打开）的捕获对象。 */
    explicit VideoFramesOStream(cv::VideoCapture capture, int maxw, int maxh) noexcept
        : FramesOStream(maxw, maxh), capture_(std::move(capture)) {}

    cv::Mat  readNextFrame() override;
    cv::Size readFrameSize() const override;

    cv::VideoCapture capture_;

private:
    long long maxFrameCount_     = INVALID_INDEX;
    long long currentFrameIndex_ = INVALID_INDEX;
    bool      isEnd_             = false;
};

/** 实时视频流（如摄像头设备）的图像帧数据流。 */
class RealTimeFramesOStream : public VideoFramesOStream
{
public:
    /** 以摄像头设备索引构造。 */
    explicit RealTimeFramesOStream(int cameraIndex, int maxw = -1, int maxh = -1) noexcept
        : VideoFramesOStream(cv::VideoCapture(cameraIndex), maxw, maxh) {}
    ~RealTimeFramesOStream() override = default;

    /** 实时流总帧数未知，总是返回 #INVALID_INDEX。 */
    long long frameCount() const override { return INVALID_INDEX; }
};
