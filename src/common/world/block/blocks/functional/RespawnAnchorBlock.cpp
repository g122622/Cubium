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

#include "RespawnAnchorBlock.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../item/items/block/BlockItemRegistry.hpp"
#include "../../../../resource/ResourceLocation.hpp"
#include "../../../../sound/SoundCategory.hpp"
#include "../../../../util/assert/AssertAll.hpp"
#include "../../../IWorld.hpp"
#include "../../../dimension/DimensionType.hpp"
#include "../../../explosion/ExplosionMode.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <cmath>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ========== RespawnAnchorBlock 实现 ==========

RespawnAnchorBlock::RespawnAnchorBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::CHARGES_0_4())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::CHARGES_0_4(), 0));

    // 重生锚形状是完整方块
    m_shape = CollisionShape::fullBlock();
}

BlockState RespawnAnchorBlock::getStateForPlacement(BlockItemUseContext& context)
{
    return defaultState();
}

void RespawnAnchorBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 重生锚的tick处理
    // 当前没有特殊的tick逻辑
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    MC_UNUSED(random);
}

void RespawnAnchorBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 重生锚没有随机刻逻辑
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    MC_UNUSED(random);
}

const CollisionShape& RespawnAnchorBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

int RespawnAnchorBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{

    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 比较器输出 = 充能等级
    return getCharges(state);
}

u8 RespawnAnchorBlock::getLightLevel(const BlockState& state, IWorld* world, const BlockPos* pos) const
{

    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 光照等级 = charges * 3.75，向下取整
    // 0 -> 0, 1 -> 3, 2 -> 7, 3 -> 11, 4 -> 15
    int charges = getCharges(state);
    return static_cast<u8>(std::floor(charges * 3.75f));
}

BlockState RespawnAnchorBlock::charge(IWorld& world, const BlockPos& pos, BlockState& state)
{
    int charges = getCharges(state);
    if (charges < 4) {
        BlockState newState = state.with(BlockStateProperties::CHARGES_0_4(), charges + 1);
        world.setBlockState(pos, &newState, 3);
        // 注意：充能音效和粒子效果由调用方处理（onBlockActivated 中播放）
        return newState;
    }
    return state;
}

void RespawnAnchorBlock::discharge(IWorld& world, const BlockPos& pos, BlockState& state)
{
    int charges = getCharges(state);
    if (charges > 0) {
        BlockState newState = state.with(BlockStateProperties::CHARGES_0_4(), charges - 1);
        world.setBlockState(pos, &newState, 3);
    }
}

BlockActionResult RespawnAnchorBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{

    MC_UNUSED(hit);

    // 获取维度信息
    DimensionType dimType = DimensionType::fromId(world.dimension());

    // 获取手持物品
    ItemStack& heldItem = player.getHeldItem(hand);

    // 检查是否用萤石充能
    bool hasGlowstone = false;
    if (!heldItem.isEmpty()) {
        const Item* item = heldItem.getItem();
        if (item != nullptr) {
            const Block* block = BlockItemRegistry::instance().getBlock(item->itemId());
            hasGlowstone = (block == VanillaBlocks::GLOWSTONE);
        }
    }

    if (hasGlowstone && getCharges(state) < 4) {
        // 充能
        BlockState newState = charge(world, pos, const_cast<BlockState&>(state));

        // 播放充能音效
        world.playSound(ResourceLocation("minecraft:block.respawn_anchor.charge"),
            sound::SoundCategory::Blocks,
            pos.center(),
            1.0f,
            1.0f);

        // 消耗萤石
        heldItem.shrink(1);

        return ActionResultType::Success;
    }

    // 检查重生锚是否在此维度可用
    if (!dimType.respawnAnchorWorks()) {
        // 在非下界使用重生锚会爆炸
        // 移除重生锚
        world.setBlockState(pos, nullptr, 11);

        // 爆炸强度为 5.0，破坏方块但不生成火焰
        world.createExplosion(pos.center(),
            5.0f, // 爆炸半径
            world::explosion::ExplosionMode::Destroy,
            false // 不生成火焰
        );

        return ActionResultType::Success;
    }

    // 在下界使用重生锚设置重生点
    if (getCharges(state) > 0) {
        // 消耗一次充能
        BlockState mutableState = state;
        discharge(world, pos, mutableState);

        // 设置玩家的重生点
        player.setSpawnPoint(world.dimension(), pos, false);

        // 播放设置重生点音效
        world.playSound(ResourceLocation("minecraft:block.respawn_anchor.set_spawn"),
            sound::SoundCategory::Blocks,
            pos.center(),
            1.0f,
            1.0f);

        return ActionResultType::Success;
    }

    return ActionResultType::Pass;
}

} // namespace blocks
} // namespace mc
