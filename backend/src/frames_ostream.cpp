#include <frames_ostream.hpp>

#include <image_utilities.hpp>

cv::Mat FramesOStream::nextFrame()
{
    cv::Mat frame = readNextFrame();
    if (frame.empty())
        return frame;

    const cv::Size targetSize = frameSize();
    if (!targetSize.empty() && (frame.cols != targetSize.width || frame.rows != targetSize.height))
        frame = resizeImage(frame, targetSize);

    return convertColorToBgra(frame);
}

cv::Size FramesOStream::frameSize() const
{
    if (cachedSize_.empty())
    {
        const cv::Size rawSize = readFrameSize();
        if (!rawSize.empty())
            cachedSize_ = limitSize(rawSize, maxw_, maxh_);
    }
    return cachedSize_;
}
