/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the above copyright notice
 * and this permission notice shall be included in all copies or substantial portions
 * of the Software.
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
 * @brief Projectile 族组件序列化器
 *
 * 把 projectile 族 20 个类的特有状态字段 NBT 读写逻辑从 OOP 虚函数链
 * （addAdditionalSaveData/readAdditionalSaveData 逐层 super）搬到按组件注册的自由函数
 * 序列化器。对齐 vanilla 1.21.11 各 projectile 类的持久化字段清单。
 *
 * 序列化器按承载组件注册（注册表键 = entt::type_id<ComponentT>），序列化器内部用
 * tryGetComponent<ComponentT>() 早退（无组件的实体不参与该组件字段的存读盘）：
 *
 * | 组件 | 对齐 vanilla 类 | 持久化字段 | NBT 键 |
 * |---|---|---|---|
 * | ProjectileOwnerComponent | Projectile 基类 | shooterUuid + leftShooter + hasBeenShot | OwnerUUIDMost/Least +
 * LeftOwner + HasBeenShot | | ProjectileArrowStateComponent | AbstractArrow |
 * life/shake/inGround/pickup/damage/crit/PierceLevel/item(+)dealtDamage |
 * life/shake/inGround/pickup/damage/crit/PierceLevel/item/DealtDamage | | TridentStateComponent | ThrownTrident |
 * tridentStack(item) | Trident | | DamagingProjectileComponent | AbstractHurtingProjectile | accelerationX/Y/Z |
 * acceleration_power | | FireballStateComponent | Fireball/WitherSkull | explosionPower(Fireball)/blue(WitherSkull) |
 * ExplosionPower/dangerous | | FireworkRocketComponent | FireworkRocket |
 * lifetime/lifeTime/fireworkItem/shotFromCrossbow | Life/LifeTime/FireworksItem/ShotAtAngle | | EvokerFangsComponent |
 * EvokerFangs | warmupDelay + ownerUuid | Warmup + OwnerUUIDMost/Least | | ShulkerBulletComponent | ShulkerBullet |
 * targetUuid/direction/flightSteps/targetDelta | Target/Dir/Steps/TXD/TYD/TZD | | EyeOfEnderComponent | EyeOfEnder |
 * (vanilla 存 Item，项目无 item 字段) | 标 TODO 占位 | | ProjectileItemComponent | ThrowableItemProjectile/Spear |
 * itemStack | item |
 *
 * load 顺序（priority）：TridentStateComponent=0 先 load（读 item 后由 setItemStack 重算 loyalty），
 * ProjectileArrowStateComponent=10 后 load（读 dealtDamage，依赖父类字段已就位）。
 * 其余组件 priority=0（无跨组件依赖）。
 *
 * owner UUID 格式：双 long（OwnerUUIDMost/Least），与项目既有 EvokerFangs/AreaEffectCloud
 * 实现一致（零迁移成本），非 vanilla 1.21.11 EntityReference 单键格式。
 *
 * 不持久化的组件（vanilla 也不存盘，注册序列化器但 save 空实现）：FishingBobberComponent
 * （vanilla FishingHook 两个方法体全空）、ArrowEffectsComponent/SpectralArrowComponent/
 * PotionProjectileComponent/ExperienceBottleComponent/WindChargeStateComponent（vanilla 这些
 * 子类无自有 NBT 键，纯继承）。这些组件当前不注册序列化器（无字段需存盘）。
 *
 * 接通方式：本批完成后，EvokerFangs/FireworkRocket/Spear 的 OOP addAdditionalSaveData/
 * readAdditionalSaveData override 删除（搬注册表后双重写入会键冲突），回落到 Entity 基类空实现。
 *
 * 批次6 子目标2 Step6（持久化补齐对齐 vanilla）。
 */

/** 注册 Projectile 族全部组件序列化器到注册表（在 ComponentSerializerRegistry::registerAll 内调用） */
void registerProjectileComponentSerializers(ComponentSerializerRegistry& registry);

} // namespace mc::entity::serialization::components
