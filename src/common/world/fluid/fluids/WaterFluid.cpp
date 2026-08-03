/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "WaterFluid.hpp"
#include "common/core/Types.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/loot/LootTable.hpp"
#include "common/item/loot/LootTableManager.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/FluidProperties.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/fluid/FlowingFluid.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace fluid {

// ============================================================================
// WaterFluid 基类实现
// ============================================================================

const BlockState* WaterFluid::getBlockState(const FluidState& state) const
{
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
        blockLevel = state.isFalling() ? SOURCE_LEVEL : 0;
    } else {
        i32 fluidLevel = state.getLevel();
        blockLevel = SOURCE_LEVEL - fluidLevel;
        if (state.isFalling()) {
            blockLevel = SOURCE_LEVEL;
        }
    }

    // 设置LEVEL属性
    const auto& levelProp = BlockStateProperties::LEVEL_0_15();
    return &waterBlock->defaultState().with(levelProp, blockLevel);
}

void WaterFluid::beforeReplacingBlock(IWorld& world, const BlockPos& pos, const BlockState* state)
{
    // 水替换方块时，生成方块掉落物
    if (state == nullptr || state->isAir()) {
        return;
    }

    // 获取掉落表管理器
    const loot::LootTableManager* lootTableManager = world.lootTableManager();
    if (lootTableManager == nullptr) {
        // 没有掉落表管理器，无法生成掉落物
        return;
    }

    // 获取方块的掉落表
    const Block& block = state->owner();
    const loot::LootTable* lootTable = block.getLootTable(*lootTableManager);

    std::vector<ItemStack> drops;

    if (lootTable != nullptr) {
        // 使用掉落表生成掉落物
        math::Random rng(static_cast<u64>(world.seed() ^ static_cast<u64>(pos.x ^ pos.z)));

        auto context = loot::LootContextBuilder(world)
                           .withRandom(rng)
                           .withSeed(world.seed() ^ static_cast<u64>(pos.x ^ pos.z))
                           .build();

        if (context) {
            // 设置方块状态和位置参数
            context->set(loot::LootParams::BLOCK_STATE, const_cast<BlockState*>(state));
            context->set(loot::LootParams::BLOCK_POS, const_cast<BlockPos*>(&pos));

            // 设置掉落表解析器
            context->setLootTableResolver([&lootTableManager](const std::string& id) -> const loot::LootTable* {
                return lootTableManager->getTable(id);
            });
            context->setPredicateResolver([&lootTableManager](const std::string& id) -> const loot::LootCondition* {
                return lootTableManager->getPredicate(id);
            });

            // 生成掉落物
            drops = lootTable->generate(*context);
        }
    }
    // 如果没有掉落表，则没有掉落物（水流破坏方块不使用默认掉落逻辑）

    // 如果有掉落物，生成物品实体
    if (!drops.empty()) {
        // 使用固定种子生成随机速度
        math::Random rng(static_cast<u64>(world.seed() ^ static_cast<u64>(pos.x ^ pos.z)));

        // 使用 ItemDropHelper 生成物品实体
        ItemDropHelper::spawnItemEntities(&world, pos, drops, rng);
    }
}

bool WaterFluid::isEquivalentTo(const Fluid& fluid) const noexcept
{
    // 水和流动水视为等效
    const auto& loc = fluid.fluidLocation();
    return loc.namespace_() == "minecraft" && (loc.path() == "water" || loc.path() == "flowing_water");
}

bool WaterFluid::canDisplace(
    const FluidState& state, IWorld& world, const BlockPos& pos, const Fluid& fluid, Direction dir) const
{
    (void)state;
    (void)world;
    (void)pos;
    return dir == Direction::Down && !fluid.isIn(FluidTags::WATER());
}

// ============================================================================
// WaterSourceFluid 实现
// ============================================================================

WaterSourceFluid::WaterSourceFluid()
{
    // 源头没有LEVEL属性，只有FALLING
    auto container =
        StateContainer<Fluid, FluidState>::Builder(*this)
            .add(FluidProperties::FALLING())
            .create([this](const Fluid& fluid,
                        auto values,
                        const std::vector<StateHolder<Fluid, FluidState>::PropertyLayout>* propertyLayouts,
                        const std::vector<FluidState*>* allStates,
                        u32 id) {
                return std::make_unique<FluidState>(fluid, std::move(values), propertyLayouts, allStates, id);
            });
    createFluidState(std::move(container));
    setDefaultState(stateContainer().baseState());
}

FlowingFluid& WaterSourceFluid::getFlowing()
{
    if (m_flowingCache == nullptr) {
        m_flowingCache =
            static_cast<FlowingFluid*>(FluidRegistry::instance().getFluid(ResourceLocation("minecraft:flowing_water")));
    }
    return *m_flowingCache;
}

// ============================================================================
// WaterFlowingFluid 实现
// ============================================================================

WaterFlowingFluid::WaterFlowingFluid()
{
    // 流动水有LEVEL_1_8和FALLING属性
    auto container =
        StateContainer<Fluid, FluidState>::Builder(*this)
            .add(FluidProperties::LEVEL_1_8())
            .add(FluidProperties::FALLING())
            .create([this](const Fluid& fluid,
                        auto values,
                        const std::vector<StateHolder<Fluid, FluidState>::PropertyLayout>* propertyLayouts,
                        const std::vector<FluidState*>* allStates,
                        u32 id) {
                return std::make_unique<FluidState>(fluid, std::move(values), propertyLayouts, allStates, id);
            });
    createFluidState(std::move(container));
    setDefaultState(stateContainer().baseState());
}

i32 WaterFlowingFluid::getLevel(const FluidState& state) const
{
    auto& levelProp = FluidProperties::LEVEL_1_8();
    auto opt = state.getOptional(levelProp);
    return opt.has_value() ? opt.value() : SOURCE_LEVEL;
}

FlowingFluid& WaterFlowingFluid::getStill()
{
    if (m_stillCache == nullptr) {
        m_stillCache =
            static_cast<FlowingFluid*>(FluidRegistry::instance().getFluid(ResourceLocation("minecraft:water")));
    }
    return *m_stillCache;
}

bool WaterFlowingFluid::isEquivalentTo(const Fluid& fluid) const noexcept
{
    // 水和流动水视为等效
    const auto& loc = fluid.fluidLocation();
    return loc.namespace_() == "minecraft" && (loc.path() == "water" || loc.path() == "flowing_water");
}

} // namespace fluid
} // namespace mc
