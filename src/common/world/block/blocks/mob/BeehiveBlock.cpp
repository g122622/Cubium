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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN THE EVENT OF LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "BeehiveBlock.hpp"

#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/blocks/decorative/CampfireBlock.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/BeehiveBlockEntity.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ============================================================================
// 构造函数
// ============================================================================

BeehiveBlock::BeehiveBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 创建状态容器
    // HONEY_LEVEL 范围 0-5，表示蜂巢中的蜂蜜量
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .add(BlockStateProperties::HONEY_LEVEL_0_5())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
            .with(BlockStateProperties::HONEY_LEVEL_0_5(), 0));
}

// ============================================================================
// 状态属性
// ============================================================================

i32 BeehiveBlock::getHoneyLevel(const BlockState& state) const
{
    return state.get(BlockStateProperties::HONEY_LEVEL_0_5());
}

BlockState BeehiveBlock::withHoneyLevel(const BlockState& state, i32 level) const
{
    return state.with(BlockStateProperties::HONEY_LEVEL_0_5(), std::clamp(level, 0, 5));
}

// ============================================================================
// 放置逻辑
// ============================================================================

BlockState BeehiveBlock::getStateForPlacement(BlockItemUseContext& context)
{
    Direction facing = context.horizontalDirection();
    return defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), facing);
}

// ============================================================================
// 旋转/镜像
// ============================================================================

const BlockState& BeehiveBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction newFacing = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), newFacing);
}

const BlockState& BeehiveBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction newFacing = facing;

    switch (mirror) {
        case Mirror::LeftRight:
            if (facing == Direction::East) {
                newFacing = Direction::West;
            } else if (facing == Direction::West) {
                newFacing = Direction::East;
            }
            break;
        case Mirror::FrontBack:
            if (facing == Direction::North) {
                newFacing = Direction::South;
            } else if (facing == Direction::South) {
                newFacing = Direction::North;
            }
            break;
        case Mirror::None:
        default:
            break;
    }

    return state.with(BlockStateProperties::HORIZONTAL_FACING(), newFacing);
}

// ============================================================================
// 交互
// ============================================================================

BlockActionResult BeehiveBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    i32 honeyLevel = getHoneyLevel(state);
    if (honeyLevel < 5) {
        return ActionResultType::Pass;
    }

    ItemStack& heldItem = player.getHeldItem(hand);
    bool success = false;

    // 剪刀交互：掉落3个蜜脾
    if (heldItem.getItem() == Items::SHEARS) {
        dropHoneycomb(world, pos);

        // 消耗剪刀耐久
        heldItem.attemptDamageItem(1, &player);

        world.playSound(SoundEvents::BLOCK_BEEHIVE_SHEAR,
            sound::SoundCategory::Blocks,
            Vector3(static_cast<f32>(pos.x + 0.5), static_cast<f32>(pos.y + 0.5), static_cast<f32>(pos.z + 0.5)),
            1.0f,
            1.0f);

        success = true;
    }
    // 玻璃瓶交互：消耗1个玻璃瓶，获得1个蜂蜜瓶
    else if (heldItem.getItem() == Items::GLASS_BOTTLE) {
        // 消耗1个玻璃瓶（参考CauldronBlock的模式）
        if (!player.abilities().creativeMode) {
            heldItem.shrink(1);
        }

        // 给予蜂蜜瓶
        ItemStack honeyBottle(Items::HONEY_BOTTLE, 1);
        if (heldItem.isEmpty()) {
            // 手持物品已消耗完，直接替换
            heldItem = honeyBottle;
            player.inventory().setChanged();
        } else {
            // 手持物品还有剩余，添加到背包
            i32 remaining = player.inventory().add(honeyBottle);
            if (remaining > 0) {
                // 背包满了，掉落在地上
                ItemDropHelper::spawnItemAtEntity(&player, honeyBottle, 0.5f, world.getRandom());
            }
        }

        world.playSound(SoundEvents::ITEM_BOTTLE_FILL,
            sound::SoundCategory::Blocks,
            Vector3(static_cast<f32>(pos.x + 0.5), static_cast<f32>(pos.y + 0.5), static_cast<f32>(pos.z + 0.5)),
            1.0f,
            1.0f);

        success = true;
    }

    if (success) {
        // 检查下方是否有营火烟熏
        if (!CampfireBlock::isSmokeyPos(world, pos)) {
            // 无营火烟熏 -> 激怒蜜蜂并紧急释放
            if (blockentity::BeehiveBlockEntity::hiveContainsBees(world, pos)) {
                angerNearbyBees(world, pos, player);
            }
            releaseBeesAndResetHoneyLevel(world, state, pos, &player, blockentity::BeeReleaseStatus::Emergency);
        } else {
            // 有营火烟熏 -> 仅重置蜂蜜等级，不激怒蜜蜂
            resetHoneyLevel(world, state, pos);
        }
        return ActionResultType::Success;
    }

    return ActionResultType::Pass;
}

// ============================================================================
// 方块实体
// ============================================================================

std::unique_ptr<BlockEntity> BeehiveBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::BeehiveBlockEntity>(pos);
}

// ============================================================================
// 蜂巢逻辑
// ============================================================================

void BeehiveBlock::resetHoneyLevel(IWorld& world, const BlockState& state, const BlockPos& pos)
{
    BlockState newState = withHoneyLevel(state, 0);
    world.setBlockState(pos, &newState);
}

void BeehiveBlock::releaseBeesAndResetHoneyLevel(IWorld& world,
    const BlockState& state,
    const BlockPos& pos,
    Player* player,
    blockentity::BeeReleaseStatus releaseStatus)
{
    resetHoneyLevel(world, state, pos);

    auto* blockEntity = world.getBlockEntity(pos);
    if (blockEntity && blockEntity->getType() == BlockEntityType::Beehive) {
        auto* beehive = static_cast<blockentity::BeehiveBlockEntity*>(blockEntity);
        beehive->emptyAllLivingFromHive(world, player, state, releaseStatus);
    }
}

void BeehiveBlock::dropHoneycomb(IWorld& world, const BlockPos& pos)
{
    // 掉落3个蜜脾物品
    ItemStack honeycombStack(Items::HONEYCOMB, 3);
    ItemDropHelper::spawnItemEntities(&world, pos, {honeycombStack}, world.getRandom());
}

void BeehiveBlock::angerNearbyBees(IWorld& world, const BlockPos& pos, Player& player)
{
    blockentity::BeehiveBlockEntity::angerNearbyBees(world, pos, player);
}

// ============================================================================
// 红石
// ============================================================================

i32 BeehiveBlock::getAnalogOutputSignal(const BlockState& state) const
{
    return getHoneyLevel(state);
}

} // namespace blocks
} // namespace mc
