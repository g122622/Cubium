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

#include "SpawnerBlock.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/special/SpawnEggItem.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/IBlockAnimateContext.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/spawner/MobSpawnerBlockEntity.hpp"
#include <memory>

namespace mc {
namespace blocks {

SpawnerBlock::SpawnerBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 刷怪笼没有特殊状态
}

BlockActionResult SpawnerBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(state);
    MC_UNUSED(hit);

    // 客户端直接预测成功
    if (world.isClientSide()) {
        return ActionResultType::Success;
    }

    // 检查玩家手中是否持有刷怪蛋
    ItemStack& heldItem = player.getHeldItem(hand);
    if (heldItem.isEmpty() || heldItem.getItem() == nullptr) {
        return ActionResultType::Pass;
    }

    const auto* spawnEgg = dynamic_cast<const item::SpawnEggItem*>(heldItem.getItem());
    if (spawnEgg == nullptr) {
        return ActionResultType::Pass;
    }

    // 获取刷怪笼方块实体
    BlockEntity* entity = world.getBlockEntity(pos);
    if (entity == nullptr || entity->getType() != BlockEntityType::MobSpawner) {
        return ActionResultType::Pass;
    }

    auto* spawner = static_cast<blockentity::MobSpawnerBlockEntity*>(entity);

    // 设置刷怪笼的实体类型
    const entity::EntityType& entityType = spawnEgg->getEntityType();
    ResourceLocation entityId(entityType.name());
    math::Random& rng = world.getRandom();
    spawner->setEntityId(entityId, rng);

    // 非创造模式下消耗刷怪蛋
    if (!player.isCreative()) {
        heldItem.shrink(1);
    }

    return ActionResultType::Consume;
}

std::unique_ptr<BlockEntity> SpawnerBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::MobSpawnerBlockEntity>(pos);
}

void SpawnerBlock::animateTick(
    IBlockAnimateContext& context, const BlockPos& pos, const BlockState& state, math::IRandom& random) const
{
    MC_UNUSED(state);

    // 在刷怪笼方块内随机位置生成烟雾和火焰粒子
    f32 x = static_cast<f32>(pos.x) + random.nextFloat();
    f32 y = static_cast<f32>(pos.y) + random.nextFloat();
    f32 z = static_cast<f32>(pos.z) + random.nextFloat();

    context.addAnimateParticle(particle::ParticleTypeId::Smoke, Vector3(x, y, z), Vector3(0.0f, 0.0f, 0.0f));
    context.addAnimateParticle(particle::ParticleTypeId::Flame, Vector3(x, y, z), Vector3(0.0f, 0.0f, 0.0f));
}

} // namespace blocks
} // namespace mc
