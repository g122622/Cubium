#include "IceBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../BlockRegistry.hpp"
#include "../../../fluid/FluidRegistry.hpp"
#include "../../../fluid/Fluid.hpp"
#include "../../../../util/math/random/IRandom.hpp"

namespace mc {
namespace blocks {

// ============================================================================
// 常量
// ============================================================================

namespace {

/// 冰融化的最小光照等级
constexpr i32 MELT_LIGHT_LEVEL = 11;

/// 冰融化的随机概率（基于randomTickSpeed）
constexpr i32 MELT_PROBABILITY_DIVISOR = 40; // 约2.5%概率

} // namespace

// ============================================================================
// IceBlock 实现
// ============================================================================

IceBlock::IceBlock(BlockProperties properties)
    : Block(std::move(properties))
{
    // 设置摩擦力为冰的摩擦力（0.98）
    // 注意：摩擦力是通过BlockProperties设置的
}

void IceBlock::onBlockRemoved(
    IWorld& world,
    const BlockPos& pos,
    const BlockState& state
) {
    MC_UNUSED(state);

    // 检查是否在寒冷生物群系（不融化）
    // TODO: 生物群系温度检查

    // 在非寒冷环境中，冰变成水
    // 检查下方是否有固体方块阻挡
    BlockPos belowPos = pos.down();
    const BlockState* belowState = world.getBlockState(belowPos.x, belowPos.y, belowPos.z);

    // 获取水源方块状态
    fluid::Fluid* waterFluid = fluid::Fluid::getFluid(fluid::FluidRegistry::WATER_ID);
    const BlockState* waterState = nullptr;
    if (waterFluid != nullptr) {
        waterState = waterFluid->getBlockState(waterFluid->defaultState());
    }

    // 如果下方不是固体，放置水源；否则放置空气
    if (belowState == nullptr || !belowState->owner().isSolid(*belowState)) {
        // 如果下方不是固体，放置水源
        if (waterState != nullptr) {
            world.setBlockState(pos.x, pos.y, pos.z, waterState, 3);
        }
    } else {
        // 否则放置空气
        const BlockState* airState = BlockRegistry::instance().airState();
        if (airState != nullptr) {
            world.setBlockState(pos.x, pos.y, pos.z, airState, 3);
        }
    }
}

void IceBlock::randomTick(
    IWorld& world,
    const BlockPos& pos,
    BlockState& state,
    math::IRandom& random
) {
    MC_UNUSED(state);

    // 检查周围光照等级
    // 获取方块光照和天空光照的最大值
    u8 blockLight = world.getBlockLight(pos.x, pos.y, pos.z);
    u8 skyLight = world.getSkyLight(pos.x, pos.y, pos.z);
    i32 lightLevel = static_cast<i32>(std::max(blockLight, skyLight));

    // 光照等级 >= 11 时，冰可能融化
    if (lightLevel >= MELT_LIGHT_LEVEL) {
        // 随机融化
        if (random.nextInt(MELT_PROBABILITY_DIVISOR) == 0) {
            // 检查生物群系温度
            // TODO: 生物群系温度检查

            // 融化成水
            onBlockRemoved(world, pos, state);
        }
    }
}

// ============================================================================
// PackedIceBlock 实现
// ============================================================================

PackedIceBlock::PackedIceBlock(BlockProperties properties)
    : Block(std::move(properties))
{
    // 浮冰不需要特殊逻辑，不融化
}

// ============================================================================
// BlueIceBlock 实现
// ============================================================================

BlueIceBlock::BlueIceBlock(BlockProperties properties)
    : Block(std::move(properties))
{
    // 蓝冰摩擦力最低（0.989），通过BlockProperties设置
}

// ============================================================================
// FrostedIceBlock 实现
// ============================================================================

FrostedIceBlock::FrostedIceBlock(BlockProperties properties)
    : Block(std::move(properties))
{
    // 霜冰的摩擦力与普通冰相同
}

void FrostedIceBlock::randomTick(
    IWorld& world,
    const BlockPos& pos,
    BlockState& state,
    math::IRandom& random
) {
    MC_UNUSED(state);

    // 霜冰在光源附近融化更快
    u8 blockLight = world.getBlockLight(pos.x, pos.y, pos.z);
    u8 skyLight = world.getSkyLight(pos.x, pos.y, pos.z);
    i32 lightLevel = static_cast<i32>(std::max(blockLight, skyLight));

    // 光照等级越高，融化概率越大
    if (lightLevel >= MELT_LIGHT_LEVEL) {
        // 更高的光照 = 更快的融化
        i32 divisor = std::max(1, MELT_PROBABILITY_DIVISOR - (lightLevel - MELT_LIGHT_LEVEL) * 5);
        if (random.nextInt(divisor) == 0) {
            // 融化成水
            fluid::Fluid* waterFluid = fluid::Fluid::getFluid(fluid::FluidRegistry::WATER_ID);
            if (waterFluid != nullptr) {
                const BlockState* waterState = waterFluid->getBlockState(waterFluid->defaultState());
                if (waterState != nullptr) {
                    world.setBlockState(pos.x, pos.y, pos.z, waterState, 3);
                }
            }
        }
    }
}

} // namespace blocks
} // namespace mc
