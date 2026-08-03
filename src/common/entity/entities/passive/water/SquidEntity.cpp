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

#include "SquidEntity.hpp"

#include "common/core/Types.hpp"
#include "common/entity/ai/goal/goals/special/SquidGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/passive/water/WaterMobEntity.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include <memory>

namespace mc {

// ==================== 同步数据参数静态成员初始化 ====================
// 占位对齐 vanilla 1.21.11 AgeableMob.DATA_BABY(id16)，id 由 registerData 沿继承链分配为 16。
entity::DataParameter<bool> SquidEntity::DATA_BABY_PLACEHOLDER_PARAM = entity::EntityDataManager::createKey<bool>();

// 继承链标识（parent = WaterMobEntity::classInfo()）。
// vanilla 1.21.11 Squid 经 AgeableWaterCreature→AgeableMob，id16=DATA_BABY(Boolean)。项目
// WaterMobEntity 不经 AgeableEntity，故在 SquidEntity 层补 Boolean 占位对齐 id16。
const entity::EntityClassInfo& SquidEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"SquidEntity", &WaterMobEntity::classInfo()};
    return s_classInfo;
}

void SquidEntity::registerData()
{
    // 先调用父类方法。WaterMobEntity/CreatureEntity 均无 registerData override，显式指
    // MobEntity::registerData() 避免名字查找落空，确保 Mob(id15) 及以下基类参数已注册。
    MobEntity::registerData();

    // 标记当前正在注册 SquidEntity 类的字段，使 registerParam 沿继承链分配 id
    // （续接 Mob id15 之后）。RAII 守卫自动配对压栈/弹栈。
    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    // 注册 Boolean 占位字段对齐 vanilla 1.21.11 AgeableMob.DATA_BABY(id16)。鱿鱼无幼体
    // 语义，占位恒 false，仅占位 id16 使子类 GlowSquid.DATA_DARK_TICKS 落 id17。
    m_dataManager.registerParam(DATA_BABY_PLACEHOLDER_PARAM, false);
}

SquidEntity::SquidEntity(EntityInstanceId id)
    : WaterMobEntity(id)
{
    // 显式调用 registerData() 注册占位字段（C++ 基类构造期虚函数不派发，
    // Entity::Entity() 内部调用的 registerData() 解析到 MobEntity 而非本类）。
    registerData();

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> SquidEntity::create(IWorld* /*world*/)
{
    return std::make_unique<SquidEntity>(0);
}

void SquidEntity::sprayInk()
{
    if (m_sprayingInk) {
        return;
    }
    m_sprayingInk = true;
    m_sprayTimer = SPRAY_INK_DURATION;

    // 播放喷墨音效
    auto squirtSound = getSquirtSound();
    if (squirtSound.has_value()) {
        playSound(*squirtSound, 1.0f, 1.0f);
    }

    // 在鱿鱼位置生成墨汁粒子
    if (world() != nullptr && world()->isClientSide()) {
        using namespace mc::particle;
        math::Random& random = world()->getRandom();

        // 生成多个墨汁粒子形成云状效果
        for (i32 i = 0; i < 30; ++i) {
            // 粒子位置：在鱿鱼周围随机分布
            f32 px = static_cast<f32>(x()) + (random.nextFloat() - 0.5f) * width() * 2.0f;
            f32 py = static_cast<f32>(y()) + random.nextFloat() * height();
            f32 pz = static_cast<f32>(z()) + (random.nextFloat() - 0.5f) * width() * 2.0f;

            // 粒子速度：向外扩散
            f32 vx = (random.nextFloat() - 0.5f) * 0.5f;
            f32 vy = random.nextFloat() * 0.1f;
            f32 vz = (random.nextFloat() - 0.5f) * 0.5f;

            world()->addParticle(getInkParticle(), Vector3(px, py, pz), Vector3(vx, vy, vz));
        }
    }
}

bool SquidEntity::hurt(DamageSource& source, f32 amount)
{
    // 调用父类 hurt 处理实际伤害；仅当成功受伤且有复仇目标时才喷墨逃跑
    if (WaterMobEntity::hurt(source, amount) && getLastHurtBy() != nullptr) {
        sprayInk();
        return true;
    }
    return false;
}

void SquidEntity::tick()
{
    WaterMobEntity::tick();

    // 更新喷墨计时器
    if (m_sprayingInk && m_sprayTimer > 0) {
        m_sprayTimer--;
        if (m_sprayTimer <= 0) {
            m_sprayingInk = false;
        }
    }

    // 更新游泳行为
    if (isInWater()) {
        m_swimTimer++;
        m_changeDirectionTimer++;

        // 随机改变方向
        if (m_changeDirectionTimer >= 100) {
            math::Random& rng = getRandom();
            m_targetSwimAngle = rng.nextFloat(0.0f, 360.0f);
            m_changeDirectionTimer = 0;
        }

        // 平滑转向
        f32 angleDiff = m_targetSwimAngle - m_swimAngle;
        while (angleDiff > 180.0f)
            angleDiff -= 360.0f;
        while (angleDiff < -180.0f)
            angleDiff += 360.0f;
        m_swimAngle += angleDiff * 0.1f;

        // 游泳推进
        if (m_swimTimer >= SWIM_DURATION) {
            m_swimming = false;
            m_swimTimer = 0;
        }
    } else {
        // 在陆地上扑腾
        m_swimming = false;
    }
}

void SquidEntity::setMovementVector(f32 x, f32 y, f32 z)
{
    m_randomMotionVecX = x;
    m_randomMotionVecY = y;
    m_randomMotionVecZ = z;
}

bool SquidEntity::hasMovementVector() const
{
    return m_randomMotionVecX != 0.0f || m_randomMotionVecY != 0.0f || m_randomMotionVecZ != 0.0f;
}

void SquidEntity::registerGoals()
{
    // 优先级 0: 随机游泳（最高优先级）
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::SquidMoveRandomGoal>(this));

    // 优先级 1: 逃跑目标（受攻击时逃跑）
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::SquidFleeGoal>(this));
}

void SquidEntity::registerAttributes()
{
    // 调用父类方法
    WaterMobEntity::registerAttributes();

    // 鱿鱼的属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
}

} // namespace mc
