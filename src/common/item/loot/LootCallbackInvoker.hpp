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

#pragma once

#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/LootConsumer.hpp"
#include <functional>
#include <string>

namespace mc {

class Entity;
class IWorld;

namespace ecs {
class EntityRegistry;
}

namespace loot {

/**
 * @brief 取世界持有的实体注册表
 *
 * 战利品表掷骰由调用方发起，产出既可能落进容器、也可能直接变成掉落物；接收方
 * （实体注册表）由调用方按「这个世界的实体」取得。世界没有接入 ECS 时抛
 * `std::logic_error`——静默回退会让产出落到错误的地方。
 *
 * @param world 世界
 * @return 该世界的实体注册表
 */
[[nodiscard]] ecs::EntityRegistry& registryOf(IWorld& world);

/**
 * @brief 把指定附魔写到实体主手上
 *
 * 原版没有「掠夺等级」这类上下文键：`enchanted_count_increase` 与
 * `random_chance_with_enchanted_bonus` 都从 ATTACKING_ENTITY 的装备读附魔。
 * 主手原本为空时先放一把 `fallbackItemId` 武器，与真实击杀时的形态一致。
 *
 * @param attacker 攻击者
 * @param level 附魔等级；<= 0 时什么也不做
 * @param enchantmentId 附魔 ID
 * @param fallbackItemId 主手为空时用来承载附魔的物品 ID
 */
void equipLooting(Entity& attacker,
    i32 level,
    const std::string& enchantmentId = "minecraft:looting",
    const std::string& fallbackItemId = "minecraft:diamond_sword");

} // namespace loot
} // namespace mc
