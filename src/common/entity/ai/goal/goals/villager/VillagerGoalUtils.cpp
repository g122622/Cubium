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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "VillagerGoalUtils.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"

#include <cmath>
#include <limits>
#include <memory>
#include <unordered_map>
#include <utility>

namespace mc {
namespace entity {
namespace ai {
namespace goal {
namespace villager {

f32 distanceToBlockCenter(const Entity* entity, const BlockPos& pos)
{
    if (!entity) return std::numeric_limits<f32>::max();
    return entity->distanceSqTo(pos.x + 0.5f, static_cast<f32>(pos.y), pos.z + 0.5f);
}

bool isWithinDistance(const Entity* entity, const BlockPos& pos, f32 maxDistance)
{
    f32 distSq = distanceToBlockCenter(entity, pos);
    return distSq < maxDistance * maxDistance;
}

bool throwHalfStackToTarget(VillagerEntity* villager,
    IInventory& inventory,
    const std::unordered_map<const Item*, i32>& itemFilter,
    LivingEntity* target)
{
    if (!villager || !target) return false;
    IWorld* world = villager->world();
    if (!world) return false;

    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        ItemStack stack = inventory.getItem(i);
        if (stack.isEmpty()) continue;

        const Item* item = stack.getItem();
        if (itemFilter.find(item) == itemFilter.end()) continue;

        // 计算要抛出的数量
        i32 count = stack.getCount();
        i32 maxStackSize = stack.getMaxStackSize();
        i32 throwCount = 0;

        if (count > maxStackSize / 2) {
            // 超过半组，抛出一半
            throwCount = count / 2;
        } else if (count > 24) {
            // 超过24个但不超过半组，保留24个，抛出剩余
            throwCount = count - 24;
        }

        if (throwCount > 0) {
            // 从源库存中移除物品
            inventory.removeItem(i, throwCount);

            // 创建新的物品堆并抛向目标
            ItemStack throwStack(item, throwCount);

            // 在村民眼睛高度略微偏下位置生成物品
            f64 spawnX = villager->x();
            f64 spawnY = villager->y() + villager->eyeHeight() - 0.3;
            f64 spawnZ = villager->z();

            // 计算朝向目标的方向向量
            f64 dx = target->x() - spawnX;
            f64 dy = target->y() + target->eyeHeight() * 0.5 - spawnY;
            f64 dz = target->z() - spawnZ;
            f64 dist = std::sqrt(dx * dx + dy * dy + dz * dz);

            // 抛出速度（与 MC 的 throwItem 一致，约 0.3-0.5）
            static constexpr f32 THROW_SPEED = 0.35f;

            f32 vx = 0.0f, vy = 0.0f, vz = 0.0f;
            if (dist > 0.001) {
                vx = static_cast<f32>(dx / dist) * THROW_SPEED;
                vy = static_cast<f32>(dy / dist) * THROW_SPEED + 0.1f; // 略微向上抛
                vz = static_cast<f32>(dz / dist) * THROW_SPEED;
            } else {
                vy = 0.1f; // 距离太近时直接向上抛
            }

            // 添加随机偏移（与 MC 的 spread(0.3, 0.3, 0.3) 对应）
            math::Random& rng = villager->getRandom();
            vx += (rng.nextFloat() - 0.5f) * 0.3f;
            vy += (rng.nextFloat() - 0.5f) * 0.3f;
            vz += (rng.nextFloat() - 0.5f) * 0.3f;

            // 生成物品实体
            auto itemEntity =
                std::make_unique<ItemEntity>(EntityInstanceId(0), throwStack, spawnX, spawnY, spawnZ, vx, vy, vz);

            // 直接构造的实体需要显式设置 typeId（注册表路径会自动设置）
            itemEntity->setTypeId(EntityTypeKeys::ITEM);

            // 设置拾取延迟（防止村民立即捡回自己扔出的物品）
            static constexpr i32 ITEM_THROW_PICKUP_DELAY = 40; // 2秒
            itemEntity->setPickupDelay(ITEM_THROW_PICKUP_DELAY);
            itemEntity->setOwner(villager->uuid());

            world->spawnEntity(std::move(itemEntity));
            return true;
        }
    }

    return false;
}

} // namespace villager
} // namespace goal
} // namespace ai
} // namespace entity
} // namespace mc
