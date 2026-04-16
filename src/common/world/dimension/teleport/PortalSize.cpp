#include "PortalSize.hpp"
#include "../../block/Block.hpp"
#include "../../block/VanillaBlocks.hpp"
#include "../../IWorld.hpp"
#include "../../../util/assert/AssertAll.hpp"
#include "../../../util/Direction.hpp"
#include "../../../util/property/Properties.hpp"

namespace mc {

// ============================================================================
// PortalSizeResult 方法
// ============================================================================

std::vector<BlockPos> PortalSizeResult::getPortalBlocks() const {
    std::vector<BlockPos> blocks;

    if (!valid) {
        return blocks;
    }

    blocks.reserve(static_cast<size_t>(width) * static_cast<size_t>(height));

    // 根据轴向确定方向
    // axis == North 表示 Z 轴传送门（框架沿 Z 方向延伸）
    // axis == East 表示 X 轴传送门（框架沿 X 方向延伸）

    for (i32 h = 0; h < height; ++h) {
        for (i32 w = 0; w < width; ++w) {
            BlockPos pos = corner;

            // 根据 w 和 h 偏移
            if (axis == Direction::East) {
                // X 轴传送门：宽度沿 X 方向
                pos.x += w;
                pos.y += h;
            } else {
                // Z 轴传送门：宽度沿 Z 方向
                pos.z += w;
                pos.y += h;
            }

            blocks.push_back(pos);
        }
    }

    return blocks;
}

// ============================================================================
// 下界传送门
// ============================================================================

std::optional<PortalSizeResult> PortalSize::findNetherPortal(IWorld& world, const BlockPos& pos) {
    // 获取黑曜石方块
    if (VanillaBlocks::OBSIDIAN == nullptr) {
        return std::nullopt;
    }

    const BlockState* obsidian = &VanillaBlocks::OBSIDIAN->defaultState();

    // 尝试在两个轴向上检测
    // Direction::East = X 轴传送门（框架沿 X 方向）
    // Direction::North = Z 轴传送门（框架沿 Z 方向）
    for (Direction axis : {Direction::East, Direction::North}) {
        auto result = tryFindPortalOnAxis(world, pos, axis, obsidian);
        if (result.has_value() && result->valid) {
            return result;
        }
    }

    return std::nullopt;
}

bool PortalSize::isNetherPortalAt(IWorld& world, const BlockPos& pos) {
    auto result = findNetherPortal(world, pos);
    return result.has_value() && result->valid;
}

bool PortalSize::lightNetherPortal(IWorld& world, const PortalSizeResult& portal) {
    if (!portal.valid) {
        return false;
    }

    // 获取传送门方块位置
    auto blocks = portal.getPortalBlocks();
    if (blocks.empty()) {
        return false;
    }

    // 检查 NETHER_PORTAL 方块是否已注册
    if (VanillaBlocks::NETHER_PORTAL == nullptr) {
        return false;
    }

    // 根据 MC 1.16.5 NetherPortalBlock，设置轴向
    // Direction::East 表示 X 轴传送门，Direction::North 表示 Z 轴传送门
    Axis axis = (portal.axis == Direction::East) ? Axis::X : Axis::Z;
    const BlockState* netherPortalState = &VanillaBlocks::NETHER_PORTAL->defaultState().with(
        BlockStateProperties::HORIZONTAL_AXIS(), axis);

    // 在传送门内部放置传送门方块
    for (const BlockPos& pos : blocks) {
        world.setBlockState(pos, netherPortalState, 2);
    }

    return true;
}

// ============================================================================
// 末地传送门
// ============================================================================

std::optional<PortalSizeResult> PortalSize::findEndPortal(IWorld& world, const BlockPos& pos) {
    // 末地传送门是固定的 3x3 框架，检测逻辑不同
    // 需要检查末地传送门框架方块（EndPortalFrameBlock）
    // TODO: 实现末地传送门检测
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return std::nullopt;
}

bool PortalSize::isEndPortalAt(IWorld& world, const BlockPos& pos) {
    auto result = findEndPortal(world, pos);
    return result.has_value() && result->valid;
}

bool PortalSize::activateEndPortal(IWorld& world, const PortalSizeResult& portal) {
    if (!portal.valid) {
        return false;
    }

    // 获取传送门方块位置
    auto blocks = portal.getPortalBlocks();
    if (blocks.empty()) {
        return false;
    }

    // TODO: 在内部放置末地传送门方块
    // const BlockState* endPortal = VanillaBlocks::END_PORTAL->defaultState();
    // for (const BlockPos& pos : blocks) {
    //     world.setBlockState(pos, endPortal, 2);
    // }

    MC_UNUSED(world);
    return true;
}

// ============================================================================
// 工具方法
// ============================================================================

std::vector<BlockPos> PortalSize::getPortalBlocks(const PortalSizeResult& portal) {
    return portal.getPortalBlocks();
}

// ============================================================================
// 内部检测方法
// ============================================================================

i32 PortalSize::checkHorizontalFrame(
    IWorld& world,
    const BlockPos& start,
    Direction direction,
    i32 maxLen,
    const BlockState* frameBlock)
{
    // 检测水平方向的黑曜石框架
    // 返回连续黑曜石的长度，如果遇到非黑曜石或达到最大长度则停止
    i32 length = 0;
    BlockPos pos = start;

    for (i32 i = 0; i < maxLen; ++i) {
        const BlockState* state = world.getBlockState(pos);
        if (state == nullptr || state != frameBlock) {
            break;
        }
        ++length;
        pos = pos.offset(direction);
    }

    // 需要至少找到最小宽度的框架
    if (length < MIN_WIDTH) {
        return -1;
    }

    return length;
}

i32 PortalSize::checkVerticalFrame(
    IWorld& world,
    const BlockPos& start,
    i32 maxLen,
    const BlockState* frameBlock)
{
    // 检测垂直方向的黑曜石框架
    // 从底部向上检测
    i32 length = 0;
    BlockPos pos = start;

    for (i32 i = 0; i < maxLen; ++i) {
        const BlockState* state = world.getBlockState(pos);
        if (state == nullptr || state != frameBlock) {
            break;
        }
        ++length;
        pos = pos.offset(Direction::Up);
    }

    // 需要至少找到最小高度的框架
    if (length < MIN_HEIGHT) {
        return -1;
    }

    return length;
}

bool PortalSize::checkInteriorEmpty(
    IWorld& world,
    const BlockPos& corner,
    i32 width,
    i32 height,
    Direction axis)
{
    // 检查传送门内部是否为空气
    for (i32 h = 0; h < height; ++h) {
        for (i32 w = 0; w < width; ++w) {
            BlockPos pos = corner;

            if (axis == Direction::East) {
                // X 轴传送门
                pos.x += w;
                pos.y += h;
            } else {
                // Z 轴传送门
                pos.z += w;
                pos.y += h;
            }

            const BlockState* state = world.getBlockState(pos);
            if (state != nullptr && !state->isAir()) {
                return false;
            }
        }
    }

    return true;
}

std::optional<PortalSizeResult> PortalSize::tryFindPortalOnAxis(
    IWorld& world,
    const BlockPos& pos,
    Direction axis,
    const BlockState* frameBlock)
{
    // 参考 MC 1.16.5 PortalSize
    // 检测算法：从给定位置向四个方向搜索可能的传送门框架

    // 确定水平方向（垂直于传送门朝向）
    Direction widthDir;
    if (axis == Direction::East) {
        // X 轴传送门，宽度方向为 X+
        widthDir = Direction::East;
    } else {
        // Z 轴传送门，宽度方向为 Z+
        widthDir = Direction::South;
    }

    // 遍历可能的框架底部位置
    // 从 pos 向下和左右搜索可能的底部框架

    for (i32 yOffset = 0; yOffset <= 1; ++yOffset) {
        BlockPos basePos(pos.x, pos.y - yOffset, pos.z);

        // 向两个水平方向搜索可能的角落
        for (i32 offset1 = -MAX_WIDTH; offset1 <= MAX_WIDTH; ++offset1) {
            BlockPos corner1 = basePos.offset(widthDir, offset1);

            // 检查是否为黑曜石
            const BlockState* state = world.getBlockState(corner1);
            if (state != frameBlock) {
                continue;
            }

            // 从这个位置开始检测框架
            auto result = detectFrameFromCorner(world, corner1, widthDir, frameBlock);
            if (result.has_value()) {
                return result;
            }
        }
    }

    return std::nullopt;
}

std::optional<PortalSizeResult> PortalSize::detectFrameFromCorner(
    IWorld& world,
    const BlockPos& bottomLeft,
    Direction widthDir,
    const BlockState* frameBlock)
{
    // 检测底部框架（黑曜石底边）
    i32 width = 0;
    BlockPos pos = bottomLeft;

    // 向宽度方向检测底部框架
    for (i32 w = 0; w <= MAX_WIDTH; ++w) {
        const BlockState* state = world.getBlockState(pos);
        if (state != frameBlock) {
            break;
        }
        ++width;
        pos = pos.offset(widthDir);
    }

    // 验证宽度（底部框架包括两个角落，所以宽度至少要 MIN_WIDTH + 2）
    // 例如：最小传送门宽度为 2，框架宽度为 2 + 2 = 4
    if (width < MIN_WIDTH + 2 || width > MAX_WIDTH + 2) {
        return std::nullopt;
    }

    i32 frameWidth = width;

    // 检测左边框架（左下角向上）
    i32 leftHeight = 0;
    BlockPos leftPos = bottomLeft.offset(Direction::Up);
    for (i32 h = 0; h <= MAX_HEIGHT; ++h) {
        const BlockState* state = world.getBlockState(leftPos);
        if (state != frameBlock) {
            break;
        }
        ++leftHeight;
        leftPos = leftPos.offset(Direction::Up);
    }

    // 验证高度（左侧框架包括两个角落，所以高度至少要 MIN_HEIGHT + 2）
    if (leftHeight < MIN_HEIGHT + 2 || leftHeight > MAX_HEIGHT + 2) {
        return std::nullopt;
    }

    i32 frameHeight = leftHeight;

    // 检测右边框架（右下角向上）
    BlockPos rightBottom = bottomLeft.offset(widthDir, frameWidth - 1);
    i32 rightHeight = 0;
    BlockPos rightPos = rightBottom.offset(Direction::Up);
    for (i32 h = 0; h <= MAX_HEIGHT; ++h) {
        const BlockState* state = world.getBlockState(rightPos);
        if (state != frameBlock) {
            break;
        }
        ++rightHeight;
        rightPos = rightPos.offset(Direction::Up);
    }

    // 左右高度必须相等
    if (rightHeight != frameHeight) {
        return std::nullopt;
    }

    // 检测顶部框架
    BlockPos topLeft = bottomLeft.offset(Direction::Up, frameHeight - 1);

    // 验证顶部框架（从左上角向右检测）
    BlockPos topPos = topLeft.offset(widthDir);
    for (i32 w = 1; w < frameWidth - 1; ++w) {
        const BlockState* state = world.getBlockState(topPos);
        if (state != frameBlock) {
            return std::nullopt;
        }
        topPos = topPos.offset(widthDir);
    }

    // 验证顶部右角
    BlockPos topRight = bottomLeft.offset(widthDir, frameWidth - 1).offset(Direction::Up, frameHeight - 1);
    const BlockState* topRightState = world.getBlockState(topRight);
    if (topRightState != frameBlock) {
        return std::nullopt;
    }

    // 计算内部尺寸（框架内部的空间）
    i32 interiorWidth = frameWidth - 2;
    i32 interiorHeight = frameHeight - 2;

    // 内部尺寸验证
    if (interiorWidth < MIN_WIDTH || interiorHeight < MIN_HEIGHT) {
        return std::nullopt;
    }

    // 检查内部是否为空气
    BlockPos interiorCorner = bottomLeft.offset(widthDir, 1).offset(Direction::Up, 1);

    Direction axis = (widthDir == Direction::East) ? Direction::East : Direction::North;

    if (!checkInteriorEmpty(world, interiorCorner, interiorWidth, interiorHeight, axis)) {
        return std::nullopt;
    }

    // 找到有效的传送门框架
    PortalSizeResult result;
    result.corner = interiorCorner;
    result.width = interiorWidth;
    result.height = interiorHeight;
    result.axis = axis;
    result.valid = true;

    return result;
}

} // namespace mc
