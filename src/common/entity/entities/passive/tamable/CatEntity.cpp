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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR THE DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "CatEntity.hpp"

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/AvoidEntityGoal.hpp"
#include "common/entity/ai/goal/goals/BreedGoal.hpp"
#include "common/entity/ai/goal/goals/FollowParentGoal.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/PanicGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/TemptGoal.hpp"
#include "common/entity/ai/goal/goals/interact/TameableGoals.hpp"
#include "common/entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "common/entity/ai/goal/goals/special/CatGoals.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/passive/basic/RabbitEntity.hpp"
#include "common/entity/entities/passive/special/TurtleEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/network/protocol/EntityEvents.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/color/DyeColor.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/IWorld.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>

namespace mc {

// ============================================================================
// 静态成员初始化
// ============================================================================

entity::DataParameter<bool> CatEntity::DATA_LYING_PARAM = entity::EntityDataManager::createKey<bool>();
entity::DataParameter<bool> CatEntity::DATA_RELAX_STATE_ONE_PARAM = entity::EntityDataManager::createKey<bool>();

// ============================================================================
// 继承链标识（复刻 vanilla ClassTreeIdRegistry，parent = TameableEntity::classInfo()）
// ============================================================================
const entity::EntityClassInfo& CatEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"CatEntity", &TameableEntity::classInfo()};
    return s_classInfo;
}

// ============================================================================
// CatTemptGoal 实现
// ============================================================================

CatEntity::CatTemptGoal::CatTemptGoal(CatEntity* cat, f64 speed, ItemPredicate itemPredicate, bool scaredByMovement)
    : TemptGoal(cat, speed, std::move(itemPredicate), scaredByMovement)
    , m_cat(cat)
{
    // 继承自 TemptGoal，重写 shouldExecute() 使其只在未驯服时执行
}

bool CatEntity::CatTemptGoal::shouldExecute()
{
    // 只有未驯服的猫才会被诱惑
    if (m_cat == nullptr || m_cat->isTamed()) {
        return false;
    }
    return TemptGoal::shouldExecute();
}

// ============================================================================
// CatAvoidPlayerGoal 实现
// ============================================================================

CatEntity::CatAvoidPlayerGoal::CatAvoidPlayerGoal(CatEntity* cat, f32 avoidDistance, f64 farSpeed, f64 nearSpeed)
    : AvoidEntityGoal(cat,
          avoidDistance,
          farSpeed,
          nearSpeed,
          // 只避开可以作为 AI 目标的玩家
          [](const LivingEntity* entity) -> bool {
              if (entity == nullptr) {
                  return false;
              }
              // 检查是否是玩家
              const Player* player = dynamic_cast<const Player*>(entity);
              if (player == nullptr) {
                  return false;
              }
              // 创造模式玩家也应该被避开
              return !player->isSpectator() && player->isAlive();
          })
    , m_cat(cat)
{
    // 继承自 AvoidEntityGoal，重写 shouldExecute() 和 shouldContinueExecuting() 使其只在未驯服时执行
}

bool CatEntity::CatAvoidPlayerGoal::shouldExecute()
{
    // 只有未驯服的猫才会避开玩家
    if (m_cat == nullptr || m_cat->isTamed()) {
        return false;
    }
    return AvoidEntityGoal::shouldExecute();
}

bool CatEntity::CatAvoidPlayerGoal::shouldContinueExecuting()
{
    // 只有未驯服的猫才会继续避开玩家
    if (m_cat == nullptr || m_cat->isTamed()) {
        return false;
    }
    return AvoidEntityGoal::shouldContinueExecuting();
}

// ============================================================================
// CatEntity 实现
// ============================================================================

CatEntity::CatEntity(EntityInstanceId id)
    : TameableEntity(id)
{
    // 随机设置皮肤类型
    setRandomCatType();

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();

    // 显式调用 registerData() 注册同步数据参数
    // 由于 C++ 虚函数在基类构造函数中不会派发到派生类（Entity::Entity 内部调用
    // registerData() 时调用的是 Entity::registerData 而非 CatEntity::registerData），
    // 必须在派生类构造函数中显式调用，参考 WolfEntity / ZombieVillagerEntity 模式。
    registerData();
}

std::unique_ptr<Entity> CatEntity::create(IWorld* /*world*/)
{
    return std::make_unique<CatEntity>(0);
}

void CatEntity::setRandomCatType()
{
    math::Random& rng = getRandom();
    m_catType = static_cast<CatType>(rng.nextInt(0, 10));
}

bool CatEntity::isTameItem(const ItemStack& itemStack) const
{
    // 猫用生鳕鱼或生鲑鱼驯服
    const Item* item = itemStack.getItem();
    if (item == nullptr) return false;
    return item == Items::COD || item == Items::SALMON;
}

bool CatEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // 驯服后用生鱼繁殖
    return isTameItem(itemStack);
}

bool CatEntity::isFoodItem(const ItemStack& itemStack) const
{
    // 同繁殖物品
    return isTameItem(itemStack);
}

std::unique_ptr<AnimalEntity> CatEntity::spawnBaby(AnimalEntity& /*partner*/)
{
    // 创建小猫
    auto baby = std::make_unique<CatEntity>(0);

    // 设置为幼体
    baby->setChild(true);

    // 设置位置（在父体位置附近）
    baby->setPosition(x(), y(), z());

    return baby;
}

void CatEntity::registerGoals()
{
    // 注意：AnimalEntity 基类不注册任何 goal，所以这里需要注册完整的 AI 目标列表

    // 优先级 0: 游泳（最高优先级）
    m_goalSelector.addGoal(0, new entity::ai::goal::SwimGoal(this));

    // 优先级 1: 恐慌逃跑（受到伤害或着火时）
    m_goalSelector.addGoal(1, new entity::ai::goal::PanicGoal(this, 1.5));

    // 优先级 1: 坐下目标（驯服后）- 与PanicGoal同优先级，但SitGoal会检查是否驯服
    m_goalSelector.addGoal(1, new entity::ai::goal::SitGoal(this));

    // 优先级 2: 繁殖（驯服后且成体）
    m_goalSelector.addGoal(2, new entity::ai::goal::BreedGoal(this, 0.8));

    // 优先级 3: 在主人身边放松（驯服后，主人睡觉时）
    m_goalSelector.addGoal(3, new entity::ai::goal::CatRelaxOnOwnerGoal(this, TEMPT_SPEED));

    // 优先级 4: 食物诱惑（生鱼用于驯服）
    // 注意：scaredByMovement = true，猫会被玩家快速移动吓跑
    m_temptGoal = new CatTemptGoal(
        this,
        TEMPT_SPEED,
        [](const ItemStack& stack) -> bool {
            const Item* item = stack.getItem();
            return item != nullptr && (item == Items::COD || item == Items::SALMON);
        },
        true); // scaredByMovement = true
    m_goalSelector.addGoal(3, m_temptGoal);

    // 优先级 4: 避开玩家（未驯服时）- 在 _setupTamedAI() 中动态添加
    // 初始时根据驯服状态添加
    _setupTamedAI();

    // 优先级 5: 躺在床上（驯服后）
    m_goalSelector.addGoal(5, new entity::ai::goal::CatLieOnBedGoal(this, 1.1));

    // 优先级 6: 跟随父母（幼体行为）
    m_goalSelector.addGoal(6, new entity::ai::goal::FollowParentGoal(this, 1.0));

    // 优先级 7: 跟随主人（驯服后）
    m_goalSelector.addGoal(7, new entity::ai::goal::FollowOwnerGoal(this, 1.0, 5.0f, 10.0f, 32.0f));

    // 优先级 10: 避水随机漫步
    m_goalSelector.addGoal(10, new entity::ai::goal::WaterAvoidingRandomWalkingGoal(this, 0.8, 1.0000001E-5f));

    // 优先级 12: 看向玩家
    m_goalSelector.addGoal(12, new entity::ai::goal::LookAtGoal(this, 8.0f));

    // 优先级 13: 随机看向
    m_goalSelector.addGoal(13, new entity::ai::goal::LookRandomlyGoal(this));

    // ===== 目标选择器 (targetSelector) =====

    // 优先级 1: 攻击兔子（未驯服时）
    m_targetSelector.addGoal(1,
        std::make_unique<entity::ai::goal::NonTamedTargetGoal<RabbitEntity>>(this,
            false)); // checkSight = false，不需要视线检查

    // 优先级 1: 攻击幼年海龟（未驯服时，仅攻击陆地上不在水中的幼体）
    m_targetSelector.addGoal(1,
        std::make_unique<entity::ai::goal::NonTamedTargetGoal<TurtleEntity>>(this,
            false, // checkSight = false
            [](const LivingEntity* entity) -> bool {
                const TurtleEntity* turtle = dynamic_cast<const TurtleEntity*>(entity);
                if (!turtle) return false;
                return turtle->isChild() && !turtle->isInWater();
            }));
}

void CatEntity::_setupTamedAI()
{
    // 动态添加/移除 AvoidPlayerGoal
    // 注意：removeGoal 会销毁目标对象（GoalSelector 拥有所有权），
    // 因此必须在移除后清空 m_avoidPlayerGoal 指针，避免悬空指针。

    // 先移除已有的 AvoidPlayerGoal
    if (m_avoidPlayerGoal != nullptr) {
        m_goalSelector.removeGoal(m_avoidPlayerGoal);
        m_avoidPlayerGoal = nullptr; // removeGoal 销毁对象后置空指针
    }

    // 如果未驯服，创建并添加避开玩家目标
    if (!isTamed()) {
        m_avoidPlayerGoal = new CatAvoidPlayerGoal(this, AVOID_DISTANCE, AVOID_FAR_SPEED, AVOID_NEAR_SPEED);
        m_goalSelector.addGoal(4, m_avoidPlayerGoal);
    }
}

void CatEntity::registerAttributes()
{
    // 调用父类方法
    TameableEntity::registerAttributes();

    // 猫的属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);

    // 与 MC 原版 LivingEntity 构造逻辑一致：构造完成后生命值应等于 maxHealth。
    // 由于 C++ 基类构造函数中虚函数 registerAttributes 不会派发到子类，MAX_HEALTH
    // 的覆盖在此处才生效，因此需在此显式同步生命值，否则 m_health 会停留在
    // 成员默认值 20.0f（与 maxHealth=10 不一致）。
    setHealth(maxHealth());
}

void CatEntity::registerData()
{
    // 先调用父类方法注册基础数据参数
    TameableEntity::registerData();

    // 标记当前正在注册 CatEntity 类的字段，使 registerParam 沿 CatEntity 继承链
    // 分配 id（续接 TameableEntity 之后）。RAII 守卫自动配对压栈/弹栈。
    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    // 注册猫特有的数据参数
    m_dataManager.registerParam(DATA_LYING_PARAM, false);
    m_dataManager.registerParam(DATA_RELAX_STATE_ONE_PARAM, false);
}

void CatEntity::tick()
{
    // 调用父类 tick
    TameableEntity::tick();

    // 更新躺下/放松动画
    _handleLieDown();
}

void CatEntity::_updateLieDownAmount()
{
    // 保存上一 tick 的值（用于插值）
    m_prevLieDownAmount = m_lieDownAmount;
    m_prevLieDownAmountTail = m_lieDownAmountTail;

    if (isLieDown()) {
        m_lieDownAmount = std::min(m_lieDownAmount + LIE_DOWN_AMOUNT_INCREASE, 1.0f);
        m_lieDownAmountTail = std::min(m_lieDownAmountTail + LIE_DOWN_TAIL_INCREASE, 1.0f);
    } else {
        m_lieDownAmount = std::max(m_lieDownAmount - LIE_DOWN_AMOUNT_DECREASE, 0.0f);
        m_lieDownAmountTail = std::max(m_lieDownAmountTail - LIE_DOWN_TAIL_DECREASE, 0.0f);
    }
}

void CatEntity::_updateRelaxStateOneAmount()
{
    // 保存上一 tick 的值（用于插值）
    m_prevRelaxStateOneAmount = m_relaxStateOneAmount;

    if (isRelaxStateOne()) {
        m_relaxStateOneAmount = std::min(m_relaxStateOneAmount + RELAX_AMOUNT_INCREASE, 1.0f);
    } else {
        m_relaxStateOneAmount = std::max(m_relaxStateOneAmount - RELAX_AMOUNT_DECREASE, 0.0f);
    }
}

void CatEntity::_handleLieDown()
{
    // 躺下或放松时播放呼噜声
    if ((isLieDown() || isRelaxStateOne()) && ticksExisted() % 5 == 0) {
        playSound(
            SoundEvents::ENTITY_CAT_PURR, 1.0f, 0.6f + 0.4f * (getRandom().nextFloat() - getRandom().nextFloat()));
    }

    // 更新动画进度
    _updateLieDownAmount();
    _updateRelaxStateOneAmount();

    // 检查是否躺在睡眠玩家上方
    m_lyingOnTopOfSleepingPlayer = false;
    if (isLieDown() && m_world != nullptr) {
        // 搜索附近2格内的睡眠玩家
        BlockPos catBlockPos(
            static_cast<i32>(std::floor(x())), static_cast<i32>(std::floor(y())), static_cast<i32>(std::floor(z())));
        AxisAlignedBB searchBox = boundingBox().grow(2.0f);
        auto entities = m_world->getEntitiesInAABB(searchBox);
        for (auto* entity : entities) {
            if (entity == nullptr) {
                continue;
            }
            auto* player = dynamic_cast<Player*>(entity);
            if (player != nullptr && player->isSleeping()) {
                m_lyingOnTopOfSleepingPlayer = true;
                break;
            }
        }
    }
}

void CatEntity::onTamed(bool tamed)
{
    if (tamed) {
        // 驯服后增加生命值
        m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
        setHealth(10.0f);

        // 礼物计时器初始化
        m_giftTimer = GIFT_INTERVAL;
    } else {
        m_giftTimer = 0;
    }

    // 更新 AI（添加/移除 AvoidPlayerGoal）
    _setupTamedAI();
}

std::optional<ResourceLocation> CatEntity::getAmbientSound() const
{
    // 驯服后的猫使用 ENTITY_CAT_AMBIENT
    // 未驯服的流浪猫使用 ENTITY_CAT_STRAY_AMBIENT
    if (isTamed()) {
        return SoundEvents::ENTITY_CAT_AMBIENT;
    }
    return SoundEvents::ENTITY_CAT_STRAY_AMBIENT;
}

std::optional<ResourceLocation> CatEntity::getHurtSound(DamageSource& /*source*/) const
{
    return SoundEvents::ENTITY_CAT_HURT;
}

std::optional<ResourceLocation> CatEntity::getDeathSound() const
{
    return SoundEvents::ENTITY_CAT_DEATH;
}

void CatEntity::playEatSound()
{
    playSound(SoundEvents::ENTITY_CAT_EAT, 1.0f, 1.0f);
}

void CatEntity::hiss()
{
    playSound(SoundEvents::ENTITY_CAT_HISS, 1.0f, 1.0f + (getRandom().nextFloat() - getRandom().nextFloat()) * 0.2f);
}

// ============================================================================
// interactMob 实现
// ============================================================================

ActionResultType CatEntity::interactMob(Player& player, Hand hand)
{
    ItemStack& itemStack = player.getHeldItem(hand);
    const Item* item = itemStack.getItem();

    if (isTamed()) {
        // ========== 已驯服的猫 ==========

        // 仅主人可以交互
        if (isOwner(player.uuidBytes())) {
            // 优先级1: 项圈染色（染料 + 颜色不同）
            auto dyeColor = _getDyeColorFromItem(item);
            if (dyeColor.has_value() && dyeColor.value() != m_collarColor) {
                if (!player.abilities().creativeMode) {
                    itemStack.shrink(1);
                }
                setCollarColor(dyeColor.value());
                return ActionResultType::Success;
            }

            // 优先级2: 喂食治疗（猫食 + 生命值未满）
            if (isFoodItem(itemStack) && health() < maxHealth()) {
                if (!player.abilities().creativeMode) {
                    itemStack.shrink(1);
                }
                heal(FOOD_HEAL_AMOUNT);

                // 播放吃东西声音
                if (!isSilent()) {
                    playEatSound();
                }

                return ActionResultType::Success;
            }

            // 优先级3: 调用父类处理（繁殖/成长），若父类未处理则切换坐下/站起
            ActionResultType result = TameableEntity::interactMob(player, hand);
            if (result != ActionResultType::Success && result != ActionResultType::Consume) {
                // 父类未处理，切换坐下/站起状态
                setSitting(!isSitting());
                clearNavigation();
                return ActionResultType::Success;
            }
            return result;
        }
    } else {
        // ========== 未驯服的猫 ==========

        // 手持猫食（生鳕鱼/生鲑鱼）时尝试驯服
        if (isFoodItem(itemStack)) {
            if (!player.abilities().creativeMode) {
                itemStack.shrink(1);
            }

            // 服务端处理驯服逻辑
            if (m_world != nullptr && !m_world->isClientSide()) {
                _tryToTame(player);
            }

            // 播放吃东西声音
            if (!isSilent()) {
                playEatSound();
            }

            return ActionResultType::Success;
        }
    }

    // 其他情况交给父类处理
    return TameableEntity::interactMob(player, hand);
}

// ============================================================================
// _tryToTame 实现
// ============================================================================

void CatEntity::_tryToTame(Player& player)
{
    // 1/3 概率驯服成功
    if (getRandom().nextInt(3) == 0) {
        // 驯服成功
        setTamed(true);
        setOwnerId(player.uuidBytes());
        setSitting(true);
        clearNavigation();

        // 通知世界触发进度检测
        m_world->onTameAnimal(player.playerId(), this);

        // 广播驯服成功状态（心形粒子）
        m_world->broadcastEntityStatus(id(), static_cast<u8>(network::EntityStatus::TamingSucceeded));
    } else {
        // 驯服失败，广播烟雾粒子
        m_world->broadcastEntityStatus(id(), static_cast<u8>(network::EntityStatus::TamingFailed));
    }
}

// ============================================================================
// _getDyeColorFromItem 实现
// ============================================================================

std::optional<DyeColor> CatEntity::_getDyeColorFromItem(const Item* item)
{
    if (item == nullptr) {
        return std::nullopt;
    }

    static const std::unordered_map<const Item*, DyeColor> dyeMap = {
        {Items::INK_SAC, DyeColor::Black},
        {Items::RED_DYE, DyeColor::Red},
        {Items::GREEN_DYE, DyeColor::Green},
        {Items::COCOA_BEANS, DyeColor::Brown},
        {Items::LAPIS_LAZULI_DYE, DyeColor::Blue},
        {Items::PURPLE_DYE, DyeColor::Purple},
        {Items::CYAN_DYE, DyeColor::Cyan},
        {Items::LIGHT_GRAY_DYE, DyeColor::LightGray},
        {Items::GRAY_DYE, DyeColor::Gray},
        {Items::PINK_DYE, DyeColor::Pink},
        {Items::LIME_DYE, DyeColor::Lime},
        {Items::YELLOW_DYE, DyeColor::Yellow},
        {Items::LIGHT_BLUE_DYE, DyeColor::LightBlue},
        {Items::MAGENTA_DYE, DyeColor::Magenta},
        {Items::ORANGE_DYE, DyeColor::Orange},
        {Items::WHITE_DYE, DyeColor::White},
        {Items::BONE_MEAL, DyeColor::White},
    };

    auto it = dyeMap.find(item);
    if (it != dyeMap.end()) {
        return it->second;
    }
    return std::nullopt;
}

// ============================================================================
// NBT 序列化
// ============================================================================

void CatEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    using namespace mc::entity::serialization;

    // 先调用基类实现（序列化驯服状态、坐下状态、主人、愤怒等）
    TameableEntity::addAdditionalSaveData(tag);

    // CatType (i32) - 猫皮肤类型
    tag.put(nbt_keys::CAT_TYPE, static_cast<i32>(m_catType));

    // CollarColor (i32) - 项圈颜色（仅在非默认颜色时写入）
    if (m_collarColor != DyeColor::Red) {
        tag.put(nbt_keys::COLLAR_COLOR, static_cast<i32>(m_collarColor));
    }
}

Result<void> CatEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    using namespace mc::entity::serialization;

    // 先调用基类实现（反序列化驯服状态、坐下状态、主人、愤怒等）
    MC_TRY(TameableEntity::readAdditionalSaveData(tag));

    // CatType (i32) - 猫皮肤类型
    if (auto val = nbt_helper::tryGetInt(tag, nbt_keys::CAT_TYPE)) {
        i32 typeValue = *val;
        if (typeValue >= 0 && typeValue <= 10) {
            m_catType = static_cast<CatType>(typeValue);
        }
    }

    // CollarColor (i32) - 项圈颜色
    if (auto val = nbt_helper::tryGetInt(tag, nbt_keys::COLLAR_COLOR)) {
        i32 colorValue = *val;
        if (colorValue >= 0 && colorValue < static_cast<i32>(DyeColor::Count)) {
            m_collarColor = static_cast<DyeColor>(colorValue);
        }
    }

    return Result<void>::ok();
}

} // namespace mc
