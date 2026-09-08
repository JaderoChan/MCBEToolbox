#pragma once

#include <string>

#include <opencv2/core/mat.hpp>
#include <opencv2/videoio.hpp>

/**
 * 图像帧数据流接口。
 *
 * 用于封装视频、单张图像、GIF 等不同来源的多帧图像数据，对外提供一致的按帧读取方式。
 *
 * 支持通过 maxFrameWidth/maxFrameHeight 限制输出帧的尺寸（等比例缩放），
 * 该限制对 #nextFrame() 与 #frameSize() 的返回值均生效，具体缩放规则参见 #limitsSize()。
 * 两参数默认值均为 -1，表示不对相应维度做任何限制。
 */
class ImageFramesOStream
{
public:
    // 帧数/帧索引的无效值，用于标识实时流（总帧数未知）或尚未读取任何帧
    static constexpr long long INVALID_INDEX = -1;

    /**
     * @param maxFrameWidth  输出帧宽度上限，-1 表示不限制
     * @param maxFrameHeight 输出帧高度上限，-1 表示不限制
     */
    explicit ImageFramesOStream(int maxFrameWidth = -1, int maxFrameHeight = -1);
    virtual ~ImageFramesOStream() = default;

    ImageFramesOStream(const ImageFramesOStream&)            = delete;
    ImageFramesOStream& operator=(const ImageFramesOStream&) = delete;

    int maxFrameWidth() const;
    int maxFrameHeight() const;
    void setMaxFrameWidth(int maxFrameWidth);
    void setMaxFrameHeight(int maxFrameHeight);
    void setMaxFrameSize(int maxFrameWidth, int maxFrameHeight);

    /** 获取总帧数，如果是实时流返回 #INVALID_INDEX */
    virtual long long frameCount() const = 0;

    /** 获取当前帧索引（从 0 开始计数），如果尚未读取任何帧返回 #INVALID_INDEX */
    virtual long long currentFrameIndex() const = 0;

    /**
     * 读取并前进到下一帧图像，返回的图像已根据 maxFrameWidth/maxFrameHeight 等比例缩放。
     *
     * @return 如果流已结束或读取失败返回空 #cv::Mat。
     */
    cv::Mat nextFrame();

    /**
     * 获取帧尺寸（已根据 maxFrameWidth/maxFrameHeight 等比例缩放）。
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
    int maxFrameWidth_;
    int maxFrameHeight_;
    // 缓存经尺寸限制后的帧尺寸，避免重复计算；在尺寸上限变更时失效
    mutable cv::Size cachedFrameSize_;
};

/** 单张图像的图像帧数据流（总帧数固定为 1）。 */
class SingleImageFramesOStream : public ImageFramesOStream
{
public:
    explicit SingleImageFramesOStream(cv::Mat image, int maxFrameWidth = -1, int maxFrameHeight = -1);
    ~SingleImageFramesOStream() override = default;

    long long frameCount() const override;
    long long currentFrameIndex() const override;
    bool      isEnd() const override;
    bool      isOpened() const override;

protected:
    cv::Mat  readNextFrame() override;
    cv::Size readFrameSize() const override;

private:
    cv::Mat image_;
    bool    consumed_ = false;
};

/** 基于本地视频文件的图像帧数据流。 */
class VideoImageFramesOStream : public ImageFramesOStream
{
public:
    /** 以视频文件路径构造，失败时 #isOpened() 返回 false。 */
    explicit VideoImageFramesOStream(
        const std::string& videoFilepath, int maxFrameWidth = -1, int maxFrameHeight = -1);
    ~VideoImageFramesOStream() override = default;

    /** 获取总帧数，如果无法获知（如损坏的视频文件）返回 #INVALID_INDEX */
    long long frameCount() const override;
    long long currentFrameIndex() const override;
    bool      isEnd() const override;
    bool      isOpened() const override;

protected:
    /** 供派生的实时流类使用，直接接管一个已打开（或待打开）的捕获对象。 */
    explicit VideoImageFramesOStream(cv::VideoCapture capture, int maxFrameWidth, int maxFrameHeight);

    cv::Mat  readNextFrame() override;
    cv::Size readFrameSize() const override;

    cv::VideoCapture capture_;

private:
    long long currentFrameIndex_ = INVALID_INDEX;
    bool      isEnd_             = false;
};

/**
 * 实时视频流（如摄像头设备）的图像帧数据流。
 *
 * 与 #VideoImageFramesOStream 共用基于 #cv::VideoCapture 的读帧逻辑，
 * 区别在于总帧数始终未知，因此 #frameCount() 恒返回 #INVALID_INDEX。
 */
class RealTimeVideoImageFramesOStream : public VideoImageFramesOStream
{
public:
    /** 以摄像头设备索引构造，失败时 #isOpened() 返回 false。 */
    explicit RealTimeVideoImageFramesOStream(
        int cameraIndex, int maxFrameWidth = -1, int maxFrameHeight = -1);
    ~RealTimeVideoImageFramesOStream() override = default;

    /** 实时流总帧数未知，总是返回 #INVALID_INDEX */
    long long frameCount() const override;
};
