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
 * @brief Entity 层 12 字段 + FallFlying 组件序列化器
 *
 * 把 Entity::writeToNBT/readFromNBT 中 12 个已组件化字段的 NBT 读写逻辑搬到按组件
 * 注册的自由函数序列化器。9 个序列化器对，按承载组件注册：
 *
 * | 组件 | 字段 | 读写路径 |
 * |---|---|---|
 * | StateVectorComponent | Pos | tryGetComponent 直写 m_pos（绕过 setPosition 副作用） |
 * | VelocityComponent | Motion | tryGetComponent 直写 m_velocity（setVelocity 本就纯直写，等价） |
 * | EntityRotationComponent | Rotation | tryGetComponent 直写 m_rot.yaw/pitch（绕过 setRotation 副作用） |
 * | PhysicsStateComponent | FallDistance + OnGround | FallDistance 走 setFallDistance（纯直写）；OnGround 直写
 * m_onGround（绕过 setOnGround 落地清 climbPos 副作用） | | FireComponent | Fire |
 * setRemainingFireTicks（纯直写，无副作用） | | PortalComponent | PortalCooldown |
 * setPortalCooldown（纯直写，无副作用） | | FreezeComponent | TicksFrozen | setTicksFrozen（写组件 + DataParameter
 * 同步，与 readFromNBT 现行一致） | | EntityStateComponent | Air + CustomName + CustomNameVisible + Silent + NoGravity
 * | 走对应 setter（与 readFromNBT 现行一致，含 DataParameter 同步） | | EntityFlagsComponent | FallFlying |
 * isElytraFlying 读 + addFlag/removeFlag 写（从 LivingEntity 层上提，仅依赖 Entity public 接口） |
 *
 * 字段访问策略：能调 public setter 的优先调 setter（保留 DataParameter 同步副作用——C 类字段
 * 硬约束）。3 个字段（Pos/Rotation/OnGround）现行 readFromNBT 刻意绕过 setter 副作用直写
 * m_builtIn.* 组件，序列化器经 public tryGetComponent<T>() 拿同一组件指针直写，语义完全一致
 * （m_builtIn.stateVector 就是 tryGetComponent<StateVectorComponent>() 返回值）。
 *
 * FallFlying 跨层调整：原在 LivingEntity::addAdditionalSaveData/readAdditionalSaveData 处理，
 * 本批上提为按 EntityFlagsComponent 注册的自由函数（仅依赖 Entity 基类 public 接口
 * isElytraFlying/addFlag/removeFlag，与 LivingEntity 无耦合）。Step3 从 LivingEntity 同步删除避免重复写。
 *
 * NBT 格式：保持 Java 版平铺格式（Pos/Motion/Rotation 等直接在根 tag），键名不变，零迁移兼容。
 *
 * 批次6 子目标1 Step2（序列化器就位，暂不接通 writeToNBT/readFromNBT，Step3 接通）。
 */

/** 注册 Entity 层全部组件序列化器到注册表（在 ComponentSerializerRegistry::registerAll 内调用） */
void registerEntityComponentSerializers(ComponentSerializerRegistry& registry);

} // namespace mc::entity::serialization::components
