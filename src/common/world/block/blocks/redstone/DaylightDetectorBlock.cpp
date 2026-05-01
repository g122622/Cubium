#include "DaylightDetectorBlock.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../../tick/base/TickPriority.hpp"
#include "../../../IWorld.hpp"
#include "../../../tick/manager/TickManager.hpp"
#include "../../../lighting/InternalLightUtils.hpp"
#include <unordered_map>
#include <cmath>

namespace mc {
namespace blocks {

DaylightDetectorBlock::DaylightDetectorBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::POWER_0_15())
        .add(BlockStateProperties::INVERTED())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::POWER_0_15(), 0)
        .with(BlockStateProperties::INVERTED(), false));
}

i32 DaylightDetectorBlock::getPower(const BlockState& state) {
    return state.get(BlockStateProperties::POWER_0_15());
}

BlockState DaylightDetectorBlock::withPower(BlockState state, i32 power) {
    return state.with(BlockStateProperties::POWER_0_15(), std::clamp(power, 0, 15));
}

bool DaylightDetectorBlock::isInverted(const BlockState& state) {
    return state.get(BlockStateProperties::INVERTED());
}

BlockState DaylightDetectorBlock::withInverted(BlockState state, bool inverted) {
    return state.with(BlockStateProperties::INVERTED(), inverted);
}

void DaylightDetectorBlock::toggleMode(IWorld& world, const BlockPos& pos, const BlockState& state) {
    bool newInverted = !isInverted(state);
    BlockState newState = withInverted(state, newInverted);

    // 立即更新信号强度
    i32 power = calculateSignalStrength(world, pos, newInverted);
    newState = withPower(newState, power);

    world.setBlockState(pos, &newState, 2);

    // 通知相邻方块
    notifyNeighbors(world, pos);
}

void DaylightDetectorBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // 立即更新信号强度
    updatePower(world, pos, state);
}

void DaylightDetectorBlock::neighborChanged(IWorld& world, const BlockPos& pos, Block& neighborBlock,
                                            const BlockPos& neighborPos, bool isMoving) {
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // 调度更新
    world.tickManager().scheduleBlockTick(pos, *this, UPDATE_DELAY, world::tick::TickPriority::Normal);
}

void DaylightDetectorBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state) {
    // 更新信号强度
    updatePower(world, pos, state);

    // 继续调度下一次更新
    world.tickManager().scheduleBlockTick(pos, *this, UPDATE_DELAY, world::tick::TickPriority::Normal);
}

i32 DaylightDetectorBlock::getWeakPower(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Direction side
) const {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);

    // 日光探测器向所有方向输出信号
    return getPower(state);
}

i32 DaylightDetectorBlock::getStrongPower(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Direction side
) const {
    // 日光探测器只输出弱信号
    return getWeakPower(state, world, pos, side);
}

i32 DaylightDetectorBlock::calculateSignalStrength(IWorld& world, const BlockPos& pos, bool inverted) {
    // 检查维度是否有天空光照（主世界有，下界和末地没有）
    if (!world.hasSkyLight()) {
        return 0;
    }

    // 获取天空光照
    // MC Java: int i = world.getLightFor(LightType.SKY, pos) - world.getSkylightSubtracted();
    u8 skyLight = world.getSkyLight(pos.up());

    // 使用 InternalLightUtils 计算天空减暗因子
    i64 dayTime = world.dayTime();
    i32 skyDarkening = InternalLightUtils::calculateSkyDarkening(
        dayTime,
        world.isRaining(),
        world.isThundering()
    );

    i32 i = static_cast<i32>(skyLight) - skyDarkening;

    // 获取天体角度进行余弦调整
    // MC Java: float f = world.getCelestialAngleRadians(1.0F);
    if (!inverted && i > 0) {
        f32 celestialAngle = InternalLightUtils::getCelestialAngle(dayTime);
        // 转换为弧度（getCelestialAngle 返回 0.0-1.0）
        constexpr f32 TWO_PI = 6.28318530718f;
        f32 f = celestialAngle * TWO_PI;

        // MC Java: float f1 = f < (float)Math.PI ? 0.0F : ((float)Math.PI * 2F);
        constexpr f32 PI = 3.14159265359f;
        f32 f1 = f < PI ? 0.0f : TWO_PI;

        // MC Java: f = f + (f1 - f) * 0.2F;
        f = f + (f1 - f) * 0.2f;

        // MC Java: i = Math.round((float)i * MathHelper.cos(f));
        i = static_cast<i32>(std::round(static_cast<f32>(i) * std::cos(f)));
    }

    // 反转模式
    if (inverted) {
        i = 15 - i;
    }

    return std::clamp(i, 0, 15);
}

void DaylightDetectorBlock::updatePower(IWorld& world, const BlockPos& pos, const BlockState& state) {
    bool inverted = isInverted(state);
    i32 oldPower = getPower(state);
    i32 newPower = calculateSignalStrength(world, pos, inverted);

    if (oldPower != newPower) {
        BlockState newState = withPower(state, newPower);
        world.setBlockState(pos, &newState, 2);

        // 通知相邻方块更新
        notifyNeighbors(world, pos);
    }
}

void DaylightDetectorBlock::notifyNeighbors(IWorld& world, const BlockPos& pos) {
    // 获取当前方块用于通知
    const BlockState* currentState = world.getBlockState(pos);
    if (!currentState) {
        return;
    }
    const Block& block = currentState->getBlock();

    // 通知六个方向的相邻方块
    for (Direction dir : Directions::all()) {
        BlockPos neighborPos = pos.offset(dir);
        const BlockState* neighborState = world.getBlockState(neighborPos);

        if (neighborState && !neighborState->isAir()) {
            Block& neighborBlock = const_cast<Block&>(neighborState->getBlock());
            neighborBlock.neighborChanged(world, neighborPos, const_cast<Block&>(block), pos, false);
        }
    }
}

} // namespace blocks
} // namespace mc
