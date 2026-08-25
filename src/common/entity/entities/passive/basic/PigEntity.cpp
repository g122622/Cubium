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

#include "PigEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/goal/goals/BreedGoal.hpp"
#include "common/entity/ai/goal/goals/FollowParentGoal.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp" // 包含 LookRandomlyGoal
#include "common/entity/ai/goal/goals/PanicGoal.hpp"
#include "common/entity/ai/goal/goals/RandomWalkingGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/TemptGoal.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/AgeableEntity.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include <memory>
#include <optional>

namespace mc {

std::unique_ptr<Entity> PigEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    // 使用临时ID 0，实际ID由 EntityManager 分配
    return std::make_unique<PigEntity>(0, registry);
}

PigEntity::PigEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AnimalEntity(id, registry)
{
    // 注册 AI 目标
    registerGoals();

    // 补调 registerAttributes：AnimalEntity 构造只调基类版（vtable 指向 AnimalEntity），派生 override
    // 永不执行，须在派生类构造显式调用。
    registerAttributes();
}

std::optional<ResourceLocation> PigEntity::getAmbientSound() const
{
    return makeSoundEventId("ambient");
}

std::optional<ResourceLocation> PigEntity::getHurtSound(DamageSource& /*source*/) const
{
    return makeSoundEventId("hurt");
}

std::optional<ResourceLocation> PigEntity::getDeathSound() const
{
    return makeSoundEventId("death");
}

bool PigEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // 猪用胡萝卜、马铃薯、甜菜根繁殖
    const Item* item = itemStack.getItem();
    if (item == nullptr) return false;
    return item == Items::CARROT || item == Items::POTATO || item == Items::BEETROOT;
}

bool PigEntity::canMateWith(const AnimalEntity& other) const
{
    // 检查是否是同种类（猪）
    return dynamic_cast<const PigEntity*>(&other) != nullptr && AnimalEntity::canMateWith(other);
}

std::unique_ptr<AnimalEntity> PigEntity::spawnBaby(AnimalEntity& /*partner*/)
{
    // ECS 迁移：实体构造需要 registry 句柄，ClientWorld 返回 nullptr 表客户端不接入 ECS
    auto* registry = &ecsRegistry();
    if (registry == nullptr) {
        return nullptr;
    }

    // 创建小猪
    auto baby = std::make_unique<PigEntity>(0, *registry);

    // 设置为幼体
    baby->setChild(true);

    // 设置位置
    baby->setPosition(x(), y(), z());

    return baby;
}

// ========== 玩家交互 ==========

ActionResultType PigEntity::interactMob(Player& player, Hand hand)
{
    // 对齐 Java 1.21.11 Pig.mobInteract(Player, InteractionHand)：
    //   boolean flag = isFood(itemInHand);
    //   if (!flag && isSaddled() && !isVehicle() && !player.isSecondaryUseActive()) {
    //       if (!level().isClientSide()) player.startRiding(this);
    //       return SUCCESS;
    //   } else {
    //       InteractionResult r = super.mobInteract(...);          // AnimalEntity 喂食/繁殖
    //       if (!r.consumesAction()) {
    //           ItemStack itemstack = getItemInHand(hand);
    //           return isEquippableInSlot(itemstack, SADDLE)
    //               ? itemstack.interactLivingEntity(player, this, hand)   // 装鞍
    //               : PASS;
    //       }
    //       return r;
    //   }
    //
    // 此前 Cubium PigEntity 无 interactMob override（落入 AnimalEntity::interactMob 仅处理喂食），
    // 空手右键配鞍猪返 Pass 不骑乘（对齐缺陷）。本次补全实体侧骑乘入口，与 StriderEntity 同构。
    // 装鞍不在实体侧直接做：手持鞍时返 Pass，由 Player::interactOn 第4步调
    // SaddleItem::itemInteractionForEntity 装鞍（与 StriderEntity 一致）。
    ItemStack& heldItem = player.getHeldItem(hand);
    const bool isFood = isBreedingItem(heldItem);

    // 骑乘分支：非食物 + 已配鞍 + 无乘客 + 玩家未蹲下 → 玩家骑上猪。
    // isVehicle() 用 getPassengers().empty() 等价（Cubium 无 isVehicle()，StriderEntity 同款判断）。
    // isSecondaryUseActive 对应 Cubium player.isSneaking()（蹲下时不骑乘，用于装鞍等副操作）。
    if (!isFood && hasSaddle() && getPassengers().empty() && !player.isSneaking()) {
        if (m_world != nullptr && !m_world->isClientSide()) {
            player.startRiding(*this);
        }
        return ActionResultType::Success;
    }

    // 委托父类 AnimalEntity::interactMob 处理喂食/繁殖（食物分支）。
    const ActionResultType result = AnimalEntity::interactMob(player, hand);
    if (result != ActionResultType::Pass) {
        return result;
    }

    // 父类未处理（Pass）：手持鞍 → 返 Pass，由 Player::interactOn 第4步走
    // SaddleItem::itemInteractionForEntity 装鞍（canEquip 判定鞍槽可装备）。
    if (canEquip(heldItem, 0)) {
        return ActionResultType::Pass;
    }
    return ActionResultType::Pass;
}

// ========== IRideable 接口实现 ==========

void PigEntity::onPlayerStartRiding(Player* /*player*/)
{
    // 当玩家开始骑乘时，可添加骑乘音效或动画触发
}

void PigEntity::onPlayerStopRiding(Player* /*player*/)
{
    // 停止骑乘时重置加速状态
    m_boostHelper.saddledRaw = false;
    m_boostHelper.field_233611_b_ = 0;
}

f32 PigEntity::getSteeringSpeed() const
{
    // 骑乘速度 = 基础速度 * 0.225
    f32 baseSpeed = static_cast<f32>(attributes().getValue(entity::attribute::Attributes::MOVEMENT_SPEED));
    return baseSpeed * MOUNTED_SPEED_MULT;
}

bool PigEntity::boost()
{
    // 使用 BoostHelper 进行加速
    math::Random& rng = getRandom();
    return m_boostHelper.boost(rng);
}

bool PigEntity::canBeSteered() const
{
    // 必须有鞍才能被控制
    if (!hasSaddle()) {
        return false;
    }

    // 获取控制乘客
    const auto& passengers = getPassengers();
    if (passengers.empty()) {
        return false;
    }

    // 使用 const_cast 来获取非 const 世界指针
    IWorld* nonConstWorld = const_cast<IWorld*>(world());
    if (nonConstWorld == nullptr) {
        return false;
    }

    Entity* passenger = nonConstWorld->getEntity(passengers[0]);
    if (passenger == nullptr) {
        return false;
    }

    // 检查是否是玩家
    Player* player = dynamic_cast<Player*>(passenger);
    if (player == nullptr) {
        return false;
    }

    // 检查玩家主手或副手是否持有胡萝卜钓竿
    const ItemStack& mainHand = player->getHeldItem(Hand::MainHand);
    const ItemStack& offHand = player->getHeldItem(Hand::OffHand);

    // 检查主手
    if (!mainHand.isEmpty() && mainHand.getItem() == Items::CARROT_ON_A_STICK) {
        return true;
    }

    // 检查副手
    if (!offHand.isEmpty() && offHand.getItem() == Items::CARROT_ON_A_STICK) {
        return true;
    }

    return false;
}

void PigEntity::travelTowards(const Vector3& travelVec)
{
    // 调用父类的 travel 方法处理移动
    AnimalEntity::travel(travelVec);
}

void PigEntity::travel(const Vector3& travelVec)
{
    // 设置 AI 移动速度
    f32 moveSpeed = static_cast<f32>(attributes().getValue(entity::attribute::Attributes::MOVEMENT_SPEED));
    setAIMoveSpeed(moveSpeed);

    // 调用 IRideable::ride() 处理骑乘移动
    ride(*this, m_boostHelper, travelVec);
}

void PigEntity::tick()
{
    // 调用父类的tick
    AnimalEntity::tick();

    // 更新加速计时
    m_boostHelper.tick();
}

void PigEntity::registerGoals()
{
    // 调用父类方法
    AgeableEntity::registerGoals();

    // 注意：AnimalEntity 基类不注册任何 goal，所以这里需要注册完整的 AI 目标列表
    // 注意：PigEntity 是多重继承（AnimalEntity + IRideable），需要显式转换为对应的基类类型

    // 优先级 0: 游泳（最高优先级）- SwimGoal 需要 MobEntity*
    m_goalSelector.addGoal(0, new entity::ai::goal::SwimGoal(this));

    // 优先级 1: 恐慌逃跑（受到伤害或着火时）- PanicGoal 需要 CreatureEntity*
    m_goalSelector.addGoal(1, new entity::ai::goal::PanicGoal(this, 1.5));

    // 优先级 2: 繁殖（当处于爱心状态时）- BreedGoal 需要 AnimalEntity*
    m_goalSelector.addGoal(2, new entity::ai::goal::BreedGoal(this, 1.0));

    // 优先级 3: 食物诱惑（胡萝卜、马铃薯、甜菜根）- TemptGoal 需要 CreatureEntity*
    m_goalSelector.addGoal(3,
        std::make_unique<::mc::entity::ai::goal::TemptGoal>(
            this,
            1.2,
            [](const ItemStack& stack) -> bool {
                const Item* item = stack.getItem();
                return item != nullptr && (item == Items::CARROT || item == Items::POTATO || item == Items::BEETROOT);
            },
            false)); // scaredByMovement = false

    // 优先级 4: 跟随父母（幼体行为）- FollowParentGoal 需要 AnimalEntity*
    m_goalSelector.addGoal(4, new entity::ai::goal::FollowParentGoal(this, 1.1));

    // 优先级 5: 随机漫步 - RandomWalkingGoal 需要 CreatureEntity*
    m_goalSelector.addGoal(5, new entity::ai::goal::RandomWalkingGoal(this, 1.0));

    // 优先级 6: 看向玩家 - LookAtGoal 需要 MobEntity*
    m_goalSelector.addGoal(6, new entity::ai::goal::LookAtGoal(this, 8.0f));

    // 优先级 7: 随机看向 - LookRandomlyGoal 需要 MobEntity*
    m_goalSelector.addGoal(7, new entity::ai::goal::LookRandomlyGoal(this));
}

void PigEntity::registerAttributes()
{
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // 设置猪的基础移动速度
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, PIG_SPEED);
}

void PigEntity::die(DamageSource& cause)
{
    // 先调用父类 die()
    AnimalEntity::die(cause);

    // 如果有鞍，掉落鞍物品
    if (hasSaddle() && m_world != nullptr && !m_world->isClientSide()) {
        // 使用 ItemDropHelper 在实体位置生成鞍物品
        ItemStack saddle(Items::SADDLE, 1);
        math::Random& rng = getRandom();
        ItemDropHelper::spawnItemAtEntity(this, saddle, 0.0f, rng);
    }
}

// ========== IEquipable 接口实现 ==========

ItemStack PigEntity::getEquipment(i32 slot) const
{
    // 猪只有一个鞍槽
    if (slot != 0) {
        return ItemStack::EMPTY;
    }

    // 当有鞍时返回一个鞍物品堆
    if (hasSaddle()) {
        return ItemStack(Items::SADDLE, 1);
    }

    return ItemStack::EMPTY;
}

void PigEntity::setEquipment(i32 slot, const ItemStack& item)
{
    // 猪只有一个鞍槽
    if (slot != 0) {
        return;
    }

    // 设置鞍状态（猪不存储实际的物品，只存储布尔值）
    bool isSaddle = !item.isEmpty() && item.getItem() == Items::SADDLE;
    setSaddle(isSaddle);
}

bool PigEntity::canEquip(const ItemStack& item, i32 slot) const
{
    // 槽位边界检查先于物品判断：猪只有槽位 0
    if (slot != 0) {
        return false;
    }

    // 空物品可以清空槽位
    if (item.isEmpty()) {
        return true;
    }

    // 检查是否是鞍物品
    return item.getItem() == Items::SADDLE;
}

// ========== 雷击 ==========

void PigEntity::onStruckByLightning(entity::LightningBoltEntity* lightning)
{
    // 客户端不执行实体转化逻辑
    if (m_world == nullptr || m_world->isClientSide()) {
        return;
    }

    // 对齐 vanilla Pig#thunderHit（Pig.java:176-188）：
    //   if (level.getDifficulty() != PEACEFUL) {
    //       ZombifiedPiglin z = convertTo(ZOMBIFIED_PIGLIN, ..., p -> {...setPersistenceRequired...});
    //       if (z == null) super.thunderHit(level, lightning);   // 转化失败回退基类（受5+引燃）
    //   } else {
    //       super.thunderHit(level, lightning);                  // 和平难度也调基类受5伤害
    //   }
    // 注意：vanilla 转化成功时不调 super（转化体不受伤），原体经 convertTo 内部 discard。
    // Cubium 基类 Entity::onStruckByLightning 默认 hurt(5)+引燃，此处仅在转化失败/和平分支调基类。
    if (m_world->difficulty() == Difficulty::Peaceful) {
        AnimalEntity::onStruckByLightning(lightning);
        return;
    }

    // ECS 迁移：实体构造需要 registry 句柄，ClientWorld 返回 nullptr 表客户端不接入 ECS
    auto* registry = m_world->entityRegistry();
    if (registry == nullptr) {
        // 无 registry 无法构造转化体，回退基类受 5 伤害（vanilla convertTo 失败同语义）
        AnimalEntity::onStruckByLightning(lightning);
        return;
    }

    // 经 EntityType 工厂创建僵尸猪灵，避免本目录反向依赖 monster/nether 目录
    const entity::EntityType* type =
        entity::EntityRegistry::instance().getType(entity::EntityTypeKeys::ZOMBIFIED_PIGLIN);
    if (type == nullptr) {
        // 转化实体未注册，回退基类受 5 伤害（vanilla convertTo 返 null 同语义）
        AnimalEntity::onStruckByLightning(lightning);
        return;
    }
    auto piglin = type->create(m_world, *registry);
    if (piglin == nullptr) {
        // 转化实体构造失败，回退基类受 5 伤害
        AnimalEntity::onStruckByLightning(lightning);
        return;
    }

    // 继承位置与朝向
    piglin->setPosition(x(), y(), z());
    piglin->setRotation(yaw(), pitch());

    // 继承自定义名
    if (hasCustomName()) {
        piglin->setCustomName(customNameText());
        piglin->setCustomNameVisible(isCustomNameVisible());
    }

    // 闪电转化的僵尸猪灵需持久化留存
    MobEntity* piglinMob = dynamic_cast<MobEntity*>(piglin.get());
    if (piglinMob != nullptr) {
        piglinMob->enablePersistence();
    }

    // 生成僵尸猪灵并移除原猪（对齐 vanilla convertTo 成功：不调 super、discard 原体）
    m_world->spawnEntity(std::move(piglin));
    discard();
}

} // namespace mc
