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

#include "CopperGolemStatueBlock.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/special/HoneycombItem.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/interactive/CopperGolemStatueBlockEntity.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ============================================================================
// CopperGolemStatueBlock 实现
// ============================================================================

CopperGolemStatueBlock::CopperGolemStatueBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 铜傀儡雕像碰撞形状：圆柱形（直径10像素，高度14像素）
    // 对应 MC Java: Block.column(10.0, 0.0, 14.0) = box(3, 0, 3, 13, 14, 13)
    m_shape = CollisionShape::box(3.0f, 0.0f, 3.0f, 13.0f, 14.0f, 13.0f);

    // 创建状态容器：HORIZONTAL_FACING + COPPER_GOLEM_POSE + WATERLOGGED
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .add(BlockStateProperties::COPPER_GOLEM_POSE())
            .add(BlockStateProperties::WATERLOGGED())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 默认状态：朝北、站立姿态、不含水
    setDefaultState(defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
            .with(BlockStateProperties::COPPER_GOLEM_POSE(), BlockStateProperties::CopperGolemPose::Standing)
            .with(BlockStateProperties::WATERLOGGED(), false));
}

BlockState CopperGolemStatueBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 设置 FACING 为玩家水平朝向的反方向，WATERLOGGED 根据当前位置流体状态决定
    Direction facing = Directions::opposite(context.horizontalDirection());

    BlockState state = defaultState()
                           .with(BlockStateProperties::HORIZONTAL_FACING(), facing)
                           .with(BlockStateProperties::WATERLOGGED(), false);

    if (waterloggable::shouldWaterlogAt(context.getWorld(), context.placementPos())) {
        state = state.with(BlockStateProperties::WATERLOGGED(), true);
    }

    return state;
}

const BlockState& CopperGolemStatueBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction rotated = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
}

const BlockState& CopperGolemStatueBlock::mirror(const BlockState& state, Mirror mirror) const
{
    if (mirror == Mirror::None) {
        return state;
    }

    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    return rotate(state, rotation);
}

BlockState CopperGolemStatueBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    return state;
}

const CollisionShape& CopperGolemStatueBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

BlockActionResult CopperGolemStatueBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(hit);

    // 检查玩家手持物品是否为斧头
    ItemStack& heldItem = player.getHeldItem(hand);
    if (!heldItem.isEmpty()) {
        const Item* item = heldItem.getItem();
        if (item != nullptr && item->isIn(item::tag::ItemTags::AXES())) {
            // 斧头交互分支
            // 对应 MC 1.21.11 WeatheringCopperGolemStatueBlock.useItemOn 中的斧头逻辑
            //
            // MC 架构：基础 copper_golem_statue 是 WeatheringCopperGolemStatueBlock(UNAFFECTED)，
            //          useItemOn 中检查 getAge() == UNAFFECTED 后调用 removeStatue 生成铜傀儡。
            //
            // 本项目架构：基础 copper_golem_statue 是 CopperGolemStatueBlock（不实现 IOxidizableBlock），
            //             涂蜡变体也使用 CopperGolemStatueBlock。
            //             因此在此处需要区分：
            //             1. 基础 copper_golem_statue（未涂蜡）：斧头敲击 → 生成铜傀儡
            //             2. 涂蜡变体（waxed_*）：返回 Pass，让 AxeItem 处理 wax_off
            //
            // 检查方式：HoneycombItem::getWaxedOff(state) 对涂蜡变体返回非空，对基础变体返回空。

            const bool isWaxed = item::items::HoneycombItem::getWaxedOff(state).has_value();
            if (isWaxed) {
                // 涂蜡变体：返回 Pass，让 AxeItem 处理 wax_off
                return BlockActionResult::pass();
            }

            // 基础 copper_golem_statue（未涂蜡、Unaffected 等级）
            // 对应 MC: this.getAge().equals(WeatherState.UNAFFECTED)
            // 获取方块实体并调用 removeStatue 生成铜傀儡
            BlockEntity* be = world.getBlockEntity(pos);
            if (be == nullptr) {
                return BlockActionResult::pass();
            }

            auto* statueBe = dynamic_cast<blockentity::CopperGolemStatueBlockEntity*>(be);
            if (statueBe == nullptr) {
                return BlockActionResult::pass();
            }

            // 生成铜傀儡（对应 MC: coppergolemstatueblockentity.removeStatue(state)）
            std::unique_ptr<Entity> golem = statueBe->removeStatue(state);

            // 损坏斧头（对应 MC: p_433666_.hurtAndBreak(1, p_434811_, p_434251_.asEquipmentSlot())）
            LivingEntity::hurtAndBreak(heldItem, 1, &player, LivingEntity::handToEquipmentSlot(hand));

            if (golem != nullptr) {
                // 将铜傀儡加入世界（对应 MC: p_435157_.addFreshEntity(coppergolem)）
                world.spawnEntity(std::move(golem));

                // 移除方块（对应 MC: p_435157_.removeBlock(p_435733_, false)）
                const BlockState& airState = VanillaBlocks::AIR->defaultState();
                world.setBlockState(pos, &airState, 3);

                // 返回 Success 并携带损坏后的斧头，同步到客户端物品栏
                return BlockActionResult::success(heldItem);
            }

            // 生成失败：仍消耗斧头耐久
            return BlockActionResult::consume(heldItem);
        }
    }

    // 非斧头：循环切换姿态
    updatePose(world, state, pos, player);
    return ActionResultType::Success;
}

i32 CopperGolemStatueBlock::getComparatorInputOverride(
    const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // 返回 POSE.ordinal() + 1 (范围 1-4)
    return static_cast<i32>(state.get(BlockStateProperties::COPPER_GOLEM_POSE())) + 1;
}

std::unique_ptr<BlockEntity> CopperGolemStatueBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::CopperGolemStatueBlockEntity>(pos);
}

const fluid::FluidState* CopperGolemStatueBlock::getFluidState(const BlockState& state) const
{
    return waterloggable::getWaterFluidState(state);
}

void CopperGolemStatueBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    // 状态已在构造函数中通过 Builder 创建
    MC_UNUSED(container);
}

BlockStateProperties::CopperGolemPose CopperGolemStatueBlock::getNextPose(
    BlockStateProperties::CopperGolemPose current) noexcept
{
    // Standing -> Sitting -> Running -> Star -> Standing (循环)
    // 对应 MC Java: Pose.getNextPose() = BY_ID.apply(this.ordinal() + 1)
    // BY_ID 使用 OutOfBoundsStrategy.ZERO，超出范围时回到 0（Standing）
    using Pose = BlockStateProperties::CopperGolemPose;
    switch (current) {
        case Pose::Standing:
            return Pose::Sitting;
        case Pose::Sitting:
            return Pose::Running;
        case Pose::Running:
            return Pose::Star;
        case Pose::Star:
            return Pose::Standing;
        default:
            return Pose::Standing;
    }
}

void CopperGolemStatueBlock::updatePose(
    IWorld& world, const BlockState& state, const BlockPos& pos, Player& player) const
{
    MC_UNUSED(player);

    // 播放铜傀儡变雕像音效
    // 对应 MC Java: playSound(null, pos, SoundEvents.COPPER_GOLEM_BECOME_STATUE, SoundSource.BLOCKS)
    Vector3 soundPos(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y) + 0.5f, static_cast<f32>(pos.z) + 0.5f);
    world.playSound(SoundEvents::BLOCK_COPPER_GOLEM_BECOME_STATUE, sound::SoundCategory::Blocks, soundPos, 1.0f, 1.0f);

    // 循环切换姿态并更新方块状态
    auto currentPose = state.get(BlockStateProperties::COPPER_GOLEM_POSE());
    auto nextPose = getNextPose(currentPose);
    const BlockState& newState = state.with(BlockStateProperties::COPPER_GOLEM_POSE(), nextPose);
    world.setBlockState(pos, &newState, 3);

    // 触发 BLOCK_CHANGE 游戏事件
    // 对应 MC Java: gameEvent(player, GameEvent.BLOCK_CHANGE, pos)
    world.gameEvent(gameevent::GameEvents::BLOCK_CHANGE, pos, &state);
}

// ============================================================================
// WeatheringCopperGolemStatueBlock 实现
// ============================================================================

WeatheringCopperGolemStatueBlock::WeatheringCopperGolemStatueBlock(
    const BlockProperties& properties, BlockStateProperties::OxidationLevel oxidationLevel)
    : CopperGolemStatueBlock(properties)
    , m_oxidationLevel(oxidationLevel)
{
    // 重新创建状态容器：在父类基础上额外添加 OXIDATION 属性
    // 注意：父类构造函数已经创建过一次状态容器，这里覆盖它
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .add(BlockStateProperties::COPPER_GOLEM_POSE())
            .add(BlockStateProperties::WATERLOGGED())
            .add(BlockStateProperties::OXIDATION())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 默认状态：朝北、站立姿态、不含水、当前氧化等级
    setDefaultState(defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
            .with(BlockStateProperties::COPPER_GOLEM_POSE(), BlockStateProperties::CopperGolemPose::Standing)
            .with(BlockStateProperties::WATERLOGGED(), false)
            .with(BlockStateProperties::OXIDATION(), oxidationLevel));
}

void WeatheringCopperGolemStatueBlock::randomTick(
    IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 调用 IOxidizableBlock::tryOxidize 尝试氧化到下一等级
    // tryOxidize 内部会使用 withPropertiesOf() 保留 HORIZONTAL_FACING/COPPER_GOLEM_POSE/WATERLOGGED 等共有属性
    // 对应 MC Java: WeatheringCopperGolemStatueBlock.randomTick -> changeOverTime
    // 返回值表示是否成功氧化到下一等级（此处仅需触发副作用，无需使用返回值）
    (void)tryOxidize(world, pos, state, random);
}

void WeatheringCopperGolemStatueBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    // 状态已在构造函数中通过 Builder 创建
    MC_UNUSED(container);
}

} // namespace blocks
} // namespace mc
