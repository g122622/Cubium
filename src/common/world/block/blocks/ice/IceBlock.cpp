#include "IceBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../tick/manager/TickManager.hpp"
#include "../../BlockRegistry.hpp"
#include "../../../fluid/FluidRegistry.hpp"
#include "../../../fluid/Fluid.hpp"
#include "../../../../util/math/random/IRandom.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../tick/base/TickPriority.hpp"

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
    // 创建状态容器，添加 AGE 属性
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(AGE_PROP())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(AGE_PROP(), 0));
}

void FrostedIceBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // MC 1.16.5: 放置时调度 tick
    world.tickManager().scheduleBlockTick(pos, *this, math::Random(world.seed() ^ pos.toId()).nextInt(20, 40), world::tick::TickPriority::Normal);
}

void FrostedIceBlock::neighborChanged(IWorld& world, const BlockPos& pos, Block& neighborBlock,
                                       const BlockPos& neighborPos, bool isMoving) {
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // MC 1.16.5: 如果邻居是霜冰且应该融化，则融化
    const BlockState* state = world.getBlockState(pos);
    if (state && state->is(this)) {
        IBlockReader& blockReader = static_cast<IBlockReader&>(world);
        if (shouldMelt(blockReader, pos, 2)) {
            meltIce(world, pos);
        }
    }

    Block::neighborChanged(world, pos, neighborBlock, neighborPos, isMoving);
}

void FrostedIceBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    math::Random rng(world.seed() ^ static_cast<u64>(std::hash<BlockPos>{}(pos)));

    // MC 1.16.5: 检查是否应该融化
    IBlockReader& blockReader = static_cast<IBlockReader&>(world);
    i32 age = getAge(state);

    // 光照检查：光照 > 11 - age - opacity
    u8 blockLight = world.getBlockLight(pos);
    u8 skyLight = world.getSkyLight(pos);
    i32 lightLevel = static_cast<i32>(std::max(blockLight, skyLight));
    // 霜冰的不透明度通常是 2-3
    i32 opacity = state.getOpacity();

    bool shouldMeltNow = (rng.nextInt(3) == 0 || shouldMelt(blockReader, pos, 4)) &&
                          lightLevel > 11 - age - opacity;

    if (shouldMeltNow && slightlyMelt(world, pos, state)) {
        // 完全融化，通知相邻霜冰检查
        for (Direction dir : Directions::all()) {
            BlockPos neighborPos = pos.offset(dir);
            const BlockState* neighborState = world.getBlockState(neighborPos);
            if (neighborState && neighborState->is(this)) {
                // 调度相邻霜冰的 tick
                world.tickManager().scheduleBlockTick(neighborPos, *this, rng.nextInt(20, 40), world::tick::TickPriority::Normal);
            }
        }
    } else {
        // 继续调度下一次 tick
        world.tickManager().scheduleBlockTick(pos, *this, rng.nextInt(20, 40), world::tick::TickPriority::Normal);
    }
    MC_UNUSED(random);
}

void FrostedIceBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    // MC 1.16.5: randomTick 也调用 tick
    tick(world, pos, state, random);
}

bool FrostedIceBlock::shouldMelt(IBlockReader& world, const BlockPos& pos, i32 neighborsRequired) const {
    // MC 1.16.5: 检查周围霜冰邻居数量
    // 如果霜冰邻居数量 >= neighborsRequired，则不应该融化
    i32 frostNeighborCount = 0;

    for (Direction dir : Directions::all()) {
        BlockPos neighborPos = pos.offset(dir);
        const BlockState* neighborState = world.getBlockState(neighborPos);
        if (neighborState && neighborState->is(this)) {
            ++frostNeighborCount;
            if (frostNeighborCount >= neighborsRequired) {
                return false;
            }
        }
    }

    return true;
}

bool FrostedIceBlock::slightlyMelt(IWorld& world, const BlockPos& pos, BlockState& state) {
    i32 age = getAge(state);

    if (age < 3) {
        // 增加 AGE
        BlockState newState = state.with(AGE_PROP(), age + 1);
        world.setBlockState(pos, &newState, 2);
        return false;
    } else {
        // 完全融化成水
        meltIce(world, pos);
        return true;
    }
}

} // namespace blocks
} // namespace mc
