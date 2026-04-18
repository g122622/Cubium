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

thread_local bool s_skipIceReplacementCallback = false;

class IceReplacementGuard {
public:
    IceReplacementGuard() {
        s_skipIceReplacementCallback = true;
    }

    ~IceReplacementGuard() {
        s_skipIceReplacementCallback = false;
    }
};

[[nodiscard]] const BlockState* getWaterState() {
    fluid::Fluid* waterFluid = fluid::Fluid::getFluid(fluid::FluidRegistry::WATER_ID);
    if (waterFluid != nullptr) {
        return waterFluid->getBlockState(waterFluid->defaultState());
    }
    return nullptr;
}

[[nodiscard]] const BlockState* getAirState() {
    return BlockRegistry::instance().airState();
}

void replaceIceState(IWorld& world, const BlockPos& pos, const BlockState* replacementState) {
    if (replacementState == nullptr) {
        return;
    }

    IceReplacementGuard guard;
    world.setBlockState(pos, replacementState, 3);
}

void meltIce(IWorld& world, const BlockPos& pos) {
    const BlockState* replacementState = world.isUltraWarm() ? getAirState() : getWaterState();
    replaceIceState(world, pos, replacementState);
}

void handleIceBreak(IWorld& world, const BlockPos& pos) {
    if (world.isUltraWarm()) {
        replaceIceState(world, pos, getAirState());
        return;
    }

    const BlockState* belowState = world.getBlockState(pos.down());
    if (belowState != nullptr && (belowState->isSolid() || belowState->isLiquid())) {
        replaceIceState(world, pos, getWaterState());
        return;
    }

    replaceIceState(world, pos, getAirState());
}

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

    if (s_skipIceReplacementCallback) {
        return;
    }

    handleIceBreak(world, pos);
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
    u8 blockLight = world.getBlockLight(pos);
    u8 skyLight = world.getSkyLight(pos);
    i32 lightLevel = static_cast<i32>(std::max(blockLight, skyLight));

    // 光照等级 >= 11 时，冰可能融化
    if (lightLevel >= MELT_LIGHT_LEVEL) {
        // 随机融化
        if (random.nextInt(MELT_PROBABILITY_DIVISOR) == 0) {
            meltIce(world, pos);
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
    u8 blockLight = world.getBlockLight(pos);
    u8 skyLight = world.getSkyLight(pos);
    i32 lightLevel = static_cast<i32>(std::max(blockLight, skyLight));

    // 光照等级越高，融化概率越大
    if (lightLevel >= MELT_LIGHT_LEVEL) {
        // 更高的光照 = 更快的融化
        i32 divisor = std::max(1, MELT_PROBABILITY_DIVISOR - (lightLevel - MELT_LIGHT_LEVEL) * 5);
        if (random.nextInt(divisor) == 0) {
            meltIce(world, pos);
        }
    }
}

} // namespace blocks
} // namespace mc
