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

#include "AbstractHorseEntity.hpp"

#include "common/core/Types.hpp"
#include "common/entity/ai/goal/goals/BreedGoal.hpp"
#include "common/entity/ai/goal/goals/FollowParentGoal.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/PanicGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/TemptGoal.hpp"
#include "common/entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "common/entity/ai/goal/goals/special/SpecialGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/network/protocol/EntityEvents.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/UuidUtils.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/core/SimpleInventory.hpp"
#include <algorithm>
#include <cmath>

namespace mc {

// MC 1.21.11 数据参数定义 - 必须在命名空间级别定义静态成员
entity::DataParameter<i8> AbstractHorseEntity::STATUS_PARAM = entity::EntityDataManager::createKey<i8>();

// ============================================================================
// 继承链标识（复刻 vanilla ClassTreeIdRegistry，parent = AnimalEntity::classInfo()）
// ============================================================================
const entity::EntityClassInfo& AbstractHorseEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"AbstractHorseEntity", &AnimalEntity::classInfo()};
    return s_classInfo;
}

AbstractHorseEntity::AbstractHorseEntity(EntityInstanceId id)
    : AnimalEntity(id)
{
    // MC 1.16.5: AbstractHorseEntity 构造函数中设置 stepHeight = 1.0F
    // 马类可以走上1格高的方块
    setStepHeight(1.0f);

    // 注册 AI 目标
    registerGoals();

    // 初始化随机属性
    initRandomAttributes();

    // MC 1.16.5: 初始化马背包（鞍槽 + 马铠槽）
    initHorseChest();
}

void AbstractHorseEntity::registerData()
{
    AnimalEntity::registerData();

    // 标记当前正在注册 AbstractHorseEntity 类的字段，使 registerParam 沿 AbstractHorseEntity
    // 继承链分配 id（续接 AnimalEntity 之后）。RAII 守卫自动配对压栈/弹栈。
    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    // MC 1.21.11 AbstractHorse.registerData()
    m_dataManager.registerParam(STATUS_PARAM, static_cast<i8>(0));
}

// ========== 状态标志辅助方法 ==========

bool AbstractHorseEntity::getHorseWatchableBoolean(i8 flag) const
{
    return (m_dataManager.get(STATUS_PARAM) & flag) != 0;
}

void AbstractHorseEntity::setHorseWatchableBoolean(i8 flag, bool value)
{
    i8 status = m_dataManager.get(STATUS_PARAM);
    if (value) {
        m_dataManager.set(STATUS_PARAM, static_cast<i8>(status | flag));
    } else {
        m_dataManager.set(STATUS_PARAM, static_cast<i8>(status & ~flag));
    }
}

void AbstractHorseEntity::setSaddle(bool saddle)
{
    m_saddled = saddle;
    setHorseWatchableBoolean(STATUS_FLAG_SADDLE, saddle);
}

void AbstractHorseEntity::onJump()
{
    if (!m_saddled || !m_isJumping) {
        return;
    }

    performJump();
}

void AbstractHorseEntity::setJumpPower(i32 power)
{
    // MC 1.16.5: jumpPower 范围 0-100
    m_jumpPower = std::clamp(power, 0, 100);
}

f32 AbstractHorseEntity::getMaxJumpHeight() const
{
    // 根据跳跃强度计算最大跳跃高度
    // MC 公式: 0.6 * jumpStrength^2 + 0.1 * jumpStrength + 0.3
    return 0.6f * m_jumpStrength * m_jumpStrength + 0.1f * m_jumpStrength + 0.3f;
}

bool AbstractHorseEntity::canJump() const
{
    return m_saddled && m_jumpCooldown <= 0;
}

void AbstractHorseEntity::startJumping(i32 jumpPower)
{
    if (!canJump()) {
        return;
    }

    m_isJumping = true;
    // MC 1.16.5: 设置初始跳跃力度
    setJumpPower(jumpPower);
}

void AbstractHorseEntity::stopJumping()
{
    if (!m_isJumping) {
        return;
    }

    // 执行跳跃
    performJump();
    m_isJumping = false;
    m_jumpPower = 0;
    m_jumpCooldown = 10; // 跳跃冷却
}

bool AbstractHorseEntity::isBeingRidden() const
{
    return m_rider != nullptr;
}

bool AbstractHorseEntity::canBeRiddenBy(Player* player) const
{
    // 已被骑乘或未驯服
    if (m_rider != nullptr) {
        return false;
    }

    // 需要驯服才能骑乘（子类可覆盖此逻辑）
    if (!m_tame) {
        return false;
    }

    return true;
}

void AbstractHorseEntity::setTame(bool tame)
{
    m_tame = tame;
    setHorseWatchableBoolean(STATUS_FLAG_TAME, tame);
}

// ========== 库存初始化 ==========

void AbstractHorseEntity::initHorseChest()
{
    // MC 1.16.5: 创建马背包（鞍槽 + 马铠槽）
    m_inventory = std::make_unique<blockentity::SimpleInventory>(getInventorySize());
}

// ========== IEquipable 接口实现 ==========

ItemStack AbstractHorseEntity::getEquipment(i32 slot) const
{
    if (!m_inventory || slot < 0 || slot >= getInventorySize()) {
        return ItemStack::EMPTY;
    }
    return m_inventory->getItem(slot);
}

void AbstractHorseEntity::setEquipment(i32 slot, const ItemStack& item)
{
    if (!m_inventory || slot < 0 || slot >= getInventorySize()) {
        return;
    }
    m_inventory->setItem(slot, item);

    // 更新鞍/马铠状态
    if (slot == 0) {
        // 鞍槽
        setSaddle(!item.isEmpty());
    } else if (slot == 1) {
        // 马铠槽
        setArmor(!item.isEmpty());
    }
}

bool AbstractHorseEntity::canEquip(const ItemStack& item, i32 slot) const
{
    // MC 1.16.5: AbstractHorseEntity.replaceItemInInventory()
    // 槽位边界检查必须先于物品判断，与 MC Java 中
    // `if (i >= 0 && i < 2 && i < this.horseChest.getSizeInventory())` 的顺序一致：
    // 非法槽位无论物品是否为空都应返回 false。
    if (slot < 0 || slot >= getInventorySize()) {
        return false;
    }

    // 空物品可以放入任意有效槽位（用于清空槽位）
    if (item.isEmpty()) {
        return true;
    }

    // 获取物品类型
    const Item* itemPtr = item.getItem();
    if (itemPtr == nullptr) {
        return false;
    }

    // 槽位 0：鞍槽
    // MC 1.16.5: if (i == 0 && itemStackIn.getItem() != Items.SADDLE) return false;
    if (slot == 0) {
        // 只有鞍可以放入鞍槽，且实体必须支持装备鞍
        return canEquipSaddle() && itemPtr == Items::SADDLE;
    }

    // 槽位 1：马铠/装饰槽
    // MC 1.16.5: if (i != 1 || this.func_230276_fq_() && this.isArmor(itemStackIn))
    if (slot == 1) {
        // 实体必须支持马铠槽位，且物品必须是有效的马铠/装饰
        return hasArmorSlot() && isValidArmorForSlot(item);
    }

    // 其他槽位（箱子槽位等）：默认允许
    return true;
}

bool AbstractHorseEntity::isValidArmorForSlot(const ItemStack& item) const
{
    // MC 1.16.5: AbstractHorseEntity.isArmor() 默认返回 false
    // 子类需要覆盖此方法：
    // - HorseEntity: 检查 HorseArmorItem
    // - LlamaEntity: 检查地毯
    MC_UNUSED(item);
    return false;
}

// ========== 鞍系统 ==========

bool AbstractHorseEntity::increaseTemper(i32 amount)
{
    m_temper += amount;

    if (m_temper >= m_maxTemper) {
        // 达到驯服阈值
        m_temper = m_maxTemper;
        setTame(true);
        return true;
    }

    return false;
}

bool AbstractHorseEntity::isTameItem(const ItemStack& itemStack) const
{
    // 默认情况下，马不响应任何驯服物品
    // 子类应覆盖此方法
    (void)itemStack;
    return false;
}

ActionResultType AbstractHorseEntity::interactMob(Player& player, Hand hand)
{
    // 处理玩家右键点击马匹时的交互

    // 1. 如果正在被骑乘或是幼年，交给基类处理
    if (isBeingRidden() || isChild()) {
        return AnimalEntity::interactMob(player, hand);
    }

    // 2. 已驯服且玩家按住潜行键（Shift）时，打开马背包界面
    if (isTame() && player.isSneaking()) {
        openInventory(player);
        return ActionResultType::Success;
    }

    // 3. 检查玩家手持物品
    ItemStack& itemStack = player.getHeldItem(hand);
    const Item* item = itemStack.getItem();

    if (item != nullptr && !itemStack.isEmpty()) {
        // 3a. 先让物品自身执行交互（如 SaddleItem 装备鞍）
        // 注意: Item::itemInteractionForEntity 不是 const 方法，需要 const_cast
        Item* mutableItem = const_cast<Item*>(item);
        bool itemInteractResult = mutableItem->itemInteractionForEntity(itemStack, player, *this, hand);
        if (itemInteractResult) {
            return ActionResultType::Success;
        }

        // 3b. 如果物品可以装备到马铠/装饰槽位且当前未装备马铠，则装备
        if (hasArmorSlot() && !hasArmor() && isValidArmorForSlot(itemStack)) {
            equipArmor(player, itemStack);
            return ActionResultType::Success;
        }
    }

    // 4. 让玩家骑乘马匹（包括未驯服的马也会被骑乘以触发驯服流程）
    doPlayerRide(player);
    return ActionResultType::Success;
}

void AbstractHorseEntity::openInventory(Player& player)
{
    // 条件：服务端 && (无骑乘者 || 骑乘者是自身) && 已驯服
    if (m_world != nullptr && !m_world->isClientSide()) {
        if (!isBeingRidden() || isPassenger(player.id())) {
            if (isTame()) {
                // TODO: 当马背包 ContainerMenu 系统实现后，在此打开马背包 GUI
                // 当前马背包 SimpleInventory 已存在（m_inventory），但尚未实现
                // HorseContainer（类似 HorseInventoryMenu）来连接马装备栏与玩家背包。
            }
        }
    }
}

void AbstractHorseEntity::equipArmor(Player& player, ItemStack& itemStack)
{
    // 将马铠/地毯装备到槽位 1（马铠/装饰槽）
    if (hasArmorSlot() && isValidArmorForSlot(itemStack)) {
        ItemStack armorStack(itemStack.getItem(), 1);
        setEquipment(1, armorStack);
        setArmor(true);

        // 播放马铠装备音效
        playSound(SoundEvents::ENTITY_HORSE_ARMOR, 1.0f, 1.0f);

        // 消耗一个物品（非创造模式）
        if (!player.isCreative()) {
            itemStack.shrink(1);
        }
    }
}

void AbstractHorseEntity::doPlayerRide(Player& player)
{
    // 让玩家骑乘马匹，触发驯服流程或正常骑乘
    if (!isBeingRidden()) {
        player.startRiding(*this);
    }
}

f32 AbstractHorseEntity::getSpeed() const
{
    return m_speed;
}

void AbstractHorseEntity::tick()
{
    AnimalEntity::tick();

    // 更新跳跃冷却
    if (m_jumpCooldown > 0) {
        m_jumpCooldown--;
    }

    // 更新跳跃蓄力
    if (m_isJumping) {
        updateJumpPower();
    }

    // 更新加速状态
    updateBoost();

    // MC 1.21.11 AbstractHorse.tick() 动画计数器更新

    // 张嘴计数器：递增，超过 30 tick 后关闭嘴巴
    if (m_openMouthCounter > 0 && ++m_openMouthCounter > 30) {
        m_openMouthCounter = 0;
        setMouthOpen(false);
    }

    // 扬蹄计数器：递减，归零时清除扬蹄状态
    if (m_jumpRearingCounter > 0 && --m_jumpRearingCounter <= 0) {
        clearRearing();
    }

    // 尾巴计数器：递增，超过 8 tick 后重置
    if (m_tailCounter > 0 && ++m_tailCounter > 8) {
        m_tailCounter = 0;
    }

    // 冲刺计数器：递增，超过 300 tick 后重置
    // TODO: m_sprintCounter 的初始触发由客户端渲染/动画系统设置，
    // MC 原版中由 HorseRenderer 等渲染器在外部设置 m_sprintCounter > 0，
    // 服务端 tick() 仅负责递增和超时重置逻辑。待客户端渲染系统实现后补全触发逻辑。
    if (m_sprintCounter > 0) {
        ++m_sprintCounter;
        if (m_sprintCounter > 300) {
            m_sprintCounter = 0;
        }
    }

    // 更新骑乘状态（包含动画插值）
    updateRiding();
}

void AbstractHorseEntity::aiStep()
{
    // MC 1.21.11 AbstractHorse.aiStep()

    // 随机尾巴摆动：每 tick 1/200 概率触发
    if (getRandom().nextInt(200) == 0) {
        m_tailCounter = 1;
    }

    // 调用父类 aiStep()
    AnimalEntity::aiStep();

    // 服务端逻辑
    if (m_world != nullptr && !m_world->isClientSide() && isAlive()) {
        // 自然恢复：每 tick 1/900 概率，且未在死亡过程中
        if (getRandom().nextInt(900) == 0 && deathTime() == 0) {
            heal(1.0f);
        }

        // 吃草逻辑
        if (canEatGrass()) {
            // 吃草触发：未在吃草 && 未被骑乘 && 1/300 概率 && 脚下为草方块
            if (!isEating() && !hasPassengers() && getRandom().nextInt(300) == 0) {
                // MC 1.21.11: serverlevel.getBlockState(this.blockPosition().below()).is(Blocks.GRASS_BLOCK)
                // onPos() 返回 BlockPos(floor(x), floor(y) - 1, floor(z))，等价于 blockPosition().below()
                const BlockState* belowState = m_world->getBlockState(onPos());
                if (belowState != nullptr && belowState->is(VanillaBlocks::GRASS_BLOCK)) {
                    setEating(true);
                }
            }

            // 吃草计数器：递增，超过 50 tick 后停止吃草
            if (isEating() && ++m_eatingCounter > 50) {
                m_eatingCounter = 0;
                setEating(false);
            }
        }
    }
}

void AbstractHorseEntity::travel(f32 strafing, f32 vertical, f32 forward)
{
    // MC 1.16.5 AbstractHorseEntity.travel()
    if (!isAlive()) {
        return;
    }

    // 检查是否被骑乘且可以控制（需要鞍）
    if (isBeingRidden() && canBeSteered() && m_saddled) {
        // 获取控制乘客（玩家）
        const auto& passengerIds = getPassengers();
        Entity* controllingPassenger = nullptr;
        if (!passengerIds.empty() && world() != nullptr) {
            controllingPassenger = world()->getEntity(passengerIds[0]);
        }

        if (controllingPassenger != nullptr) {
            // 同步朝向
            setRotation(controllingPassenger->yaw(), controllingPassenger->pitch() * 0.5f);

            // MC 1.16.5: 侧向移动减半
            f32 sideInput = strafing * 0.5f;
            f32 forwardInput = forward;

            // 后退时速度降低
            if (forwardInput <= 0.0f) {
                forwardInput *= 0.25f;
            }

            // 在地面且没有跳跃力且正在扬蹄时不能移动
            // MC 1.16.5: jumpPower 是 0-100 的整数
            if (onGround() && m_jumpPower == 0 && m_isJumping && !m_allowStandSliding) {
                sideInput = 0.0f;
                forwardInput = 0.0f;
            }

            // 处理跳跃
            // MC 1.16.5: jumpPower 转换为 0.0-1.0 的比例
            f32 jumpPowerFactor = static_cast<f32>(m_jumpPower) / 100.0f;
            if (jumpPowerFactor > 0.0f && !m_isJumping && onGround()) {
                // 计算跳跃力度
                // MC 1.16.5 AbstractHorseEntity.travel():
                // double d0 = this.getHorseJumpStrength() * (double)this.jumpPower * (double)this.getJumpFactor();
                f64 jumpForce = static_cast<f64>(getJumpStrength() * jumpPowerFactor);

                // MC 1.16.5: 跳跃提升药水效果加成
                // if (this.isPotionActive(Effects.JUMP_BOOST)) {
                //     d1 = d0 + (double)((float)(this.getActivePotionEffect(Effects.JUMP_BOOST).getAmplifier() + 1) *
                //     0.1F);
                // }
                const i32 jumpBoostLevel = getEffectLevel(entity::effect::EffectType::JumpBoost);
                if (jumpBoostLevel > 0) {
                    // 每级跳跃提升增加 0.1 的跳跃力度
                    jumpForce += static_cast<f64>(static_cast<f32>(jumpBoostLevel) * 0.1f);
                }

                // 设置跳跃速度
                setVelocity(velocityX(), static_cast<f32>(jumpForce), velocityZ());
                m_isJumping = true;
                m_jumpPower = 0;

                // 前进时额外推力
                if (forwardInput > 0.0f) {
                    f32 yawRad = math::toRadians(yaw());
                    f32 pushX = -0.4f * std::sin(yawRad) * jumpPowerFactor;
                    f32 pushZ = 0.4f * std::cos(yawRad) * jumpPowerFactor;
                    addVelocity(pushX, 0.0f, pushZ);
                }
            }

            // 设置AI移动速度
            // m_jumpMovementFactor = getAIMoveSpeed() * 0.1f;

            // 执行移动
            if (canPassengerSteer()) {
                // setAIMoveSpeed(static_cast<f32>(m_attributes.getValue(entity::attribute::Attributes::MOVEMENT_SPEED)));
                AnimalEntity::travel(sideInput, vertical, forwardInput);
            } else {
                // 无法控制时停止移动
                setVelocity(Vector3(0.0f, 0.0f, 0.0f));
            }

            // 着地时重置跳跃状态
            if (onGround()) {
                m_jumpPower = 0;
                m_isJumping = false;
            }
        }
    } else {
        // 未被骑乘时使用普通移动
        // m_jumpMovementFactor = 0.02f;
        AnimalEntity::travel(strafing, vertical, forward);
    }
}

void AbstractHorseEntity::registerAttributes()
{
    AnimalEntity::registerAttributes();

    // 马类基础属性
    m_attributes.registerAttribute(*entity::attribute::Attributes::horseJumpStrength());
    m_attributes.setBaseValue(entity::attribute::Attributes::HORSE_JUMP_STRENGTH, m_jumpStrength);

    // 设置生命值和速度
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, m_horseHealth);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, m_speed);
}

void AbstractHorseEntity::registerGoals()
{
    AnimalEntity::registerGoals();

    // MC 1.16.5 AbstractHorseEntity.registerGoals()
    // 所有马类实体的通用 AI 目标
    // 注意：优先级数字越小，优先级越高

    // 优先级 0: 游泳目标 - 在水中上浮
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::SwimGoal>(this));

    // 优先级 1: 恐慌逃跑 - 受伤或着火时逃跑
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::PanicGoal>(this, 1.2));

    // 优先级 1: 疯狂奔跑 - 未驯服马被骑乘时乱跑
    // 与 PanicGoal 优先级相同，两者可以同时触发
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::RunAroundLikeCrazyGoal>(this, 1.2));

    // 优先级 2: 繁殖 - 与同类交配
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::BreedGoal>(this, 1.0));

    // 优先级 4: 跟随父母 - 幼体跟随成年个体
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::FollowParentGoal>(this, 1.0));

    // 优先级 6: 避水随机行走 - 随机游荡但避开水域
    m_goalSelector.addGoal(6, std::make_unique<entity::ai::goal::WaterAvoidingRandomWalkingGoal>(this, 0.7));

    // 优先级 7: 看向玩家 - 看向 6 格内的玩家
    m_goalSelector.addGoal(7, std::make_unique<entity::ai::goal::LookAtGoal>(this, 6.0f));

    // 优先级 8: 随机看向 - 随机转头观察
    m_goalSelector.addGoal(8, std::make_unique<entity::ai::goal::LookRandomlyGoal>(this));
}

void AbstractHorseEntity::updateRiding()
{
    // MC 1.16.5: AbstractHorseEntity.tick() 中的动画更新逻辑

    // 保存上一帧动画值
    m_prevHeadLean = m_headLean;
    m_prevRearingAmount = m_rearingAmount;
    m_prevMouthOpenness = m_mouthOpenness;

    // 更新低头吃草动画
    // MC 1.16.5: headLean 动画更新
    if (isEating()) {
        m_headLean += (1.0f - m_headLean) * 0.4f + 0.05f;
        if (m_headLean > 1.0f) {
            m_headLean = 1.0f;
        }
    } else {
        m_headLean += (0.0f - m_headLean) * 0.4f - 0.05f;
        if (m_headLean < 0.0f) {
            m_headLean = 0.0f;
        }
    }

    // 更新扬蹄动画
    // MC 1.16.5: rearingAmount 动画更新
    if (isRearing()) {
        // 扬蹄时重置低头动画
        m_headLean = 0.0f;
        m_prevHeadLean = m_headLean;

        // 扬蹄动画渐入
        m_rearingAmount += (1.0f - m_rearingAmount) * 0.4f + 0.05f;
        if (m_rearingAmount > 1.0f) {
            m_rearingAmount = 1.0f;
        }
    } else {
        // 不再扬蹄时重置滑动标志
        m_allowStandSliding = false;

        // 扬蹄动画渐出（使用三次方实现平滑过渡）
        m_rearingAmount +=
            (0.8f * m_rearingAmount * m_rearingAmount * m_rearingAmount - m_rearingAmount) * 0.6f - 0.05f;
        if (m_rearingAmount < 0.0f) {
            m_rearingAmount = 0.0f;
        }
    }

    // 更新张嘴动画
    // MC 1.16.5: mouthOpenness 动画更新
    if (isMouthOpen()) {
        m_mouthOpenness += (1.0f - m_mouthOpenness) * 0.7f + 0.05f;
        if (m_mouthOpenness > 1.0f) {
            m_mouthOpenness = 1.0f;
        }
    } else {
        m_mouthOpenness += (0.0f - m_mouthOpenness) * 0.7f - 0.05f;
        if (m_mouthOpenness < 0.0f) {
            m_mouthOpenness = 0.0f;
        }
    }

    // 更新乘客位置
    // MC 1.16.5: Entity.updatePassengers() 在 tick() 中调用
    updatePassengers();
}

void AbstractHorseEntity::updatePassengerPosition(Entity& passenger)
{
    // MC 1.16.5: AbstractHorseEntity.updatePassenger(Entity)
    // 首先调用父类的基础定位
    Entity::updatePassengerPosition(passenger);

    // 如果乘客是 MobEntity，同步 renderYawOffset
    if (passenger.entityType() == entity::VanillaEntityTypeKeys::PLAYER) {
        // 对于玩家，不需要同步 renderYawOffset（玩家自己管理朝向）
    } else {
        // 对于其他 MobEntity，同步 renderYawOffset
        // 注意：这里需要检查 passenger 是否是 LivingEntity
        // MC 1.16.5: if (passenger instanceof MobEntity) { this.renderYawOffset = mobentity.renderYawOffset; }
        // 由于我们没有 MobEntity 的直接访问，这里跳过
    }

    // 扬蹄时调整乘客位置
    // MC 1.16.5: if (this.prevRearingAmount > 0.0F) { ... }
    if (m_prevRearingAmount > 0.0f) {
        // 计算基于朝向的偏移
        // MC 1.16.5: float f3 = MathHelper.sin(this.renderYawOffset * ((float)Math.PI / 180F));
        //            float f = MathHelper.cos(this.renderYawOffset * ((float)Math.PI / 180F));
        //            float f1 = 0.7F * this.prevRearingAmount;  // X方向偏移
        //            float f2 = 0.15F * this.prevRearingAmount; // Y方向额外高度
        f32 yawRad = math::toRadians(yaw());
        f32 sinYaw = std::sin(yawRad);
        f32 cosYaw = std::cos(yawRad);

        f32 offsetX = 0.7f * m_prevRearingAmount;
        f32 offsetY = 0.15f * m_prevRearingAmount;

        // 计算新的乘客位置
        // MC 1.16.5: passenger.setPosition(
        //     this.getPosX() + (double)(f1 * f3),
        //     this.getPosY() + this.getMountedYOffset() + passenger.getYOffset() + (double)f2,
        //     this.getPosZ() - (double)(f1 * f)
        // );
        f64 passengerX = static_cast<f64>(x() + offsetX * sinYaw);
        f64 passengerY =
            static_cast<f64>(y()) + getMountedYOffset() + passenger.getYOffset() + static_cast<f64>(offsetY);
        f64 passengerZ = static_cast<f64>(z() - offsetX * cosYaw);

        passenger.setPosition(static_cast<f32>(passengerX), static_cast<f32>(passengerY), static_cast<f32>(passengerZ));

        // 如果乘客是 LivingEntity，同步 renderYawOffset
        // MC 1.16.5: if (passenger instanceof LivingEntity) { ((LivingEntity)passenger).renderYawOffset =
        // this.renderYawOffset; } 由于我们没有直接访问 LivingEntity 的 renderYawOffset setter，这里跳过 renderYawOffset
        // 的同步已在 travel() 方法中通过 setRotation() 实现
    }
}

f32 AbstractHorseEntity::getRearingAmount(f32 partialTicks) const
{
    // MC 1.16.5: getRearingAmount(float partialTicks)
    // MathHelper.lerp(partialTicks, prevRearingAmount, rearingAmount)
    return math::lerp(m_prevRearingAmount, m_rearingAmount, partialTicks);
}

f32 AbstractHorseEntity::getHeadLeanAmount(f32 partialTicks) const
{
    // MC 1.16.5: getHeadLean(float partialTicks)
    return math::lerp(m_prevHeadLean, m_headLean, partialTicks);
}

f32 AbstractHorseEntity::getMouthOpennessAmount(f32 partialTicks) const
{
    // MC 1.16.5: getMouthOpennessAngle(float partialTicks)
    return math::lerp(m_prevMouthOpenness, m_mouthOpenness, partialTicks);
}

void AbstractHorseEntity::updateJumpPower()
{
    // MC 1.16.5: jumpPower 范围 0-100
    if (m_jumpPower < 100) {
        // 蓄力增加
        m_jumpPower += 5;
        m_jumpPower = std::min(m_jumpPower, 100);
    }
}

void AbstractHorseEntity::performJump()
{
    if (!canJump() || m_jumpPower <= 0) {
        return;
    }

    // MC 1.16.5: jumpPower 转换为 0.0-1.0 的比例
    f32 jumpPowerFactor = static_cast<f32>(m_jumpPower) / 100.0f;

    // 计算跳跃力度
    f32 jumpForce = m_jumpStrength * jumpPowerFactor;

    // MC 1.16.5: 跳跃提升药水效果加成
    const i32 jumpBoostLevel = getEffectLevel(entity::effect::EffectType::JumpBoost);
    if (jumpBoostLevel > 0) {
        // 每级跳跃提升增加 0.1 的跳跃力度
        jumpForce += static_cast<f32>(jumpBoostLevel) * 0.1f;
    }

    // 设置垂直速度
    setVelocity(velocityX(), jumpForce, velocityZ());

    // 设置跳跃冷却
    m_jumpCooldown = 10;
}

void AbstractHorseEntity::updateBoost()
{
    if (m_boostTime > 0) {
        m_boostTime--;

        if (m_boostTime <= 0) {
            m_isBoosting = false;
        }
    }
}

void AbstractHorseEntity::initRandomAttributes()
{
    math::Random rng(ticksExisted());

    // 随机生成马特有属性
    m_speed = rng.nextFloat(MIN_SPEED, MAX_SPEED);
    m_jumpStrength = rng.nextFloat(MIN_JUMP, MAX_JUMP);
    m_horseHealth = rng.nextFloat(MIN_HEALTH, MAX_HEALTH);
}

// ========== 驯服系统 ==========

bool AbstractHorseEntity::setTamedBy(Player* player)
{
    if (player == nullptr) {
        return false;
    }

    // MC 1.21.11: this.setOwner(player);
    setOwnerUuid(player->uuid());

    // MC 1.21.11: this.setTamed(true);
    setTame(true);

    // MC 1.21.11: CriteriaTriggers.TAME_ANIMAL.trigger((ServerPlayer)player, this);
    // 通过 IWorld 回调触发进度检测
    if (m_world != nullptr) {
        m_world->onTameAnimal(player->playerId(), this);
    }

    // MC 1.21.11: this.level().broadcastEntityEvent(this, (byte)7);
    // 发送实体状态包，让客户端显示爱心粒子
    if (m_world != nullptr) {
        m_world->broadcastEntityStatus(id(), static_cast<u8>(network::EntityStatus::TamingSucceeded));
    }

    return true;
}

void AbstractHorseEntity::makeMad()
{
    // MC 1.21.11: makeMad()
    // 条件：未扬蹄 && 非客户端
    if (!isRearing() && m_world != nullptr && !m_world->isClientSide()) {
        makeHorseRear();

        // MC 1.21.11: this.makeSound(this.getAngrySound());
        auto soundEvent = getAngrySound();
        if (soundEvent.has_value()) {
            playSound(soundEvent.value(), getSoundVolume(), getSoundPitch());
        }
    }
}

void AbstractHorseEntity::openMouth()
{
    // MC 1.21.11: openMouth()
    // 仅在服务端调用
    if (m_world == nullptr || m_world->isClientSide()) {
        return;
    }
    m_openMouthCounter = 1;
    setMouthOpen(true);
}

void AbstractHorseEntity::makeHorseRear()
{
    // MC 1.21.11: standIfPossible()
    // 条件：canPerformRearing() && (canPassengerSteer() || !isClientSide())
    if (!canPerformRearing()) {
        return;
    }

    if (canPassengerSteer() || (m_world != nullptr && !m_world->isClientSide())) {
        // setStanding(20): 设置扬蹄状态，持续 20 tick
        setRearing(true);
        m_jumpRearingCounter = 20;
    }
}

void AbstractHorseEntity::clearRearing()
{
    // MC 1.21.11: clearStanding()
    setRearing(false);
    m_jumpRearingCounter = 0;
}

bool AbstractHorseEntity::isRearing() const
{
    return getHorseWatchableBoolean(STATUS_FLAG_REARING);
}

void AbstractHorseEntity::setRearing(bool rearing)
{
    // MC 1.16.5: if (rearing) { this.setEatingHaystack(false); }
    if (rearing) {
        setHorseWatchableBoolean(STATUS_FLAG_EATING, false);
    }
    setHorseWatchableBoolean(STATUS_FLAG_REARING, rearing);
}

void AbstractHorseEntity::setOwnerUuid(const std::string& uuid)
{
    m_ownerUuid = uuid;
    // 同步驯服状态：有主人时自动标记为已驯服
    if (!uuid.empty()) {
        setTame(true);
    }
}

void AbstractHorseEntity::clearOwnerUuid()
{
    m_ownerUuid.clear();
}

LivingEntity* AbstractHorseEntity::getOwner() const
{
    if (m_ownerUuid.empty() || m_world == nullptr) {
        return nullptr;
    }
    Entity* entity = m_world->getEntityByUuid(m_ownerUuid);
    if (entity != nullptr) {
        return dynamic_cast<LivingEntity*>(entity);
    }
    return nullptr;
}

// ========== 食物处理 ==========

bool AbstractHorseEntity::isFoodItem(const ItemStack& itemStack) const
{
    // MC 1.16.5: AbstractHorseEntity.field_234235_bE_
    // Ingredient.fromItems(Items.WHEAT, Items.SUGAR, Blocks.HAY_BLOCK.asItem(),
    //                       Items.APPLE, Items.GOLDEN_CARROT, Items.GOLDEN_APPLE, Items.ENCHANTED_GOLDEN_APPLE)
    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return false;
    }

    return item == Items::WHEAT || item == Items::SUGAR || item == Items::HAY_BLOCK || item == Items::APPLE ||
        item == Items::GOLDEN_CARROT || item == Items::GOLDEN_APPLE || item == Items::ENCHANTED_GOLDEN_APPLE;
}

bool AbstractHorseEntity::handleEating(Player* player, ItemStack& itemStack)
{
    // MC 1.16.5: AbstractHorseEntity.handleEating(PlayerEntity player, ItemStack stack)
    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return false;
    }

    bool flag = false;     // 是否有任何效果
    f32 healAmount = 0.0f; // 治疗量
    i32 growthAmount = 0;  // 成长加速值（ticks）
    i32 temperAmount = 0;  // 驯服进度增加值

    // MC 1.16.5: 根据食物类型计算效果
    if (item == Items::WHEAT) {
        healAmount = 2.0f; // 治疗 2 点生命值
        growthAmount = 20; // 成长加速 20 ticks（1 秒）
        temperAmount = 3;  // 驯服进度 +3
    } else if (item == Items::SUGAR) {
        healAmount = 1.0f; // 治疗 1 点生命值
        growthAmount = 30; // 成长加速 30 ticks（1.5 秒）
        temperAmount = 3;  // 驯服进度 +3
    } else if (item == Items::HAY_BLOCK) {
        healAmount = 20.0f; // 治疗 20 点生命值
        growthAmount = 180; // 成长加速 180 ticks（9 秒）
        // 注意：干草块不增加驯服进度
    } else if (item == Items::APPLE) {
        healAmount = 3.0f; // 治疗 3 点生命值
        growthAmount = 60; // 成长加速 60 ticks（3 秒）
        temperAmount = 3;  // 驯服进度 +3
    } else if (item == Items::GOLDEN_CARROT) {
        healAmount = 4.0f; // 治疗 4 点生命值
        growthAmount = 60; // 成长加速 60 ticks（3 秒）
        temperAmount = 5;  // 驯服进度 +5
        // 金胡萝卜可以触发繁殖
        if (m_world != nullptr && !m_world->isClientSide() && isTame() && getGrowingAge() == 0 && canBreed()) {
            flag = true;
            setInLove();
        }
    } else if (item == Items::GOLDEN_APPLE || item == Items::ENCHANTED_GOLDEN_APPLE) {
        healAmount = 10.0f; // 治疗 10 点生命值
        growthAmount = 240; // 成长加速 240 ticks（12 秒）
        temperAmount = 10;  // 驯服进度 +10
        // 金苹果可以触发繁殖
        if (m_world != nullptr && !m_world->isClientSide() && isTame() && getGrowingAge() == 0 && canBreed()) {
            flag = true;
            setInLove();
        }
    } else {
        // 不是马的食物
        return false;
    }

    // 治疗效果
    if (health() < maxHealth() && healAmount > 0.0f) {
        heal(healAmount);
        flag = true;
    }

    // 幼体成长加速
    if (isChild() && growthAmount > 0) {
        // MC 1.16.5: 成长粒子效果由 AgeableEntity::updateAge() 中的 forcedAgeTimer 驱动，
        // 通过 broadcastEntityStatus(VillagerHappy) 在客户端生成开心村民粒子
        if (m_world == nullptr || !m_world->isClientSide()) {
            addGrowingAge(growthAmount);
        }
        flag = true;
    }

    // 驯服进度增加
    // MC 1.16.5: if (j > 0 && (flag || !this.isTame()) && this.getTemper() < this.getMaxTemper())
    if (temperAmount > 0 && (flag || !isTame()) && getTemper() < getMaxTemper()) {
        flag = true;
        if (m_world == nullptr || !m_world->isClientSide()) {
            increaseTemper(temperAmount);
        }
    }

    // 播放进食音效和动画
    if (flag) {
        // MC 1.21.11: this.eatingHorse()
        // 设置进食状态（用于动画）并打开嘴巴
        setEating(true);
        openMouth();

        // 播放进食音效
        auto eatSound = getEatSound();
        if (eatSound.has_value() && !isSilent() && m_world != nullptr) {
            playSound(eatSound.value(), getSoundVolume(), getSoundPitch());
        }

        // 减少 1 个物品
        if (player != nullptr && !player->isCreative()) {
            itemStack.shrink(1);
        }
    }

    return flag;
}

// ========== 属性遗传 ==========

void AbstractHorseEntity::setOffspringAttributes(const AgeableEntity& partner, AbstractHorseEntity& offspring)
{
    // MC 1.16.5: AbstractHorseEntity.setOffspringAttributes()
    // 遗传公式：(父本基础值 + 母本基础值 + 随机变异值) / 3

    // 获取父本属性（this）
    f64 parentMaxHealth = static_cast<f64>(m_horseHealth);
    f64 parentJumpStrength = static_cast<f64>(m_jumpStrength);
    f64 parentSpeed = static_cast<f64>(m_speed);

    // 获取母本属性
    const AbstractHorseEntity* partnerHorse = dynamic_cast<const AbstractHorseEntity*>(&partner);

    f64 partnerMaxHealth = 15.0;   // 默认最小生命值
    f64 partnerJumpStrength = 0.5; // 默认跳跃力
    f64 partnerSpeed = 0.175;      // 默认速度

    if (partnerHorse != nullptr) {
        partnerMaxHealth = static_cast<f64>(partnerHorse->m_horseHealth);
        partnerJumpStrength = static_cast<f64>(partnerHorse->m_jumpStrength);
        partnerSpeed = static_cast<f64>(partnerHorse->m_speed);
    }

    // 计算随机变异值
    f64 randomHealth = static_cast<f64>(getModifiedMaxHealth());
    f64 randomJump = getModifiedJumpStrength();
    f64 randomSpeed = getModifiedMovementSpeed();

    // 计算后代属性
    f64 babyMaxHealth = (parentMaxHealth + partnerMaxHealth + randomHealth) / 3.0;
    f64 babyJumpStrength = (parentJumpStrength + partnerJumpStrength + randomJump) / 3.0;
    f64 babySpeed = (parentSpeed + partnerSpeed + randomSpeed) / 3.0;

    // 设置后代属性
    offspring.m_horseHealth = static_cast<f32>(babyMaxHealth);
    offspring.setJumpStrength(static_cast<f32>(babyJumpStrength));
    offspring.m_speed = static_cast<f32>(babySpeed);

    // 更新属性
    offspring.m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, offspring.m_horseHealth);
    offspring.m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, offspring.m_speed);
    offspring.m_attributes.setBaseValue(
        entity::attribute::Attributes::HORSE_JUMP_STRENGTH, offspring.getJumpStrength());
}

f32 AbstractHorseEntity::getModifiedMaxHealth() const
{
    // MC 1.16.5: AbstractHorseEntity.getModifiedMaxHealth()
    // 返回 15.0F + rand(8) + rand(9)，范围 15-32
    math::Random rng(ticksExisted());
    return 15.0f + static_cast<f32>(rng.nextInt(8)) + static_cast<f32>(rng.nextInt(9));
}

f64 AbstractHorseEntity::getModifiedJumpStrength() const
{
    // MC 1.16.5: AbstractHorseEntity.getModifiedJumpStrength()
    // 返回 0.4 + rand*0.2 + rand*0.2 + rand*0.2，范围 0.4-1.0
    math::Random rng(ticksExisted());
    return 0.4 + rng.nextDouble() * 0.2 + rng.nextDouble() * 0.2 + rng.nextDouble() * 0.2;
}

f64 AbstractHorseEntity::getModifiedMovementSpeed() const
{
    // MC 1.16.5: AbstractHorseEntity.getModifiedMovementSpeed()
    // 返回 (0.45 + rand*0.3 + rand*0.3 + rand*0.3) * 0.25，范围 0.1125-0.3375
    math::Random rng(ticksExisted());
    return (0.45 + rng.nextDouble() * 0.3 + rng.nextDouble() * 0.3 + rng.nextDouble() * 0.3) * 0.25;
}

// ========== NBT 序列化 ==========

void AbstractHorseEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    AnimalEntity::addAdditionalSaveData(tag);

    using namespace mc::entity::serialization;

    // 主人UUID（使用 OwnerUUIDMost/OwnerUUIDLeast 双 long 格式）
    if (!m_ownerUuid.empty()) {
        auto uuidBytes = util::uuidFromString(m_ownerUuid);
        if (uuidBytes.size() == 16) {
            i64 most = (static_cast<i64>(uuidBytes[0]) << 56) | (static_cast<i64>(uuidBytes[1]) << 48) |
                (static_cast<i64>(uuidBytes[2]) << 40) | (static_cast<i64>(uuidBytes[3]) << 32) |
                (static_cast<i64>(uuidBytes[4]) << 24) | (static_cast<i64>(uuidBytes[5]) << 16) |
                (static_cast<i64>(uuidBytes[6]) << 8) | static_cast<i64>(uuidBytes[7]);

            i64 least = (static_cast<i64>(uuidBytes[8]) << 56) | (static_cast<i64>(uuidBytes[9]) << 48) |
                (static_cast<i64>(uuidBytes[10]) << 40) | (static_cast<i64>(uuidBytes[11]) << 32) |
                (static_cast<i64>(uuidBytes[12]) << 24) | (static_cast<i64>(uuidBytes[13]) << 16) |
                (static_cast<i64>(uuidBytes[14]) << 8) | static_cast<i64>(uuidBytes[15]);

            tag.put(nbt_keys::HORSE_OWNER_UUID_MOST, most);
            tag.put(nbt_keys::HORSE_OWNER_UUID_LEAST, least);
        }
    }

    // 驯服进度
    tag.put(nbt_keys::TEMPER, m_temper);

    // 是否已驯服（NBT 中布尔值使用 i8 存储）
    if (isTame()) {
        tag.put("Tame", static_cast<i8>(1));
    }

    // 是否已繁殖
    if (isBred()) {
        tag.put("Bred", static_cast<i8>(1));
    }

    // 是否装备鞍
    if (hasSaddle()) {
        tag.put("Saddle", static_cast<i8>(1));
    }

    // 跳跃强度
    tag.put(nbt_keys::HORSE_JUMP_STRENGTH, m_jumpStrength);

    // 速度
    tag.put(nbt_keys::HORSE_SPEED, m_speed);

    // 生命值
    tag.put(nbt_keys::HORSE_HEALTH, m_horseHealth);

    // 吃草状态
    if (isEating()) {
        tag.put(nbt_keys::EATING_HAYSTACK, static_cast<i8>(1));
    }
}

Result<void> AbstractHorseEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    MC_TRY(AnimalEntity::readAdditionalSaveData(tag));

    using namespace mc::entity::serialization;

    // 主人UUID（使用 OwnerUUIDMost/OwnerUUIDLeast 双 long 格式）
    auto ownerMostVal = nbt_helper::tryGetLong(tag, nbt_keys::HORSE_OWNER_UUID_MOST);
    auto ownerLeastVal = nbt_helper::tryGetLong(tag, nbt_keys::HORSE_OWNER_UUID_LEAST);
    if (ownerMostVal.has_value() && ownerLeastVal.has_value()) {
        i64 most = ownerMostVal.value();
        i64 least = ownerLeastVal.value();
        std::array<u8, 16> uuidBytes{};
        for (i32 i = 7; i >= 0; --i) {
            uuidBytes[i] = static_cast<u8>(most & 0xFF);
            most >>= 8;
        }
        for (i32 i = 15; i >= 8; --i) {
            uuidBytes[i] = static_cast<u8>(least & 0xFF);
            least >>= 8;
        }
        setOwnerUuid(util::uuidToString(uuidBytes));
    } else {
        clearOwnerUuid();
    }

    // 驯服进度
    if (auto val = nbt_helper::tryGetInt(tag, nbt_keys::TEMPER)) {
        m_temper = *val;
    }

    // 是否已驯服
    if (auto val = nbt_helper::tryGetBool(tag, "Tame")) {
        setTame(*val);
    }

    // 是否已繁殖
    if (auto val = nbt_helper::tryGetBool(tag, "Bred")) {
        setBred(*val);
    }

    // 是否装备鞍
    if (auto val = nbt_helper::tryGetBool(tag, "Saddle")) {
        setSaddle(*val);
    }

    // 跳跃强度
    if (auto val = nbt_helper::tryGetFloat(tag, nbt_keys::HORSE_JUMP_STRENGTH)) {
        m_jumpStrength = *val;
    }

    // 速度
    if (auto val = nbt_helper::tryGetFloat(tag, nbt_keys::HORSE_SPEED)) {
        m_speed = *val;
    }

    // 生命值
    if (auto val = nbt_helper::tryGetFloat(tag, nbt_keys::HORSE_HEALTH)) {
        m_horseHealth = *val;
    }

    // 吃草状态
    if (auto val = nbt_helper::tryGetBool(tag, nbt_keys::EATING_HAYSTACK)) {
        setEating(*val);
    }

    // 从NBT加载后重新初始化背包
    initHorseChest();

    return Result<void>::ok();
}

} // namespace mc
