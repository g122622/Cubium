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

#include "ItemFrameItem.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/hanging/HangingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/context/ItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include <memory>

namespace mc {
namespace item::items {

namespace {

/// mc::Direction 转为 HangingEntity::Direction（名称一一对应）
entity::HangingEntity::Direction _toHangingDirection(Direction dir)
{
    switch (dir) {
        case Direction::North:
            return entity::HangingEntity::Direction::NORTH;
        case Direction::West:
            return entity::HangingEntity::Direction::WEST;
        case Direction::East:
            return entity::HangingEntity::Direction::EAST;
        default:
            return entity::HangingEntity::Direction::SOUTH;
    }
}

} // namespace

ItemFrameItem::ItemFrameItem(bool glowing, ItemProperties properties)
    : Item(std::move(properties))
    , m_glowing(glowing)
{}

ActionResultType ItemFrameItem::onItemUse(ItemUseContext& context)
{
    const Direction direction = context.getFace();

    // 对齐 vanilla HangingEntityItem#useOn → mayPlace：不可贴在顶面/底面
    if (!Directions::isHorizontal(direction)) {
        return ActionResultType::Fail;
    }

    IWorld& world = context.getWorld();
    Player* player = context.getPlayer();

    // 放置位置为点击面外侧的方块
    const BlockPos framePos = context.blockPos().offset(direction);

    if (!world.isWithinWorldBounds(framePos)) {
        return ActionResultType::Fail;
    }

    if (!world.isClientSide()) {
        auto* registry = world.entityRegistry();
        if (registry == nullptr) {
            return ActionResultType::Fail;
        }

        auto frame = std::make_unique<entity::ItemFrameEntity>(framePos, _toHangingDirection(direction), *registry);
        frame->setWorld(&world);
        frame->setGlowing(m_glowing);

        // 对齐 vanilla：仅当悬挂位置有效（背后有可依附方块）时才放置
        if (!frame->canPlaceOn()) {
            return ActionResultType::Consume;
        }

        world.playSound(SoundEvents::ENTITY_ITEM_FRAME_PLACE,
            sound::SoundCategory::Blocks,
            Vector3(static_cast<f32>(framePos.x) + 0.5f,
                static_cast<f32>(framePos.y) + 0.5f,
                static_cast<f32>(framePos.z) + 0.5f),
            1.0f,
            1.0f);
        world.spawnEntity(std::move(frame));
    }

    // 消耗物品（创造模式由外层统一处理）
    if (player != nullptr && !player->isCreative()) {
        player->getHeldItem(context.getHand()).shrink(1);
    }

    return ActionResultType::Success;
}

} // namespace item::items
} // namespace mc
