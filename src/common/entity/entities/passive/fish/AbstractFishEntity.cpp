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

#include "AbstractFishEntity.hpp"

#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/AvoidEntityGoal.hpp"
#include "../../../ai/goal/goals/FishSwimGoal.hpp"
#include "../../../ai/goal/goals/PanicGoal.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/LivingEntity.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../../registry/VanillaEntityTypeKeys.hpp"
#include "../../player/Player.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"

namespace mc {

// ==================== 同步数据参数静态成员初始化 ====================
// 对应 vanilla 1.21.11 AbstractFish.FROM_BUCKET，id 由 registerData 沿继承链分配为 16。
entity::DataParameter<bool> AbstractFishEntity::FROM_BUCKET_PARAM = entity::EntityDataManager::createKey<bool>();

// 继承链标识（复刻 vanilla ClassTreeIdRegistry，parent = WaterMobEntity::classInfo()）。
// vanilla 1.21.11 AbstractFish 在 Mob(id15) 之后注册 FROM_BUCKET(Boolean,id16)，本类补齐。
const entity::EntityClassInfo& AbstractFishEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"AbstractFishEntity", &WaterMobEntity::classInfo()};
    return s_classInfo;
}

void AbstractFishEntity::registerData()
{
    // 先调用父类方法。WaterMobEntity/CreatureEntity 均无 registerData override，显式指
    // MobEntity::registerData() 避免名字查找落空，确保 Mob(id15) 及以下基类参数已注册。
    MobEntity::registerData();

    // 标记当前正在注册 AbstractFishEntity 类的字段，使 registerParam 沿继承链分配 id
    // （续接 Mob id15 之后）。RAII 守卫自动配对压栈/弹栈。
    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    // 注册 vanilla 1.21.11 AbstractFish.FROM_BUCKET(Boolean,id16)：
    // 桶装鱼标志同步镜像，业务权威源仍为 m_fromBucket。
    m_dataManager.registerParam(FROM_BUCKET_PARAM, false);
}

AbstractFishEntity::AbstractFishEntity(EntityInstanceId id)
    : WaterMobEntity(id)
{
    // 设置鱼类最大空气供应量（480 ticks = 24秒）
    setAir(maxAir());

    // 显式调用 registerData() 注册 FROM_BUCKET（C++ 基类构造期虚函数不派发，
    // Entity::Entity() 内部调用的 registerData() 解析到 MobEntity 而非本类）。
    registerData();

    registerGoals();
    registerAttributes();
}

void AbstractFishEntity::tick()
{
    WaterMobEntity::tick();
    updateSwimming();
    updateFlopping();
}

void AbstractFishEntity::registerGoals()
{
    // 优先级 0: 恐慌逃跑（受到伤害或着火时）
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::PanicGoal>(this, 1.25));

    // 优先级 2: 避开玩家
    m_goalSelector.addGoal(2,
        std::make_unique<entity::ai::goal::AvoidEntityGoal>(this,
            8.0f, // 避开距离：8格
            1.6,  // 近距离逃跑速度（更快）
            1.4,  // 远距离逃跑速度
            [](const LivingEntity* entity) -> bool {
                if (entity == nullptr || !entity->isAlive()) {
                    return false;
                }
                // 只躲避玩家
                if (entity->entityType() != entity::VanillaEntityTypeKeys::PLAYER) {
                    return false;
                }
                // 不躲避旁观者模式和创造模式玩家
                const Player* player = dynamic_cast<const Player*>(entity);
                if (player != nullptr && (player->isSpectator() || player->isCreative())) {
                    return false;
                }
                return true;
            }));

    // 优先级 4: 鱼类游泳目标
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::FishSwimGoal>(this, 1.0, 40));
}

void AbstractFishEntity::registerAttributes()
{
    WaterMobEntity::registerAttributes();
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 3.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
}

void AbstractFishEntity::updateSwimming()
{
    if (isInWater()) {
        m_swimming = true;
        m_flopping = false;
        return;
    }

    m_swimming = false;
    m_flopping = true;
}

void AbstractFishEntity::updateFlopping()
{
    if (isInWater()) {
        m_flopTimer = 0;
        m_flopping = false;
        return;
    }

    ++m_flopTimer;

    // 每 100 tick 执行一次扑腾（跳跃和声音）
    if (m_flopTimer >= 100 && onGround()) {
        // 重置计时器
        m_flopTimer = 0;

        // 计算随机跳跃速度
        math::Random& rng = getRandom();
        f32 dx = (rng.nextFloat() * 2.0f - 1.0f) * 0.05f;
        f32 dy = 0.4f;
        f32 dz = (rng.nextFloat() * 2.0f - 1.0f) * 0.05f;

        addVelocity(dx, dy, dz);

        // 设置不在地面
        setOnGround(false);

        // 播放扑腾声音
        auto flopSound = getFlopSound();
        if (flopSound.has_value()) {
            playSound(*flopSound, getSoundVolume(), getSoundPitch());
        }
    }
}

} // namespace mc
