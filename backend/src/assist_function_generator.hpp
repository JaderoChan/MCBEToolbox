#pragma once

#include <string>
#include <vector>

#include <block.hpp>

struct Position
{
    enum CoordType
    {
        ABSOLUTE,
        RELATIVE,
        LOCAL
    };

    Position() = default;
    Position(int x, int y, int z) : x(x), y(y), z(z) {}

    std::string toString(CoordType type = RELATIVE) const;

    int x = 0;
    int y = 0;
    int z = 0;
};

/** 仅支持图像结构和分文件的视频/GIF结构。 */
class AssistFunctionGenerator
{
public:
    using MCFunction = std::vector<std::string>;

    AssistFunctionGenerator(
        int frameWidth, int frameHeight, SurfaceDirection surfaceDirection, std::string structureName);
    ~AssistFunctionGenerator() = default;

    virtual MCFunction generateSetOriginFunction()        const;
    virtual MCFunction generateClearOriginFunction()      const;
    virtual MCFunction generateMoveToWatchPointFunction() const;
    virtual MCFunction generateDestroyFunction()          const;
    virtual MCFunction generateBuildFunction()            const = 0;

protected:
    static void subjoin(MCFunction& out, const MCFunction& supplement);

    const int              frameWidth_;
    const int              frameHeight_;
    const SurfaceDirection surfaceDirection_;
    const std::string      structureName_;

    const std::string normalOriginName_;
    const std::string normalOriginNameId_;
    const std::string baseOriginName_;
    const std::string baseOriginNameId_;
    const std::string originTag_;
    const std::string originTagId_;

    struct TickArea
    {
        TickArea() = default;

        Position from = {0, 0, 0};
        Position to   = {0, 0, 0};
        std::string name;
    };
    std::vector<TickArea> tickingAreaList_; // 加载结构时必须的常加载区域信息，现假定不存在常加载区域数量限制（游戏内上限为 10）
};

class ImageAssistFunctionGenerator final : public AssistFunctionGenerator
{
public:
    using AssistFunctionGenerator::AssistFunctionGenerator;

    MCFunction generateBuildFunction() const override;
};

class VideoAssistFunctionGenerator final : public AssistFunctionGenerator
{
public:
    static constexpr std::size_t MAX_COMMAND_COUNT_PER_MCFUNCTION = 9000;

    VideoAssistFunctionGenerator(
        int frameWidth, int frameHeight, SurfaceDirection surfaceDirection,
        std::string structureName, int frameCount);

    MCFunction generateSetOriginFunction()    const override;
    MCFunction generateClearOriginFunction()  const override;
    MCFunction generateBuildFunction()        const override;
    MCFunction generatePlayFunction()         const;
    MCFunction generatePauseFunction()        const;
    MCFunction generateStopFunction()         const;
    MCFunction generateWorkPerTickFunction1(const std::string& function1ContentPrefix) const;
    MCFunction generateWorkPerTickFunction2() const;
    std::vector<MCFunction> generateWorkPerTickFunction1Content() const;

private:
    const int         frameCount_;
    const std::string scoreObjectName_;
    const std::string playingTag_;
    const std::string playingTagId_;
};
