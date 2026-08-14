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

#include "common/entity/serialization/components/ComponentSerializerRegistry.hpp"

namespace mc::entity::serialization::components {

/**
 * @brief LivingEntity 层 5 字段组件序列化器
 *
 * 把 LivingEntity::addAdditionalSaveData/readAdditionalSaveData 中 5 个已组件化字段的
 * NBT 读写逻辑搬到按组件注册的自由函数序列化器。3 个序列化器对，按承载组件注册：
 *
 * | 组件 | 字段 | 读写路径 |
 * |---|---|---|
 * | HealthComponent | Health | setHealth（clamp(0,maxHealth)，maxHealth 构造期就位）+ 置 m_healthSynced=true
 * 避免首帧覆盖 | | HurtStateComponent | Absorption + HurtTime + DeathTime | Absorption 走
 * setAbsorptionAmount（virtual，Player override 下发镜像）；HurtTime/DeathTime 无 setter 直写组件 | |
 * EquipmentComponent | Equipment | getEquipment/setEquipment（virtual，Player 派发到 PlayerInventory），含旧格式
 * HandItems/ArmorItems 回退 |
 *
 * dynamic_cast 早退：序列化器经 Entity& 调用，内部 dynamic_cast<LivingEntity*>。非 LivingEntity
 * 实体（ItemEntity/ItemFrame 等）返回 nullptr 早退，无副作用。LivingEntity 非 final、Entity 虚析构，
 * RTTI 可用。
 *
 * Health load 顺序依赖：setHealth 内 clamp(0, maxHealth) 读 AttributeMap，但 AttributeMap 在
 * 构造期 registerAttributes 已就位（派生类 MAX_HEALTH 默认值），非 NBT load 顺序依赖。本批不迁
 * Attributes（仍留 readAdditionalSaveData 虚函数内），维持现状。loadAll 在 readAdditionalSaveData
 * 之前调，Health load 时 Attributes NBT 尚未读入，与原顺序一致。
 *
 * 批次6 子目标1 Step4。
 */

/** 注册 LivingEntity 层全部组件序列化器到注册表（在 ComponentSerializerRegistry::registerAll 内调用） */
void registerLivingEntityComponentSerializers(ComponentSerializerRegistry& registry);

} // namespace mc::entity::serialization::components
