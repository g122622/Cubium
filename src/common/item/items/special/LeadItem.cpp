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
 * copies of substantial portions of the Software.
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

#include "LeadItem.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/hanging/HangingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/context/ItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include <utility>

namespace mc::item::items {

LeadItem::LeadItem(ItemProperties properties)
    : Item(std::move(properties))
{}

ActionResultType LeadItem::onItemUse(ItemUseContext& context)
{
    IWorld& world = context.world();
    const BlockPos& pos = context.blockPos();

    // 只对栅栏方块生效
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr || !BlockTags::FENCES().contains(*state)) {
        return ActionResultType::Pass;
    }

    Player* player = context.player();
    if (player == nullptr) {
        return ActionResultType::Pass;
    }

    // 客户端直接返回成功
    if (world.isClientSide()) {
        return ActionResultType::Success;
    }

    // 搜索以栅栏中心为圆心、半径16格内所有被当前玩家拴住的生物
    constexpr f32 SEARCH_RADIUS = 16.0f;
    Vector3 centerPos(pos.x + 0.5f, pos.y + 0.5f, pos.z + 0.5f);

    // 获取搜索范围内的实体
    auto entities = world.getEntitiesInRange(centerPos, SEARCH_RADIUS);

    // 获取或创建栅栏位置上的拴绳结
    entity::LeashKnotEntity* knotEntity = nullptr;
    bool anyLeashed = false;

    for (Entity* ent : entities) {
        auto* mob = dynamic_cast<MobEntity*>(ent);
        if (mob == nullptr || !mob->isAlive()) {
            continue;
        }

        // 检查是否被当前玩家拴住
        if (!mob->isLeashed()) {
            continue;
        }

        const auto& holderUuid = mob->leashHolderUuid();
        if (!holderUuid.has_value() || *holderUuid != player->uuid()) {
            continue;
        }

        // 检查距离是否在拴绳范围内（拴绳最大距离为12格）
        constexpr f64 MAX_LEASH_DISTANCE = 12.0;
        Vector3d mobPos(mob->x(), mob->y(), mob->z());
        f64 distance = mobPos.distance(Vector3d(centerPos.x, centerPos.y, centerPos.z));
        if (distance > MAX_LEASH_DISTANCE) {
            continue;
        }

        // 检查是否可以被拴住
        if (!mob->canBeLeashed()) {
            continue;
        }

        // 延迟创建拴绳结（只创建一次）
        if (knotEntity == nullptr) {
            knotEntity = entity::LeashKnotEntity::getOrCreateKnot(world, pos);
            if (knotEntity == nullptr) {
                continue;
            }
        }

        // 将生物从拴在玩家身上改为拴在栅栏结上
        mob->setLeashedToFence(pos);
        knotEntity->attachLeash(mob);
        mob->enablePersistence();

        anyLeashed = true;
    }

    if (anyLeashed) {
        // 播放拴绳绑到栅栏的音效
        Vector3 soundPos(pos.x + 0.5f, pos.y + 0.5f, pos.z + 0.5f);
        world.playSound(SoundEvents::ENTITY_LEASH_KNOT_PLACE, sound::SoundCategory::Neutral, soundPos, 1.0f, 1.0f);
        world.gameEvent(gameevent::GameEvents::BLOCK_ATTACH, pos, state);
        return ActionResultType::Success;
    }

    return ActionResultType::Pass;
}

} // namespace mc::item::items
