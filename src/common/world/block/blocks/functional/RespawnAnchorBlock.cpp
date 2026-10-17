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
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/vehicle/DismountHelper.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockUpdateFlags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/dimension/DimensionType.hpp"
#include "common/world/explosion/ExplosionMode.hpp"
#include "common/world/spawn/RespawnConfig.hpp"
#include <array>
#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>
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
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
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

i32 RespawnAnchorBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
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
    i32 charges = getCharges(state);
    return static_cast<u8>(std::floor(charges * 3.75f));
}

BlockState RespawnAnchorBlock::charge(IWorld& world, const BlockPos& pos, BlockState& state)
{
    i32 charges = getCharges(state);
    if (charges < 4) {
        BlockState newState = state.with(BlockStateProperties::CHARGES_0_4(), charges + 1);
        world.setBlockState(pos, &newState, world::BlockUpdateFlags::UPDATE_ALL);
        // 注意：充能音效和粒子效果由调用方处理（useItemOn 中播放）
        return newState;
    }
    return state;
}

void RespawnAnchorBlock::discharge(IWorld& world, const BlockPos& pos, BlockState& state)
{
    i32 charges = getCharges(state);
    if (charges > 0) {
        BlockState newState = state.with(BlockStateProperties::CHARGES_0_4(), charges - 1);
        world.setBlockState(pos, &newState, world::BlockUpdateFlags::UPDATE_ALL);
    }
}

bool RespawnAnchorBlock::canSetSpawn(const IWorld& world)
{
    return DimensionType::fromId(world.dimension()).respawnAnchorWorks();
}

bool RespawnAnchorBlock::isRespawnFuel(const ItemStack& stack)
{
    if (stack.isEmpty()) {
        return false;
    }
    const Item* item = stack.getItem();
    if (item == nullptr) {
        return false;
    }
    return BlockItemRegistry::instance().getBlock(item->itemId()) == VanillaBlocks::GLOWSTONE;
}

std::optional<Vector3> RespawnAnchorBlock::findStandUpPosition(
    const IWorld& world, const BlockPos& anchorPos, f32 width, f32 height, bool isPlayer, bool fireImmune)
{
    // 水平环的 8 个相邻位置
    static constexpr std::array<std::pair<i32, i32>, 8> HORIZONTAL_OFFSETS = {
        {{0, -1}, {-1, 0}, {0, 1}, {1, 0}, {-1, -1}, {1, -1}, {-1, 1}, {1, 1}}};

    std::array<BlockPos, 25> candidates{};
    size_t count = 0;
    for (const auto& [dx, dz] : HORIZONTAL_OFFSETS) {
        candidates[count++] = BlockPos(anchorPos.x + dx, anchorPos.y, anchorPos.z + dz);
    }
    for (const auto& [dx, dz] : HORIZONTAL_OFFSETS) {
        candidates[count++] = BlockPos(anchorPos.x + dx, anchorPos.y - 1, anchorPos.z + dz);
    }
    for (const auto& [dx, dz] : HORIZONTAL_OFFSETS) {
        candidates[count++] = BlockPos(anchorPos.x + dx, anchorPos.y + 1, anchorPos.z + dz);
    }
    candidates[count++] = anchorPos.up();
    MC_ASSERT_RELEASE(count == candidates.size());

    const auto search = [&](bool safeMode) -> std::optional<Vector3> {
        for (size_t i = 0; i < count; ++i) {
            auto position = DismountHelper::findSafeDismountLocation(
                world, candidates[i], width, height, isPlayer, fireImmune, safeMode);
            if (position.has_value()) {
                return position;
            }
        }
        return std::nullopt;
    };

    if (auto result = search(true); result.has_value()) {
        return result;
    }
    return search(false);
}

BlockActionResult RespawnAnchorBlock::useItemOn(ItemStack& stack,
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(hit);

    if (isRespawnFuel(stack) && canBeCharged(state)) {
        charge(world, pos, const_cast<BlockState&>(state));
        world.playSound(ResourceLocation("minecraft:block.respawn_anchor.charge"),
            sound::SoundCategory::Blocks,
            pos.center(),
            1.0f,
            1.0f);
        stack.shrink(1);
        return ActionResultType::Success;
    }

    // 主手持非燃料物品、但副手是燃料且仍可充能时，交给交互流程尝试副手。
    if (hand == Hand::MainHand && isRespawnFuel(player.getHeldItem(Hand::OffHand)) && canBeCharged(state)) {
        return ActionResultType::Pass;
    }

    // 其余情况转入空手交互（设置重生点或爆炸）。
    return ActionResultType::TryEmptyHandInteraction;
}

BlockActionResult RespawnAnchorBlock::useWithoutItem(
    const BlockState& state, IWorld& world, const BlockPos& pos, Player& player, const BlockRaycastResult& hit)
{
    MC_UNUSED(hit);

    if (getCharges(state) == MIN_CHARGES) {
        return ActionResultType::Pass;
    }

    if (!canSetSpawn(world)) {
        // 维度不允许：移除重生锚并产生不生成火焰的 5 级爆炸。
        world.setBlockState(pos, nullptr, world::BlockUpdateFlags::UPDATE_ALL_IMMEDIATE);
        world.createExplosion(pos.center(), 5.0f, world::explosion::ExplosionMode::Destroy, false);
        return ActionResultType::SuccessServer;
    }

    const RespawnConfig candidate(RespawnData::of(world.dimension(), pos, 0.0f, 0.0f), false);
    const auto existing = player.getRespawnConfig();
    if (!existing.has_value() || !candidate.isSamePosition(existing)) {
        player.setRespawnPosition(candidate, true);
        world.playSound(ResourceLocation("minecraft:block.respawn_anchor.set_spawn"),
            sound::SoundCategory::Blocks,
            pos.center(),
            1.0f,
            1.0f);
        return ActionResultType::SuccessServer;
    }

    return ActionResultType::Consume;
}

} // namespace blocks
} // namespace mc
