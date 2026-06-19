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

#include "MobEntity.hpp"
#include "../../core/Types.hpp"
#include "../../item/Items.hpp"
#include "../../item/core/ActionResult.hpp"
#include "../../item/core/Item.hpp"
#include "../../item/core/ItemStack.hpp"
#include "../../item/enchantment/EnchantmentHelper.hpp"
#include "../../item/enchantment/enchantments/AllEnchantments.hpp"
#include "../../item/items/special/NameTagItem.hpp"
#include "../../item/items/special/SpawnEggItem.hpp"
#include "../../util/math/MathUtils.hpp"
#include "../../util/math/random/Random.hpp"
#include "../../world/IWorld.hpp"
#include "../ai/EntitySenses.hpp"
#include "../ai/controller/JumpController.hpp"
#include "../ai/controller/LookController.hpp"
#include "../ai/controller/MovementController.hpp"
#include "../ai/pathfinding/PathNavigator.hpp"
#include "../attribute/Attributes.hpp"
#include "../combat/DifficultyHelper.hpp"
#include "../combat/PlayerAttackHelper.hpp"
#include "../core/AgeableEntity.hpp"
#include "../damage/DamageSource.hpp"
#include "../entities/player/Player.hpp"
#include "../entities/vehicle/BoatEntity.hpp"
#include "../experience/ExperienceDropHandler.hpp"
#include "../interfaces/IMob.hpp"
#include "../serialization/EntityNbtKeys.hpp"
#include "../serialization/NbtHelper.hpp"
#include "EntityRegistry.hpp"
#include "EntitySpawnPlacementRegistry.hpp"
#include "EntityTypeIdNumber.hpp"

namespace mc {

MobEntity::MobEntity(EntityId id)
    : LivingEntity(id)
    , m_lookController(std::make_unique<entity::ai::controller::LookController>(this))
    , m_moveController(std::make_unique<entity::ai::controller::MovementController>(this))
    , m_jumpController(std::make_unique<entity::ai::controller::JumpController>(this))
    , m_senses(std::make_unique<entity::ai::EntitySenses>(this))
    , m_navigator(std::make_unique<entity::ai::pathfinding::PathNavigator>(this))
{
    // 初始化装备掉落概率为默认值
    m_equipmentDropChances.fill(DEFAULT_EQUIPMENT_DROP_CHANCE);
}

MobEntity::~MobEntity() = default;

void MobEntity::registerAttributes()
{
    // 在 LivingEntity 基础上注册和设置属性
    LivingEntity::registerAttributes();

    // 注册并设置跟随范围，默认值为 16.0
    m_attributes.registerAttribute(*entity::attribute::Attributes::followRange());
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 16.0);
}

entity::ai::controller::LookController* MobEntity::lookController()
{
    return m_lookController.get();
}

const entity::ai::controller::LookController* MobEntity::lookController() const
{
    return m_lookController.get();
}

entity::ai::controller::MovementController* MobEntity::moveController()
{
    return m_moveController.get();
}

const entity::ai::controller::MovementController* MobEntity::moveController() const
{
    return m_moveController.get();
}

entity::ai::controller::JumpController* MobEntity::jumpController()
{
    return m_jumpController.get();
}

const entity::ai::controller::JumpController* MobEntity::jumpController() const
{
    return m_jumpController.get();
}

entity::ai::pathfinding::PathNavigator* MobEntity::navigator()
{
    return m_navigator.get();
}

const entity::ai::pathfinding::PathNavigator* MobEntity::navigator() const
{
    return m_navigator.get();
}

entity::ai::EntitySenses* MobEntity::senses()
{
    return m_senses.get();
}

const entity::ai::EntitySenses* MobEntity::senses() const
{
    return m_senses.get();
}

math::Random MobEntity::getRandom() const
{
    // 基于实体ID和tick生成随机数种子
    return math::Random(static_cast<u64>(m_id) | (static_cast<u64>(m_ticksExisted) << 32));
}

bool MobEntity::isBeingRidden() const
{
    return hasPassengers();
}

void MobEntity::clearNavigation()
{
    if (m_navigator) {
        m_navigator->clearPath();
    }
}

void MobEntity::playAmbientSound()
{
    auto soundEvent = getAmbientSound();
    if (soundEvent.has_value()) {
        playSound(*soundEvent, getSoundVolume(), getSoundPitch());
    }
}

void MobEntity::playAttackSound(LivingEntity& target)
{
    (void)target;
}

void MobEntity::lookAt(const Entity& target, f32 deltaYaw, f32 deltaPitch)
{
    lookAt(target.x(), target.y() + target.eyeHeight(), target.z(), deltaYaw, deltaPitch);
}

void MobEntity::lookAt(f64 x, f64 y, f64 z, f32 deltaYaw, f32 deltaPitch)
{
    if (m_lookController) {
        m_lookController->setLookPosition(x, y, z, deltaYaw, deltaPitch);
    }
}

void MobEntity::tick()
{
    // 更新父类（LivingEntity::tick() 已经调用 aiStep()）
    LivingEntity::tick();

    // 空闲时间在 tick 开头递增
    ++m_idleTime;

    // 环境声音检查
    if (isAlive()) {
        math::Random random = getRandom();
        if (random.nextInt(1000) < m_livingSoundTime++) {
            m_livingSoundTime = -getTalkInterval();
            playAmbientSound();
        }
    }

    // updateEntityActionState() 顺序:
    // 1. 感知更新
    if (m_senses) {
        m_senses->tick();
    }

    // 2. 目标选择器 (先于 goalSelector)
    // 3. 行为目标选择器
    if (m_aiEnabled) {
        m_targetSelector.tick();
        m_goalSelector.tick();

        // 4. 导航器更新
        if (m_navigator) {
            m_navigator->tick();
        }

        // 5. AI 任务更新 (子类可重写)
        updateAITasks();

        // 每 5 tick 更新移动目标标志
        if (m_ticksExisted % 5 == 0) {
            updateMovementGoalFlags();
        }
    }

    // 6. 控制器更新 (顺序: move -> look -> jump)
    if (m_moveController) {
        m_moveController->tick();
    }
    if (m_lookController) {
        m_lookController->tick();
    }
    if (m_jumpController) {
        m_jumpController->tick();
    }

    // 注意：aiStep() 已在 LivingEntity::tick() 中调用，这里不需要再次调用
}

void MobEntity::updateMovementGoalFlags()
{
    // 根据骑乘状态更新目标标志
    // 如果被骑乘，禁用 MOVE/JUMP/LOOK 标志
    bool canMove = !isBeingRidden();

    m_goalSelector.setFlag(entity::ai::GoalFlag::Move, canMove);
    m_goalSelector.setFlag(entity::ai::GoalFlag::Jump, canMove);
    m_goalSelector.setFlag(entity::ai::GoalFlag::Look, canMove);
}

std::optional<ResourceLocation> MobEntity::getAmbientSound() const
{
    return makeSoundEventId("ambient");
}

void MobEntity::playHurtSound(DamageSource& source)
{
    m_livingSoundTime = -getTalkInterval();
    LivingEntity::playHurtSound(source);
}

void MobEntity::dropExperience()
{
    // 如果有经验值，生成经验球
    if (m_experienceValue > 0 && m_world) {
        math::Random rng = getRandom();
        entity::ExperienceDropHandler::spawnHostileMobExperience(m_world, x(), y(), z(), m_experienceValue, &rng);
    }
}

bool MobEntity::isInDaylight() const
{
    // 检查条件：
    // 1. 不在客户端
    // 2. 世界为白天 (isDaytime)
    // 3. 亮度 > 0.5
    // 4. 随机检查（亮度越高概率越大）
    // 5. 不在水中或雨中
    // 6. 天空可见 (canSeeSky)

    if (m_world == nullptr || m_world->isClientSide()) {
        return false;
    }

    // dayTime < 12000 为白天
    if (!m_world->isDaytime()) {
        return false;
    }

    // getBrightness() > 0.5F
    f32 brightness = getBrightness();
    if (brightness <= 0.5f) {
        return false;
    }

    // 随机检查，亮度越高越容易触发
    math::Random rng = getRandom();
    f32 randomCheck = rng.nextFloat() * 30.0f;
    f32 brightnessThreshold = (brightness - 0.4f) * 2.0f;
    if (randomCheck >= brightnessThreshold) {
        return false;
    }

    // 在水中或雨中时不燃烧
    if (isWet()) {
        return false;
    }

    // 获取检测位置
    // 如果骑乘船，检测位置向上偏移一格
    BlockPos pos(
        static_cast<i32>(std::floor(x())), static_cast<i32>(std::round(y())), static_cast<i32>(std::floor(z())));

    // 如果实体骑乘船，检测位置向上偏移一格
    // 原因：船在水面上，生物坐在船中位置较低，需要向上偏移才能正确检测天空可见性
    if (isRiding()) {
        EntityId vehicleId = getVehicle();
        if (vehicleId != INVALID_ENTITY_ID && m_world != nullptr) {
            const Entity* vehicle = m_world->getEntity(vehicleId);
            if (vehicle != nullptr && dynamic_cast<const entity::BoatEntity*>(vehicle) != nullptr) {
                pos = pos.up();
            }
        }
    }

    return m_world->canSeeSky(pos);
}

void MobEntity::burnUndead()
{
    if (!isAlive() || !isInDaylight()) {
        return;
    }

    // 获取防护槽位中的物品
    EquipmentSlot protectionSlot = sunProtectionSlot();
    ItemStack& protectionItem = m_equipment[static_cast<size_t>(protectionSlot)];

    if (!protectionItem.isEmpty()) {
        // 防护槽位有物品：如果物品可损坏，则物品承受耐久损耗
        // 注意：此处直接增加伤害值，绕过耐久保护附魔，与 MC 原版行为一致
        if (protectionItem.isDamageable()) {
            math::Random rng = getRandom();
            i32 addedDamage = rng.nextInt(2); // 0 或 1
            if (addedDamage > 0) {
                i32 newDamage = protectionItem.getDamage() + addedDamage;
                protectionItem.setDamage(newDamage);
            }
        }
        // 如果物品不可损坏（如附魔绑定/无限耐久），实体也不会燃烧
    } else {
        // 防护槽位为空：实体被点燃 8 秒
        setFire(8);
    }
}

bool MobEntity::canAttackType(entity::EntityTypeId typeId) const
{
    // 对应 MC 原版 Mob.canAttackType()
    // MC 原版基类排除恶魂：return p_21399_ != EntityType.GHAST;
    // 恶魂悬浮在下界高空，大多数近战型 Mob 无法接近恶魂，
    // 将恶魂排除在攻击目标之外可以避免 Mob 徒劳地试图攻击一个它们够不着的敌人
    return typeId != entity::EntityTypeIdNumber::GHAST;
}

bool MobEntity::attackEntityAsMob(LivingEntity& target)
{
    // 1. 获取攻击伤害属性
    f32 attackDamage = static_cast<f32>(getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE, 1.0));

    // 2. 获取击退强度属性
    f32 knockbackStrength = static_cast<f32>(getAttributeValue(entity::attribute::Attributes::ATTACK_KNOCKBACK, 0.0));

    // 3. 如果目标有武器，应用附魔伤害加成和击退附魔
    // 获取主手武器
    const ItemStack& mainHand = getMainHandItem();

    if (!mainHand.isEmpty()) {
        // 附魔伤害加成（锋利、亡灵杀手、节肢杀手）
        // 使用 PlayerAttackHelper::getEnchantmentDamageBonus 计算附魔伤害
        attackDamage +=
            entity::combat::PlayerAttackHelper::getEnchantmentDamageBonus(mainHand, target.getCreatureAttribute());

        // 击退附魔
        i32 knockbackLevel =
            item::enchant::EnchantmentHelper::getEnchantmentLevel(mainHand, &item::enchant::AllEnchantments::KNOCKBACK);
        if (knockbackLevel > 0) {
            knockbackStrength += static_cast<f32>(knockbackLevel) * 0.5f;
        }
    }

    // 4. 火焰附加（在攻击前应用）
    i32 fireAspectLevel = 0;
    if (!mainHand.isEmpty()) {
        fireAspectLevel = item::enchant::EnchantmentHelper::getEnchantmentLevel(
            mainHand, &item::enchant::AllEnchantments::FIRE_ASPECT);
    }

    // 5. 创建伤害来源并应用伤害
    EntityDamageSource damageSource = DamageSources::mobAttack(this);

    // 如果有火焰附加，在攻击前点燃目标 1 秒（用于燃烧传递）
    // setFire 接收 ticks，1 秒 = 20 ticks
    if (fireAspectLevel > 0) {
        target.setFire(20); // 1 秒 = 20 ticks
    }

    bool attacked = target.hurt(damageSource, attackDamage);

    if (attacked) {
        // 6. 应用击退
        if (knockbackStrength > 0.0f) {
            // 计算击退方向
            f64 ratioX = static_cast<f64>(position().x - target.position().x);
            f64 ratioZ = static_cast<f64>(position().z - target.position().z);

            // 归一化方向向量
            f64 length = std::sqrt(ratioX * ratioX + ratioZ * ratioZ);
            if (length > 0.0) {
                ratioX /= length;
                ratioZ /= length;

                // 击退受击退抗性影响
                knockbackStrength = static_cast<f32>(static_cast<f64>(knockbackStrength) *
                    (1.0 - target.getAttributeValue(entity::attribute::Attributes::KNOCKBACK_RESISTANCE, 0.0)));

                if (knockbackStrength > 0.0f) {
                    // LivingEntity.applyKnockback()
                    Vector3 velocity = target.velocity();

                    f64 knockbackX = ratioX * static_cast<f64>(knockbackStrength);
                    f64 knockbackZ = ratioZ * static_cast<f64>(knockbackStrength);

                    f64 newVelocityY;
                    if (target.onGround()) {
                        newVelocityY =
                            std::min(0.4, static_cast<f64>(velocity.y) / 2.0 + static_cast<f64>(knockbackStrength));
                    } else {
                        newVelocityY = static_cast<f64>(velocity.y);
                    }

                    target.setVelocity(static_cast<f32>(static_cast<f64>(velocity.x) / 2.0 - knockbackX),
                        static_cast<f32>(newVelocityY),
                        static_cast<f32>(static_cast<f64>(velocity.z) / 2.0 - knockbackZ));
                    target.setOnGround(false);
                }
            }
        }

        // 7. 应用火焰附加（攻击后应用完整燃烧时间）
        if (fireAspectLevel > 0) {
            // 火焰附加持续时间 = level * 4 秒
            target.setFire(fireAspectLevel * 4 * 20); // 20 ticks per second
        }

        // 8. 设置最后攻击者
        target.setLastHurtBy(this);

        // 9. 播放攻击声音
        playAttackSound(target);

        // 10. 设置攻击者的速度（击退反作用）
        setVelocity(velocity().x * 0.6f, velocity().y, velocity().z * 0.6f);
    }

    return attacked;
}

// ========== 玩家交互 ==========

ActionResultType MobEntity::processInitialInteract(Player& player, Hand hand)
{
    if (!isAlive()) {
        return ActionResultType::Pass;
    }

    ItemStack& heldItem = player.getHeldItem(hand);
    const Item* item = heldItem.getItem();

    // 1. 命名牌交互：如果玩家手持命名牌，调用 NameTagItem 的交互方法
    //    命名牌必须先在铁砧上命名，才能对生物使用
    //    注意: ItemStack::getItem() 返回 const Item*，需要转换为非 const
    //    这是安全的，因为 itemInteractionForEntity 只修改 ItemStack 参数，不修改 Item 本身
    if (item != nullptr && dynamic_cast<const item::items::NameTagItem*>(item) != nullptr) {
        bool success = const_cast<Item*>(item)->itemInteractionForEntity(heldItem, player, *this, hand);
        if (success) {
            return ActionResultType::Success;
        }
    }

    // 2. 刷怪蛋交互：如果玩家手持刷怪蛋，生成幼体
    //    注意：此路径依赖 SpawnEggItem 在 Items 注册表中的注册（如 Items::PIG_SPAWN_EGG），
    //    当前 Items 注册表尚未注册任何刷怪蛋物品，因此通过正常游戏流程无法触发此分支。
    //    待 Items::registerSpawnEggs() 实现后，此路径将可通过 processInitialInteract 触发。
    if (item != nullptr) {
        auto* spawnEgg = dynamic_cast<const item::SpawnEggItem*>(item);
        if (spawnEgg != nullptr) {
            if (m_world != nullptr && !m_world->isClientSide()) {
                if (_spawnOffspringFromSpawnEgg(player, *spawnEgg, heldItem)) {
                    return ActionResultType::Success;
                }
            } else {
                // 客户端直接预测成功
                return ActionResultType::Success;
            }
            return ActionResultType::Pass;
        }
    }

    // 3. 拴绳交互
    //    TODO: 拴绳系统需要完整的 Leashable 接口、拴绳物理、拴绳结实体交互、
    //    网络同步包等基础设施。以下仅实现基本的拴绳附着逻辑，
    //    完整的拴绳系统（Leashable接口、tickLeash物理、LeashKnotEntity交互、
    //    ClientboundSetEntityLinkPacket同步等）待后续实现。
    if (item != nullptr && item == Items::LEAD) {
        // TODO: 完整的拴绳交互逻辑：
        // - 如果实体已被当前玩家拴住，解除拴绳（掉落拴绳物品）
        // - 如果实体可以被拴住且未被其他玩家拴住，将拴绳拴在实体上
        // 当前暂不实现，等待 Leashable 接口完善
    }

    // 调用子类的交互逻辑
    ActionResultType result = interactMob(player, hand);
    if (result == ActionResultType::Success || result == ActionResultType::Consume) {
        return result;
    }

    // 调用父类实现
    return LivingEntity::processInitialInteract(player, hand);
}

ActionResultType MobEntity::interactMob(Player& /*player*/, Hand /*hand*/)
{
    // 基类默认返回 Pass，子类可重写以处理特定交互
    return ActionResultType::Pass;
}

bool MobEntity::canBeLeashed() const
{
    // 敌对生物不能被拴住
    return dynamic_cast<const entity::IMob*>(this) == nullptr;
}

bool MobEntity::_spawnOffspringFromSpawnEgg(Player& player, const item::SpawnEggItem& spawnEgg, ItemStack& heldItem)
{
    // TODO: 此方法的核心逻辑（实体生成、物品消耗、类型匹配、AgeableEntity判断）
    // 缺少单元测试覆盖。完整的测试需要 Items 注册表中注册 SpawnEggItem 实例
    // （如 Items::PIG_SPAWN_EGG），当前 Items 注册表尚未注册任何刷怪蛋物品。
    // 待刷怪蛋注册后应补充以下测试场景：
    //   - 刷怪蛋类型匹配时成功生成幼体
    //   - 刷怪蛋类型不匹配时返回 false
    //   - 非 AgeableEntity 实体（如 ZombieEntity）使用匹配类型刷怪蛋返回 false
    //   - 创造模式下刷怪蛋不消耗物品
    //   - 客户端（isClientSide）预测返回 Success
    // 检查刷怪蛋的实体类型是否与当前实体类型匹配
    // 只有相同类型的刷怪蛋才能在该实体上生成幼体
    // 使用实体类型名称字符串进行比较，避免 EntityType 不可拷贝的问题
    const std::string& eggEntityTypeName = spawnEgg.getEntityType().name();
    if (eggEntityTypeName != getTypeId()) {
        return false;
    }

    // 检查实体类型是否可序列化（与 MC 一致性检查）
    const entity::EntityType* myType = entity::EntityRegistry::instance().getType(getTypeId());
    if (myType == nullptr || !myType->serializable()) {
        return false;
    }

    if (m_world == nullptr) {
        return false;
    }

    // 创建幼体实体
    auto baby = myType->create(m_world);
    if (baby == nullptr) {
        return false;
    }

    // 转换为 MobEntity 进行设置
    auto* babyMob = dynamic_cast<MobEntity*>(baby.get());
    if (babyMob == nullptr) {
        return false;
    }

    // 设置为幼体
    auto* babyAgeable = dynamic_cast<AgeableEntity*>(babyMob);
    if (babyAgeable != nullptr) {
        // 年龄型实体（动物等）：通过 AgeableEntity::setChild 设置幼体状态
        babyAgeable->setChild(true);
    } else {
        // 非年龄型实体：不支持幼体状态，无法生成幼体
        return false;
    }

    // 将幼体放置在父实体位置
    babyMob->setPosition(x(), y(), z());
    babyMob->setRotation(yaw(), pitch());

    // 初始化生成
    entity::combat::DifficultyInstance difficultyInstance(m_world->difficulty());
    babyMob->finalizeSpawn(*m_world, difficultyInstance, world::spawn::SpawnReason::SpawnEgg);

    // 将幼体添加到世界
    m_world->spawnEntity(std::move(baby));

    // 消耗一个刷怪蛋（创造模式不消耗）
    if (!player.isCreative()) {
        heldItem.shrink(1);
    }

    return true;
}

// ============================================================================
// 掉落概率
// ============================================================================

f32 MobEntity::getEquipmentDropChance(EquipmentSlot slot) const
{
    auto idx = static_cast<size_t>(slot);
    if (idx >= m_equipmentDropChances.size()) {
        return DEFAULT_EQUIPMENT_DROP_CHANCE;
    }
    return m_equipmentDropChances[idx];
}

void MobEntity::setEquipmentDropChance(EquipmentSlot slot, f32 chance)
{
    auto idx = static_cast<size_t>(slot);
    if (idx < m_equipmentDropChances.size()) {
        m_equipmentDropChances[idx] = chance;
    }
}

void MobEntity::setGuaranteedDrop(EquipmentSlot slot)
{
    setEquipmentDropChance(slot, PRESERVE_ITEM_DROP_CHANCE);
}

bool MobEntity::isEquipmentDropPreserved(EquipmentSlot slot) const
{
    return getEquipmentDropChance(slot) > 1.0f;
}

// ============================================================================
// 拴绳系统
// ============================================================================

void MobEntity::setLeashedToEntity(const std::string& holderUuid)
{
    m_isLeashed = true;
    m_leashHolderUuid = holderUuid;
    m_leashFencePos = std::nullopt;
}

void MobEntity::setLeashedToFence(const BlockPos& pos)
{
    m_isLeashed = true;
    m_leashHolderUuid = std::nullopt;
    m_leashFencePos = pos;
}

void MobEntity::clearLeash()
{
    m_isLeashed = false;
    m_leashHolderUuid = std::nullopt;
    m_leashFencePos = std::nullopt;
    m_leashDelayInfo.targetUuid = std::nullopt;
    m_leashDelayInfo.fencePos = std::nullopt;
}

// ============================================================================
// NBT 序列化
// ============================================================================

void MobEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    using namespace mc::entity::serialization;

    // 先调用基类实现
    LivingEntity::addAdditionalSaveData(tag);

    // CanPickUpLoot (byte) - 是否可以拾取物品
    tag.put(nbt_keys::CAN_PICK_UP_LOOT, static_cast<i8>(m_canPickUpLoot ? 1 : 0));

    // PersistenceRequired (byte) - 是否需要持久化
    tag.put(nbt_keys::PERSISTENCE_REQUIRED, static_cast<i8>(m_persistenceRequired ? 1 : 0));

    // LeftHanded (byte) - 左撇子
    tag.put(nbt_keys::LEFT_HANDED, static_cast<i8>(m_primaryHand == HandSide::Left ? 1 : 0));

    // NoAI (byte) - 无 AI
    tag.put(nbt_keys::NO_AI, static_cast<i8>(m_aiEnabled ? 0 : 1));

    // DropChances（compound，仅包含非默认值）
    // 新格式：drop_chances compound，与 MC Java DropChances.filterDefaultValues 一致。
    // 旧格式 HandDropChances/ArmorDropChances（float list）已废弃，不再写入，
    // 但加载时仍兼容旧格式以保证存档兼容性。
    {
        nbt::tags::compound_tag dropChancesTag;
        // 装备槽位名称映射
        static constexpr struct {
            EquipmentSlot slot;
            const char* name;
        } slotNames[] = {
            {EquipmentSlot::MainHand, "mainhand"},
            {EquipmentSlot::OffHand, "offhand"},
            {EquipmentSlot::Feet, "feet"},
            {EquipmentSlot::Legs, "legs"},
            {EquipmentSlot::Chest, "chest"},
            {EquipmentSlot::Head, "head"},
        };
        for (const auto& entry : slotNames) {
            f32 chance = m_equipmentDropChances[static_cast<size_t>(entry.slot)];
            // 仅写入非默认值（与 MC DropChances.filterDefaultValues 一致）
            if (chance != DEFAULT_EQUIPMENT_DROP_CHANCE) {
                dropChancesTag.put(entry.name, chance);
            }
        }
        if (!dropChancesTag.value.empty()) {
            tag.value.emplace(
                nbt_keys::DROP_CHANCES, std::make_unique<nbt::tags::compound_tag>(std::move(dropChancesTag)));
        }
    }

    // Leash (compound) - 拴绳数据
    // 格式：Leash = {UUIDMost: long, UUIDLeast: long}（拴在实体上）
    //   或  Leash = {X: int, Y: int, Z: int}（拴在栅栏柱上）
    if (m_isLeashed) {
        nbt::tags::compound_tag leashTag;
        if (m_leashHolderUuid.has_value()) {
            // 拴在实体上：写入 UUIDMost + UUIDLeast
            nbt_helper::putUuid(leashTag, *m_leashHolderUuid);
        } else if (m_leashFencePos.has_value()) {
            // 拴在栅栏柱上：写入 X + Y + Z
            leashTag.put(nbt_keys::LEASH_X, m_leashFencePos->x);
            leashTag.put(nbt_keys::LEASH_Y, m_leashFencePos->y);
            leashTag.put(nbt_keys::LEASH_Z, m_leashFencePos->z);
        }
        if (!leashTag.value.empty()) {
            tag.value.emplace(nbt_keys::LEASH, std::make_unique<nbt::tags::compound_tag>(std::move(leashTag)));
        }
    }

    // DeathLootTable / DeathLootTableSeed
    // 仅在有自定义掉落表时写入，否则使用实体类型的默认掉落表。
    if (m_deathLootTable.has_value()) {
        tag.put(nbt_keys::DEATH_LOOT_TABLE, std::string(*m_deathLootTable));
        if (m_lootTableSeed != 0) {
            tag.put(nbt_keys::DEATH_LOOT_TABLE_SEED, m_lootTableSeed);
        }
    }
}

Result<void> MobEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    using namespace mc::entity::serialization;

    // 先调用基类实现
    MC_TRY(LivingEntity::readAdditionalSaveData(tag));

    // CanPickUpLoot (byte)
    if (auto val = nbt_helper::tryGetBool(tag, nbt_keys::CAN_PICK_UP_LOOT)) {
        m_canPickUpLoot = *val;
    }

    // PersistenceRequired (byte)
    if (auto val = nbt_helper::tryGetBool(tag, nbt_keys::PERSISTENCE_REQUIRED)) {
        m_persistenceRequired = *val;
    }

    // LeftHanded (byte)
    if (auto val = nbt_helper::tryGetBool(tag, nbt_keys::LEFT_HANDED)) {
        m_primaryHand = *val ? HandSide::Left : HandSide::Right;
    }

    // NoAI (byte)
    if (auto val = nbt_helper::tryGetBool(tag, nbt_keys::NO_AI)) {
        m_aiEnabled = !(*val);
    }

    // HandDropChances / ArmorDropChances / drop_chances
    // 读取时优先使用新格式 drop_chances（compound），然后回退到旧格式。
    // 新格式中的槽位会覆盖旧格式的值，未指定的槽位保持默认值。
    {
        bool readNewFormat = false;

        // 新格式：drop_chances（compound）
        if (auto* dropChancesCompound = nbt_helper::tryGetCompound(tag, nbt_keys::DROP_CHANCES)) {
            readNewFormat = true;
            // 装备槽位名称映射
            static constexpr struct {
                const char* name;
                EquipmentSlot slot;
            } slotNames[] = {
                {"mainhand", EquipmentSlot::MainHand},
                {"offhand", EquipmentSlot::OffHand},
                {"feet", EquipmentSlot::Feet},
                {"legs", EquipmentSlot::Legs},
                {"chest", EquipmentSlot::Chest},
                {"head", EquipmentSlot::Head},
            };
            for (const auto& entry : slotNames) {
                if (auto chance = nbt_helper::tryGetFloat(*dropChancesCompound, entry.name)) {
                    m_equipmentDropChances[static_cast<size_t>(entry.slot)] = *chance;
                }
            }
        }

        // 旧格式：HandDropChances（float list，长度 2）
        if (auto* handList = nbt_helper::tryGetList(tag, nbt_keys::HAND_DROP_CHANCES)) {
            if (handList->element_id() == nbt::TagId::Float) {
                auto& floatList = dynamic_cast<const nbt::tags::float_list_tag&>(*handList);
                if (floatList.value.size() >= 1) {
                    // 仅在新格式未设置时使用旧格式的值
                    if (!readNewFormat ||
                        m_equipmentDropChances[static_cast<size_t>(EquipmentSlot::MainHand)] ==
                            DEFAULT_EQUIPMENT_DROP_CHANCE) {
                        m_equipmentDropChances[static_cast<size_t>(EquipmentSlot::MainHand)] = floatList.value[0];
                    }
                }
                if (floatList.value.size() >= 2) {
                    if (!readNewFormat ||
                        m_equipmentDropChances[static_cast<size_t>(EquipmentSlot::OffHand)] ==
                            DEFAULT_EQUIPMENT_DROP_CHANCE) {
                        m_equipmentDropChances[static_cast<size_t>(EquipmentSlot::OffHand)] = floatList.value[1];
                    }
                }
            }
        }

        // 旧格式：ArmorDropChances（float list，长度 4）
        if (auto* armorList = nbt_helper::tryGetList(tag, nbt_keys::ARMOR_DROP_CHANCES)) {
            if (armorList->element_id() == nbt::TagId::Float) {
                auto& floatList = dynamic_cast<const nbt::tags::float_list_tag&>(*armorList);
                constexpr std::array<EquipmentSlot, 4> armorSlots = {
                    EquipmentSlot::Feet, EquipmentSlot::Legs, EquipmentSlot::Chest, EquipmentSlot::Head};
                for (size_t i = 0; i < armorSlots.size() && i < floatList.value.size(); ++i) {
                    if (!readNewFormat ||
                        m_equipmentDropChances[static_cast<size_t>(armorSlots[i])] == DEFAULT_EQUIPMENT_DROP_CHANCE) {
                        m_equipmentDropChances[static_cast<size_t>(armorSlots[i])] = floatList.value[i];
                    }
                }
            }
        }
    }

    // Leash (compound) - 拴绳数据
    // 读取拴绳绑定信息，支持实体 UUID 和栅栏柱坐标两种格式
    {
        auto* leashCompound = nbt_helper::tryGetCompound(tag, nbt_keys::LEASH);
        if (leashCompound != nullptr) {
            // 检查是拴在实体上（UUIDMost + UUIDLeast）还是栅栏柱上（X + Y + Z）
            auto uuidMost = nbt_helper::tryGetLong(*leashCompound, nbt_keys::LEASH_UUID_MOST);
            auto uuidLeast = nbt_helper::tryGetLong(*leashCompound, nbt_keys::LEASH_UUID_LEAST);

            if (uuidMost.has_value() && uuidLeast.has_value()) {
                // 拴在实体上
                std::string uuid = nbt_helper::getUuid(*leashCompound);
                if (!uuid.empty()) {
                    m_isLeashed = true;
                    m_leashHolderUuid = uuid;
                    m_leashDelayInfo.targetUuid = uuid;
                    // TODO: 实际的拴绳绑定需要在实体加载后延迟处理，
                    // 因为此时目标实体可能尚未加载到世界中。
                    // 需要实现 restoreLeashFromSave() 方法，在 tick 中
                    // 尝试通过 UUID 查找目标实体并完成实际绑定。
                    // 当前仅存储了 UUID 但未完成实体引用绑定，
                    // 拴绳的视觉效果和物理约束暂未生效。
                }
            } else {
                // 拴在栅栏柱上
                auto x = nbt_helper::tryGetInt(*leashCompound, nbt_keys::LEASH_X);
                auto y = nbt_helper::tryGetInt(*leashCompound, nbt_keys::LEASH_Y);
                auto z = nbt_helper::tryGetInt(*leashCompound, nbt_keys::LEASH_Z);
                if (x.has_value() && y.has_value() && z.has_value()) {
                    m_leashDelayInfo.fencePos = BlockPos(*x, *y, *z);
                    // 拴在栅栏柱上的情况可以立即绑定
                    setLeashedToFence(*m_leashDelayInfo.fencePos);
                }
            }
        } else if (m_isLeashed) {
            // 如果之前被拴住，但 NBT 中没有 Leash 数据，则解除拴绳
            clearLeash();
        }
    }

    // DeathLootTable / DeathLootTableSeed
    if (auto lootTableStr = nbt_helper::tryGetString(tag, nbt_keys::DEATH_LOOT_TABLE)) {
        m_deathLootTable = *lootTableStr;
    } else {
        m_deathLootTable = std::nullopt;
    }

    if (auto seed = nbt_helper::tryGetLong(tag, nbt_keys::DEATH_LOOT_TABLE_SEED)) {
        m_lootTableSeed = *seed;
    } else {
        m_lootTableSeed = 0;
    }

    return Result<void>::ok();
}

// ============================================================================
// 生成初始化
// ============================================================================

void MobEntity::finalizeSpawn(
    IWorld& world, const entity::combat::DifficultyInstance& difficulty, world::spawn::SpawnReason spawnReason)
{
    (void)world;
    (void)spawnReason;

    // 根据区域难度设置拾取物品能力
    f32 specialMultiplier = difficulty.getSpecialMultiplier();
    math::Random rng = getRandom();
    if (rng.nextFloat() < 0.55f * specialMultiplier) {
        setCanPickUpLoot(true);
    }

    // 填充默认装备（基于难度）
    populateDefaultEquipmentSlots(rng, difficulty);

    // 附魔默认装备（基于难度）
    populateDefaultEquipmentEnchantments(rng, difficulty);
}

void MobEntity::populateDefaultEquipmentSlots(
    math::Random& random, const entity::combat::DifficultyInstance& difficulty)
{
    f32 specialMultiplier = difficulty.getSpecialMultiplier();

    // 概率 = 0.15 * specialMultiplier，决定是否穿戴任何护甲
    if (random.nextFloat() >= 0.15f * specialMultiplier) {
        return;
    }

    // 随机生成护甲等级 (0~2 为基础，最多+3)
    i32 armorLevel = random.nextInt(3);
    for (i32 j = 1; j <= 3; ++j) {
        if (random.nextFloat() < 0.1087f) {
            ++armorLevel;
        }
    }

    // 根据 Hard 难度决定中断概率
    // Hard: 0.1 (更可能填满全身护甲)
    // 其他: 0.25
    f32 skipChance = (difficulty.getDifficulty() == Difficulty::Hard) ? 0.1f : 0.25f;

    // 按 Head -> Chest -> Legs -> Feet 顺序填充护甲
    // 护甲生成顺序
    static constexpr EquipmentSlot armorSlots[] = {
        EquipmentSlot::Head,
        EquipmentSlot::Chest,
        EquipmentSlot::Legs,
        EquipmentSlot::Feet,
    };

    bool firstSlot = true;
    for (EquipmentSlot slot : armorSlots) {
        // 第一个槽位必定尝试填充，之后的槽位有一定概率跳过
        if (!firstSlot && random.nextFloat() < skipChance) {
            break;
        }
        firstSlot = false;

        // 只在槽位为空时填充
        if (getEquipment(slot).isEmpty()) {
            const Item* item = getEquipmentForSlot(slot, armorLevel);
            if (item != nullptr) {
                setEquipment(slot, ItemStack(*item, 1));
            }
        }
    }
}

void MobEntity::populateDefaultEquipmentEnchantments(
    math::Random& random, const entity::combat::DifficultyInstance& difficulty)
{
    f32 specialMultiplier = difficulty.getSpecialMultiplier();

    // 附魔主手武器（概率 = 0.25 * specialMultiplier）
    enchantSpawnedWeapon(random, difficulty, specialMultiplier);

    // 附魔护甲（每个护甲槽位独立检定，概率 = 0.5 * specialMultiplier）
    enchantSpawnedArmor(random, difficulty, specialMultiplier);
}

const Item* MobEntity::getEquipmentForSlot(EquipmentSlot slot, i32 armorLevel)
{
    // 对应 Minecraft 原版 Mob.getEquipmentForSlot()
    // armorLevel: 0=皮革, 1=铁(原版1.21.11中为铜), 2=金, 3=锁链, 4=铁, 5=钻石
    // 注意：原版 MC 1.21.11 中 level 1 是铜护甲，但当前项目尚未实现铜护甲，
    // 因此 level 1 使用铁护甲作为替代

    switch (slot) {
        case EquipmentSlot::Head:
            switch (armorLevel) {
                case 0:
                    return Items::LEATHER_HELMET;
                case 1:
                    return Items::IRON_HELMET; // TODO: 替换为 COPPER_HELMET 当铜护甲实现后
                case 2:
                    return Items::GOLDEN_HELMET;
                case 3:
                    return Items::CHAINMAIL_HELMET;
                case 4:
                    return Items::IRON_HELMET;
                case 5:
                    return Items::DIAMOND_HELMET;
                default:
                    return nullptr;
            }
        case EquipmentSlot::Chest:
            switch (armorLevel) {
                case 0:
                    return Items::LEATHER_CHESTPLATE;
                case 1:
                    return Items::IRON_CHESTPLATE; // TODO: 替换为 COPPER_CHESTPLATE
                case 2:
                    return Items::GOLDEN_CHESTPLATE;
                case 3:
                    return Items::CHAINMAIL_CHESTPLATE;
                case 4:
                    return Items::IRON_CHESTPLATE;
                case 5:
                    return Items::DIAMOND_CHESTPLATE;
                default:
                    return nullptr;
            }
        case EquipmentSlot::Legs:
            switch (armorLevel) {
                case 0:
                    return Items::LEATHER_LEGGINGS;
                case 1:
                    return Items::IRON_LEGGINGS; // TODO: 替换为 COPPER_LEGGINGS
                case 2:
                    return Items::GOLDEN_LEGGINGS;
                case 3:
                    return Items::CHAINMAIL_LEGGINGS;
                case 4:
                    return Items::IRON_LEGGINGS;
                case 5:
                    return Items::DIAMOND_LEGGINGS;
                default:
                    return nullptr;
            }
        case EquipmentSlot::Feet:
            switch (armorLevel) {
                case 0:
                    return Items::LEATHER_BOOTS;
                case 1:
                    return Items::IRON_BOOTS; // TODO: 替换为 COPPER_BOOTS
                case 2:
                    return Items::GOLDEN_BOOTS;
                case 3:
                    return Items::CHAINMAIL_BOOTS;
                case 4:
                    return Items::IRON_BOOTS;
                case 5:
                    return Items::DIAMOND_BOOTS;
                default:
                    return nullptr;
            }
        default:
            return nullptr;
    }
}

void MobEntity::enchantSpawnedWeapon(
    math::Random& random, const entity::combat::DifficultyInstance& difficulty, f32 specialMultiplier)
{
    (void)difficulty;
    ItemStack mainHand = getEquipment(EquipmentSlot::MainHand);
    if (!mainHand.isEmpty() && random.nextFloat() < 0.25f * specialMultiplier) {
        // 附魔等级范围：5~17
        i32 level = 5 + random.nextInt(13);
        mainHand = item::enchant::EnchantmentHelper::addRandomEnchantment(random, mainHand, level, false);
        setEquipment(EquipmentSlot::MainHand, mainHand);
    }
}

void MobEntity::enchantSpawnedArmor(
    math::Random& random, const entity::combat::DifficultyInstance& difficulty, f32 specialMultiplier)
{
    (void)difficulty;
    // 对每个护甲槽位独立检定
    static constexpr EquipmentSlot armorSlots[] = {
        EquipmentSlot::Head,
        EquipmentSlot::Chest,
        EquipmentSlot::Legs,
        EquipmentSlot::Feet,
    };

    for (EquipmentSlot slot : armorSlots) {
        ItemStack armor = getEquipment(slot);
        if (!armor.isEmpty() && random.nextFloat() < 0.5f * specialMultiplier) {
            // 附魔等级范围：5~17
            i32 level = 5 + random.nextInt(13);
            armor = item::enchant::EnchantmentHelper::addRandomEnchantment(random, armor, level, false);
            setEquipment(slot, armor);
        }
    }
}

} // namespace mc
