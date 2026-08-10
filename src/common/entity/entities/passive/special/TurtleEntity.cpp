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

#include "TurtleEntity.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../item/Items.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../sound/SoundCategory.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/math/Vector3.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../world/block/BlockPos.hpp"
#include "../../../../world/block/BlockTags.hpp"
#include "../../../../world/block/blocks/mob/TurtleEggBlock.hpp"
#include "../../../../world/fluid/Fluid.hpp"
#include "../../../../world/fluid/FluidTags.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../ai/goal/goals/special/TurtleGoals.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/LivingEntity.hpp"
#include "../../../registry/VanillaEntityTypeKeys.hpp"
#include "../../../serialization/EntityNbtKeys.hpp"
#include "../../../serialization/NbtHelper.hpp"
#include "common/core/Result.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>

namespace mc {

// ========== DataParameter 静态定义 ==========

entity::DataParameter<bool> TurtleEntity::DATA_HAS_EGG_PARAM = entity::EntityDataManager::createKey<bool>();
entity::DataParameter<bool> TurtleEntity::DATA_LAYING_EGG_PARAM = entity::EntityDataManager::createKey<bool>();

const entity::EntityClassInfo& TurtleEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"TurtleEntity", &AnimalEntity::classInfo()};
    return s_classInfo;
}

TurtleEntity::TurtleEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AnimalEntity(id, registry)
{
    // 海龟可以走上1格高的方块
    setStepHeight(1.0f);

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();

    // 补调 registerData：AnimalEntity 构造只调 registerAttributes 不调 registerData（vtable 在基类
    // 构造期间指向 AnimalEntity，派生 override 永不执行），须在派生类构造显式调用。
    // Turtle 的 registerData 注册 DATA_HAS_EGG / DATA_LAYING_EGG 同步参数。
    registerData();
}

std::unique_ptr<Entity> TurtleEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<TurtleEntity>(0, registry);
}

void TurtleEntity::setHomePos(const BlockPos& pos)
{
    m_homePos = pos;
    m_hasHomePos = true;
}

bool TurtleEntity::isInWater() const
{
    return Entity::isInWater();
}

std::optional<ResourceLocation> TurtleEntity::getAmbientSound() const
{
    // 对齐原版 Turtle.getAmbientSound：仅“不在水中 + 在地面 + 非幼体”时播放陆地环境音，
    // 其余情况（水中游泳、幼体）不播放。sounds.json 中无 entity.turtle.ambient，
    // 仅有 entity.turtle.ambient_land，故不能走默认 makeSoundEventId("ambient")。
    if (!isInWater() && onGround() && !isChild()) {
        return SoundEvents::ENTITY_TURTLE_AMBIENT_LAND;
    }
    return std::nullopt;
}

bool TurtleEntity::isLayingEgg() const
{
    return m_dataManager.get(DATA_LAYING_EGG_PARAM);
}

void TurtleEntity::setLayingEgg(bool laying)
{
    m_dataManager.set(DATA_LAYING_EGG_PARAM, laying);
}

bool TurtleEntity::hasEgg() const
{
    return m_dataManager.get(DATA_HAS_EGG_PARAM);
}

void TurtleEntity::setHasEgg(bool hasEgg)
{
    m_dataManager.set(DATA_HAS_EGG_PARAM, hasEgg);
}

bool TurtleEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // 海龟仅接受海草作为繁殖物品
    const Item* item = itemStack.getItem();
    if (item == nullptr) return false;
    return item == Items::SEAGRASS;
}

bool TurtleEntity::canBreed() const
{
    // 海龟只有在没有蛋的情况下才能繁殖
    return AnimalEntity::canBreed() && !hasEgg();
}

std::unique_ptr<AnimalEntity> TurtleEntity::spawnBaby(AnimalEntity& /*partner*/)
{
    // ECS 迁移：实体构造需要 registry 句柄，ClientWorld 返回 nullptr 表客户端不接入 ECS
    auto* registry = &ecsRegistry();
    if (registry == nullptr) {
        return nullptr;
    }

    // 创建小海龟，继承出生地记忆
    auto baby = std::make_unique<TurtleEntity>(0, *registry);

    // 设置为幼体
    baby->setChild(true);

    // 小海龟继承父母的出生地，长大后也会回到这里产卵
    if (hasHomePos()) {
        baby->setHomePos(m_homePos);
    }

    // 设置位置（在父体位置附近）
    baby->setPosition(x(), y(), z());

    return baby;
}

void TurtleEntity::tick()
{
    AnimalEntity::tick();

    // 更新产卵计时器
    if (isLayingEgg() && m_layEggTimer > 0) {
        m_layEggTimer--;
        if (m_layEggTimer <= 0) {
            // 产卵完成
            setLayingEgg(false);
            setHasEgg(false);

            // 在脚下生成海龟蛋方块
            _layEgg();
        }
    }
}

void TurtleEntity::_layEgg()
{
    if (world() == nullptr) {
        return;
    }

    // 获取海龟脚下位置
    BlockPos footPos(
        static_cast<i32>(std::floor(x())), static_cast<i32>(std::floor(y())), static_cast<i32>(std::floor(z())));

    // 检查脚下是否是沙子类方块
    const BlockState* belowState = world()->getBlockState(footPos.down());
    if (belowState == nullptr || !BlockTags::SAND().contains(*belowState)) {
        // 不是沙子，无法下蛋
        return;
    }

    // 检查目标位置是否为空气（沙子上方）
    const BlockState* currentPos = world()->getBlockState(footPos);
    if (currentPos == nullptr || !currentPos->isAir()) {
        // 位置被占用
        return;
    }

    // 随机生成 1-4 个蛋
    i32 eggCount = 1 + getRandom().nextInt(4);

    // 获取海龟蛋方块
    Block* turtleEggBlock = VanillaBlocks::TURTLE_EGG;
    if (turtleEggBlock == nullptr) {
        return;
    }

    // 创建海龟蛋方块状态
    // 注意：withEggs 返回值类型，需要保存后再取地址
    auto* turtleEgg = static_cast<blocks::TurtleEggBlock*>(turtleEggBlock);
    BlockState eggState = turtleEgg->withEggs(eggCount);

    // 放置海龟蛋方块
    // flags = 3: 通知客户端 + 通知邻居
    world()->setBlockState(footPos, &eggState, 3);

    // 播放下蛋音效
    f32 pitch = 0.9f + getRandom().nextFloat() * 0.2f;
    world()->playSound(SoundEvents::ENTITY_TURTLE_LAY_EGG,
        sound::SoundCategory::Blocks,
        Vector3(
            static_cast<f32>(footPos.x) + 0.5f, static_cast<f32>(footPos.y) + 0.5f, static_cast<f32>(footPos.z) + 0.5f),
        0.3f,
        pitch);
}

void TurtleEntity::registerGoals()
{
    // 优先级 0: 恐慌逃跑（最高优先级）
    // 海龟恐慌时优先寻找水源
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::TurtlePanicGoal>(this, 1.2));

    // 优先级 1: 繁殖和产卵
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::TurtleMateGoal>(this, 1.0));
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::TurtleLayEggGoal>(this, 1.0));

    // 优先级 2: 海草诱惑
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::TurtleTemptGoal>(this, 1.1));

    // 优先级 3: 前往水中
    m_goalSelector.addGoal(3, std::make_unique<entity::ai::goal::TurtleGoToWaterGoal>(this, 1.0));

    // 优先级 4: 返回出生地
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::TurtleGoHomeGoal>(this, 1.0));

    // 优先级 5: 跟随父母（幼年海龟）
    // 由 AnimalEntity::registerGoals() 注册

    // 优先级 7: 旅行（在水中随机游泳）
    m_goalSelector.addGoal(7, std::make_unique<entity::ai::goal::TurtleTravelGoal>(this, 1.0));

    // 优先级 8: 看向玩家
    m_goalSelector.addGoal(
        8, std::make_unique<entity::ai::goal::LookAtGoal>(this, 8.0f, 0.02f, [](const LivingEntity* entity) -> bool {
            // 只看向玩家
            return entity != nullptr && entity->entityType() == entity::VanillaEntityTypeKeys::PLAYER;
        }));

    // 优先级 9: 随机游荡（只在陆地上）
    m_goalSelector.addGoal(9, std::make_unique<entity::ai::goal::TurtleWanderGoal>(this, 1.0, 100));

    // 调用父类方法注册基础动物 AI（包括 SwimGoal、FollowParentGoal 等）
    AnimalEntity::registerGoals();
}

void TurtleEntity::registerAttributes()
{
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // 海龟的属性
    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 30.0);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
    // 海龟在陆地上移动较慢，通过 travel() 方法实现
    // 陆地速度 = max(AIMoveSpeed / 2.0, 0.06F)，约为水中速度的 24%
}

void TurtleEntity::registerData()
{
    AnimalEntity::registerData();

    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    m_dataManager.registerParam(DATA_HAS_EGG_PARAM, false);
    m_dataManager.registerParam(DATA_LAYING_EGG_PARAM, false);
}

void TurtleEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    using namespace mc::entity::serialization;
    AnimalEntity::addAdditionalSaveData(tag);

    // 出生位置
    if (m_hasHomePos) {
        tag.put(nbt_keys::HOME_X, m_homePos.x);
        tag.put(nbt_keys::HOME_Y, m_homePos.y);
        tag.put(nbt_keys::HOME_Z, m_homePos.z);
    }

    // 是否有蛋
    tag.put(nbt_keys::HAS_EGG, static_cast<i8>(hasEgg() ? 1 : 0));
}

Result<void> TurtleEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    using namespace mc::entity::serialization;
    MC_TRY(AnimalEntity::readAdditionalSaveData(tag));

    // 出生位置
    auto homeX = nbt_helper::tryGetInt(tag, nbt_keys::HOME_X);
    auto homeY = nbt_helper::tryGetInt(tag, nbt_keys::HOME_Y);
    auto homeZ = nbt_helper::tryGetInt(tag, nbt_keys::HOME_Z);
    if (homeX.has_value() && homeY.has_value() && homeZ.has_value()) {
        setHomePos(BlockPos(*homeX, *homeY, *homeZ));
    }

    // 是否有蛋
    if (auto val = nbt_helper::tryGetBool(tag, nbt_keys::HAS_EGG)) {
        setHasEgg(*val);
    }

    return Result<void>::ok();
}

f32 TurtleEntity::getPathWeight(f32 x, f32 y, f32 z) const
{
    const IWorld* worldPtr = world();
    if (worldPtr == nullptr) {
        return 0.0f;
    }

    BlockPos pos(static_cast<i32>(x), static_cast<i32>(y), static_cast<i32>(z));

    // 非回家状态 + 水中：返回 10.0f（偏好水域）
    if (!m_goingHome) {
        const fluid::FluidState* fluid = worldPtr->getFluidState(pos);
        if (fluid != nullptr && !fluid->isEmpty() && fluid->getFluid().isIn(fluid::FluidTags::WATER())) {
            return 10.0f;
        }
    }

    // 沙滩上（脚下是沙子）：返回 10.0f（偏好沙滩产卵）
    if (_isOnSand(*worldPtr, pos)) {
        return 10.0f;
    }

    // 其他位置：基于亮度
    f32 brightness = worldPtr->getBrightness(pos);
    return brightness - 0.5f;
}

bool TurtleEntity::_isOnSand(const IWorld& world, const BlockPos& pos)
{
    BlockPos belowPos = pos.down();
    const BlockState* belowState = world.getBlockState(belowPos);
    return belowState != nullptr && BlockTags::SAND().contains(*belowState);
}

void TurtleEntity::travel(const Vector3& travelVec)
{
    // 获取基础移动速度
    f32 baseSpeed = static_cast<f32>(attributes().getValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25));

    if (isInWater()) {
        // 水中移动
        f32 swimSpeed = baseSpeed;

        // 检查是否远离出生地超过 16 格
        if (m_hasHomePos) {
            Vector3 homePosF(static_cast<f32>(m_homePos.x) + 0.5f,
                static_cast<f32>(m_homePos.y),
                static_cast<f32>(m_homePos.z) + 0.5f);
            f32 distanceSq = position().distanceSquared(homePosF);
            if (distanceSq > 256.0f) { // 16 * 16 = 256
                // 远离出生地时速度减半，最低 0.08F
                swimSpeed = std::max(swimSpeed * 0.5f, 0.08f);
            }
        }

        // 幼体在水中速度更低
        if (isChild()) {
            // 幼体速度 = max(speed / 3.0, 0.06F)
            swimSpeed = std::max(swimSpeed / 3.0f, 0.06f);
        }

        setAIMoveSpeed(swimSpeed);

        // 水中给予轻微上升动力
        Vector3 vel = velocity();
        vel.y += 0.005;
        setVelocity(vel);
    } else if (onGround()) {
        // 陆地移动
        f32 landSpeed = std::max(baseSpeed * 0.5f, 0.06f);
        setAIMoveSpeed(landSpeed);
    } else {
        // 空中（跳跃或下落）：保持当前 AI 速度，不做额外调整
    }

    // 调用父类处理实际移动
    AnimalEntity::travel(travelVec);
}

} // namespace mc
