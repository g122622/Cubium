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

#include "AbstractNautilusEntity.hpp"

#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/BreedGoal.hpp"
#include "common/entity/ai/goal/goals/FindWaterGoal.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/MeleeAttackGoal.hpp"
#include "common/entity/ai/goal/goals/PanicGoal.hpp"
#include "common/entity/ai/goal/goals/RandomSwimmingGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/TemptGoal.hpp"
#include "common/entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/network/packet/EntityPackets.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/blockentity/core/SimpleInventory.hpp"

#include <cmath>

namespace mc {

// 静态成员初始化
entity::DataParameter<bool> AbstractNautilusEntity::DATA_DASH_PARAM = entity::EntityDataManager::createKey<bool>();

// ============================================================================
// 构造函数
// ============================================================================

AbstractNautilusEntity::AbstractNautilusEntity(EntityId id)
    : TameableEntity(id)
{
    // 设置步进高度，鹦鹉螺可以走上 1 格高的方块
    setStepHeight(1.0f);

    // 设置初始空气值
    setAir(maxAir());

    // 创建物品栏（鞍槽 + 鹦鹉螺铠甲槽）
    createInventory();

    // 注册 AI 目标
    registerGoals();
}

void AbstractNautilusEntity::registerData()
{
    // 调用父类方法，确保基类数据参数已注册
    TameableEntity::registerData();

    // 注册冲刺状态同步参数
    m_dataManager.registerParam(DATA_DASH_PARAM, false);
}

// ============================================================================
// 物品栏
// ============================================================================

void AbstractNautilusEntity::createInventory()
{
    // 创建物品栏：槽位 0 = 鞍，槽位 1 = 鹦鹉螺铠甲
    m_inventory = std::make_unique<blockentity::SimpleInventory>(getEquipmentSlotCount());
}

// ============================================================================
// IEquipable 接口实现
// ============================================================================

ItemStack AbstractNautilusEntity::getEquipment(i32 slot) const
{
    if (!m_inventory || slot < 0 || slot >= getEquipmentSlotCount()) {
        return ItemStack::EMPTY;
    }
    return m_inventory->getItem(slot);
}

void AbstractNautilusEntity::setEquipment(i32 slot, const ItemStack& item)
{
    if (!m_inventory || slot < 0 || slot >= getEquipmentSlotCount()) {
        return;
    }
    m_inventory->setItem(slot, item);
}

bool AbstractNautilusEntity::canEquip(const ItemStack& item, i32 slot) const
{
    // 边界检查
    if (slot < 0 || slot >= getEquipmentSlotCount()) {
        return false;
    }

    // 空物品可以放入任意有效槽位（用于清空槽位）
    if (item.isEmpty()) {
        return true;
    }

    const Item* itemPtr = item.getItem();
    if (itemPtr == nullptr) {
        return false;
    }

    // 槽位 0：鞍槽
    if (slot == 0) {
        return canEquipSaddle() && itemPtr == Items::SADDLE;
    }

    // 槽位 1：鹦鹉螺铠甲槽
    if (slot == 1) {
        return canEquipBodyArmor() && isNautilusArmor(item);
    }

    return false;
}

bool AbstractNautilusEntity::isSaddled() const
{
    // 通过鞍槽是否有物品判断
    return !getEquipment(0).isEmpty();
}

bool AbstractNautilusEntity::canEquipSaddle() const
{
    // 对应 MC 1.21.11 AbstractNautilus.canUseSlot(SADDLE)：
    // 实体存活、非幼年、已驯服时才能装备鞍
    return isAlive() && !isChild() && isTamed();
}

bool AbstractNautilusEntity::canEquipBodyArmor() const
{
    // 对应 MC 1.21.11 AbstractNautilus.canUseSlot(BODY)：
    // 实体存活、非幼年、已驯服时才能装备鹦鹉螺铠甲
    return isAlive() && !isChild() && isTamed();
}

bool AbstractNautilusEntity::isTamingItem(const ItemStack& itemStack) const
{
    // 默认无驯服物品，子类可重写
    MC_UNUSED(itemStack);
    return false;
}

bool AbstractNautilusEntity::isNautilusFood(const ItemStack& itemStack) const
{
    // 默认无食物，子类可重写
    MC_UNUSED(itemStack);
    return false;
}

bool AbstractNautilusEntity::isNautilusArmor(const ItemStack& itemStack) const
{
    // 检查物品是否为任意一种鹦鹉螺铠甲
    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return false;
    }
    return item == Items::COPPER_NAUTILUS_ARMOR || item == Items::IRON_NAUTILUS_ARMOR ||
        item == Items::GOLDEN_NAUTILUS_ARMOR || item == Items::DIAMOND_NAUTILUS_ARMOR ||
        item == Items::NETHERITE_NAUTILUS_ARMOR;
}

std::optional<ResourceLocation> AbstractNautilusEntity::getEquipSound(EquipmentSlot slot) const
{
    // 鞍装备音效：水下/陆地不同
    if (slot == EquipmentSlot::Saddle) {
        if (isInWater()) {
            return SoundEvents::ENTITY_NAUTILUS_SADDLE_UNDERWATER_EQUIP;
        }
        return SoundEvents::ENTITY_NAUTILUS_SADDLE_EQUIP;
    }
    // 其他槽位使用默认音效
    return SoundEvents::ITEM_ARMOR_EQUIP_GENERIC;
}

// ============================================================================
// IJumpingMount 接口实现
// ============================================================================

void AbstractNautilusEntity::onJump()
{
    // 玩家请求跳跃时触发冲刺
    if (canJump() && m_playerJumpPendingScale > 0.0f && !isDashing()) {
        // 获取骑乘者
        const auto& passengers = getPassengers();
        if (!passengers.empty() && world() != nullptr) {
            Entity* passengerEntity = world()->getEntity(passengers[0]);
            if (passengerEntity != nullptr) {
                Player* rider = dynamic_cast<Player*>(passengerEntity);
                if (rider != nullptr) {
                    executeRidersJump(m_playerJumpPendingScale, *rider);
                }
            }
        }
        m_playerJumpPendingScale = 0.0f;
    }
}

void AbstractNautilusEntity::setJumpPower(i32 power)
{
    // 对应 MC 1.21.11 PlayerRideableJumping.onPlayerJump()
    // 将玩家跳跃蓄力转换为 0.0-1.0 的比例
    if (isSaddled() && m_dashCooldown <= 0) {
        m_playerJumpPendingScale = static_cast<f32>(std::clamp(power, 0, 100)) / 100.0f;
    }
}

f32 AbstractNautilusEntity::getMaxJumpHeight() const
{
    // 鹦鹉螺跳跃高度（冲刺力度）
    return isInWater() ? DASH_MOMENTUM_IN_WATER : DASH_MOMENTUM_ON_LAND;
}

bool AbstractNautilusEntity::canJump() const
{
    // 对应 MC 1.21.11 AbstractNautilus.canJump()
    // 需要装备鞍且冲刺冷却结束
    return isSaddled() && m_dashCooldown <= 0;
}

void AbstractNautilusEntity::startJumping(i32 jumpPower)
{
    // 开始蓄力跳跃
    setJumpPower(jumpPower);
}

void AbstractNautilusEntity::stopJumping()
{
    // 停止跳跃（执行冲刺）
    onJump();
}

void AbstractNautilusEntity::executeRidersJump(f32 jumpScale, Player& rider)
{
    // 对应 MC 1.21.11 AbstractNautilus.executeRidersJump()
    // 根据骑乘者视线方向施加冲刺力
    // 注意：LivingEntity::getLookAngle() 是 protected 方法，无法通过 rider.getLookAngle() 调用。
    // 这里直接用 rider 的公开 yaw()/pitch() 自行计算视线方向（与 LivingEntity::getLookAngle 一致）。
    const f32 yawRad = math::toRadians(rider.yaw());
    const f32 pitchRad = math::toRadians(rider.pitch());
    const f32 cosYaw = std::cos(yawRad);
    const f32 sinYaw = std::sin(yawRad);
    const f32 cosPitch = std::cos(pitchRad);
    const f32 sinPitch = std::sin(pitchRad);
    // 视线方向（MC 风格：-sin(yaw)*cos(pitch), -sin(pitch), cos(yaw)*cos(pitch)）
    Vector3 lookAngle(-sinYaw * cosPitch, -sinPitch, cosYaw * cosPitch);

    // 冲刺力度：水中 1.2，陆地 0.5
    f32 dashMomentum = isInWater() ? DASH_MOMENTUM_IN_WATER : DASH_MOMENTUM_ON_LAND;

    // 计算冲刺速度向量
    f32 speedFactor = dashMomentum * jumpScale;
    // 使用移动速度属性作为基础速度
    f64 movementSpeed = m_attributes.getValue(entity::attribute::Attributes::MOVEMENT_SPEED);
    Vector3 dashVelocity = lookAngle * (speedFactor * static_cast<f32>(movementSpeed));

    // 施加速度
    addVelocity(dashVelocity.x, dashVelocity.y, dashVelocity.z);

    // 设置冲刺冷却
    m_dashCooldown = DASH_COOLDOWN_TICKS;
    setDashing(true);

    // 播放冲刺音效
    auto dashSound = getDashSound();
    if (dashSound.has_value()) {
        playSound(dashSound.value(), getSoundVolume(), getSoundPitch());
    }
}

// ============================================================================
// 冲刺系统
// ============================================================================

void AbstractNautilusEntity::setDashing(bool dashing)
{
    m_dataManager.set(DATA_DASH_PARAM, dashing);
}

void AbstractNautilusEntity::updateDashCooldown()
{
    // 冲刺状态在冷却 < 35 时自动结束
    if (isDashing() && m_dashCooldown < DASH_COOLDOWN_TICKS - DASH_MINIMUM_DURATION_TICKS) {
        setDashing(false);
    }

    // 冷却倒计时
    if (m_dashCooldown > 0) {
        m_dashCooldown--;
        // 冷却结束时播放冲刺就绪音效
        if (m_dashCooldown == 0) {
            auto readySound = getDashReadySound();
            if (readySound.has_value()) {
                playSound(readySound.value(), getSoundVolume(), getSoundPitch());
            }
        }
    }
}

void AbstractNautilusEntity::spawnBubbles()
{
    // 对应 MC 1.21.11 AbstractNautilus.spawnBubbles()
    // 根据速度大小概率性生成气泡粒子，双端执行（服务端广播，客户端本地生成）
    f64 speed = std::sqrt(static_cast<f64>(velocityX() * velocityX()) + static_cast<f64>(velocityY() * velocityY()) +
        static_cast<f64>(velocityZ() * velocityZ()));
    f64 bubbleProb = std::clamp(speed * 2.0, 0.15, 1.0);

    // 由世界生成气泡粒子
    if (m_world != nullptr && getRandom().nextFloat() < static_cast<f32>(bubbleProb)) {
        // 计算视线方向（限制俯仰角在 -10 到 10 度之间）
        // 对应 MC 1.21.11 AbstractNautilus.spawnBubbles() 中 calculateViewVector(clampedPitch, yaw)
        // 注意：yaw()/pitch() 是本类的公开成员函数，使用别名避免局部变量遮蔽
        const f32 currentYaw = yaw();
        const f32 clampedPitch = std::clamp(pitch(), -10.0f, 10.0f);
        const f32 yawRad = math::toRadians(currentYaw);
        const f32 pitchRad = math::toRadians(clampedPitch);
        const f32 cosPitch = std::cos(pitchRad);
        const f32 sinPitch = std::sin(pitchRad);
        const f32 cosYaw = std::cos(yawRad);
        const f32 sinYaw = std::sin(yawRad);
        // 视线方向（MC 风格：-sin(yaw)*cos(pitch), -sin(pitch), cos(yaw)*cos(pitch)）
        const f32 viewX = -sinYaw * cosPitch;
        const f32 viewY = -sinPitch;
        const f32 viewZ = cosYaw * cosPitch;

        // 气泡生成位置：实体位置后方 1.1 格、上方 0.25 格
        const f32 bubbleX = static_cast<f32>(x()) - viewX * 1.1f;
        const f32 bubbleY = static_cast<f32>(y()) - viewY + 0.25f;
        const f32 bubbleZ = static_cast<f32>(z()) - viewZ * 1.1f;

        // 随机扩散速度：spread = nextDouble() * 0.8 * (1 + speed)
        // 每个分量 (nextFloat() - 0.5) * spread，对应 MC 原版 d2/d3/d4/d5
        math::Random& rng = getRandom();
        const f64 spread = rng.nextDouble() * 0.8 * (1.0 + speed);
        const f32 velX = (rng.nextFloat() - 0.5f) * static_cast<f32>(spread);
        const f32 velY = (rng.nextFloat() - 0.5f) * static_cast<f32>(spread);
        const f32 velZ = (rng.nextFloat() - 0.5f) * static_cast<f32>(spread);

        // 生成 BUBBLE 粒子（服务端通过 ServerWorld::addParticle 广播给附近玩家，
        // 客户端通过 ClientWorld::addParticle 本地生成）
        m_world->addParticle(
            particle::ParticleTypeId::Bubble, Vector3(bubbleX, bubbleY, bubbleZ), Vector3(velX, velY, velZ));
    }
}

// ============================================================================
// 水生行为
// ============================================================================

f32 AbstractNautilusEntity::getPathWeight(f32 x, f32 y, f32 z) const
{
    // 对应 MC 1.21.11 AbstractNautilus.getWalkTargetValue()：返回 0.0f
    MC_UNUSED(x);
    MC_UNUSED(y);
    MC_UNUSED(z);
    return 0.0f;
}

// ============================================================================
// 骑乘系统
// ============================================================================

void AbstractNautilusEntity::doPlayerRide(Player& player)
{
    // 对应 MC 1.21.11 AbstractNautilus.doPlayerRide()
    if (m_world == nullptr || m_world->isClientSide()) {
        return;
    }
    player.startRiding(*this);
}

void AbstractNautilusEntity::applyRiderEffects()
{
    // 对应 MC 1.21.11 AbstractNautilus.applyEffects()
    // 给骑乘者添加水下呼吸效果（简化版：原版使用 BREATH_OF_THE_NAUTILUS）
    const auto& passengers = getPassengers();
    if (passengers.empty() || m_world == nullptr) {
        return;
    }

    Entity* passengerEntity = m_world->getEntity(passengers[0]);
    if (passengerEntity == nullptr) {
        return;
    }

    Player* player = dynamic_cast<Player*>(passengerEntity);
    if (player == nullptr) {
        return;
    }

    // 检查是否需要刷新效果（每 40 tick 刷新一次）
    bool hasEffect = player->hasEffect(entity::effect::EffectType::WaterBreathing);
    bool shouldRefresh = (m_world->dayTime() % EFFECT_REFRESH_RATE) == 0;

    if (!hasEffect || shouldRefresh) {
        // 添加水下呼吸效果，持续 60 tick
        player->addEffect(entity::effect::EffectInstance(entity::effect::EffectType::WaterBreathing,
            EFFECT_DURATION,
            0,
            true, // ambient
            true, // visible
            true  // showIcon
            ));
    }
}

f32 AbstractNautilusEntity::getRiddenSpeed() const
{
    // 对应 MC 1.21.11 AbstractNautilus.getRiddenSpeed()
    f64 movementSpeed = m_attributes.getValue(entity::attribute::Attributes::MOVEMENT_SPEED);
    if (isInWater()) {
        return RIDDEN_SPEED_MODIFIER_IN_WATER * static_cast<f32>(movementSpeed);
    }
    return RIDDEN_SPEED_MODIFIER_ON_LAND * static_cast<f32>(movementSpeed);
}

// ============================================================================
// 生命周期
// ============================================================================

void AbstractNautilusEntity::tick()
{
    TameableEntity::tick();

    // 服务端：应用骑乘效果
    if (m_world != nullptr && !m_world->isClientSide()) {
        applyRiderEffects();
    }

    // 更新冲刺冷却
    updateDashCooldown();

    // 水中生成气泡粒子
    if (isInWater()) {
        spawnBubbles();
    }
}

void AbstractNautilusEntity::travel(f32 strafing, f32 vertical, f32 forward)
{
    // 对应 MC 1.21.11 AbstractNautilus.travel() 简化版
    if (!isAlive()) {
        return;
    }

    // 检查是否被骑乘且可以控制（需要鞍）
    if (isBeingRidden() && isSaddled()) {
        // 获取控制乘客（玩家）
        const auto& passengers = getPassengers();
        Entity* controllingPassenger = nullptr;
        if (!passengers.empty() && world() != nullptr) {
            controllingPassenger = world()->getEntity(passengers[0]);
        }

        if (controllingPassenger != nullptr) {
            // 同步朝向
            setRotation(controllingPassenger->yaw(), controllingPassenger->pitch() * 0.5f);

            // 使用骑乘速度
            f32 speed = getRiddenSpeed();
            f32 forwardInput = forward;
            f32 sideInput = strafing * 0.5f;

            // 后退时速度降低
            if (forwardInput <= 0.0f) {
                forwardInput *= 0.25f;
            }

            // 执行移动
            if (canPassengerSteer()) {
                // 设置 AI 移动速度
                AnimalEntity::travel(sideInput, vertical, forwardInput);
            } else {
                // 无法控制时停止移动
                setVelocity(Vector3(0.0f, 0.0f, 0.0f));
            }
        }
    } else {
        // 未被骑乘时使用普通移动
        AnimalEntity::travel(strafing, vertical, forward);
    }
}

// ============================================================================
// 玩家交互
// ============================================================================

ActionResultType AbstractNautilusEntity::interactMob(Player& player, Hand hand)
{
    // 对应 MC 1.21.11 AbstractNautilus.mobInteract()
    ItemStack& itemStack = player.getHeldItem(hand);
    const Item* item = itemStack.getItem();

    // 1. 幼年 → 交给基类处理
    if (isChild()) {
        return TameableEntity::interactMob(player, hand);
    }

    // 2. 已驯服 + Shift → 打开背包界面
    if (isTamed() && player.isSneaking()) {
        openInventory(player);
        return ActionResultType::Success;
    }

    // 3. 手持物品时的处理
    if (item != nullptr && !itemStack.isEmpty()) {
        // 3a. 未驯服 + 驯服物品 → 尝试驯服
        if (!isTamed() && isTamingItem(itemStack)) {
            // 消耗物品
            if (!player.isCreative()) {
                itemStack.shrink(1);
            }
            tryToTame(player);
            return ActionResultType::Success;
        }

        // 3b. 已驯服 + 食物 + 未满血 → 喂食治疗
        if (isNautilusFood(itemStack) && health() < maxHealth()) {
            // 治疗 2 点生命值（简化版，原版使用 FoodProperties.nutrition * 2）
            heal(2.0f);
            if (!player.isCreative()) {
                itemStack.shrink(1);
            }
            // 播放进食音效
            auto eatSound = getEatSound();
            if (eatSound.has_value()) {
                playSound(eatSound.value(), getSoundVolume(), getSoundPitch());
            }
            return ActionResultType::Success;
        }

        // 3c. 物品交互（鞍、鹦鹉螺铠甲）
        // 装备鞍
        if (item == Items::SADDLE && canEquipSaddle() && !isSaddled()) {
            ItemStack saddleStack = itemStack.split(1);
            setEquipment(0, saddleStack);
            // 播放装备音效
            auto equipSound = getEquipSound(EquipmentSlot::Saddle);
            if (equipSound.has_value()) {
                playSound(equipSound.value(), getSoundVolume(), getSoundPitch());
            }
            if (!player.isCreative() && !itemStack.isEmpty()) {
                // 物品已通过 split 消耗
            }
            return ActionResultType::Success;
        }

        // 装备鹦鹉螺铠甲
        if (isNautilusArmor(itemStack) && canEquipBodyArmor() && getEquipment(1).isEmpty()) {
            ItemStack armorStack = itemStack.split(1);
            setEquipment(1, armorStack);
            // 播放装备音效
            playSound(SoundEvents::ITEM_ARMOR_EQUIP_GENERIC, getSoundVolume(), getSoundPitch());
            return ActionResultType::Success;
        }
    }

    // 4. 已驯服 + 非Shift + 非食物 → 骑乘
    if (isTamed() && !player.isSneaking() && (item == nullptr || itemStack.isEmpty() || !isNautilusFood(itemStack))) {
        doPlayerRide(player);
        return ActionResultType::Success;
    }

    // 5. 其他情况交给基类
    return TameableEntity::interactMob(player, hand);
}

void AbstractNautilusEntity::openInventory(Player& player)
{
    // 对应 MC 1.21.11 AbstractNautilus.openCustomInventoryScreen()
    // 条件：服务端 && (无骑乘者 || 骑乘者是自身) && 已驯服
    if (m_world != nullptr && !m_world->isClientSide()) {
        if (!isBeingRidden() || isPassenger(player.id())) {
            if (isTamed()) {
                // TODO: 当实体背包 ContainerMenu 系统实现后，在此打开鹦鹉螺背包 GUI。
                // 当前物品栏 SimpleInventory 已存在（m_inventory），但实体背包 GUI 链路尚未打通，
                // 缺失以下关键组件：
                //   1. ServerWorld::setOnOpenEntityContainer 回调未接线（MinecraftServer 未注册）
                //   2. ContainerManager 不支持实体容器（仅支持方块容器，签名依赖 BlockPos）
                //   3. NautilusContainer 菜单类未实现（参考 ChestContainer 模式）
                //   4. ContainerType 枚举无动物背包类型
                //   5. 客户端 OpenContainerPacket 处理器不支持实体容器分支
                //   6. 客户端 NautilusInventoryScreen 未实现
                // 与 AbstractHorseEntity::openInventory 的 TODO 是同一阻塞点，应一起收敛。
                // MC 原版流程：openCustomInventoryScreen → ServerPlayer.openNautilusInventory
                // → 发送 ClientboundMountScreenOpenPacket + 创建 NautilusInventoryMenu
                // → initMenu → sendInitialData 发送 ClientboundContainerSetContentPacket。
            }
        }
    }
}

void AbstractNautilusEntity::tryToTame(Player& player)
{
    // 对应 MC 1.21.11 AbstractNautilus.tryToTame()
    // 1/3 概率驯服成功
    if (getRandom().nextInt(TAME_PROBABILITY_DENOMINATOR) == 0) {
        // 驯服成功
        setTamed(true);
        setOwnerId(player.playerId());
        // 清除导航路径
        clearNavigation();

        // 广播驯服成功事件（爱心粒子）
        if (m_world != nullptr) {
            m_world->broadcastEntityStatus(id(), static_cast<u8>(network::EntityStatusPacket::Status::TamingSucceeded));
        }
    } else {
        // 驯服失败（烟雾粒子）
        if (m_world != nullptr) {
            m_world->broadcastEntityStatus(id(), static_cast<u8>(network::EntityStatusPacket::Status::TamingFailed));
        }
    }

    // 播放进食音效
    auto eatSound = getEatSound();
    if (eatSound.has_value()) {
        playSound(eatSound.value(), getSoundVolume(), getSoundPitch());
    }
}

// ============================================================================
// AI 目标注册
// ============================================================================

void AbstractNautilusEntity::registerGoals()
{
    // 调用父类方法
    TameableEntity::registerGoals();

    // 鹦鹉螺通用 AI 目标（优先级数字越小越优先）

    // 优先级 0: 水中上浮和寻找水源
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::SwimGoal>(this));
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::FindWaterGoal>(this));

    // 优先级 1: 恐慌逃跑（受伤或着火时）
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::PanicGoal>(this, 1.6));

    // 优先级 3: 跟随食物（驯服物品/食物）
    m_goalSelector.addGoal(3,
        std::make_unique<entity::ai::goal::TemptGoal>(
            this,
            1.3,
            [this](const ItemStack& stack) -> bool { return isTamingItem(stack) || isNautilusFood(stack); },
            false));

    // 优先级 4: 近战攻击
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::MeleeAttackGoal>(this, 0.6, true));

    // 优先级 5: 随机游泳
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::RandomSwimmingGoal>(this, 1.0, 10));

    // 优先级 6: 看向玩家
    m_goalSelector.addGoal(
        6, std::make_unique<entity::ai::goal::LookAtGoal>(this, 6.0f, 0.02f, [](const LivingEntity* entity) -> bool {
            return entity != nullptr && entity->typeId() == entity::EntityTypeIdNumber::PLAYER;
        }));

    // 优先级 7: 随机看向
    m_goalSelector.addGoal(7, std::make_unique<entity::ai::goal::LookRandomlyGoal>(this));
}

void AbstractNautilusEntity::registerAttributes()
{
    // 调用父类方法
    TameableEntity::registerAttributes();

    // 鹦鹉螺基础属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 15.0f);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 1.0f);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 3.0f);
    m_attributes.setBaseValue(entity::attribute::Attributes::KNOCKBACK_RESISTANCE, 0.3f);
}

// ============================================================================
// NBT 序列化
// ============================================================================

void AbstractNautilusEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    // 调用父类方法
    TameableEntity::addAdditionalSaveData(tag);

    using namespace mc::entity::serialization;

    // 冲刺冷却
    if (m_dashCooldown > 0) {
        tag.put("DashCooldown", m_dashCooldown);
    }

    // 物品栏内容（鞍 + 鹦鹉螺铠甲）
    // 使用 "Items" 列表 + Slot 索引模式保存完整 ItemStack（含附魔、耐久、自定义名称等），
    // 与 ChestBoatEntity、LootableContainerBlockEntity、PlayerInventory 等保持一致。
    // 空槽位不写入，与 MC 1.21.11 EntityEquipment.CODEC 的 map.values().removeIf(isEmpty) 一致。
    if (m_inventory != nullptr) {
        auto itemsList = std::make_unique<nbt::tags::compound_list_tag>();
        for (i32 slot = 0; slot < getEquipmentSlotCount(); ++slot) {
            const ItemStack& stack = m_inventory->getItem(slot);
            if (!stack.isEmpty()) {
                nbt::tags::compound_tag itemTag;
                itemTag.put("Slot", static_cast<i8>(slot));
                stack.toNbt(itemTag);
                itemsList->value.push_back(std::move(itemTag));
            }
        }
        if (!itemsList->value.empty()) {
            tag.value.insert_or_assign(nbt_keys::ITEMS, std::move(itemsList));
        }
    }
}

Result<void> AbstractNautilusEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    MC_TRY(TameableEntity::readAdditionalSaveData(tag));

    using namespace mc::entity::serialization;

    // 冲刺冷却
    if (auto val = nbt_helper::tryGetInt(tag, "DashCooldown")) {
        m_dashCooldown = *val;
    }

    // 物品栏内容
    // 优先读取新格式 "Items" 列表（完整 ItemStack NBT），
    // 兼容旧格式 "SaddleItem" 布尔标记（早期版本仅记录槽位占用状态）。
    if (m_inventory != nullptr) {
        const auto* itemsList = nbt_helper::tryGetList(tag, nbt_keys::ITEMS);
        if (itemsList != nullptr && itemsList->element_id() == nbt::TagId::Compound) {
            // 新格式：从 "Items" 列表读取完整 ItemStack
            const auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(*itemsList);
            for (const auto& itemTag : compoundList.value) {
                i8 slot = 0;
                if (auto slotOpt = nbt_helper::tryGetByte(itemTag, "Slot")) {
                    slot = *slotOpt;
                }
                if (slot >= 0 && slot < getEquipmentSlotCount()) {
                    auto stackResult = ItemStack::fromNbt(itemTag);
                    if (stackResult.success() && !stackResult.value().isEmpty()) {
                        setEquipment(slot, stackResult.value());
                    }
                }
            }
        } else if (nbt_helper::tryGetBool(tag, "SaddleItem").value_or(false)) {
            // 旧格式向后兼容：仅有布尔标记，恢复为默认鞍物品
            if (Items::SADDLE != nullptr) {
                setEquipment(0, ItemStack(Items::SADDLE, 1));
            }
        }
    }

    return Result<void>::ok();
}

} // namespace mc
