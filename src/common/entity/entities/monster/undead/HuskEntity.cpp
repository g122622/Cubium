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

#include "HuskEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/monster/undead/ZombieEntity.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include <memory>

namespace mc {

// ============================================================================
// 继承链标识（parent = ZombieEntity::classInfo()）。透传层无自身同步字段，
// classInfo 仅作父链遍历节点。
// ============================================================================
const entity::EntityClassInfo& HuskEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"HuskEntity", &ZombieEntity::classInfo()};
    return s_classInfo;
}

HuskEntity::HuskEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : ZombieEntity(id, registry)
{
    // 显式调用 registerData() 确保 Zombie 三字段沿正确继承链注册（C++ 基类构造期
    // 虚函数不派发，ZombieEntity 构造函数已调，但 Husk 无 override 故不重复注册自身字段）。
    // 此处调用幂等：DataParameter 静态成员跨实例共享，assignId 双重检查锁。
    registerData();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> HuskEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<HuskEntity>(EntityInstanceId(0), registry);
}

void HuskEntity::registerAttributes()
{
    // 调用父类方法
    ZombieEntity::registerAttributes();

    // 尸壳的属性与僵尸相同
}

bool HuskEntity::attackEntityAsMob(LivingEntity& target)
{
    // 首先调用父类方法执行基础攻击（含燃烧传递等 Zombie 逻辑）。
    // 对齐 Java Husk.doHurtTarget（Husk.java:57-65）：super.doHurtTarget 先执行。
    if (!ZombieEntity::attackEntityAsMob(target)) {
        return false;
    }

    // 空手近战攻击命中时对目标施加饥饿效果。
    // 对齐 Java Husk.doHurtTarget：flag && getMainHandItem().isEmpty() && target instanceof LivingEntity。
    // 此处 target 已是 LivingEntity&（attackEntityAsMob 签名），只需判主手为空。
    if (getMainHandItem().isEmpty()) {
        IWorld* worldPtr = world();
        if (worldPtr != nullptr) {
            // 取区域难度 effectiveDifficulty（对齐 Java
            // getCurrentDifficultyAt(blockPosition).getEffectiveDifficulty()）。 区域难度范围：Easy 0.75~1.375,
            // Normal 1.5~3.5, Hard 2.25~6.75。
            entity::combat::DifficultyInstance difficultyInstance = entity::combat::DifficultyInstance::at(*worldPtr,
                BlockPos(static_cast<i32>(std::floor(x())), static_cast<i32>(y()), static_cast<i32>(std::floor(z()))));
            f32 effectiveDifficulty = difficultyInstance.getEffectiveDifficulty();

            // 饥饿持续时间 = 140 * (int)effectiveDifficulty ticks（对齐 Java 140 * (int)f）。
            // Normal 难度 effectiveDifficulty≈2.0 → 280 ticks ≈ 14 秒。等级 0 = 饥饿 I。
            i32 hungerDuration = 140 * static_cast<i32>(effectiveDifficulty);
            if (hungerDuration > 0) {
                target.addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Hunger, hungerDuration, 0));
            }
        }
    }

    return true;
}

} // namespace mc
