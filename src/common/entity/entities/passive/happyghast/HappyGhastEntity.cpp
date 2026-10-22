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

#include "HappyGhastEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/controller/GhastMovementController.hpp"
#include "common/entity/ai/goal/goals/TemptGoal.hpp"
#include "common/entity/ai/goal/goals/special/GhastGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/world/IWorld.hpp"

#include <memory>
#include <optional>

namespace mc {

// 继承链标识（复刻 vanilla ClassTreeIdRegistry，parent = AnimalEntity::classInfo()）。
const entity::EntityClassInfo& HappyGhastEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"HappyGhastEntity", &AnimalEntity::classInfo()};
    return s_classInfo;
}

std::unique_ptr<Entity> HappyGhastEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<HappyGhastEntity>(EntityInstanceId(0), registry);
}

HappyGhastEntity::HappyGhastEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AnimalEntity(id, registry)
{
    // 飞行生物：与恶魂共用带碰撞检测的飞行移动控制器。
    m_moveController = std::make_unique<entity::ai::controller::GhastMovementController>(this);

    // 飞行生物不受重力（飞行物理由 travel() 的 travelFlying 分支承担）。
    setNoGravity(true);

    // 补调 registerGoals / registerAttributes：AnimalEntity 构造期间 vtable 指向基类，
    // 派生 override 永不执行，须在派生类构造显式调用。
    registerGoals();
    registerAttributes();
}

// ========== 年龄 / 尺寸 ==========

f32 HappyGhastEntity::getBaseWidth() const
{
    // 成体宽度 4.0（对齐 vanilla HappyGhast.getDefaultDimensions 继承 Ghast 的 4×4）；
    // 幼体尺寸由 AgeableEntity::width() 经 babyScale()（0.2375）缩放得到。
    return 4.0f;
}

f32 HappyGhastEntity::getBaseHeight() const
{
    return 4.0f;
}

// ========== 繁殖 ==========

bool HappyGhastEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // 对齐 vanilla HappyGhast.isFood：ItemStack.is(ItemTags.HAPPY_GHAST_FOOD)（雪球）。
    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return false;
    }
    return item->isIn(item::tag::ItemTags::HAPPY_GHAST_FOOD());
}

bool HappyGhastEntity::canBreed() const
{
    // 对齐 vanilla HappyGhast.canFallInLove = false：快乐恶魂不能进入发情状态。
    // 喂食雪球只用于诱惑/加速幼体成长，幼体只能由干燥恶魂方块孵化产生。
    return false;
}

bool HappyGhastEntity::canMateWith(const AnimalEntity& other) const
{
    return AnimalEntity::canMateWith(other);
}

std::unique_ptr<AnimalEntity> HappyGhastEntity::spawnBaby(AnimalEntity& /*partner*/)
{
    auto* registry = &ecsRegistry();
    if (registry == nullptr) {
        return nullptr;
    }

    auto baby = std::make_unique<HappyGhastEntity>(EntityInstanceId(0), *registry);
    baby->setTypeId(entity::EntityTypeKeys::HAPPY_GHAST); // 工厂绕过补救：直接构造缺 typeId
    baby->setChild(true);
    baby->setPosition(x(), y(), z());
    return baby;
}

// ========== 飞行物理 ==========

void HappyGhastEntity::travel(f32 strafing, f32 vertical, f32 forward)
{
    // 对齐 vanilla LivingEntity.travelFlying（与 GhastEntity::travel 同一公式）：
    //   水中：moveRelative(0.02) → move → scale(0.8)
    //   岩浆：moveRelative(0.02) → move → scale(0.5)
    //   其余：moveRelative(0.02) → move → scale(0.91)
    constexpr f32 kFlyAcceleration = 0.02f;
    if (isInWater()) {
        moveRelative(kFlyAcceleration, strafing, vertical, forward);
        move(entity::MoverType::Self, velocity());
        scaleVelocity(0.8f);
    } else if (isInLava()) {
        moveRelative(kFlyAcceleration, strafing, vertical, forward);
        move(entity::MoverType::Self, velocity());
        scaleVelocity(0.5f);
    } else {
        moveRelative(kFlyAcceleration, strafing, vertical, forward);
        move(entity::MoverType::Self, velocity());
        scaleVelocity(0.91f);
    }
    updateTravelAnimation(false);
}

// ========== 摔落 ==========

void HappyGhastEntity::handleFallDamage(f32 /*distance*/, f32 /*damageMultiplier*/)
{
    // 对齐 vanilla HappyGhast.checkFallDamage：空实现——飞行生物免疫摔落伤害。
    // 不调用 LivingEntity::handleFallDamage，故既不结算摔伤也不播放摔落音效。
}

// ========== 声音 ==========

std::optional<ResourceLocation> HappyGhastEntity::getAmbientSound() const
{
    // 对齐 vanilla HappyGhast.getAmbientSound：幼体用 GHASTLING_AMBIENT，成体用 HAPPY_GHAST_AMBIENT。
    return isChild() ? SoundEvents::GHASTLING_AMBIENT : SoundEvents::HAPPY_GHAST_AMBIENT;
}

std::optional<ResourceLocation> HappyGhastEntity::getHurtSound(DamageSource& /*source*/) const
{
    // 对齐 vanilla HappyGhast.getHurtSound：幼体 GHASTLING_HURT，成体 HAPPY_GHAST_HURT。
    return isChild() ? SoundEvents::GHASTLING_HURT : SoundEvents::HAPPY_GHAST_HURT;
}

std::optional<ResourceLocation> HappyGhastEntity::getDeathSound() const
{
    // 对齐 vanilla HappyGhast.getDeathSound：幼体 GHASTLING_DEATH，成体 HAPPY_GHAST_DEATH。
    return isChild() ? SoundEvents::GHASTLING_DEATH : SoundEvents::HAPPY_GHAST_DEATH;
}

f32 HappyGhastEntity::getSoundVolume() const
{
    // 对齐 vanilla HappyGhast.getSoundVolume：幼体 1.0，成体 4.0。
    return isChild() ? 1.0f : 4.0f;
}

// ========== 乘客 ==========

void HappyGhastEntity::onPassengerAdded(Entity& passenger)
{
    // 对齐 vanilla HappyGhast.addPassenger:294-307：首个乘客上车时播放护目镜放下音效；
    // 若上方没有玩家则立即取消静止超时，否则把超时压到 10 tick 内。
    if (m_world != nullptr && !m_world->isClientSide()) {
        if (getPassengers().size() == 1) {
            playSound(SoundEvents::HARNESS_GOGGLES_DOWN, 1.0f, 1.0f);
        }
        if (!scanPlayerAbove()) {
            _setServerStillTimeout(0);
        } else if (m_serverStillTimeout > 10) {
            _setServerStillTimeout(10);
        }
    }
    AnimalEntity::onPassengerAdded(passenger);
}

void HappyGhastEntity::onPassengerRemoved(Entity& passenger)
{
    // 对齐 vanilla HappyGhast.removePassenger:309-320：任何乘客下车都把静止超时设为 10
    // （避免刚下车又被立即驾驶）；最后一名乘客离开时清空 home 并播放护目镜抬起音效。
    // 挽具被剪下（onSlotCleared）走同一 clearHome 清理。
    if (m_world != nullptr && !m_world->isClientSide()) {
        _setServerStillTimeout(10);
        if (getPassengers().empty()) {
            clearHome();
            playSound(SoundEvents::HARNESS_GOGGLES_UP, 1.0f, 1.0f);
        }
    }
    AnimalEntity::onPassengerRemoved(passenger);
}

// ========== 静止超时 ==========

void HappyGhastEntity::_setServerStillTimeout(i32 ticks)
{
    // vanilla 在 0 → 正数（刚进入静止）时向追踪玩家补发一次位置同步
    // （ClientboundEntityPositionSyncPacket），使客户端立即收敛到服务端位置。
    // Cubium 尚无独立的该类包，位置同步依赖常规实体追踪包，故此处只维护计数。
    // TODO: 待 ClientboundEntityPositionSyncPacket 实现后在此补发位置同步。
    m_serverStillTimeout = std::max(0, ticks);
}

bool HappyGhastEntity::scanPlayerAbove() const
{
    // 对齐 vanilla HappyGhast.scanPlayerAboveGhast:532-546。
    if (m_world == nullptr) {
        return false;
    }

    const AxisAlignedBB box = boundingBox();
    const f64 height = box.maxY - box.minY;
    const AxisAlignedBB scanBox(
        box.minX - 1.0, box.maxY - 1.0e-5, box.minZ - 1.0, box.maxX + 1.0, box.maxY + height / 2.0, box.maxZ + 1.0);

    for (Entity* playerEntity : m_world->getPlayers()) {
        if (playerEntity == nullptr || playerEntity->isSpectator()) {
            continue;
        }

        // 玩家所在"根载具"（骑乘链最底层实体；未骑乘时即玩家本身）
        Entity* root = const_cast<Entity*>(playerEntity->getLowestRidingEntity());
        if (root == nullptr || root == this) {
            continue;
        }
        if (scanBox.contains(root->position())) {
            return true;
        }
    }
    return false;
}

void HappyGhastEntity::_stopInPlace()
{
    // 对齐 vanilla Ghast.GhastMoveControl：shouldBeStopped 为真时 operation=WAIT 并
    // stopInPlace()。Cubium 的飞行控制器没有"停住"开关，这里清空速度并中断导航，
    // 让快乐恶魂原地悬停。
    setVelocity(Vector3(0.0, 0.0, 0.0));
    clearNavigation();
}

EntityInstanceId HappyGhastEntity::getControllingPassenger() const
{
    // 对齐 vanilla HappyGhast.getControllingPassenger（HappyGhast.java:328-332）：
    //   装备挽具 && 非静止超时 && 首名乘客是玩家 → 该玩家可驾驶，否则回退基类。
    if (hasHarness() && !isOnStillTimeout() && m_world != nullptr) {
        const EntityInstanceId first = getFirstPassenger();
        if (first != INVALID_ENTITY_ID) {
            const Entity* passenger = m_world->getEntity(first);
            if (passenger != nullptr && passenger->entityType() == entity::VanillaEntityTypeKeys::PLAYER) {
                return first;
            }
        }
    }
    return Entity::getControllingPassenger();
}

void HappyGhastEntity::tick()
{
    AnimalEntity::tick();

    if (m_world == nullptr || m_world->isClientSide()) {
        return;
    }

    // 对齐 vanilla HappyGhast.tick:409-419：
    //   serverStillTimeout > 0 时自 tickCount>60 起每 tick 递减；之后若仍有玩家停在上方，
    //   把超时重置为 10（持续骑乘期间保持可驾驶）。
    if (m_serverStillTimeout > 0 && ticksExisted() > 60) {
        _setServerStillTimeout(m_serverStillTimeout - 1);
    }
    if (scanPlayerAbove()) {
        _setServerStillTimeout(10);
    }

    // 刚进入静止超时时原地停住（vanilla 由 GhastMoveControl 每 tick 检查，效果等价）。
    const bool onStillTimeout = isOnStillTimeout();
    if (onStillTimeout && !m_wasOnStillTimeout) {
        _stopInPlace();
    }
    m_wasOnStillTimeout = onStillTimeout;

    // 对齐 vanilla HappyGhast.aiStep:430 的 continuousHeal。
    _continuousHeal();
}

void HappyGhastEntity::_continuousHeal()
{
    if (m_world == nullptr || !isAlive() || isDead() || health() >= maxHealth()) {
        return;
    }

    // 云层中或该位置正在降水 → 每 20 tick 回 1 点，否则每 600 tick 回 1 点。
    // vanilla 用 precipitationAt(...) != NONE（雨雪皆算），故此处同样不区分雨雪。
    const bool inClouds = m_world->isInCloudLayer(y(), y() + height());
    const bool precipitating = inClouds ||
        m_world->precipitationAt(BlockPos(x(), y(), z())) != world::biome::BiomeClimate::Precipitation::None;
    const i64 interval = precipitating ? 20 : 600;
    if (static_cast<i64>(ticksExisted()) % interval == 0) {
        heal(1.0f);
    }
}

// ========== 挽具 / 骑乘 ==========

bool HappyGhastEntity::hasHarness() const
{
    return !getEquipment(EquipmentSlot::Body).isEmpty();
}

const ItemStack& HappyGhastEntity::harnessStack() const
{
    return getEquipment(EquipmentSlot::Body);
}

bool HappyGhastEntity::canBeRidden(const Entity& vehicle) const
{
    // 注意：本方法问的是"载具是否可被骑"，不是"乘客能否上载具"——vanilla Entity.canRide
    // 是乘客侧方法（检查乘客潜行状态），对应 Cubium 的 Entity::canBeRidden(passenger)，
    // 但 Cubium 的虚派发落在载具上，不能把基类的乘客侧条件（!this->isSneaking()）转调进来，
    // 否则乘客潜行状态永远不会被检查（实体会用载具自身状态判断）。
    //
    // 对齐 vanilla HappyGhast：只有"存活 + 非幼体 + 已装备挽具"的成体承载玩家。
    (void)vehicle;
    return isAlive() && !isChild() && hasHarness();
}

ActionResultType HappyGhastEntity::interactMob(Player& player, Hand hand)
{
    // 对齐 vanilla HappyGhast.mobInteract（HappyGhast.java:266-285）：
    //   幼体 → 交基类（AnimalEntity 喂食雪球加速成长）；
    //   成体 → 手持物品先走 item.interactLivingEntity（装备挽具），可执行则结束；
    //          已装备挽具且玩家未潜行（isSecondaryUseActive=false）→ 让玩家骑乘；
    //          否则交基类。
    if (!isChild()) {
        // 装备挽具的入口由 MobEntity::processInitialInteract 的第 4 步统一提供
        // （canEquipItemOnInteract 判定），此处只处理"已装备 → 骑乘"分支。
        // vanilla 在 mobInteract 内重复调用 interactLivingEntity 属冗余，行为等价。

        if (hasHarness() && !player.isSneaking()) {
            if (m_world != nullptr && !m_world->isClientSide()) {
                player.startRiding(*this);
            }
            return ActionResultType::Success;
        }
    }

    return AnimalEntity::interactMob(player, hand);
}

bool HappyGhastEntity::canEquipItemOnInteract(const ItemStack& stack, EquipmentSlot slot) const
{
    // 对齐 vanilla Equippable.harness：slot=BODY、equivOnInteract=true、
    // allowedEntities=CAN_EQUIP_HARNESS。Cubium 以实体侧判定等价表达：
    // 目标必须是可装备挽具的实体（实时查标签，数据包扩展成员自动生效），
    // 且挽具只能装到 Body 槽。
    const Item* item = stack.getItem();
    if (item == nullptr || slot != EquipmentSlot::Body) {
        return false;
    }
    // 标签未初始化时按"不可装备"处理（拒绝而非越权装备）。
    if (!EntityTypeTags::isInitialized()) {
        return false;
    }
    return EntityTypeTags::CAN_EQUIP_HARNESS().contains(getTypeId());
}

std::optional<ResourceLocation> HappyGhastEntity::shearingSoundForSlot(EquipmentSlot slot) const
{
    // vanilla Equippable.harness 的 setShearingSound(HARNESS_UNEQUIP)。快乐恶魂只有 Body 槽
    // 会装挽具（CAN_EQUIP_HARNESS 范畴内 Body 槽即为挽具槽），其余槽位沿用基类默认音效。
    if (slot == EquipmentSlot::Body) {
        return SoundEvents::HARNESS_UNEQUIP;
    }
    return std::nullopt;
}

void HappyGhastEntity::onSlotCleared(EquipmentSlot slot)
{
    // 对齐 vanilla HappyGhast.removePassenger:316-317：挽具被剪下后清空 home 位置
    // （快乐恶魂不再有"归巢"目的地，需重新建立一个）。
    if (slot == EquipmentSlot::Body) {
        clearHome();
    }
}

// ========== AI 目标注册 ==========

void HappyGhastEntity::registerGoals()
{
    AnimalEntity::registerGoals();

    // 对齐 vanilla HappyGhast.registerGoals（registerGoals 内的目标优先级）：
    //   3: FloatGoal（幼体持续上浮）——vanilla 的 HappyGhastFloatGoal + FlyingMoveControl(180, true)
    //   4: TemptGoal（TemptGoal.ForNonPathfinders，速度 1.0）
    //   5: Ghast.RandomFloatAroundGoal（16 格半径随机巡游）——此处复用成年随机飞行目标
    // 本项目无 Brain，vanilla HappyGhastAi 的其余行为（跟随拴绳持有者、载客等）尚未实现（见类注释 TODO）。
    m_goalSelector.addGoal(3, std::make_unique<entity::ai::goal::GhastHappyFloatGoal>(this));

    // 优先级 4: 食物诱惑（对齐 vanilla TemptGoal.ForNonPathfinders，速度 1.0）。原版判据：
    //   未装备身体防具（挽具）且为成体 → HAPPY_GHAST_TEMPT_ITEMS（食物 + 挽具）；
    //   其余（幼体或已装备挽具）→ HAPPY_GHAST_FOOD（雪球）。
    m_goalSelector.addGoal(4,
        std::make_unique<entity::ai::goal::TemptGoal>(
            this,
            1.0,
            [this](const ItemStack& stack) -> bool {
                const Item* item = stack.getItem();
                if (item == nullptr) {
                    return false;
                }
                if (!isWearingBodyArmor() && !isChild()) {
                    return item->isIn(item::tag::ItemTags::HAPPY_GHAST_TEMPT_ITEMS());
                }
                return item->isIn(item::tag::ItemTags::HAPPY_GHAST_FOOD());
            },
            false));

    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::GhastRandomFlyGoal>(this));
}

// ========== 属性注册 ==========

void HappyGhastEntity::registerAttributes()
{
    AnimalEntity::registerAttributes();

    // 对齐 vanilla HappyGhast.createAttributes：
    //   Animal.createAnimalAttributes() + MAX_HEALTH=20 + TEMPT_RANGE=16 + FLYING_SPEED=0.05
    //   + MOVEMENT_SPEED=0.05 + FOLLOW_RANGE=16 + CAMERA_DISTANCE=8
    // （TEMPT_RANGE / CAMERA_DISTANCE 属性本项目未建模，故只设置已存在的四项。）
    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
    attributes().setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 16.0);
    attributes().setBaseValue(entity::attribute::Attributes::FLYING_SPEED, 0.05);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.05);
}

} // namespace mc