#include "LavaFluid.hpp"
#include "../FluidRegistry.hpp"
#include "../FluidTags.hpp"
#include "../../../util/property/FluidProperties.hpp"
#include "../../../util/property/Properties.hpp"
#include "../../../util/math/random/IRandom.hpp"
#include "../../block/VanillaBlocks.hpp"
#include "../../block/Block.hpp"
#include "../../block/Material.hpp"
#include "../../IWorld.hpp"
#include "../../../util/Direction.hpp"
#include <cmath>

namespace mc {
namespace fluid {

// ============================================================================
// LavaFluid 基类实现
// ============================================================================

const BlockState* LavaFluid::getBlockState(const FluidState& state) const {
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

i32 LavaFluid::getTickDelay(IWorld& world) const {
    return world.isUltraWarm() ? 10 : 30;
}

bool LavaFluid::canDisplace(const FluidState& state, IWorld& world,
                            const BlockPos& pos, const Fluid& fluid,
                            Direction dir) const {
    (void)dir;
    return state.getActualHeight(world, pos) >= 0.44444445f && fluid.isIn(FluidTags::WATER());
}

namespace {

[[nodiscard]] bool isWaterFluid(const Fluid& fluid) {
    return fluid.isIn(FluidTags::WATER());
}

} // namespace

void LavaFluid::randomTick(IWorld& world, const BlockPos& pos,
                            const FluidState& state, math::IRandom& random) {
    // TODO: 目前 IWorld 还没有 DO_FIRE_TICK 游戏规则接口，先保留原版的火焰检查骨架。
    (void)state;

    if (VanillaBlocks::FIRE == nullptr) {
        return;
    }

    if (random.nextInt(3) != 0) {
        return;
    }

    BlockPos checkPos = pos;
    const i32 steps = random.nextInt(3);
    if (steps <= 0) {
        return;
    }

    for (i32 index = 0; index < steps; ++index) {
        checkPos = BlockPos(
            checkPos.x + random.nextInt(3) - 1,
            checkPos.y + 1,
            checkPos.z + random.nextInt(3) - 1);

        if (!world.isWithinWorldBounds(checkPos.x, checkPos.y, checkPos.z)) {
            return;
        }

        const BlockState* blockState = world.getBlockState(checkPos.x, checkPos.y, checkPos.z);
        if (blockState == nullptr) {
            return;
        }

        if (blockState->isAir()) {
            if (isSurroundingBlockFlammable(world, checkPos)) {
                world.setBlock(checkPos.x, checkPos.y, checkPos.z, &VanillaBlocks::FIRE->defaultState());
                return;
            }
        } else if (blockState->owner().material().blocksMovement()) {
            BlockPos above = checkPos.up();
            const BlockState* aboveState = world.getBlockState(above.x, above.y, above.z);
            if (aboveState != nullptr && aboveState->isAir() && isBlockFlammable(world, checkPos)) {
                world.setBlock(above.x, above.y, above.z, &VanillaBlocks::FIRE->defaultState());
                return;
            }
        }
    }
}

bool LavaFluid::isSurroundingBlockFlammable(IWorld& world, const BlockPos& pos) const {
    for (Direction dir : Directions::all()) {
        BlockPos neighborPos = pos.offset(Directions::toBlockFace(dir));
        if (isBlockFlammable(world, neighborPos)) {
            return true;
        }
    }
    return false;
}

bool LavaFluid::isBlockFlammable(IWorld& world, const BlockPos& pos) const {
    if (pos.y < 0 || pos.y >= 256) {
        return false;
    }

    const BlockState* blockState = world.getBlockState(pos.x, pos.y, pos.z);
    if (blockState == nullptr) {
        return false;
    }

    return blockState->owner().material().isFlammable();
}

void LavaFluid::beforeReplacingBlock(IWorld& world, const BlockPos& pos,
                                      const BlockState* state) {
    // 岩浆替换方块前触发效果
    triggerEffects(world, pos);
}

bool LavaFluid::checkForMixing(IWorld& world, const BlockPos& pos, Direction direction) {
    // 检查岩浆是否遇到水
    BlockPos neighborPos = pos.offset(Directions::toBlockFace(direction));
    const FluidState* neighborFluid = world.getFluidState(neighborPos.x, neighborPos.y, neighborPos.z);

    if (neighborFluid == nullptr || neighborFluid->isEmpty()) {
        return false;
    }

    if (!isWaterFluid(neighborFluid->getFluid())) {
        return false;
    }

    if (direction == Direction::Down) {
        if (VanillaBlocks::STONE != nullptr) {
            world.setBlock(pos.x, pos.y, pos.z, &VanillaBlocks::STONE->defaultState());
        }
        triggerEffects(world, pos);
        return true;
    }

    if (VanillaBlocks::OBSIDIAN == nullptr || VanillaBlocks::COBBLESTONE == nullptr) {
        return false;
    }

    const BlockState* replacement = isSource(defaultState())
        ? &VanillaBlocks::OBSIDIAN->defaultState()
        : &VanillaBlocks::COBBLESTONE->defaultState();

    world.setBlock(pos.x, pos.y, pos.z, replacement);

    triggerEffects(world, pos);
    return true;
}

void LavaFluid::triggerEffects(IWorld& world, const BlockPos& pos) {
    // TODO: 触发烟雾和嘶嘶声音效果
    // world.playEvent(1501, pos, 0);
    (void)world;
    (void)pos;
}

void LavaFluid::flowInto(IWorld& world, const BlockPos& pos, const BlockState* blockState,
                          Direction dir, const FluidState& fluidState) {
    // 重写flowInto以处理岩浆与水的交互
    if (checkForMixing(world, pos, dir)) {
        return;
    }

    // 调用父类方法
    FlowingFluid::flowInto(world, pos, blockState, dir, fluidState);
}

bool LavaFluid::isEquivalentTo(const Fluid& fluid) const {
    // 岩浆和流动岩浆视为等效
    const auto& loc = fluid.fluidLocation();
    return loc.namespace_() == "minecraft" &&
           (loc.path() == "lava" || loc.path() == "flowing_lava");
}

i32 LavaFluid::getLevelDecrease(IWorld& world) const {
    return world.isUltraWarm() ? 1 : 2;
}

i32 LavaFluid::getSpreadDistance(IWorld& world) const {
    return world.isUltraWarm() ? 6 : 4;
}

// ============================================================================
// LavaSourceFluid 实现
// ============================================================================

LavaSourceFluid::LavaSourceFluid() {
    // 源头没有LEVEL属性，只有FALLING
    auto container = StateContainer<Fluid, FluidState>::Builder(*this)
        .add(FluidProperties::FALLING())
        .create([this](const Fluid& fluid, auto values, u32 id) {
            return std::make_unique<FluidState>(fluid, std::move(values), id);
        });
    createFluidState(std::move(container));
    setDefaultState(stateContainer().baseState());
}

FlowingFluid& LavaSourceFluid::getFlowing() {
    if (m_flowingCache == nullptr) {
        m_flowingCache = static_cast<FlowingFluid*>(
            FluidRegistry::instance().getFluid(ResourceLocation("minecraft:flowing_lava")));
    }
    return *m_flowingCache;
}

bool LavaSourceFluid::isEquivalentTo(const Fluid& fluid) const {
    const auto& loc = fluid.fluidLocation();
    return loc.namespace_() == "minecraft" &&
           (loc.path() == "lava" || loc.path() == "flowing_lava");
}

// ============================================================================
// LavaFlowingFluid 实现
// ============================================================================

LavaFlowingFluid::LavaFlowingFluid() {
    auto container = StateContainer<Fluid, FluidState>::Builder(*this)
        .add(FluidProperties::LEVEL_1_8())
        .add(FluidProperties::FALLING())
        .create([this](const Fluid& fluid, auto values, u32 id) {
            return std::make_unique<FluidState>(fluid, std::move(values), id);
        });
    createFluidState(std::move(container));
    setDefaultState(stateContainer().baseState());
}

i32 LavaFlowingFluid::getLevel(const FluidState& state) const {
    auto& levelProp = FluidProperties::LEVEL_1_8();
    auto opt = state.getOptional(levelProp);
    return opt.has_value() ? opt.value() : 8;
}

FlowingFluid& LavaFlowingFluid::getStill() {
    if (m_stillCache == nullptr) {
        m_stillCache = static_cast<FlowingFluid*>(
            FluidRegistry::instance().getFluid(ResourceLocation("minecraft:lava")));
    }
    return *m_stillCache;
}

bool LavaFlowingFluid::isEquivalentTo(const Fluid& fluid) const {
    const auto& loc = fluid.fluidLocation();
    return loc.namespace_() == "minecraft" &&
           (loc.path() == "lava" || loc.path() == "flowing_lava");
}

} // namespace fluid
} // namespace mc
