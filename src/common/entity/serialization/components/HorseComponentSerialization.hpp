/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
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
 * @brief Horse 族组件序列化器
 *
 * 把 Horse 族基类（AbstractHorseEntity）的特有持久化字段 NBT 读写逻辑从 OOP 虚函数链
 * （addAdditionalSaveData/readAdditionalSaveData override）搬到按组件注册的自由函数
 * 序列化器。对齐 vanilla 1.21.11 AbstractHorse 的持久化字段清单。
 *
 * 序列化器按承载组件注册（注册表键 = entt::type_id<ComponentT>），序列化器内部用
 * tryGetComponent<ComponentT>() 早退（无组件的实体不参与该组件字段的存读盘）：
 *
 * | 组件 | 对齐 vanilla 字段 | NBT 键 | priority |
 * |---|---|---|---|
 * | HorseTamingComponent | Temper / OwnerUUIDMost / OwnerUUIDLeast | nbt_keys::TEMPER / HORSE_OWNER_UUID_MOST /
 * HORSE_OWNER_UUID_LEAST | 0 | | HorseJumpComponent | JumpStrength | nbt_keys::HORSE_JUMP_STRENGTH | 0 | |
 * HorseStatusComponent | Tame / Bred / Saddle / EatingHaystack | 字面量 "Tame"/"Bred"/"Saddle" +
 * nbt_keys::EATING_HAYSTACK | 10 | | HorseAttributeComponent | Speed / HorseHealth | nbt_keys::HORSE_SPEED /
 * HORSE_HEALTH | 20 |
 *
 * load 顺序（priority 升序）：
 * - HorseTamingComponent=0：ownerUuid 先 load 调 setOwnerUuid 触发 setTame(true) 联动写
 *   STATUS_PARAM（C 类同步字段镜像副作用硬约束，必须走 setter）。
 * - HorseJumpComponent=0：jumpStrength load 后同步 AttributeMap（HORSE_JUMP_STRENGTH）。
 * - HorseStatusComponent=10：tame/bred/saddle/eating 后 load。NBT 中 Tame 与 OwnerUUID 一致，
 *   ownerUuid 联动的 setTame 覆盖幂等。全部走 setter 触发 _syncStatusFlags 写 STATUS_PARAM。
 * - HorseAttributeComponent=20：speed/horseHealth 最后 load，同步 AttributeMap（MAX_HEALTH/
 *   MOVEMENT_SPEED）。
 *
 * 不注册序列化器的组件：HorseBoostComponent（运行时无持久化）、HorseInventoryComponent
 * （库存内容走 LootableContainer 体系，项目当前未接通 vehicle/马匹容器持久化 TODO）、
 * HorseAnimationComponent（运行时动画计数器/插值量不存盘）、ChestedHorseComponent
 * （m_hasChest 标志项目当前未存盘，1.16.5 残留缺陷 TODO）。
 *
 * 接通方式：本批完成后，AbstractHorseEntity 的 addAdditionalSaveData override 删除（搬注册表
 * 后双重写入键冲突），readAdditionalSaveData 改薄壳（仅基类调用 + initHorseChest，因
 * initHorseChest 需在 load 后按新 NBT 重置库存规模，loadAll 在 readAdditionalSaveData 之前
 * 调，薄壳中 initHorseChest 时组件已就位）。
 *
 * 批次8 子批1 Step5（持久化搬注册表基类部分）。
 */

/** 注册 Horse 族基类全部组件序列化器到注册表（在 ComponentSerializerRegistry::registerAll 内调用） */
void registerHorseComponentSerializers(ComponentSerializerRegistry& registry);

} // namespace mc::entity::serialization::components
