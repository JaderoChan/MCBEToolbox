#pragma once

#include <opencv2/core/mat.hpp> // cv::Mat

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
    // 帧数/帧索引的无效值，用于标识实时流（总帧数未知）或尚未读取任何帧。
    static constexpr long long INVALID_INDEX = -1;

    /**
     * @param maxw 输出帧宽度上限，-1 表示不限制
     * @param maxh 输出帧高度上限，-1 表示不限制
     */
    explicit FramesOStream(int maxw = -1, int maxh = -1) noexcept : maxw_(maxw), maxh_(maxh) {}
    virtual  ~FramesOStream() = default;

    FramesOStream(const FramesOStream&)            = delete;
    FramesOStream& operator=(const FramesOStream&) = delete;

    /** 获取输出帧宽度上限。 */
    int  maxWidth()  const         { return maxw_; }
    /** 获取输出帧高度上限。 */
    int  maxHeight() const         { return maxh_; }
    /** 设置输出帧宽度上限。 */
    void setMaxWidth(int maxw)     { maxw_ = maxw; cachedSize_ = cv::Size(); }
    /** 设置输出帧高度上限。 */
    void setMaxHeight(int maxh)    { maxh_ = maxh; cachedSize_ = cv::Size(); }

    /** 判断流是否成功打开/处于可用状态。 */
    virtual bool      isOpened()   const = 0;
    /** 判断流是否已结束。 */
    virtual bool      isEnd()      const = 0;
    /** 获取总帧数。 */
    virtual long long frameCount() const = 0;
    /** 获取当前帧索引（从 0 开始计数）。 */
    virtual long long frameIndex() const = 0;

    /**
     * 读取并前进到下一帧图像。
     *
     * - 返回的图像已根据 maxWidth/maxHeight 等比例缩放。
     * - 返回的图像始终是 CV_8UC4 BGRA8888 格式。
     *
     * @return 如果流已结束或读取失败返回空 #cv::Mat。
     */
    cv::Mat  nextFrame();
    /**
     * 获取帧尺寸，返回的尺寸已根据 maxWidth/maxHeight 等比例缩放。
     *
     * @return 如果失败（如实时流还未读取首帧）返回空 #cv::Size。
     */
    cv::Size frameSize() const;

protected:
    virtual cv::Mat  readNextFrame()       = 0;
    virtual cv::Size readFrameSize() const = 0;

private:
    int maxw_;
    int maxh_;
    mutable cv::Size cachedSize_;
};
