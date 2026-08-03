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

#include "common/world/block/blocks/ShulkerBoxBlock.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/stats/Stats.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/color/DyeColor.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/storage/ShulkerBoxEntity.hpp"
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ========== 构造函数 ==========

ShulkerBoxBlock::ShulkerBoxBlock(const BlockProperties& properties)
    : ShulkerBoxBlock(std::nullopt, properties)
{}

ShulkerBoxBlock::ShulkerBoxBlock(DyeColor color, const BlockProperties& properties)
    : ShulkerBoxBlock(std::optional<DyeColor>(color), properties)
{}

ShulkerBoxBlock::ShulkerBoxBlock(std::optional<DyeColor> color, const BlockProperties& properties)
    : Block(properties)
    , m_color(color)
{
    // 潜影盒使用 FACING 属性（6 个方向）
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::FACING())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 默认朝向是 UP
    setDefaultState(defaultState().with(BlockStateProperties::FACING(), Direction::Up));
}

// ========== 放置 ==========

BlockState ShulkerBoxBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 潜影盒朝向放置面
    Direction facing = context.face();

    // 如果放置面是 DOWN，朝向保持 DOWN；否则使用放置面
    return defaultState().with(BlockStateProperties::FACING(), facing);
}

// ========== 方块实体 ==========

std::unique_ptr<BlockEntity> ShulkerBoxBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::ShulkerBoxEntity>(pos);
}

// ========== 交互 ==========

BlockActionResult ShulkerBoxBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // 客户端只返回成功
    if (world.isClientSide()) {
        return ActionResultType::Success;
    }

    // 旁观者模式返回 CONSUME
    if (player.isSpectator()) {
        return ActionResultType::Consume;
    }

    // 获取方块实体
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (!blockEntity || blockEntity->getType() != BlockEntityType::ShulkerBox) {
        return ActionResultType::Pass;
    }

    auto* shulkerBox = static_cast<blockentity::ShulkerBoxEntity*>(blockEntity);

    // 检查是否可以打开
    Direction facing = state.get(BlockStateProperties::FACING());
    if (!canOpen(world, pos, facing)) {
        // 播放锁定音效
        world.playSound(ResourceLocation("minecraft:block.shulker_box.locked"),
            sound::SoundCategory::Blocks,
            pos.center(),
            1.0f,
            1.0f);
        return ActionResultType::Success;
    }

    // 检查锁定状态和空间是否足够
    if (!shulkerBox->canOpen(world)) {
        return ActionResultType::Success;
    }

    // 打开潜影盒 GUI
    if (world.openContainer(ContainerType::ShulkerBox, pos, player)) {
        shulkerBox->openContainer(&player);
        player.awardCustomStat(ResourceLocation(stats::OPEN_SHULKER_BOX), 1);
        return ActionResultType::Consume;
    }

    return ActionResultType::Pass;
}

// ========== 红石 ==========

i32 ShulkerBoxBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    MC_UNUSED(state);

    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (!blockEntity || blockEntity->getType() != BlockEntityType::ShulkerBox) {
        return 0;
    }

    auto* shulkerBox = static_cast<blockentity::ShulkerBoxEntity*>(blockEntity);
    return shulkerBox->getComparatorSignal(world);
}

// ========== 移除处理 ==========

void ShulkerBoxBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 潜影盒被破坏时会保留物品，物品会通过 BlockItem 保留到 ItemStack 的 NBT 中
    // 这里不需要像普通容器那样掉落物品

    // 清理方块实体
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity != nullptr && blockEntity->getType() == BlockEntityType::ShulkerBox) {
        // 标记方块实体已移除
        blockEntity->remove();
    }

    // 调用基类处理
    Block::onBlockRemoved(world, pos, state);
}

// ========== 静态工具方法 ==========

bool ShulkerBoxBlock::canOpen(IWorld& world, const BlockPos& pos, Direction facing)
{
    // 检查潜影盒打开方向是否有碰撞空间
    AxisAlignedBB openBox = getOpenBoundingBox(pos, facing);
    return world.getBlockCollisions(openBox).empty();
}

AxisAlignedBB ShulkerBoxBlock::getOpenBoundingBox(const BlockPos& pos, Direction facing)
{
    // 潜影盒打开时会向朝向方向扩展
    f32 minX = static_cast<f32>(pos.x);
    f32 minY = static_cast<f32>(pos.y);
    f32 minZ = static_cast<f32>(pos.z);
    f32 maxX = static_cast<f32>(pos.x + 1);
    f32 maxY = static_cast<f32>(pos.y + 1);
    f32 maxZ = static_cast<f32>(pos.z + 1);

    // 获取相反方向的偏移
    Direction opposite = Directions::opposite(facing);

    // 先向朝向方向扩展 0.5，然后向反方向收缩 1.0
    f32 offsetX = static_cast<f32>(Directions::xOffset(facing)) * 0.5f;
    f32 offsetY = static_cast<f32>(Directions::yOffset(facing)) * 0.5f;
    f32 offsetZ = static_cast<f32>(Directions::zOffset(facing)) * 0.5f;

    // 扩展
    minX -= offsetX;
    minY -= offsetY;
    minZ -= offsetZ;
    maxX += offsetX;
    maxY += offsetY;
    maxZ += offsetZ;

    // 向反方向收缩（只保留扩展部分）
    f32 shrinkX = static_cast<f32>(Directions::xOffset(opposite));
    f32 shrinkY = static_cast<f32>(Directions::yOffset(opposite));
    f32 shrinkZ = static_cast<f32>(Directions::zOffset(opposite));

    if (shrinkX > 0) {
        maxX -= shrinkX;
    } else if (shrinkX < 0) {
        minX -= shrinkX;
    }

    if (shrinkY > 0) {
        maxY -= shrinkY;
    } else if (shrinkY < 0) {
        minY -= shrinkY;
    }

    if (shrinkZ > 0) {
        maxZ -= shrinkZ;
    } else if (shrinkZ < 0) {
        minZ -= shrinkZ;
    }

    return AxisAlignedBB(minX, minY, minZ, maxX, maxY, maxZ);
}

// ========== 静态类型检查 ==========

bool ShulkerBoxBlock::isShulkerBox(const Block& block)
{
    return dynamic_cast<const ShulkerBoxBlock*>(&block) != nullptr;
}

} // namespace blocks
} // namespace mc
