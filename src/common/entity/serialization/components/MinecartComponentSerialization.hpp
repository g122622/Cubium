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
 * @brief Minecart 族组件序列化器
 *
 * 把 minecart 族叶子类的特有持久化字段 NBT 读写逻辑从 OOP 虚函数链
 * （addAdditionalSaveData/readAdditionalSaveData override）搬到按组件注册的自由函数
 * 序列化器。对齐 vanilla 1.21.11 各 minecart 叶子类的持久化字段清单。
 *
 * 序列化器按承载组件注册（注册表键 = entt::type_id<ComponentT>），序列化器内部用
 * tryGetComponent<ComponentT>() 早退（无组件的实体不参与该组件字段的存读盘）：
 *
 * | 组件 | 对齐 vanilla 类 | 持久化字段 | NBT 键 |
 * |---|---|---|---|
 * | SpawnerMinecartComponent | MinecartSpawner | SpawnerLogic 全部参数（Delay/MinSpawnDelay/
 * MaxSpawnDelay/SpawnCount/MaxNearbyEntities/RequiredPlayerRange/SpawnRange/SpawnData/SpawnPotentials） | SpawnerLogic
 * 内部字符串字面量 |
 *
 * SpawnerLogic 自身已实现 saveToNBT/loadFromNBT（与 MobSpawnerBlockEntity 共用同一逻辑类），
 * 序列化器仅作透传：取组件内 SpawnerLogic 引用，直接调其 saveToNBT/loadFromNBT 把键平铺到
 * 实体 compound 根层。SpawnerLogic 键（Delay/SpawnData 等）与 minecart 基类组件序列化键
 * （Pos/Motion/Rotation 等）无冲突，平铺安全。
 *
 * 其余 minecart 叶子类组件（ChestMinecart/FurnaceMinecart/TntMinecart/HopperMinecart/
 * CommandBlockMinecart）当前无自有持久化字段：
 * - ChestMinecart/HopperMinecart 的库存内容走 LootableContainer 体系（容器 NBT 由
 *   ContainerEntity 层处理，非实体 addAdditionalSaveData），且项目当前未接通 vehicle 容器
 *   持久化（TODO）。
 * - FurnaceMinecart 的 fuel/pushX/pushZ 是运行时状态（vanilla MinecartFurnace 不存盘）。
 * - TntMinecart 的 fuse/ignitionSource 是运行时状态（vanilla MinecartTNT 不存盘）。
 * - CommandBlockMinecart 的 command/lastOutput/successCount 走 CommandBlockEntity 体系，
 *   vanilla MinecartCommandBlock 持久化 Command/LastOutput/SuccessCount，项目当前未接通
 *   （TODO，待 command block 矿车持久化业务接入后补序列化器）。
 * 故本批仅注册 SpawnerMinecartComponent 序列化器，其余叶子类组件不注册（无字段需存盘）。
 *
 * 接通方式：本批完成后，SpawnerMinecartEntity 的 addAdditionalSaveData/readAdditionalSaveData
 * override 删除（搬注册表后双重写入会键冲突），回落到 AbstractMinecartEntity/Entity 基类空实现。
 *
 * 批次7 子批2 Step3（持久化补齐 SpawnerMinecart 搬注册表）。
 */

/** 注册 Minecart 族全部组件序列化器到注册表（在 ComponentSerializerRegistry::registerAll 内调用） */
void registerMinecartComponentSerializers(ComponentSerializerRegistry& registry);

} // namespace mc::entity::serialization::components
