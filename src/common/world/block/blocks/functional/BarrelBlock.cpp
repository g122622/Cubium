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

#include "BarrelBlock.hpp"

#include "common/core/Types.hpp"
#include "common/entity/ai/util/PiglinAi.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/stats/Stats.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/storage/BarrelEntity.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ========== BarrelBlock 实现 ==========

BarrelBlock::BarrelBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::FACING())
            .add(BlockStateProperties::OPEN())
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
            .with(BlockStateProperties::FACING(), Direction::North)
            .with(BlockStateProperties::OPEN(), false));

    // 木桶形状是完整方块
    m_shape = CollisionShape::fullBlock();
}

BlockState BarrelBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 木桶朝向玩家看向的方向的反方向
    Direction facing = context.getClickedFace();
    return defaultState().with(BlockStateProperties::FACING(), facing);
}

const BlockState& BarrelBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::FACING());
    Direction rotated = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::FACING(), rotated);
}

const BlockState& BarrelBlock::mirror(const BlockState& state, Mirror mirror) const
{
    if (mirror == Mirror::None) {
        return state;
    }

    Direction facing = state.get(BlockStateProperties::FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    return rotate(state, rotation);
}

const CollisionShape& BarrelBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

std::unique_ptr<BlockEntity> BarrelBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::BarrelEntity>(pos);
}

int BarrelBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{

    MC_UNUSED(state);

    // 从木桶方块实体获取比较器信号
    BlockEntity* entity = world.getBlockEntity(pos);
    if (entity != nullptr && entity->getType() == BlockEntityType::Barrel) {
        auto* barrel = static_cast<blockentity::BarrelEntity*>(entity);
        return barrel->getComparatorSignal(world);
    }

    return 0;
}

BlockActionResult BarrelBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(state);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    if (world.isClientSide()) {
        return ActionResultType::Success;
    }

    BlockEntity* entity = world.getBlockEntity(pos);
    if (entity != nullptr && entity->getType() == BlockEntityType::Barrel) {
        auto* barrel = static_cast<blockentity::BarrelEntity*>(entity);
        if (world.openContainer(ContainerType::Generic9x3, pos, player)) {
            barrel->openContainer(&player);
            player.awardCustomStat(ResourceLocation(stats::OPEN_BARREL), 1);
            // 打开木桶时激怒附近能看到玩家的猪灵
            entity::PiglinAi::angerNearbyPiglins(world, player, true);
            return ActionResultType::Consume;
        }
    }

    return ActionResultType::Pass;
}

void BarrelBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);

    // 方块移除时掉落木桶内的物品
    BlockEntity* entity = world.getBlockEntity(pos);
    if (entity != nullptr && entity->getType() == BlockEntityType::Barrel) {
        auto* barrel = static_cast<blockentity::BarrelEntity*>(entity);
        IInventory* inventory = barrel->getInventory();

        // 掉落所有物品
        math::Random rng;
        for (i32 i = 0; i < inventory->getContainerSize(); ++i) {
            ItemStack stack = inventory->removeItemNoUpdate(i);
            if (!stack.isEmpty()) {
                ItemDropHelper::spawnItemEntity(&world, stack, pos.x + 0.5, pos.y + 0.5, pos.z + 0.5, rng);
            }
        }
    }

    Block::onBlockRemoved(world, pos, state);
}

} // namespace blocks
} // namespace mc
