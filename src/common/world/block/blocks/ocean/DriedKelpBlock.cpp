#include "DriedKelpBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../VanillaBlocks.hpp"

namespace mc {
namespace blocks {

// ============================================================================
// DriedKelpBlock 实现
// ============================================================================

DriedKelpBlock::DriedKelpBlock(BlockProperties properties)
    : Block(std::move(properties))
{
    // 干海带块不需要特殊逻辑
}

// ============================================================================
// ConduitBlock 实现
// ============================================================================

ConduitBlock::ConduitBlock(BlockProperties properties)
    : Block(std::move(properties))
{
    // 潮涌核心需要潮涌框架激活
}

void ConduitBlock::onBlockAdded(
    IWorld& world,
    const BlockPos& pos,
    const BlockState& state)
{
    MC_UNUSED(state);

    // 检测潮涌框架
    i32 frameCount = detectFrame(world, pos);
    updateActivation(world, pos, frameCount);
}

void ConduitBlock::onBlockRemoved(
    IWorld& world,
    const BlockPos& pos,
    const BlockState& state)
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);

    // TODO: 清除潮涌效果
    // 需要方块实体系统来管理效果范围
}

i32 ConduitBlock::detectFrame(IWorld& world, const BlockPos& pos) const
{
    i32 frameCount = 0;

    // 检测5x5x5范围内的海晶石框架
    // 潮涌框架需要16个海晶石方块组成十字形结构
    for (i32 dx = -2; dx <= 2; ++dx) {
        for (i32 dy = -2; dy <= 2; ++dy) {
            for (i32 dz = -2; dz <= 2; ++dz) {
                // 跳过中心（潮涌核心位置）
                if (dx == 0 && dy == 0 && dz == 0) {
                    continue;
                }

                // 只检查十字结构的位置
                bool isCrossPosition = (dx == 0 && dy == 0) ||
                                       (dx == 0 && dz == 0) ||
                                       (dy == 0 && dz == 0);

                if (!isCrossPosition) {
                    continue;
                }

                BlockPos checkPos(pos.x + dx, pos.y + dy, pos.z + dz);
                if (isFrameBlock(world, checkPos)) {
                    ++frameCount;
                }
            }
        }
    }

    return frameCount;
}

bool ConduitBlock::isFrameBlock(IWorld& world, const BlockPos& pos) const
{
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return false;
    }

    // 检查是否为海晶石系列方块
    // PRISMARINE, PRISMARINE_BRICKS, DARK_PRISMARINE, SEA_LANTERN
    if (VanillaBlocks::PRISMARINE && state->blockId() == VanillaBlocks::PRISMARINE->blockId()) {
        return true;
    }
    if (VanillaBlocks::PRISMARINE_BRICKS && state->blockId() == VanillaBlocks::PRISMARINE_BRICKS->blockId()) {
        return true;
    }
    if (VanillaBlocks::DARK_PRISMARINE && state->blockId() == VanillaBlocks::DARK_PRISMARINE->blockId()) {
        return true;
    }
    if (VanillaBlocks::SEA_LANTERN && state->blockId() == VanillaBlocks::SEA_LANTERN->blockId()) {
        return true;
    }

    return false;
}

void ConduitBlock::updateActivation(IWorld& world, const BlockPos& pos, i32 frameCount) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 需要16个框架方块才能激活
    if (frameCount >= 16) {
        // TODO: 激活潮涌核心
        // 需要方块实体系统来管理：
        // 1. 效果范围（最小32格，每多4个框架增加16格）
        // 2. 每秒给予玩家潮涌能量效果
        // 3. 攻击敌对生物
    }
}

} // namespace blocks
} // namespace mc
