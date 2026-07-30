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

#include "AnimalEntity.hpp"
#include "../../../../item/Items.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../world/block/BlockPos.hpp"
#include "../../../ai/goal/goals/BreedGoal.hpp"
#include "../../../ai/goal/goals/FollowParentGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../ai/goal/goals/PanicGoal.hpp"
#include "../../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/TemptGoal.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityDataManager.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../../serialization/EntityNbtKeys.hpp"
#include "../../../serialization/NbtHelper.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/network/protocol/EntityEvents.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {

// 使用序列化命名空间
using namespace entity::serialization;

// 继承链标识（复刻 vanilla ClassTreeIdRegistry，parent = AgeableEntity::classInfo()）。
// 本类无同步字段，classInfo 仅作父链遍历节点：子类 ClassRegisterGuard 沿父链查找最高 id
// 时穿过本类（lastAssignedId=-1）直达父链已分配 id 的基类，子类首字段续接其后。
const entity::EntityClassInfo& AnimalEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"AnimalEntity", &AgeableEntity::classInfo()};
    return s_classInfo;
}

AnimalEntity::AnimalEntity(EntityInstanceId id)
    : AgeableEntity(id)
{
    // 注册属性
    registerAttributes();

    // 设置路径优先级
    // setPathPriority(PathNodeType.DANGER_FIRE, 16.0F);
    // setPathPriority(PathNodeType.DAMAGE_FIRE, -1.0F);
}

bool AnimalEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // 默认检查是否为小麦
    // 子类应该重写此方法来定义特定的繁殖物品
    return !itemStack.isEmpty() && itemStack.getItem() == Items::WHEAT;
}

ActionResultType AnimalEntity::interactMob(Player& player, Hand hand)
{
    // 与 MC 1.16.5 AnimalEntity.func_230254_b_(mobInteract) 对齐：
    //   手持繁殖物品(isBreedingItem) 时：
    //     - 成体(getGrowingAge==0) 且可繁殖：消耗物品 + setInLove 进入求爱
    //     - 幼体(isChild)：消耗物品 + ageUp 加速成长
    //   否则交由父类 MobEntity::interactMob 处理（默认 Pass）
    //
    // 注：原版 AnimalEntity.mobInteract 不播放进食音效；本项目沿用 WolfEntity
    // 的约定，在喂食繁殖/成长时播放一次通用进食音效，与既有测试用例断言一致。
    ItemStack& itemStack = player.getHeldItem(hand);

    if (isBreedingItem(itemStack)) {
        const i32 growingAge = getGrowingAge();

        if (growingAge == 0 && canBreed()) {
            // 成体喂食 → 进入求爱状态
            if (!player.abilities().creativeMode) {
                itemStack.shrink(1);
            }
            setInLove(player.playerId());

            if (!isSilent()) {
                auto soundEvent = makeSoundEventId("eat");
                if (soundEvent.has_value()) {
                    playSound(*soundEvent, 1.0f, 1.0f + (getRandom().nextFloat() - getRandom().nextFloat()) * 0.2f);
                }
            }
            return ActionResultType::Success;
        }

        if (isChild()) {
            // 幼体喂食 → 加速成长（与 MC 1.16.5 ageUp((int)((-i/20)*0.1F)) 一致）
            if (!player.abilities().creativeMode) {
                itemStack.shrink(1);
            }
            const i32 remainingTicks = -growingAge;
            const i32 accelerateSeconds = static_cast<i32>(remainingTicks * 0.1f) / 20;
            ageUp(accelerateSeconds);

            if (!isSilent()) {
                auto soundEvent = makeSoundEventId("eat");
                if (soundEvent.has_value()) {
                    playSound(*soundEvent, 1.0f, 1.0f + (getRandom().nextFloat() - getRandom().nextFloat()) * 0.2f);
                }
            }
            return ActionResultType::Success;
        }
    }

    return MobEntity::interactMob(player, hand);
}

bool AnimalEntity::canMateWith(const AnimalEntity& other) const
{
    // 检查双方都是成体、都处于爱心状态、是同类
    if (this == &other) {
        return false;
    }

    // 使用实体类型比较（避免 RTTI 开销）
    if (entityType() != other.entityType()) {
        return false;
    }

    // 检查双方都是成体且都处于爱心状态
    return isInLove() && other.isInLove();
}

bool AnimalEntity::canBreed() const
{
    // 年龄为0且不处于爱心状态
    return getGrowingAge() == 0 && !isInLove();
}

void AnimalEntity::setInLove(u64 playerUuid)
{
    // 设置爱心状态持续 600 ticks（30秒）
    // 调用 AgeableEntity::setInLove() 设置计时器
    AgeableEntity::setInLove(playerUuid);

    // 记录喂食玩家的 UUID
    m_loveCause = playerUuid;

    // 广播状态更新（用于客户端繁殖爱心粒子效果）
    if (world() != nullptr) {
        world()->broadcastEntityStatus(id(), static_cast<u8>(network::EntityStatus::LoveHeart));
    }
}

void AnimalEntity::resetInLove()
{
    // 清空爱心计时器
    resetLove();
    m_loveCause = 0;
}

i32 AnimalEntity::getExperiencePoints() const
{
    // 返回 1-3 经验
    math::Random& rng = getRandom();
    return 1 + rng.nextInt(3);
}

void AnimalEntity::tick()
{
    AgeableEntity::tick();

    updateInLove();

    // 成体时清空爱心状态
    // updateAITasks() 中会检查年龄并清空爱心
}

void AnimalEntity::registerGoals()
{
    // 调用父类方法
    AgeableEntity::registerGoals();

    // AnimalEntity 基类不注册任何 goal
    // 每个具体的动物子类（如 PigEntity、CowEntity）需要自己注册完整的 AI 目标列表
    //
    // 典型的动物 AI 目标结构（优先级从高到低）：
    // 0: SwimGoal - 游泳（最高优先级）
    // 1: PanicGoal - 恐慌逃跑
    // 2: BreedGoal - 繁殖
    // 3: TemptGoal - 食物诱惑（子类定义食物）
    // 4: FollowParentGoal - 跟随父母
    // 5: WaterAvoidingRandomWalkingGoal - 避水随机漫步
    // 6: LookAtGoal - 看向玩家
    // 7: LookRandomlyGoal - 随机看向
    //
    // 注意：子类必须调用 AgeableEntity::registerGoals() 而不是 AnimalEntity::registerGoals()
    // 以继承 AgeableEntity 的目标（如 FollowParentGoal 对幼体很重要）
}

void AnimalEntity::registerAttributes()
{
    // 调用父类方法
    AgeableEntity::registerAttributes();

    // 动物的基础属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.2);
}

void AnimalEntity::updateInLove()
{
    // 快速路径：使用 AgeableEntity 的爱心计时器
    // AgeableEntity::updateLove() 已经处理了爱心计时器递减
    // 这里只需要处理粒子效果

    i32 loveTimer = getLoveTimer();
    if (loveTimer <= 0) {
        return;
    }

    // 成体时如果有年龄（繁殖冷却），清空爱心状态
    if (getGrowingAge() != 0) {
        resetLove();
        m_loveCause = 0;
        return;
    }

    // 每10tick生成爱心粒子
    if ((loveTimer % 10) == 0) {
        spawnHeartParticles();
    }
}

void AnimalEntity::spawnHeartParticles()
{
    // 生成心形粒子
    if (!m_world) {
        return;
    }

    mc::math::Random& rng = getRandom();
    f32 ox = (rng.nextFloat() * 2.0f - 1.0f) * width();
    f32 oy = height() + 0.5f + rng.nextFloat() * 0.5f;
    f32 oz = (rng.nextFloat() * 2.0f - 1.0f) * width();

    Vector3 pos(x() + ox, y() + oy, z() + oz);
    Vector3 vel(0.0f, 0.0f, 0.0f);

    m_world->addParticle(particle::ParticleTypeId::Heart, pos, vel);
}

bool AnimalEntity::hurt(DamageSource& source, f32 amount)
{
    // 受伤时清空爱心状态（不重置繁殖冷却）
    resetInLove();

    return AgeableEntity::hurt(source, amount);
}

// ========== 寻路权重 ==========

f32 AnimalEntity::getPathWeight(f32 x, f32 y, f32 z) const
{
    const IWorld* worldPtr = this->world();
    if (!worldPtr) {
        return 0.0f;
    }

    BlockPos pos(static_cast<i32>(x), static_cast<i32>(y) - 1, static_cast<i32>(z));
    const BlockState* groundBlock = worldPtr->getBlockState(pos);

    // 检查脚下是否为草方块
    if (groundBlock != nullptr && groundBlock->is(VanillaBlocks::GRASS_BLOCK)) {
        return 10.0f;
    }

    // 否则返回亮度 - 0.5F
    BlockPos abovePos(static_cast<i32>(x), static_cast<i32>(y), static_cast<i32>(z));
    f32 brightness = worldPtr->getBrightness(abovePos);
    return brightness - 0.5f;
}

// ============================================================================
// NBT 序列化
// ============================================================================

void AnimalEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    using namespace mc::entity::serialization;

    // 先调用基类实现
    AgeableEntity::addAdditionalSaveData(tag);

    // LoveCause (i64) - 爱心来源玩家的 UUID（存储为单个 i64）
    // 实际存储为 UUIDMost/UUIDLeast，但我们的 m_loveCause 是 u64
    // 这里使用 "LoveCause" 键直接存储 i64 值
    if (isInLove() && m_loveCause != 0) {
        tag.put(nbt_keys::LOVE_CAUSE, static_cast<i64>(m_loveCause));
    }
}

Result<void> AnimalEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    using namespace mc::entity::serialization;

    // 先调用基类实现
    MC_TRY(AgeableEntity::readAdditionalSaveData(tag));

    // LoveCause (i64) - 读取爱心来源玩家的 UUID
    if (auto val = nbt_helper::tryGetLong(tag, nbt_keys::LOVE_CAUSE)) {
        m_loveCause = static_cast<u64>(*val);
    }

    return Result<void>::ok();
}

} // namespace mc
