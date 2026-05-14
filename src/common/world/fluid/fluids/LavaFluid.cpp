#include "LavaFluid.hpp"
#include "../../../core/Constants.hpp"
#include "../../../util/Direction.hpp"
#include "../../../util/math/random/IRandom.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../util/property/FluidProperties.hpp"
#include "../../../util/property/Properties.hpp"
#include "../../IWorld.hpp"
#include "../../WorldEvents.hpp"
#include "../../block/Block.hpp"
#include "../../block/Material.hpp"
#include "../../block/VanillaBlocks.hpp"
#include "../FluidRegistry.hpp"
#include "../FluidTags.hpp"
#include <cmath>

namespace mc {
namespace fluid {

// ============================================================================
// LavaFluid 基类实现
// ============================================================================

const BlockState* LavaFluid::getBlockState(const FluidState& state) const
{
    // 方块LEVEL映射:
    // - 源头(level=8, isSource=true) -> 方块level=0
    // - 流动(level=1-7) -> 方块level=8-level
    // - 下落(level=8, falling=true) -> 方块level=8

    if (VanillaBlocks::LAVA == nullptr) {
        return nullptr;
    }

    // 获取岩浆的方块
    Block* lavaBlock = isSource(state) ? VanillaBlocks::LAVA : VanillaBlocks::LAVA;

    if (lavaBlock == nullptr) {
        return nullptr;
    }

    // 计算方块level
    i32 blockLevel;
    if (isSource(state)) {
        blockLevel = state.isFalling() ? 8 : 0;
    } else {
        i32 fluidLevel = state.getLevel();
        blockLevel = 8 - fluidLevel;
        if (state.isFalling()) {
            blockLevel = 8;
        }
    }

    // 设置LEVEL属性
    const auto& levelProp = BlockStateProperties::LEVEL_0_15();
    return &lavaBlock->defaultState().with(levelProp, blockLevel);
}

i32 LavaFluid::getTickDelay(IWorld& world) const
{
    return world.isUltraWarm() ? 10 : 30;
}

i32 LavaFluid::getTickDelay(
    IWorld& world, const BlockPos& pos, const FluidState& state, const FluidState& correctState) const
{
    i32 tickDelay = getTickDelay(world);
    if (!state.isEmpty() && !correctState.isEmpty() && !state.isFalling() && !correctState.isFalling() &&
        correctState.getActualHeight(world, pos) > state.getActualHeight(world, pos)) {
        u64 seed = world.seed();
        seed ^= static_cast<u64>(pos.x) * 341873128712ULL;
        seed ^= static_cast<u64>(pos.y) * 132897987541ULL;
        seed ^= static_cast<u64>(pos.z) * 42317861ULL;
        seed ^= world.currentTick();

        math::Random random(seed);
        if (random.nextInt(4) != 0) {
            tickDelay *= 4;
        }
    }

    return tickDelay;
}

bool LavaFluid::canDisplace(
    const FluidState& state, IWorld& world, const BlockPos& pos, const Fluid& fluid, Direction dir) const
{
    (void)dir;
    return state.getActualHeight(world, pos) >= 0.44444445f && fluid.isIn(FluidTags::WATER());
}

void LavaFluid::randomTick(IWorld& world, const BlockPos& pos, const FluidState& state, math::IRandom& random)
{
    // 参考: net.minecraft.fluid.LavaFluid#randomTick
    // MC 1.16.5 源码逻辑:
    // int i = random.nextInt(3);
    // if (i > 0) {
    //     // 向上搜索可燃位置
    //     BlockPos blockpos = pos;
    //     for(int j = 0; j < i; ++j) {
    //         blockpos = blockpos.add(random.nextInt(3) - 1, 1, random.nextInt(3) - 1);
    //         if (blockstate.isAir()) {
    //             if (isSurroundingBlockFlammable(...)) { 点火; return; }
    //         } else if (blockstate.getMaterial().blocksMovement()) {
    //             return; // 阻挡移动，直接返回
    //         }
    //     }
    // } else {
    //     // i == 0: 水平搜索
    //     for(int k = 0; k < 3; ++k) {
    //         BlockPos blockpos1 = pos.add(random.nextInt(3) - 1, 0, random.nextInt(3) - 1);
    //         if (上方是空气 && 下方方块可燃) { 在上方点火; }
    //     }
    // }

    if (!world.doFireTick()) {
        return;
    }

    (void)state;

    if (VanillaBlocks::FIRE == nullptr) {
        return;
    }

    i32 i = random.nextInt(3);

    if (i > 0) {
        // 向上搜索可燃位置
        BlockPos checkPos = pos;
        for (i32 j = 0; j < i; ++j) {
            checkPos = BlockPos(checkPos.x + random.nextInt(3) - 1, checkPos.y + 1, checkPos.z + random.nextInt(3) - 1);

            if (!world.isWithinWorldBounds(checkPos)) {
                return;
            }

            const BlockState* blockState = world.getBlockState(checkPos);
            if (blockState == nullptr) {
                return;
            }

            if (blockState->isAir()) {
                // 检查周围是否有可燃方块
                if (isSurroundingBlockFlammable(world, checkPos)) {
                    world.setBlockState(checkPos, &VanillaBlocks::FIRE->defaultState());
                    return;
                }
            } else if (blockState->owner().material().blocksMovement()) {
                // 阻挡移动的方块，停止搜索
                return;
            }
        }
    } else {
        // i == 0: 水平搜索，在可燃方块上方点火
        for (i32 k = 0; k < 3; ++k) {
            BlockPos checkPos = BlockPos(pos.x + random.nextInt(3) - 1, pos.y, pos.z + random.nextInt(3) - 1);

            if (!world.isWithinWorldBounds(checkPos)) {
                continue;
            }

            const BlockState* blockState = world.getBlockState(checkPos);
            if (blockState == nullptr) {
                continue;
            }

            // 检查上方是否是空气，且当前方块是否可燃
            BlockPos abovePos = checkPos.up();
            if (!world.isWithinWorldBounds(abovePos)) {
                continue;
            }

            const BlockState* aboveState = world.getBlockState(abovePos);
            if (aboveState != nullptr && aboveState->isAir() && isBlockFlammable(world, checkPos)) {
                world.setBlockState(abovePos, &VanillaBlocks::FIRE->defaultState());
            }
        }
    }
}

bool LavaFluid::isSurroundingBlockFlammable(IWorld& world, const BlockPos& pos) const
{
    for (Direction dir : Directions::all()) {
        BlockPos neighborPos = pos.offset(Directions::toBlockFace(dir));
        if (isBlockFlammable(world, neighborPos)) {
            return true;
        }
    }
    return false;
}

bool LavaFluid::isBlockFlammable(IWorld& world, const BlockPos& pos) const
{
    if (pos.y < world::MIN_BUILD_HEIGHT || pos.y >= world::MAX_BUILD_HEIGHT) {
        return false;
    }

    const BlockState* blockState = world.getBlockState(pos);
    if (blockState == nullptr) {
        return false;
    }

    return blockState->owner().material().isFlammable();
}

void LavaFluid::beforeReplacingBlock(IWorld& world, const BlockPos& pos, const BlockState* state)
{
    // 岩浆替换方块前触发效果
    triggerEffects(world, pos);
}

void LavaFluid::triggerEffects(IWorld& world, const BlockPos& pos)
{
    // 触发烟雾和嘶嘶声音效果
    // 参考: net.minecraft.fluid.LavaFluid#triggerEffects
    world.playEvent(world::WorldEvents::LAVA_EXTINGUISH, pos, 0);
}

void LavaFluid::flowInto(
    IWorld& world, const BlockPos& pos, const BlockState* blockState, Direction dir, const FluidState& fluidState)
{
    // 参考: net.minecraft.fluid.LavaFluid#flowInto
    // MC 1.16.5 行为: 只有在向下流动时(direction == DOWN)才检查水交互
    // 当岩浆向下流入水时，把目标位置变成石头
    if (dir == Direction::Down) {
        // 检查目标位置是否有水
        const FluidState* targetFluid = world.getFluidState(pos);
        if (targetFluid != nullptr && !targetFluid->isEmpty() && targetFluid->getFluid().isIn(FluidTags::WATER())) {
            // 岩浆向下流入水 -> 生成石头
            // MC 还检查目标方块是否是 FlowingFluidBlock，这里简化处理
            if (VanillaBlocks::STONE != nullptr) {
                world.setBlockState(pos, &VanillaBlocks::STONE->defaultState(), 3);
            }
            triggerEffects(world, pos);
            return;
        }
    }

    // 调用父类方法处理正常流动
    FlowingFluid::flowInto(world, pos, blockState, dir, fluidState);
}

bool LavaFluid::isEquivalentTo(const Fluid& fluid) const
{
    // 岩浆和流动岩浆视为等效
    const auto& loc = fluid.fluidLocation();
    return loc.namespace_() == "minecraft" && (loc.path() == "lava" || loc.path() == "flowing_lava");
}

i32 LavaFluid::getLevelDecrease(IWorld& world) const
{
    return world.isUltraWarm() ? 1 : 2;
}

i32 LavaFluid::getSpreadDistance(IWorld& world) const
{
    return world.isUltraWarm() ? 4 : 2;
}

// ============================================================================
// LavaSourceFluid 实现
// ============================================================================

LavaSourceFluid::LavaSourceFluid()
{
    // 源头没有LEVEL属性，只有FALLING
    auto container = StateContainer<Fluid, FluidState>::Builder(*this)
                         .add(FluidProperties::FALLING())
                         .create([this](const Fluid& fluid, auto values, u32 id) {
                             return std::make_unique<FluidState>(fluid, std::move(values), id);
                         });
    createFluidState(std::move(container));
    setDefaultState(stateContainer().baseState());
}

FlowingFluid& LavaSourceFluid::getFlowing()
{
    if (m_flowingCache == nullptr) {
        m_flowingCache =
            static_cast<FlowingFluid*>(FluidRegistry::instance().getFluid(ResourceLocation("minecraft:flowing_lava")));
    }
    return *m_flowingCache;
}

bool LavaSourceFluid::isEquivalentTo(const Fluid& fluid) const
{
    const auto& loc = fluid.fluidLocation();
    return loc.namespace_() == "minecraft" && (loc.path() == "lava" || loc.path() == "flowing_lava");
}

// ============================================================================
// LavaFlowingFluid 实现
// ============================================================================

LavaFlowingFluid::LavaFlowingFluid()
{
    auto container = StateContainer<Fluid, FluidState>::Builder(*this)
                         .add(FluidProperties::LEVEL_1_8())
                         .add(FluidProperties::FALLING())
                         .create([this](const Fluid& fluid, auto values, u32 id) {
                             return std::make_unique<FluidState>(fluid, std::move(values), id);
                         });
    createFluidState(std::move(container));
    setDefaultState(stateContainer().baseState());
}

i32 LavaFlowingFluid::getLevel(const FluidState& state) const
{
    auto& levelProp = FluidProperties::LEVEL_1_8();
    auto opt = state.getOptional(levelProp);
    return opt.has_value() ? opt.value() : 8;
}

FlowingFluid& LavaFlowingFluid::getStill()
{
    if (m_stillCache == nullptr) {
        m_stillCache =
            static_cast<FlowingFluid*>(FluidRegistry::instance().getFluid(ResourceLocation("minecraft:lava")));
    }
    return *m_stillCache;
}

bool LavaFlowingFluid::isEquivalentTo(const Fluid& fluid) const
{
    const auto& loc = fluid.fluidLocation();
    return loc.namespace_() == "minecraft" && (loc.path() == "lava" || loc.path() == "flowing_lava");
}

} // namespace fluid
} // namespace mc
