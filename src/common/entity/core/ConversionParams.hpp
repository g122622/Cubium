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

#include "common/entity/damage/DamageSource.hpp"
#include "common/util/nbt/Nbt.hpp"

#include <memory>
#include <optional>

namespace mc {

/**
 * @brief 实体转化参数（对齐 vanilla ConversionParams）
 *
 * 描述一次 "实体 A 转化为实体 B" 的过程对原实体（及新实体）的影响：
 * - preserveEquipment / preserveCanPickUpLoot / preserveAttributes：新实体是否继承原实体的
 *   装备、可拾取物品标志与属性；
 * - copyNbt：转化时复制到新实体上的额外 NBT（对齐 vanilla 的 copyCustomFormat 数据）；
 * - source/playerDamager：若由伤害触发转化，记录伤害来源，供新实体的 onConversion 处理
 *   （如村民被僵尸攻击后转化）。
 *
 * 转化后的新实体由 Entity::convertTo 创建并送入世界，原实体会被静默移除（Discarded），
 * 不触发死亡掉落与经验。
 */
class Entity;

/**
 * @brief 实体转化参数（对齐 vanilla ConversionParams）
 *
 * 描述一次 "实体 A 转化为实体 B" 的过程对原实体（及新实体）的影响：
 * - preserveEquipment / preserveCanPickUpLoot：新实体是否继承原实体的装备与可拾取物品标志；
 * - copyNbt：转化时复制到新实体上的额外 NBT（对齐 vanilla 的 copyCustomFormat 数据）；
 * - source/playerDamager：若由伤害触发转化，记录伤害来源，供新实体的 onConversion 处理
 *   （如村民被僵尸攻击后转化）。
 *
 * 转化后的新实体由 Entity::convertTo 创建并送入世界，原实体会被静默移除（Discarded），
 * 不触发死亡掉落与经验。
 */
struct ConversionParams {
    /// 新实体继承原实体装备（对齐 preserveEquipment）
    bool preserveEquipment = false;
    /// 新实体继承原实体"可拾取物品"标志（对齐 preserveCanPickUpLoot）
    bool preserveCanPickUpLoot = false;
    /// 新实体继承原实体属性值（对齐 preserveAttributes）
    bool preserveAttributes = false;
    /// 复制到新实体上的额外 NBT（对齐 copyCustomFormat）
    std::optional<nbt::tags::compound_tag> copyNbt;
    /// 触发本次转化的伤害来源（无则视为非伤害触发）
    const DamageSource* source = nullptr;
    /// 触发本次转化的伤害者（玩家/生物），可能为空
    LivingEntity* playerDamager = nullptr;

    /**
     * @brief 单实体的简单转化参数
     *
     * 对齐 vanilla ConversionParams.single：仅指定装备/可拾取标志继承与是否复制 NBT。
     *
     * @param from 被转化的原实体（仅用于维持调用点语义，转化逻辑自行处理原实体）
     * @param preserveEquipment 是否继承装备
     * @param preserveCanPickUpLoot 是否继承可拾取物品标志
     * @param copyNbt 是否复制原实体的自定义 NBT（custom data）
     * @return 构造好的转化参数
     */
    [[nodiscard]] static ConversionParams single(
        const Entity& from, bool preserveEquipment, bool preserveCanPickUpLoot, bool copyNbt = false);
};

} // namespace mc
