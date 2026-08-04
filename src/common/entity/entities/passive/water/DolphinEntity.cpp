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

#include "DolphinEntity.hpp"

#include "common/core/Types.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/AvoidEntityGoal.hpp"
#include "common/entity/ai/goal/goals/FindWaterGoal.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/MeleeAttackGoal.hpp"
#include "common/entity/ai/goal/goals/RandomSwimmingGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/special/DolphinGoals.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/ai/pathfinding/Path.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/passive/water/WaterMobEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"

#include <cmath>
#include <memory>
#include <optional>

namespace mc {

// ==================== 同步数据参数静态成员初始化 ====================
// id 由 registerData 沿继承链分配：BABY 占位 16、GOT_FISH 17、MOISTNESS_LEVEL 18。
entity::DataParameter<bool> DolphinEntity::DATA_BABY_PLACEHOLDER_PARAM = entity::EntityDataManager::createKey<bool>();
entity::DataParameter<bool> DolphinEntity::DATA_GOT_FISH_PARAM = entity::EntityDataManager::createKey<bool>();
entity::DataParameter<i32> DolphinEntity::DATA_MOISTNESS_LEVEL_PARAM = entity::EntityDataManager::createKey<i32>();

// 继承链标识（parent = WaterMobEntity::classInfo()）。
// vanilla 1.21.11 Dolphin 经 AgeableWaterCreature→AgeableMob，id16=DATA_BABY(Boolean，继承自 AgeableMob)。
// 项目 WaterMobEntity 不经 AgeableEntity，故在 DolphinEntity 层补 BABY 占位 + GOT_FISH/MOISTNESS。
const entity::EntityClassInfo& DolphinEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"DolphinEntity", &WaterMobEntity::classInfo()};
    return s_classInfo;
}

void DolphinEntity::registerData()
{
    // 先调用父类方法。WaterMobEntity/CreatureEntity 均无 registerData override，显式指
    // MobEntity::registerData() 避免名字查找落空，确保 Mob(id15) 及以下基类参数已注册。
    MobEntity::registerData();

    // 标记当前正在注册 DolphinEntity 类的字段，使 registerParam 沿继承链分配 id
    // （续接 Mob id15 之后）。RAII 守卫自动配对压栈/弹栈。
    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    // 注册 3 字段对齐 vanilla 1.21.11 Dolphin：BABY 占位@16、GOT_FISH@17、MOISTNESS_LEVEL@18。
    // 默认值与 vanilla defineSynchedData 一致（BABY=false、GOT_FISH=false、MOISTNESS=2400）。
    m_dataManager.registerParam(DATA_BABY_PLACEHOLDER_PARAM, false);
    m_dataManager.registerParam(DATA_GOT_FISH_PARAM, false);
    m_dataManager.registerParam(DATA_MOISTNESS_LEVEL_PARAM, 2400);
}

DolphinEntity::DolphinEntity(EntityInstanceId id)
    : WaterMobEntity(id)
{
    // 设置空气值（4800 tick = 4分钟）
    setAir(MAX_AIR);

    // 显式调用 registerData() 注册 Dolphin 字段（C++ 基类构造期虚函数不派发，
    // Entity::Entity() 内部调用的 registerData() 解析到 MobEntity 而非本类）。
    registerData();

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> DolphinEntity::create(IWorld* /*world*/)
{
    return std::make_unique<DolphinEntity>(0);
}

bool DolphinEntity::canJumpOutOfWater() const
{
    // 海豚可以跳出水面当且仅当:
    // 1. 当前在水中
    // 2. 上方有空气（接近水面）

    if (!isInWater()) {
        return false;
    }

    // 检查上方是否有空气
    const Vector3 pos = position();
    const BlockPos headPos(static_cast<i32>(std::floor(pos.x)),
        static_cast<i32>(std::floor(pos.y)) + 1,
        static_cast<i32>(std::floor(pos.z)));
    const IWorld* worldPtr = world();
    if (worldPtr == nullptr) {
        return false;
    }

    const BlockState* stateAbove = worldPtr->getBlockState(headPos);
    return stateAbove != nullptr && stateAbove->isAir();
}

void DolphinEntity::setTreasurePos(const BlockPos& pos)
{
    m_treasurePos = pos;
    m_hasTreasure = true;
}

void DolphinEntity::clearTreasureTarget()
{
    m_hasTreasure = false;
    m_guidingPlayer = false;
    m_guidedPlayerId = 0;
    m_guideTimer = 0;
}

void DolphinEntity::setGuidingPlayer(bool guiding, u64 playerId)
{
    m_guidingPlayer = guiding;
    m_guidedPlayerId = playerId;
    if (guiding) {
        m_guideTimer = GUIDE_DURATION;
    }
}

bool DolphinEntity::isFoodItem(const ItemStack& itemStack) const
{
    // 海豚食物 - 鳕鱼、鲑鱼、河豚、热带鱼
    const Item* item = itemStack.getItem();
    return item == Items::COD || item == Items::SALMON || item == Items::PUFFERFISH || item == Items::TROPICAL_FISH;
}

bool DolphinEntity::closeToTarget() const
{
    // 检查是否接近导航目标
    auto* nav = navigator();
    if (nav == nullptr) {
        return false;
    }

    const entity::ai::pathfinding::Path* path = nav->getPath();
    if (path == nullptr || path->isFinished()) {
        return false;
    }

    BlockPos targetPos = path->getTarget();
    if (targetPos.x == 0 && targetPos.y == 0 && targetPos.z == 0) {
        return false;
    }

    // 检查是否在指定距离范围内
    Vector3 targetCenter(
        static_cast<f64>(targetPos.x) + 0.5, static_cast<f64>(targetPos.y), static_cast<f64>(targetPos.z) + 0.5);

    f64 distSq = (position() - targetCenter).lengthSquared();
    constexpr f64 CLOSE_DISTANCE_SQ = CLOSE_DISTANCE * CLOSE_DISTANCE;

    return distSq < CLOSE_DISTANCE_SQ;
}

bool DolphinEntity::hasPath() const
{
    auto* nav = navigator();
    return nav != nullptr && nav->hasPath();
}

void DolphinEntity::clearNavigationPath()
{
    auto* nav = navigator();
    if (nav != nullptr) {
        nav->clearPath();
    }
}

bool DolphinEntity::tryMoveToEntity(const Entity& entity, f64 speed)
{
    // 使用 CreatureEntity::tryMoveTo
    return tryMoveTo(entity.x(), entity.y(), entity.z(), speed);
}

void DolphinEntity::onLeaveWater()
{
    WaterMobEntity::onLeaveWater();
    playSound(SoundEvents::ENTITY_DOLPHIN_JUMP, 1.0f, 1.0f);
}

std::optional<ResourceLocation> DolphinEntity::getAmbientSound() const
{
    if (isInWater()) {
        return SoundEvents::ENTITY_DOLPHIN_AMBIENT_WATER;
    }
    return SoundEvents::ENTITY_DOLPHIN_AMBIENT;
}

void DolphinEntity::playAttackSound(LivingEntity& /*target*/)
{
    playSound(SoundEvents::ENTITY_DOLPHIN_ATTACK, 1.0f, 1.0f);
}

void DolphinEntity::tick()
{
    WaterMobEntity::tick();

    // 更新引导计时器
    if (m_guidingPlayer && m_guideTimer > 0) {
        m_guideTimer--;
        if (m_guideTimer <= 0) {
            clearTreasureTarget();
        }
    }

    // 更新游泳行为
    if (isInWater()) {
        m_swimTimer++;

        // 随机跳跃
        if (m_swimTimer >= SWIM_JUMP_INTERVAL && canJumpOutOfWater()) {
            math::Random& rng = getRandom();
            if (rng.nextInt(1, JUMP_CHANCE_DENOMINATOR) == 1) {
                m_jumping = true;
                m_swimTimer = 0;
            }
        }
    } else {
        m_jumping = false;
    }
}

void DolphinEntity::registerGoals()
{
    // 海豚 AI 目标优先级

    // 优先级 0: 呼吸空气和寻找水源
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::SwimGoal>(this));
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::FindWaterGoal>(this));

    // 优先级 1: 游向宝藏
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::SwimToTreasureGoal>(this));

    // 优先级 2: 与玩家同游
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::SwimWithPlayerGoal>(this, 4.0));

    // 优先级 4: 随机游泳和随机看向
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::RandomSwimmingGoal>(this, 1.0, 10));
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::LookRandomlyGoal>(this));

    // 优先级 5: 看向玩家和跳跃
    m_goalSelector.addGoal(
        5, std::make_unique<entity::ai::goal::LookAtGoal>(this, 6.0f, 0.02f, [](const LivingEntity* entity) -> bool {
            return entity != nullptr && entity->entityType() == entity::VanillaEntityTypeKeys::PLAYER;
        }));
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::DolphinJumpGoal>(this, 10));

    // 优先级 6: 近战攻击
    m_goalSelector.addGoal(6, std::make_unique<entity::ai::goal::MeleeAttackGoal>(this, 1.2, true));

    // 优先级 8: 玩物品和跟随船
    m_goalSelector.addGoal(8, std::make_unique<entity::ai::goal::PlayWithItemsGoal>(this));
    m_goalSelector.addGoal(8, std::make_unique<entity::ai::goal::FollowBoatGoal>(this));

    // 优先级 9: 避开守卫者和远古守卫者（8格检测距离，1.0速度）
    m_goalSelector.addGoal(9,
        std::make_unique<entity::ai::goal::AvoidEntityGoal>(
            this, 8.0f, 1.0, 1.0, [](const LivingEntity* entity) -> bool {
                if (!entity) return false;
                auto type = entity->entityType();
                return type == entity::VanillaEntityTypeKeys::GUARDIAN ||
                    type == entity::VanillaEntityTypeKeys::ELDER_GUARDIAN;
            }));

    // 目标选择器
    // 优先级 1: 被攻击后反击，并呼叫同类
    // MC 原版: HurtByTargetGoal(this, Guardian.class).setAlertOthers()
    // 海豚不会反击守卫者和远古守卫者，但会警醒同类
    m_targetSelector.addGoal(
        1, std::make_unique<entity::ai::goal::HurtByTargetGoal>(this, true, [](const LivingEntity* attacker) -> bool {
            if (!attacker) return false;
            auto type = attacker->entityType();
            return type == entity::VanillaEntityTypeKeys::GUARDIAN ||
                type == entity::VanillaEntityTypeKeys::ELDER_GUARDIAN;
        }));
}

void DolphinEntity::registerAttributes()
{
    // 调用父类方法
    WaterMobEntity::registerAttributes();

    // 海豚属性
    // 最大生命值: 10.0
    // 移动速度: 1.2
    // 攻击伤害: 3.0
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 1.2);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 3.0);
}

} // namespace mc
