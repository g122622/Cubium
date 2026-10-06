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

#include "EndCrystalItem.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/entities/effect/EffectEntities.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/context/ItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <memory>

namespace mc {
namespace item::items {

EndCrystalItem::EndCrystalItem(ItemProperties properties)
    : Item(std::move(properties))
{}

ActionResultType EndCrystalItem::onItemUse(ItemUseContext& context)
{
    IWorld& world = context.getWorld();
    Player* player = context.getPlayer();
    const BlockPos blockPos = context.blockPos();
    const BlockState* clickedState = world.getBlockState(blockPos);

    // 对齐 vanilla EndCrystalItem#useOn：仅在黑曜石或基岩上放置
    if (clickedState == nullptr || (!clickedState->is(VanillaBlocks::OBSIDIAN) &&
                                       !clickedState->is(VanillaBlocks::BEDROCK))) {
        return ActionResultType::Fail;
    }

    // 目标位置（点击位置上方）必须为空气。
    // 注：空区块段中 getBlockState 可能返回 nullptr（等价于空气），故 nullptr 视为可放置。
    const BlockPos above = BlockPos(blockPos.x, blockPos.y + 1, blockPos.z);
    const BlockState* aboveState = world.getBlockState(above);
    if (aboveState != nullptr && !aboveState->isAir()) {
        return ActionResultType::Fail;
    }

    // 目标位置不得有实体占据（对齐 vanilla 检查 [0,1]x[1,3]x[0,1] 范围内实体）
    const AxisAlignedBB checkBox(static_cast<f32>(above.x),
        static_cast<f32>(above.y),
        static_cast<f32>(above.z),
        static_cast<f32>(above.x) + 1.0f,
        static_cast<f32>(above.y) + 2.0f,
        static_cast<f32>(above.z) + 1.0f);
    if (!world.getEntitiesInAABB(checkBox, nullptr).empty()) {
        return ActionResultType::Fail;
    }

    if (!world.isClientSide()) {
        auto* registry = world.entityRegistry();
        if (registry == nullptr) {
            return ActionResultType::Fail;
        }
        const entity::EntityType* crystalType = entity::EntityRegistry::instance().getType("minecraft:end_crystal");
        if (crystalType == nullptr) {
            return ActionResultType::Fail;
        }

        auto crystal = crystalType->create(&world, *registry);
        if (crystal == nullptr) {
            return ActionResultType::Fail;
        }

        crystal->setPosition(
            static_cast<f32>(above.x) + 0.5f, static_cast<f32>(above.y), static_cast<f32>(above.z) + 0.5f);
        if (auto* endCrystal = dynamic_cast<entity::EnderCrystalEntity*>(crystal.get())) {
            endCrystal->setShowBottom(false);
        }
        world.spawnEntity(std::move(crystal));

        // TODO: 末影龙战斗系统接入后调用 EndDragonFight::tryRespawn()（对齐 vanilla
        //   EndCrystalItem#useOn 末尾的 ((ServerLevel)level).getDragonFight().tryRespawn()）。
    }

    // 消耗物品（创造模式由外层统一处理）
    if (player != nullptr && !player->isCreative()) {
        player->getHeldItem(context.getHand()).shrink(1);
    }

    return ActionResultType::Success;
}

} // namespace item::items
} // namespace mc
