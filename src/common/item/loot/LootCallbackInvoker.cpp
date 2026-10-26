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

#include "LootCallbackInvoker.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EquipmentSlot.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/ecs/components/EquipmentComponent.hpp"
#include "common/world/IWorld.hpp"
#include <stdexcept>

namespace mc {
namespace loot {

ecs::EntityRegistry& registryOf(IWorld& world)
{
    ecs::EntityRegistry* registry = world.entityRegistry();
    if (registry == nullptr) {
        throw std::logic_error("loot: 世界没有接入 ECS，无法取得战利品投递的接收实体");
    }
    return *registry;
}

void equipLooting(Entity& attacker, i32 level, const std::string& enchantmentId, const std::string& fallbackItemId)
{
    if (level <= 0) {
        return;
    }
    auto* living = dynamic_cast<LivingEntity*>(&attacker);
    if (living == nullptr) {
        return;
    }
    const auto* component = living->tryGetComponent<ecs::EquipmentComponent>();
    if (component == nullptr) {
        return;
    }

    // 副本上改完再写回：原版的 `setItemSlot` 同样触发 `ItemStack` 的复制语义。
    ItemStack weapon = component->m_equipment[static_cast<size_t>(EquipmentSlot::MainHand)];
    if (weapon.isEmpty()) {
        if (auto parsed = ItemStack::fromJson(nlohmann::json{{"id", fallbackItemId}, {"Count", 1}}); parsed.success()) {
            weapon = std::move(parsed.value());
        }
    }
    if (weapon.isEmpty()) {
        return;
    }
    weapon.addEnchantment(enchantmentId, level);
    living->setMainHandItem(weapon);
}

} // namespace loot
} // namespace mc
