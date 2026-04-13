#include "WaterFluid.hpp"
#include "../FluidRegistry.hpp"
#include "../FluidTags.hpp"
#include "../../../util/property/FluidProperties.hpp"
#include "../../../util/property/Properties.hpp"
#include "../../block/VanillaBlocks.hpp"
#include "../../block/Block.hpp"

namespace mc {
namespace fluid {

// ============================================================================
// WaterFluid 基类实现
// ============================================================================

const BlockState* WaterFluid::getBlockState(const FluidState& state) const {
    // 方块LEVEL映射:
    // - 源头(level=8, isSource=true) -> 方块level=0
    // - 流动(level=1-7) -> 方块level=8-level
    // - 下落(level=8, falling=true) -> 方块level=8

    if (VanillaBlocks::WATER == nullptr) {
        return nullptr;
    }

    // 获取水的方块
    Block* waterBlock = isSource(state) ? VanillaBlocks::WATER : VanillaBlocks::WATER;

    if (waterBlock == nullptr) {
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
    return &waterBlock->defaultState().with(levelProp, blockLevel);
}

void WaterFluid::beforeReplacingBlock(IWorld& world, const BlockPos& pos,
                                       const BlockState* state) {
    // 水替换方块前：掉落方块物品
    if (state == nullptr || state->isAir()) {
        return;
    }

    // TODO: 掉落方块物品
    // Block::dropBlockAsItem(world, pos, state, 0);
    (void)world;
    (void)pos;
    (void)state;
}

bool WaterFluid::isEquivalentTo(const Fluid& fluid) const {
    // 水和流动水视为等效
    const auto& loc = fluid.fluidLocation();
    return loc.namespace_() == "minecraft" &&
           (loc.path() == "water" || loc.path() == "flowing_water");
}

bool WaterFluid::canDisplace(const FluidState& state, IWorld& world,
                             const BlockPos& pos, const Fluid& fluid,
                             Direction dir) const {
    (void)state;
    (void)world;
    (void)pos;
    return dir == Direction::Down && !fluid.isIn(FluidTags::WATER());
}

// ============================================================================
// WaterSourceFluid 实现
// ============================================================================

WaterSourceFluid::WaterSourceFluid() {
    // 源头没有LEVEL属性，只有FALLING
    auto container = StateContainer<Fluid, FluidState>::Builder(*this)
        .add(FluidProperties::FALLING())
        .create([this](const Fluid& fluid, auto values, u32 id) {
            return std::make_unique<FluidState>(fluid, std::move(values), id);
        });
    createFluidState(std::move(container));
    setDefaultState(stateContainer().baseState());
}

FlowingFluid& WaterSourceFluid::getFlowing() {
    if (m_flowingCache == nullptr) {
        m_flowingCache = static_cast<FlowingFluid*>(
            FluidRegistry::instance().getFluid(ResourceLocation("minecraft:flowing_water")));
    }
    return *m_flowingCache;
}

// ============================================================================
// WaterFlowingFluid 实现
// ============================================================================

WaterFlowingFluid::WaterFlowingFluid() {
    // 流动水有LEVEL_1_8和FALLING属性
    auto container = StateContainer<Fluid, FluidState>::Builder(*this)
        .add(FluidProperties::LEVEL_1_8())
        .add(FluidProperties::FALLING())
        .create([this](const Fluid& fluid, auto values, u32 id) {
            return std::make_unique<FluidState>(fluid, std::move(values), id);
        });
    createFluidState(std::move(container));
    setDefaultState(stateContainer().baseState());
}

i32 WaterFlowingFluid::getLevel(const FluidState& state) const {
    auto& levelProp = FluidProperties::LEVEL_1_8();
    auto opt = state.getOptional(levelProp);
    return opt.has_value() ? opt.value() : 8;
}

FlowingFluid& WaterFlowingFluid::getStill() {
    if (m_stillCache == nullptr) {
        m_stillCache = static_cast<FlowingFluid*>(
            FluidRegistry::instance().getFluid(ResourceLocation("minecraft:water")));
    }
    return *m_stillCache;
}

bool WaterFlowingFluid::isEquivalentTo(const Fluid& fluid) const {
    // 水和流动水视为等效
    const auto& loc = fluid.fluidLocation();
    return loc.namespace_() == "minecraft" &&
           (loc.path() == "water" || loc.path() == "flowing_water");
}

} // namespace fluid
} // namespace mc
