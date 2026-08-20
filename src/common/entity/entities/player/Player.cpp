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

#include "Player.hpp"
#include "../../../core/Constants.hpp"
#include "../../../item/armor/ArmorMaterial.hpp"
#include "../../../item/core/ActionResult.hpp"
#include "../../../item/enchantment/EnchantmentHelper.hpp"
#include "../../../item/enchantment/enchantments/AllEnchantments.hpp"
#include "../../../item/enchantment/enchantments/mace/WindBurstEnchantment.hpp"
#include "../../../item/items/armor/ArmorItem.hpp"
#include "../../../item/items/tool/SwordItem.hpp"
#include "../../../item/items/trial/MaceItem.hpp"
#include "../../../physics/PhysicsConstants.hpp"
#include "../../../physics/PhysicsEngine.hpp"
#include "../../../sound/SoundCategory.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../../util/AxisAlignedBB.hpp"
#include "../../../util/math/MathUtils.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../util/math/ray/Raycast.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/Block.hpp"
#include "../../../world/block/BlockState.hpp"
#include "../../../world/dimension/MapDimensionId.hpp"
#include "../../../world/explosion/ExplosionImmunityContext.hpp"
#include "../../../world/gamerule/GameRules.hpp"
#include "../../attribute/AttributeModifier.hpp"
#include "../../attribute/AttributeModifierUUIDs.hpp"
#include "../../attribute/EntityDefaultAttributes.hpp"
#include "../../combat/PlayerAttackHelper.hpp"
#include "../../core/EntityRegistry.hpp"
#include "../../damage/DamageSource.hpp"
#include "../../entities/effect/EffectEntities.hpp"
#include "../../entities/item/ItemEntity.hpp"
#include "../../experience/ExperienceDropHandler.hpp"
#include "../../experience/ExperienceManager.hpp"
#include "../../inventory/CreativeInventory.hpp"
#include "../../inventory/INamedContainerProvider.hpp"
#include "../../inventory/Slot.hpp"
#include "../../registry/VanillaEntityTypeKeys.hpp"
#include "../../serialization/EntityNbtKeys.hpp"
#include "../../serialization/NbtHelper.hpp"
#include "../../utils/ItemDropHelper.hpp"
#include "GameModeUtils.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/ecs/components/PlayerScoreComponent.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/player/PlayerModelPart.hpp"
#include "common/entity/player/SleepResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/enchantment/enchantments/tool/EfficiencyEnchantment.hpp"
#include "common/mod/bedrock/addon/component/ItemComponentEvents.hpp"
#include "common/mod/bedrock/addon/component/ItemComponentRegistry.hpp"
#include "common/network/protocol/EntityEvents.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/scoreboard/core/Team.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/Vector2.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/ray/Ray.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/GlobalPos.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace mc {

namespace {

/// 获取玩家指定姿态的宽度
/// Sleeping 姿态宽度为 0.2，其他姿态为 0.6
[[nodiscard]] f32 getPlayerPoseWidth(EntityPose pose)
{
    if (pose == EntityPose::Sleeping) {
        return 0.2f;
    }
    return Player::PLAYER_WIDTH;
}

[[nodiscard]] f32 getPlayerPoseHeight(EntityPose pose)
{
    switch (pose) {
        case EntityPose::Sleeping:
            return 0.2f;
        case EntityPose::Swimming:
        case EntityPose::FallFlying:
        case EntityPose::SpinAttack:
            return Player::PLAYER_SWIM_HEIGHT;
        case EntityPose::Crouching:
            return Player::PLAYER_CROUCH_HEIGHT;
        default:
            return Player::PLAYER_HEIGHT;
    }
}

[[nodiscard]] f32 getPlayerPoseEyeHeight(EntityPose pose)
{
    switch (pose) {
        case EntityPose::Sleeping:
            return 0.2f;
        case EntityPose::Swimming:
        case EntityPose::FallFlying:
        case EntityPose::SpinAttack:
            return 0.4f;
        case EntityPose::Crouching:
            return 1.27f;
        default:
            return Player::PLAYER_EYE_HEIGHT;
    }
}

constexpr f32 PLAYER_POSE_FIT_EPSILON = 1.0e-4f;

} // namespace

// ============================================================================
// Player 实现
// ============================================================================

// 静态数据参数（createKey 返回哨兵 0xFFFF，真实 id 在 registerData/registerParam
// 时沿继承链分配，对齐 vanilla ClassTreeIdRegistry）。
entity::DataParameter<entity::HumanoidArmValue> Player::DATA_PLAYER_MAIN_HAND_PARAM =
    entity::EntityDataManager::createKey<entity::HumanoidArmValue>();
entity::DataParameter<i8> Player::DATA_PLAYER_MODE_CUSTOMISATION_PARAM = entity::EntityDataManager::createKey<i8>();
entity::DataParameter<f32> Player::DATA_PLAYER_ABSORPTION_PARAM = entity::EntityDataManager::createKey<f32>();
entity::DataParameter<i32> Player::DATA_PLAYER_SCORE_PARAM = entity::EntityDataManager::createKey<i32>();
entity::DataParameter<entity::OptionalUnsignedIntValue> Player::DATA_PLAYER_SHOULDER_PARROT_LEFT_PARAM =
    entity::EntityDataManager::createKey<entity::OptionalUnsignedIntValue>();
entity::DataParameter<entity::OptionalUnsignedIntValue> Player::DATA_PLAYER_SHOULDER_PARROT_RIGHT_PARAM =
    entity::EntityDataManager::createKey<entity::OptionalUnsignedIntValue>();

// 继承链标识（复刻 vanilla ClassTreeIdRegistry，parent = LivingEntity::classInfo()）。
const entity::EntityClassInfo& Player::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"Player", &LivingEntity::classInfo()};
    return s_classInfo;
}

Player::Player(EntityInstanceId id, const std::string& username, ecs::EntityRegistry& registry)
    : LivingEntity(id, nullptr, registry)
    , m_username(username)
    , m_experienceManager(std::make_unique<entity::experience::ExperienceManager>(*this))
{
    // 第四批：attach PlayerScoreComponent（承载 m_score，真相源，DATA_PLAYER_SCORE_PARAM 退为镜像）。
    // 须在 registerData() 之前 attach——后者经 getScore() 取组件填默认值。LivingEntity 基类构造
    // 已完成，m_entityContext 已就位。
    m_entityContext->enttRegistry().emplace<ecs::PlayerScoreComponent>(m_entityContext->entity());

    // 绑定玩家类型标识。Player 在 EntityRegistry 中注册为 "minecraft:player"
    // （factory=nullptr，Player 不走注册表工厂）。此处仅设置 m_typeId，entityType()
    // 首次访问时懒查表填充指针，与 VanillaEntityTypeKeys::PLAYER 同源可指针比较。
    setTypeId(entity::EntityTypeKeys::PLAYER);

    // 注册玩家属性
    registerAttributes();

    // 显式调用 registerData() 注册同步数据参数
    // 由于 C++ 虚函数在基类构造函数中不会派发到派生类（Entity::Entity 内部调用
    // registerData() 时调用的是 Entity::registerData 而非 Player::registerData），
    // 必须在派生类构造函数中显式调用，参考 MobEntity 模式。
    registerData();

    // 生成随机XP seed
    math::Random rng(static_cast<u64>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    m_experienceManager->resetXpSeed(rng);
}

Player::~Player() = default;

void Player::registerData()
{
    // 先调用父类方法，确保 LivingEntity(id8-14) 与 Entity(id0-7) 字段已注册。
    LivingEntity::registerData();

    // 标记当前正在注册 Player 类的字段，使 registerParam 沿 Player 继承链分配 id
    // （续接 LivingEntity id14 之后）。RAII 守卫自动配对压栈/弹栈。
    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    // 扁平化方案（同 BoatEntity，对齐 vanilla Avatar + Player 字段集）：
    // 先注册 vanilla Avatar 两字段，再注册 vanilla Player 四字段。
    // Avatar 字段（vanilla net.minecraft.world.entity.player.Avatar.defineId）：
    //   DATA_PLAYER_MAIN_HAND(id15) HumanoidArm，默认 RIGHT(ordinal 1)
    //   DATA_PLAYER_MODE_CUSTOMISATION(id16) Byte，默认 127（皮肤全部件显示）
    m_dataManager.registerParam(DATA_PLAYER_MAIN_HAND_PARAM, entity::HumanoidArmValue{1}); // RIGHT
    m_dataManager.registerParam(DATA_PLAYER_MODE_CUSTOMISATION_PARAM, static_cast<i8>(PLAYER_MODEL_PARTS_ALL_MASK));

    // Player 字段（vanilla net.minecraft.world.entity.player.Player.defineId）：
    //   DATA_PLAYER_ABSORPTION(id17) Float，默认 0.0F
    //   DATA_PLAYER_SCORE(id18) Int，默认 0
    //   DATA_PLAYER_SHOULDER_PARROT_LEFT(id19) OptionalUnsignedInt，默认 absent
    //   DATA_PLAYER_SHOULDER_PARROT_RIGHT(id20) OptionalUnsignedInt，默认 absent
    // DATA_PLAYER_ABSORPTION 由 Player::setAbsorptionAmount 重写下发（调基类写 HurtStateComponent
    //   含 clamp 后，再 m_dataManager.set 同步该字段）。真相源在 HurtStateComponent.m_absorption。
    //   肩膀鹦鹉两字段本项目无对应玩法，恒为 absent，不影响 wire 正确性。
    m_dataManager.registerParam(DATA_PLAYER_ABSORPTION_PARAM, 0.0f);
    m_dataManager.registerParam(DATA_PLAYER_SCORE_PARAM, static_cast<i32>(0));
    m_dataManager.registerParam(DATA_PLAYER_SHOULDER_PARROT_LEFT_PARAM, entity::OptionalUnsignedIntValue{false, 0});
    m_dataManager.registerParam(DATA_PLAYER_SHOULDER_PARROT_RIGHT_PARAM, entity::OptionalUnsignedIntValue{false, 0});
}

void Player::setPosition(f32 x, f32 y, f32 z)
{
    Entity::setPosition(x, y, z);
    snapshotInterpolationState();

    // 外部改坐标时同步复位步距采样，避免沿用旧位移或旧脚步阈值
    m_moveDistanceSamplePosition = m_builtIn.stateVector->m_pos;
    m_moveDistanceWalked = 0.0f;
    m_prevMoveDistanceWalked = 0.0f;
    m_moveDistanceSwam = 0.0f;
    m_prevMoveDistanceSwam = 0.0f;
    m_cameraYaw = 0.0f;
    m_prevCameraYaw = 0.0f;
    m_distanceWalkedOnStep = 0.0f;
    m_nextStepDistance = 1.0f;
    m_shouldPlayStepSound = false;
    m_shouldPlaySwimSound = false;
    m_swimSoundVolume = 0.0f;
}

void Player::setCameraEntityId(std::optional<EntityInstanceId> entityId)
{
    if (m_cameraEntityId == entityId) {
        return;
    }
    std::optional<EntityInstanceId> oldCameraId = m_cameraEntityId;
    m_cameraEntityId = entityId;
    onCameraEntityChanged(oldCameraId, entityId);
}

void Player::setGameMode(GameMode mode)
{
    const GameMode oldMode = m_gameMode;
    m_gameMode = mode;

    // 使用 GameModeUtils 更新能力
    m_abilities = entity::GameModeUtils::getAbilitiesForGameMode(mode);

    // 切换到创造模式时重置冲量上下文（对应 MC ServerPlayerGameMode 切换游戏模式时的重置）
    if (isCreative()) {
        resetCurrentImpulseContext();
    }

    // 旁观者模式 noclip 设置
    // 切换到旁观者模式时启用 noclip（穿透碰撞），离开旁观者模式时关闭
    if (isSpectator()) {
        setNoClip(true);
    } else if (oldMode == GameMode::Spectator) {
        setNoClip(false);
        // 离开旁观者模式时清除旁观目标
        // 通过 setCameraEntityId() 触发 onCameraEntityChanged()，
        // ServerPlayer 将发送 SetCameraPacket 给客户端以同步摄像机状态
        setCameraEntityId(std::nullopt);
    }

    // 同步移动速度到属性系统
    // PlayerAbilities 是配置层，属性系统是计算层
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, static_cast<f64>(m_abilities.walkSpeed));

    // 根据游戏模式刷新创造模式交互距离修饰符
    // 对应 MC 1.21.11 ServerPlayer.updatePlayerAttributes()：创造模式 +0.5/+2.0，其他模式移除
    _applyCreativeInteractionRangeModifiers();
}

bool Player::mayInteract(IWorld& world, const BlockPos& pos) const
{
    // 旁观模式：完全禁止交互
    if (isSpectator()) {
        return false;
    }

    // 非冒险模式：允许交互
    if (!isAdventure()) {
        return true;
    }

    // 冒险模式：检查手持物品的 CanPlaceOn 标签
    // 参考 MC Java 的 Player.mayUseItemAt(BlockPos, Direction, ItemStack)：
    // 冒险模式下，玩家可以使用主手或副手中带有 CanPlaceOn 标签的物品与方块交互。
    // 任一只手的物品有匹配的 CanPlaceOn 标签即允许交互。
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr || state->isAir()) {
        return false;
    }

    // 检查主手
    const ItemStack& mainHand = getHeldItem(Hand::MainHand);
    if (!mainHand.isEmpty() && mainHand.hasCanPlaceOn()) {
        if (mainHand.canPlaceOnBlockInAdventureMode(world, pos, *state)) {
            return true;
        }
    }

    // 检查副手
    const ItemStack& offHand = getHeldItem(Hand::OffHand);
    if (!offHand.isEmpty() && offHand.hasCanPlaceOn()) {
        if (offHand.canPlaceOnBlockInAdventureMode(world, pos, *state)) {
            return true;
        }
    }

    // 冒险模式下没有 CanPlaceOn 标签的物品不能与方块交互
    return false;
}

bool Player::mayUseItemAt(IWorld& world, const BlockPos& pos, Direction facing, const ItemStack& itemStack) const
{
    // 有建造权限时直接允许
    if (m_abilities.allowEdit) {
        return true;
    }

    // 无建造权限（冒险/旁观模式）：检查物品的 CanPlaceOn 标签
    // 参考 MC Java Player.mayUseItemAt：
    // 检查目标方块对面的方块是否在物品的 CanPlaceOn 列表中
    const BlockPos adjacentPos = pos.offset(Directions::opposite(facing));
    const BlockState* adjacentState = world.getBlockState(adjacentPos);
    if (adjacentState == nullptr || adjacentState->isAir()) {
        return false;
    }

    if (!itemStack.isEmpty() && itemStack.hasCanPlaceOn()) {
        return itemStack.canPlaceOnBlockInAdventureMode(world, adjacentPos, *adjacentState);
    }

    return false;
}

bool Player::blockActionRestricted(IWorld& world, const BlockPos& pos) const
{
    // 非冒险/旁观模式：不限制
    if (!entity::GameModeUtils::isBlockPlacingRestricted(m_gameMode)) {
        return false;
    }

    // 旁观模式：完全限制
    if (isSpectator()) {
        return true;
    }

    // 有建造权限时不限制
    if (m_abilities.allowEdit) {
        return false;
    }

    // 冒险模式无建造权限：检查主手物品的 CanDestroy 标签
    const ItemStack& mainHand = getHeldItem(Hand::MainHand);
    if (mainHand.isEmpty()) {
        return true;
    }

    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return true;
    }

    return !mainHand.canBreakBlockInAdventureMode(world, pos, *state);
}

// ============================================================================
// 生命值管理（覆盖 LivingEntity 方法）
// ============================================================================

void Player::setHealth(f32 health)
{
    // 直接调用父类方法
    LivingEntity::setHealth(health);
}

void Player::heal(f32 amount)
{
    // 直接调用父类方法
    LivingEntity::heal(amount);
}

// ============================================================================
// 经验系统 - 委托给 ExperienceManager
// ============================================================================

void Player::addExperience(i32 amount)
{
    m_experienceManager->addExperience(amount);
}

void Player::setExperienceLevel(i32 level)
{
    m_experienceManager->setLevel(level);
}

void Player::addExperienceLevels(i32 levels)
{
    m_experienceManager->addLevels(levels);
}

bool Player::consumeExperience(i32 amount)
{
    return m_experienceManager->consumeExperience(amount);
}

bool Player::consumeExperienceLevels(i32 levels)
{
    return m_experienceManager->consumeLevels(levels);
}

i32 Player::experienceBarCapacity() const
{
    return m_experienceManager->getExperienceForNextLevel();
}

void Player::setExperience(i32 level, f32 progress, i32 totalExperience)
{
    m_experienceManager->setExperience(level, progress, totalExperience);
}

void Player::sendStatusMessage(const std::string& message, bool actionBar)
{
    // Player 基类默认实现：空操作
    // ServerPlayer 会重写此方法以发送网络消息
    // 客户端 Player 可能会直接显示在聊天界面
    (void)message;
    (void)actionBar;
}

void Player::dropExperience()
{
    // 玩家死亡时掉落经验
    if (m_world && m_experienceManager->getLevel() > 0) {
        i32 xpToDrop = m_experienceManager->calculateDeathDropXp();
        if (xpToDrop > 0) {
            math::Random rng(
                static_cast<u64>(m_id) ^ static_cast<u64>(std::chrono::system_clock::now().time_since_epoch().count()));
            entity::ExperienceDropHandler::spawnExperienceOrbs(m_world, x(), y(), z(), xpToDrop, &rng);
        }
    }
}

ItemEntity* Player::dropItem(ItemStack& stack, bool dropAround, bool traceItem)
{
    if (stack.isEmpty() || m_world == nullptr) {
        return nullptr;
    }

    // 获取随机数生成器
    math::Random rng(
        static_cast<u64>(m_id) ^ static_cast<u64>(std::chrono::system_clock::now().time_since_epoch().count()));

    // 计算掉落位置（玩家眼睛高度 - 0.3）
    f64 dropY = static_cast<f64>(y()) + static_cast<f64>(eyeHeight()) - 0.3;

    // 获取掉落速度
    Vector3 velocity = ItemDropHelper::getPlayerDropVelocity(
        rng, dropAround, m_builtIn.rotation->m_rot.x, m_builtIn.rotation->m_rot.y);

    // 生成物品实体
    ItemEntity* itemEntity = ItemDropHelper::spawnItemEntity(m_world,
        stack,
        x(),
        dropY,
        z(),
        velocity.x,
        velocity.y,
        velocity.z,
        ItemDropHelper::DEFAULT_PICKUP_DELAY,
        traceItem ? uuid() : "");

    if (itemEntity != nullptr) {
        // 挥手动画（客户端）
        // 清空物品堆
        stack = ItemStack::EMPTY;
    }

    return itemEntity;
}

ItemEntity* Player::dropItem(ItemStack& stack, bool unused)
{
    MC_UNUSED(unused);
    // 简化实现：直接调用完整版本
    return dropItem(stack, false, true);
}

void Player::damageArmor(DamageSource& source, f32 amount)
{
    m_inventory.damageArmor(source, amount);
}

// ============================================================================

void Player::setSprinting(bool sprinting)
{
    m_isSprinting = sprinting;
    if (sprinting) {
        addFlag(EntityFlags::Sprinting);
    } else {
        removeFlag(EntityFlags::Sprinting);
    }
}

void Player::setSneaking(bool sneaking)
{
    if (sneaking) {
        m_isSneaking = true;
        addFlag(EntityFlags::Crouching);
        setPose(EntityPose::Crouching);
        return;
    }

    if (_canFitPose(EntityPose::Standing)) {
        m_isSneaking = false;
        removeFlag(EntityFlags::Crouching);
        setPose(EntityPose::Standing);
        return;
    }

    m_isSneaking = true;
    addFlag(EntityFlags::Crouching);
    setPose(EntityPose::Crouching);
}

void Player::setSwimming(bool swimming)
{
    m_isSwimming = swimming;
    if (swimming) {
        addFlag(EntityFlags::Swimming);
        setPose(EntityPose::Swimming);
        return;
    }

    removeFlag(EntityFlags::Swimming);

    if (_canFitPose(EntityPose::Standing)) {
        m_isSneaking = false;
        removeFlag(EntityFlags::Crouching);
        setPose(EntityPose::Standing);
        return;
    }

    setSneaking(true);
}

void Player::toggleFlying()
{
    if (!m_abilities.canFly) {
        return; // 不允许飞行则无法切换
    }
    m_abilities.flying = !m_abilities.flying;
}

void Player::setSleeping(bool sleeping)
{
    m_isSleeping = sleeping;
    if (sleeping) {
        setPose(EntityPose::Sleeping);
        return;
    }

    if (_canFitPose(EntityPose::Standing)) {
        m_isSneaking = false;
        removeFlag(EntityFlags::Crouching);
        setPose(EntityPose::Standing);
        return;
    }

    setSneaking(true);
}

void Player::startSleeping(const BlockPos& pos)
{
    // 设置睡眠位置
    m_sleepingPosition = pos;

    // 设置睡眠状态（这会切换姿态）
    setSleeping(true);

    // 重置睡眠计时器
    m_sleepTimer = 0;

    // 清除速度
    setVelocity(Vector3(0.0f, 0.0f, 0.0f));

    // 通知世界玩家睡眠状态变化
    // ServerWorld 会重写此方法调用 updateAllPlayersSleepingFlag()
    if (m_world != nullptr) {
        m_world->onPlayerSleepingChanged();
    }
}

entity::SleepResult Player::tryStartSleeping(const BlockPos& bedPos)
{
    // 基类实现：简单调用 startSleeping，不做验证
    // ServerPlayer 会重写此方法进行完整验证
    startSleeping(bedPos);
    return entity::SleepResult::OK;
}

void Player::stopSleeping()
{
    if (!m_isSleeping) {
        return;
    }

    // 清除睡眠位置
    m_sleepingPosition = std::nullopt;

    // 设置睡眠状态为 false（这会尝试切换到站立姿态）
    setSleeping(false);

    // 通知世界玩家睡眠状态变化
    // ServerWorld 会重写此方法调用 updateAllPlayersSleepingFlag()
    if (m_world != nullptr) {
        m_world->onPlayerSleepingChanged();
    }

    // 注意：睡眠计时器在 tick() 中会处理唤醒后的渐变
    // 唤醒后 m_sleepTimer 会继续增加到 110 然后重置
}

void Player::setSpawnPoint(DimensionId dimension, const BlockPos& pos, bool forced)
{
    m_spawnPoint = GlobalPos(dimension, pos);
    m_spawnForced = forced;
}

f32 Player::height() const
{
    return getPlayerPoseHeight(pose());
}

f32 Player::eyeHeight() const
{
    return getPlayerPoseEyeHeight(pose());
}

entity::EntitySize Player::getDimensions(EntityPose pose) const
{
    // Sleeping 姿态使用固定宽度 0.2
    return entity::EntitySize(getPlayerPoseWidth(pose), getPlayerPoseHeight(pose), getPlayerPoseEyeHeight(pose), false);
}

bool Player::_canFitPose(EntityPose pose) const
{
    if (pose == this->pose() || m_world == nullptr) {
        return true;
    }

    AxisAlignedBB candidateBox =
        getDimensions(pose)
            .makeBoundingBox(
                m_builtIn.stateVector->m_pos.x, m_builtIn.stateVector->m_pos.y, m_builtIn.stateVector->m_pos.z)
            .shrink(PLAYER_POSE_FIT_EPSILON);
    return !m_world->hasBlockCollision(candidateBox) && !m_world->hasEntityCollision(candidateBox, this);
}

void Player::tick()
{
    LivingEntity::tick();

    // 注意：此遍历列方块用于调试观察，且无世界时（如基类单元测试直接 tick）不应崩溃。
    // 与下方 worldBorder / foodStats 等访问保持一致的 m_world 空检查守卫。
    // if (m_world != nullptr) {
    //     for (i32 i = world::MIN_BUILD_HEIGHT; i < world::MAX_BUILD_HEIGHT; ++i) {
    //         auto bs = m_world->getBlockState(BlockPos(x(), i, z()));
    //         if (bs == nullptr) {
    //             // spdlog::error("Failed to get block state at y={}", i);
    //             continue;
    //         }
    //         std::stringstream ss;
    //         ss << "Block at y=" << i << " is " << bs->getBlock().toString();
    //         // spdlog::warn(ss.str());
    //     }
    // }

    // 更新 XP 冷却
    if (m_xpCooldown > 0) {
        m_xpCooldown--;
    }

    // 更新冲量上下文宽限期计时器
    if (m_currentImpulseContextResetGraceTime > 0) {
        m_currentImpulseContextResetGraceTime--;
    }

    // 当玩家着地、进入水中或正在攀爬时，尝试重置冲量上下文
    // 对应 MC ServerGamePacketListenerImpl 中处理玩家移动数据包时的 tryResetCurrentImpulseContext 调用
    // 注意：宽限期期间不会重置（tryResetCurrentImpulseContext 会检查 graceTime == 0）
    if (m_builtIn.physicsState->m_onGround || isInWater() || isOnLadder()) {
        tryResetCurrentImpulseContext();
    }

    // 更新攻击冷却
    m_ticksSinceLastAttack++;

    // 更新物品切换缩放计时器（对应 MC Player.tick：itemSwapTicker++ 并在主手物品种类
    // 切换时重置）。getItemSwapScale 据此计算第一人称装备动画的“举起”进度。
    m_itemSwapTicker++;
    const ItemStack mainHandItem = getMainHandItem();
    if (!m_lastItemInMainHand.isSameItem(mainHandItem)) {
        m_itemSwapTicker = 0;
    }
    m_lastItemInMainHand = mainHandItem;

    // 更新物品冷却追踪器
    m_cooldownTracker.tick();

    // 更新监守者警告效果（每 tick 递减冷却，冷却归零后递减警告等级）
    m_wardenWarningEffect.tick();

    // 世界边界伤害检测
    // 只有玩家会受到边界伤害
    if (m_world != nullptr && !isSpectator() && !m_abilities.invulnerable) {
        const auto& border = m_world->worldBorder();

        // 检查玩家碰撞箱是否与边界相交
        if (!border.intersects(boundingBox())) {
            // 玩家在边界外，计算伤害
            // MC: distance = getClosestDistance(entity) + damageBuffer
            double distance = border.getClosestDistance(boundingBox()) + border.getDamageBuffer();

            // 距离为负表示超出缓冲区
            if (distance < 0.0 && border.getDamagePerBlock() > 0.0) {
                // 计算伤害：max(1, floor(-distance * damagePerBlock))
                i32 damage = std::max(1, static_cast<i32>(std::floor(-distance * border.getDamagePerBlock())));

                // 应用 IN_WALL 伤害
                auto damageSource = DamageSources::inWall();
                hurt(damageSource, static_cast<f32>(damage));
            }
        }
    }

    // 睡眠计时器逻辑
    // 睡眠时：每 tick 递增，上限 100
    // 唤醒后：计时器继续增加到 110 后才重置为 0（用于唤醒动画）
    if (m_isSleeping) {
        m_sleepTimer++;
        if (m_sleepTimer > 100) {
            m_sleepTimer = 100;
        }
    } else if (m_sleepTimer > 0) {
        m_sleepTimer++;
        if (m_sleepTimer >= 110) {
            m_sleepTimer = 0;
        }
    }

    // 饥饿系统 tick
    // 只有生存模式和冒险模式才处理饥饿
    if (m_gameMode == GameMode::Survival || m_gameMode == GameMode::Adventure) {
        // 从世界获取游戏规则 naturalRegeneration
        bool naturalRegeneration =
            m_world ? m_world->getGameRules().getBoolean(world::gamerule::GameRuleKeys::NATURAL_REGENERATION) : true;
        // 从世界获取难度，如果没有世界则默认为 Normal
        Difficulty difficulty = m_world ? m_world->difficulty() : Difficulty::Normal;
        m_foodStats.tick(*this, difficulty, naturalRegeneration);
    }

    // 更新游泳状态和动画
    updateSwimming();

    // 更新姿态
    updatePose();

    // 更新空气供应和溺水
    updateAirSupply();

    // 更新移动距离（用于视野晃动）
    updateMoveDistance();

    // 检测与附近实体的碰撞（拾取物品、箭矢等）
    checkEntityCollisions();
}

void Player::checkEntityCollisions()
{
    // 只在存活且非观察者模式时检测碰撞
    if (!isAlive() || isSpectator()) {
        return;
    }

    if (m_world == nullptr) {
        return;
    }

    // 创建搜索盒：玩家碰撞箱扩展1格（水平和垂直）
    AxisAlignedBB searchBox = boundingBox().expand(1.0f, 0.5f, 1.0f);

    // 获取搜索盒内的所有实体
    std::vector<Entity*> nearbyEntities = m_world->getEntitiesInAABB(searchBox, this);

    for (Entity* entity : nearbyEntities) {
        if (entity == nullptr || entity->isRemoved()) {
            continue;
        }
        // 调用实体的碰撞回调
        entity->onCollideWithPlayer(*this);
    }
}

void Player::update()
{
    Entity::update();
}

/**
 * @brief 处理移动输入
 *
 * MC坐标系: yaw=0 看向 -Z, yaw=90 看向 +X
 * - forward: 正值向前走, 负值向后走
 * - strafe: 正值向右走, 负值向左走
 *
 * MC公式:
 *   sinYaw = sin(yaw * PI/180)
 *   cosYaw = cos(yaw * PI/180)
 *   moveX = strafe * cosYaw - forward * sinYaw
 *   moveZ = forward * cosYaw + strafe * sinYaw
 *
 * 重要：MC中 moveRelative 是将速度**添加**到当前速度，而不是替换！
 */
void Player::handleMovementInput(f32 forward, f32 strafe, bool jumping, bool sneaking)
{
    m_inputForward = forward;
    m_inputStrafe = strafe;
    m_inputJumping = jumping;
    m_inputSneaking = sneaking;
    m_isJumping = jumping;
}

void Player::_applyCachedMovementInput(f32 groundSlipperiness)
{
    const f32 forward = m_inputForward;
    const f32 strafe = m_inputStrafe;
    const bool jumping = m_inputJumping;
    const bool sneaking = m_inputSneaking;

    // 更新跳跃状态（用于动画等）
    m_isJumping = jumping;
    const bool wantsSneaking = sneaking && !m_abilities.flying;
    if (wantsSneaking || m_isSneaking) {
        setSneaking(wantsSneaking);
    }

    // 水中移动使用特殊物理
    if (isInWater() && !m_abilities.flying) {
        _handleWaterMovement(forward, strafe, jumping, sneaking);
        return;
    }

    // 岩浆中移动
    if (isInLava() && !m_abilities.flying) {
        _handleLavaMovement(forward, strafe, jumping, sneaking);
        return;
    }

    // 计算移动速度因子
    f32 speedFactor = m_abilities.walkSpeed;
    if (!m_abilities.flying) {
        if (m_builtIn.physicsState->m_onGround) {
            speedFactor = physics::getGroundMoveFactor(
                static_cast<f32>(
                    getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED, m_abilities.walkSpeed)),
                groundSlipperiness);
            // 地面移动因子需要乘以 getBlockSpeedFactor()
            // getBlockSpeedFactor() 使用 MOVEMENT_EFFICIENCY 属性在方块 speedFactor 和 1.0 之间插值
            speedFactor *= getBlockSpeedFactor();
        } else {
            speedFactor = m_isSprinting ? physics::SPRINT_JUMP_MOVEMENT_FACTOR : physics::JUMP_MOVEMENT_FACTOR;
        }
    }
    if (m_isSprinting && m_builtIn.physicsState->m_onGround && !m_abilities.flying) {
        speedFactor *= physics::SPRINT_SPEED_MULTIPLIER;
    }
    if (sneaking && !m_abilities.flying) {
        speedFactor *= physics::SNEAK_SPEED_MULTIPLIER;
    }
    if (m_abilities.flying) {
        speedFactor = m_abilities.flySpeed * (m_isSprinting ? physics::SPRINT_FLY_MULTIPLIER : 1.0f);
    }

    if (forward != 0.0f || strafe != 0.0f) {
        f32 yawRad = m_builtIn.rotation->m_rot.x * math::DEG_TO_RAD;
        f32 sinYaw = std::sin(yawRad);
        f32 cosYaw = std::cos(yawRad);

        f32 moveX = strafe * cosYaw - forward * sinYaw;
        f32 moveZ = forward * cosYaw + strafe * sinYaw;

        f32 length = std::sqrt(moveX * moveX + moveZ * moveZ);
        if (length > 0.0f) {
            moveX /= length;
            moveZ /= length;
        }

        m_builtIn.velocity->m_velocity.x += moveX * speedFactor;
        m_builtIn.velocity->m_velocity.z += moveZ * speedFactor;
    }

    if (m_abilities.flying) {
        i32 verticalInput = 0;
        if (jumping) {
            verticalInput += 1;
        }
        if (sneaking) {
            verticalInput -= 1;
        }
        if (verticalInput != 0) {
            f32 verticalSpeed = m_abilities.flySpeed * physics::FLY_VERTICAL_INPUT_MULTIPLIER *
                (m_isSprinting ? physics::SPRINT_FLY_MULTIPLIER : 1.0f);
            m_builtIn.velocity->m_velocity.y += static_cast<f32>(verticalInput) * verticalSpeed;
        }
    } else {
        if (jumping && m_builtIn.physicsState->m_onGround && m_jumpTicks == 0) {
            jump();
        }
    }
}

void Player::_handleWaterMovement(f32 forward, f32 strafe, bool jumping, bool sneaking)
{
    // 水中物理处理
    // 关键逻辑：
    // 1. 水中重力减弱（浮力）
    // 2. 水中阻力
    // 3. 深度守卫附魔增加速度
    // 4. 海豚的恩惠减少阻力
    // 5. 水中移动

    // 基础水中游泳速度
    f32 swimSpeed = physics::SWIM_SPEED_BASE;

    // 冲刺时增加速度
    if (m_isSprinting) {
        swimSpeed *= physics::SWIM_SPEED_SPRINT_MULTIPLIER;
    }

    // 深度守卫附魔加成
    i32 depthStriderLevel = getDepthStriderLevel();
    if (depthStriderLevel > 0) {
        // 深度守卫效果：
        // 1. 增加游泳速度
        swimSpeed += depthStriderLevel * physics::DEPTH_STRIDER_SPEED_BONUS;
        // 2. 速度上限
        swimSpeed = std::min(swimSpeed, 0.1f);
    }

    // 海豚的恩惠药水效果
    bool hasDolphinsGrace = hasEffect(entity::effect::EffectType::DolphinsGrace);

    // 水中阻力
    f32 waterDrag = m_isSprinting ? physics::WATER_DRAG_SPRINT : physics::WATER_DRAG;

    // 深度守卫对阻力的影响
    if (depthStriderLevel > 0) {
        waterDrag += (physics::DEPTH_STRIDER_MAX_DRAG - waterDrag) * static_cast<f32>(depthStriderLevel) / 3.0f;
    }

    // 海豚的恩惠大幅减少水中阻力
    if (hasDolphinsGrace) {
        waterDrag = physics::DOLPHINS_GRACE_WATER_DRAG;
    }

    // 根据朝向计算水平移动方向
    if (forward != 0.0f || strafe != 0.0f) {
        f32 yawRad = m_builtIn.rotation->m_rot.x * math::DEG_TO_RAD;
        f32 sinYaw = std::sin(yawRad);
        f32 cosYaw = std::cos(yawRad);

        // MC的getAbsoluteMotion公式
        f32 moveX = strafe * cosYaw - forward * sinYaw;
        f32 moveZ = forward * cosYaw + strafe * sinYaw;

        // 归一化
        f32 length = std::sqrt(moveX * moveX + moveZ * moveZ);
        if (length > 0.0f) {
            moveX /= length;
            moveZ /= length;
        }

        // 添加到速度
        m_builtIn.velocity->m_velocity.x += moveX * swimSpeed;
        m_builtIn.velocity->m_velocity.z += moveZ * swimSpeed;
    }

    // 垂直移动（跳跃向上，潜行向下）
    if (jumping) {
        // 向上游泳
        // MC: this.setMotion(this.getMotion().add(0.0D, 0.04D, 0.0D));
        m_builtIn.velocity->m_velocity.y += physics::SWIM_UP_SPEED;
    } else if (sneaking) {
        // 向下潜
        m_builtIn.velocity->m_velocity.y -= physics::SWIM_DOWN_SPEED;
    }

    // 应用水中阻力
    // 垂直方向阻力固定为 0.8
    m_builtIn.velocity->m_velocity.x *= waterDrag;
    m_builtIn.velocity->m_velocity.y *= 0.8f;
    m_builtIn.velocity->m_velocity.z *= waterDrag;

    // 应用水中的"浮力"效果
    // 重力减少到 1/16 (0.08 / 16 = 0.005)
    if (!m_abilities.flying && !hasNoGravity()) {
        f32 gravity = physics::GRAVITY;
        f32 buoyancy = gravity / 16.0f;

        // 下落时应用浮力
        if (m_builtIn.velocity->m_velocity.y < 0.0f) {
            m_builtIn.velocity->m_velocity.y += buoyancy;
        }
    }

    // 碰撞到墙后尝试上跳（爬出水面的行为）
    if (m_builtIn.physicsState->m_collidedHorizontally && !m_builtIn.physicsState->m_onGround && m_physicsEngine) {
        // 尝试向上跳
        m_builtIn.velocity->m_velocity.y = physics::WATER_WALL_JUMP_VELOCITY;
    }

    // 重置过小的速度
    _clampMotion();
}

void Player::_handleLavaMovement(f32 forward, f32 strafe, bool jumping, bool sneaking)
{
    // 岩浆中移动比水中更慢

    // 岩浆中基础移动速度
    f32 lavaSpeed = physics::LAVA_SWIM_SPEED;

    // 根据朝向计算移动方向
    if (forward != 0.0f || strafe != 0.0f) {
        f32 yawRad = m_builtIn.rotation->m_rot.x * math::DEG_TO_RAD;
        f32 sinYaw = std::sin(yawRad);
        f32 cosYaw = std::cos(yawRad);

        f32 moveX = strafe * cosYaw - forward * sinYaw;
        f32 moveZ = forward * cosYaw + strafe * sinYaw;

        f32 length = std::sqrt(moveX * moveX + moveZ * moveZ);
        if (length > 0.0f) {
            moveX /= length;
            moveZ /= length;
        }

        m_builtIn.velocity->m_velocity.x += moveX * lavaSpeed;
        m_builtIn.velocity->m_velocity.z += moveZ * lavaSpeed;
    }

    // 垂直移动（岩浆中也能向上游，但更慢）
    if (jumping) {
        m_builtIn.velocity->m_velocity.y += physics::SWIM_UP_SPEED * 0.5f; // 岩浆中向上游更慢
    } else if (sneaking) {
        m_builtIn.velocity->m_velocity.y -= physics::SWIM_DOWN_SPEED * 0.5f;
    }

    // 岩浆阻力（比水更大）
    m_builtIn.velocity->m_velocity.x *= physics::LAVA_DRAG;
    m_builtIn.velocity->m_velocity.y *= physics::LAVA_DRAG;
    m_builtIn.velocity->m_velocity.z *= physics::LAVA_DRAG;

    // 岩浆中重力（减弱）
    if (!m_abilities.flying) {
        if (m_builtIn.velocity->m_velocity.y < 0.0f && !sneaking) {
            m_builtIn.velocity->m_velocity.y += physics::LAVA_GRAVITY;
        }
    }

    _clampMotion();
}

void Player::jump()
{
    if (m_builtIn.physicsState->m_onGround && m_jumpTicks == 0) {
        m_builtIn.velocity->m_velocity.y = physics::JUMP_VELOCITY;
        m_builtIn.physicsState->m_onGround = false;
        m_jumpTicks = JUMP_COOLDOWN; // 设置跳跃冷却

        // 跳跃消耗饥饿值
        if (m_isSprinting) {
            addExhaustion(EXHAUSTION_SPRINT_JUMP);
        } else {
            addExhaustion(EXHAUSTION_JUMP);
        }
    }
}

void Player::_clampMotion()
{
    if (std::abs(m_builtIn.velocity->m_velocity.x) < physics::MOTION_THRESHOLD) m_builtIn.velocity->m_velocity.x = 0.0f;
    if (std::abs(m_builtIn.velocity->m_velocity.y) < physics::MOTION_THRESHOLD) m_builtIn.velocity->m_velocity.y = 0.0f;
    if (std::abs(m_builtIn.velocity->m_velocity.z) < physics::MOTION_THRESHOLD) m_builtIn.velocity->m_velocity.z = 0.0f;
}

f32 Player::_groundSlipperiness() const
{
    if (!m_builtIn.physicsState->m_onGround || m_world == nullptr) {
        return physics::SLIPPERINESS_DEFAULT;
    }

    BlockPos blockPos(static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.x)),
        static_cast<i32>(std::floor(m_builtIn.aabbShape->m_aabb.minY - 0.001f)),
        static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.z)));
    const BlockState* blockState = m_world->getBlockState(blockPos);
    if (blockState == nullptr) {
        return physics::SLIPPERINESS_DEFAULT;
    }
    return blockState->getBlock().getSlipperiness(*blockState, m_world, &blockPos, this);
}

/**
 * @brief 潜行时检查是否可以移动到边缘
 *
 * 当玩家潜行时，检查前方是否有方块支撑，防止掉落。
 *
 * @param movement 期望移动向量
 * @return 修正后的移动向量
 */
Vector3 Player::maybeBackOffFromEdge(const Vector3& movement) const
{
    // 只在潜行时检测
    if (!m_isSneaking) {
        return movement;
    }

    // 如果没有物理引擎或向上移动，不检测
    if (!m_physicsEngine) {
        return movement;
    }

    // 只检测水平移动
    if (movement.x == 0.0f && movement.z == 0.0f) {
        return movement;
    }

    // 获取当前碰撞箱
    AxisAlignedBB box = boundingBox();

    // 计算移动后的位置
    f32 newX = m_builtIn.stateVector->m_pos.x + movement.x;
    f32 newZ = m_builtIn.stateVector->m_pos.z + movement.z;

    // 检查移动后的位置下方是否有方块
    // 向下检测一小段距离
    AxisAlignedBB testBox = AxisAlignedBB(newX - PLAYER_WIDTH / 2.0f,
        m_builtIn.stateVector->m_pos.y - SNEAK_EDGE_DISTANCE,
        newZ - PLAYER_WIDTH / 2.0f,
        newX + PLAYER_WIDTH / 2.0f,
        m_builtIn.stateVector->m_pos.y,
        newZ + PLAYER_WIDTH / 2.0f);

    // 检查是否有碰撞
    std::vector<AxisAlignedBB> boxes;
    m_physicsEngine->collectCollisionBoxes(testBox, boxes);

    if (boxes.empty()) {
        // 没有支撑，阻止移动
        return Vector3(0.0f, movement.y, 0.0f);
    }

    return movement;
}

void Player::updatePhysics()
{
    snapshotInterpolationState();

    // 0. 更新跳跃冷却（客户端物理每帧都会调用）
    // 之前仅在tick()中减少，客户端未调用tick()会导致只能跳一次。
    if (m_jumpTicks > 0) {
        m_jumpTicks--;
    }

    // 0.1 更新自动跳跃冷却
    m_autoJump.tick();

    // 1. 重置过小的速度（MC: LivingEntity.aiStep）
    _clampMotion();

    // 刷新环境状态，确保后续判断使用当前位置。
    updateEnvironmentState();
    checkOnGround();

    const f32 tickGroundSlipperiness = _groundSlipperiness();
    _applyCachedMovementInput(tickGroundSlipperiness);

    // 水中和岩浆中的物理在 handleWaterMovement/handleLavaMovement 中已处理
    // 这里只处理地面和空中的物理
    if ((isInWater() || isInLava()) && !m_abilities.flying) {
        // 水中/岩浆中的移动和碰撞
        Vector3 movement(
            m_builtIn.velocity->m_velocity.x, m_builtIn.velocity->m_velocity.y, m_builtIn.velocity->m_velocity.z);
        if (m_physicsEngine && (movement.x != 0.0f || movement.y != 0.0f || movement.z != 0.0f)) {
            moveWithCollision(movement.x, movement.y, movement.z);
        }
    }

    if (!(isInWater() || isInLava()) || m_abilities.flying) {
        // 2. 应用重力（飞行时不应用重力）
        if (!m_abilities.flying && !hasNoGravity()) {
            m_builtIn.velocity->m_velocity.y -= physics::GRAVITY;
        }

        // 3. 应用阻力
        // 飞行时阻力处理不同：Y方向用0.6，水平方向用0.91
        if (m_abilities.flying) {
            // 飞行模式
            m_builtIn.velocity->m_velocity.x *= physics::FLY_HORIZONTAL_DRAG;
            m_builtIn.velocity->m_velocity.y *= physics::FLY_VERTICAL_DRAG;
            m_builtIn.velocity->m_velocity.z *= physics::FLY_HORIZONTAL_DRAG;
        } else {
            const f32 horizontalDrag = m_builtIn.physicsState->m_onGround
                ? tickGroundSlipperiness * physics::DRAG_GROUND
                : physics::DRAG_GROUND;
            m_builtIn.velocity->m_velocity.x *= horizontalDrag;
            m_builtIn.velocity->m_velocity.y *= physics::DRAG_AIR;
            m_builtIn.velocity->m_velocity.z *= horizontalDrag;
        }

        // 4. 如果在地面，停止Y方向速度（防止下落速度累积）
        // 飞行模式下不处理
        if (!m_abilities.flying && m_builtIn.physicsState->m_onGround && m_builtIn.velocity->m_velocity.y < 0.0f) {
            m_builtIn.velocity->m_velocity.y = 0.0f;
        }

        // 5. 潜行边缘检测（飞行时不检测）
        Vector3 movement(
            m_builtIn.velocity->m_velocity.x, m_builtIn.velocity->m_velocity.y, m_builtIn.velocity->m_velocity.z);
        if (m_isSneaking && !m_abilities.flying) {
            movement = maybeBackOffFromEdge(movement);
        }

        // 6. 记录移动前的位置（用于自动跳跃检测）
        Vector3 prevPos = m_builtIn.stateVector->m_pos;

        // 7. 使用碰撞检测移动
        if (m_physicsEngine && (movement.x != 0.0f || movement.y != 0.0f || movement.z != 0.0f)) {
            Vector3 actualMovement = moveWithCollision(movement.x, movement.y, movement.z);

            // 8. 碰撞后重置速度
            // 飞行模式下碰撞时不重置水平速度（可以穿透方块边缘的感觉）
            if (!m_abilities.flying) {
                if (m_builtIn.physicsState->m_collidedHorizontally) {
                    m_builtIn.velocity->m_velocity.x = 0.0f;
                    m_builtIn.velocity->m_velocity.z = 0.0f;
                }
            }
            if (m_builtIn.physicsState->m_collidedVertically) {
                m_builtIn.velocity->m_velocity.y = 0.0f;
            }
        } else if (!m_physicsEngine && (movement.x != 0.0f || movement.y != 0.0f || movement.z != 0.0f)) {
            move(movement.x, movement.y, movement.z);
        }

        // 9. 自动跳跃检测（在移动后）
        if (m_autoJump.isEnabled() && !m_abilities.flying && m_builtIn.physicsState->m_onGround && !m_isSneaking) {
            // 计算实际移动距离
            Vector2 actualMovement(
                m_builtIn.stateVector->m_pos.x - prevPos.x, m_builtIn.stateVector->m_pos.z - prevPos.z);
            f32 moveDistSq = actualMovement.x * actualMovement.x + actualMovement.y * actualMovement.y;

            // 只有在确实移动了才检测
            if (moveDistSq > 0.0001f && m_physicsEngine) {
                auto result = m_autoJump.check(*this, *m_physicsEngine, actualMovement);
                if (result.shouldJump) {
                    jump();
                }
            }
        }
    }

    // 更新本地渲染与环境状态缓存
    updateEnvironmentState();
    updateSwimming();
    updateAirSupply();
    updateMoveDistance();

    // 10. 再次重置过小的速度
    _clampMotion();
}

void Player::_applyMovementSpeed(f32& speed, bool sneaking) const
{
    if (m_abilities.flying) {
        speed = m_abilities.flySpeed;
    } else {
        speed = m_abilities.walkSpeed;
        if (m_isSprinting) {
            speed *= physics::SPRINT_SPEED_MULTIPLIER;
        }
        if (sneaking) {
            speed *= physics::SNEAK_SPEED_MULTIPLIER;
        }
    }
}

i32 Player::armorValue() const
{
    // 计算总护甲值
    return item::items::ArmorItem::getTotalArmorValue(*this);
}

const ItemStack& Player::getEquipment(EquipmentSlot slot) const
{
    // Player 重写 getEquipment 从 PlayerInventory 获取装备
    switch (slot) {
        case EquipmentSlot::MainHand:
            return m_inventory.getSelectedStackRef();
        case EquipmentSlot::OffHand:
            return m_inventory.getOffhandItemRef();
        case EquipmentSlot::Head:
            return m_inventory.getHelmetRef();
        case EquipmentSlot::Chest:
            return m_inventory.getChestplateRef();
        case EquipmentSlot::Legs:
            return m_inventory.getLeggingsRef();
        case EquipmentSlot::Feet:
            return m_inventory.getBootsRef();
        default:
            static ItemStack empty;
            return empty;
    }
}

ItemStack& Player::getMutableEquipment(EquipmentSlot slot)
{
    // Player 重写 getMutableEquipment 从 PlayerInventory 获取装备可变引用
    switch (slot) {
        case EquipmentSlot::MainHand:
            return m_inventory.getSelectedStackRef();
        case EquipmentSlot::OffHand:
            return m_inventory.getOffhandItemRef();
        case EquipmentSlot::Head:
            return m_inventory.getHelmetRef();
        case EquipmentSlot::Chest:
            return m_inventory.getChestplateRef();
        case EquipmentSlot::Legs:
            return m_inventory.getLeggingsRef();
        case EquipmentSlot::Feet:
            return m_inventory.getBootsRef();
        default:
            static ItemStack empty;
            return empty;
    }
}

void Player::setEquipment(EquipmentSlot slot, const ItemStack& stack)
{
    // Player 重写 setEquipment 设置装备到 PlayerInventory
    switch (slot) {
        case EquipmentSlot::MainHand:
            m_inventory.getSelectedStackRef() = stack;
            break;
        case EquipmentSlot::OffHand:
            m_inventory.setOffhandItem(stack);
            break;
        case EquipmentSlot::Head:
            m_inventory.setHelmet(stack);
            break;
        case EquipmentSlot::Chest:
            m_inventory.setChestplate(stack);
            break;
        case EquipmentSlot::Legs:
            m_inventory.setLeggings(stack);
            break;
        case EquipmentSlot::Feet:
            m_inventory.setBoots(stack);
            break;
        default:
            break;
    }
}

void Player::setCreativeModeInventory()
{
    fillCreativeModeInventory(m_inventory);
}

void Player::respawn()
{
    setHealth(maxHealth());
    m_foodStats.setFoodLevel(20);
    m_foodStats.setSaturationLevel(5.0f);
    m_foodStats.setFoodTimer(0);
    m_isSleeping = false;
    setPose(EntityPose::Standing);

    // 重置经验
    m_experienceManager->reset();
}

// ============================================================================
// getHeldItem 实现
// ============================================================================

ItemStack Player::getHeldItem(Hand hand) const
{
    if (hand == Hand::MainHand) {
        return m_inventory.getSelectedStack();
    } else {
        return m_inventory.getOffhandItem();
    }
}

ItemStack& Player::getHeldItem(Hand hand)
{
    if (hand == Hand::MainHand) {
        return m_inventory.getSelectedStackRef();
    } else {
        return m_inventory.getOffhandItemRef();
    }
}

// ============================================================================
// 挖掘系统
// ============================================================================

f32 Player::getDigSpeed(const BlockState& state, const BlockPos& pos) const
{
    // 1. 获取工具基础挖掘速度
    ItemStack heldItem = getHeldItem(Hand::MainHand);
    f32 speed = heldItem.isEmpty() ? 1.0f : heldItem.getDestroySpeed(state);

    // 2. 效率附魔加成（仅当工具对当前方块有效时，即 speed > 1.0）
    if (speed > 1.0f) {
        i32 efficiencyLevel = item::enchant::EnchantmentHelper::getEfficiencyLevel(heldItem);
        if (efficiencyLevel > 0) {
            // 效率附魔加成公式: level^2 + 1
            // I: 2, II: 5, III: 10, IV: 17, V: 26
            speed += static_cast<f32>(item::enchant::EfficiencyEnchantment::getMiningSpeedBonus(efficiencyLevel));
        }
    }

    // 3. 急迫效果和潮涌能量加成
    i32 hasteLevel = -1;
    i32 conduitLevel = -1;

    const auto* hasteEffect = getEffect(entity::effect::EffectType::Haste);
    if (hasteEffect) {
        hasteLevel = hasteEffect->amplifier();
    }

    const auto* conduitEffect = getEffect(entity::effect::EffectType::ConduitPower);
    if (conduitEffect) {
        conduitLevel = conduitEffect->amplifier();
    }

    i32 maxMiningSpeedup = std::max(hasteLevel, conduitLevel);
    if (maxMiningSpeedup >= 0) {
        // 计算乘数: 1.0 + (amplifier + 1) * 0.2
        // I级: 1.2, II级: 1.4, III级: 1.6, ...
        speed *= 1.0f + static_cast<f32>(maxMiningSpeedup + 1) * 0.2f;
    }

    // 4. 挖掘疲劳惩罚
    const auto* fatigueEffect = getEffect(entity::effect::EffectType::MiningFatigue);
    if (fatigueEffect) {
        i32 amplifier = fatigueEffect->amplifier();
        // 挖掘疲劳乘数表
        static constexpr f32 FATIGUE_MULTIPLIERS[] = {0.3f, 0.09f, 0.0027f, 0.00081f};
        if (amplifier >= 0 && static_cast<size_t>(amplifier) < 4) {
            speed *= FATIGUE_MULTIPLIERS[amplifier];
        } else {
            speed *= 0.00081f; // IV级及以上使用最小值
        }
    }

    // 5. 水下挖掘惩罚（仅当眼睛在水中且没有水下速掘附魔时）
    if (areEyesInWater()) {
        // 检查头盔是否有水下速掘附魔
        const ItemStack& helmet = m_inventory.getHelmet();
        if (!item::enchant::EnchantmentHelper::hasAquaAffinity(helmet)) {
            speed /= 5.0f;
        }
    }

    // 6. 空中挖掘惩罚（不在地面时）
    if (!m_builtIn.physicsState->m_onGround) {
        speed /= 5.0f;
    }

    return speed;
}

bool Player::canHarvestBlock(const BlockState& state) const
{
    // 如果方块不需要工具，总是可以采集
    if (!state.requiresTool()) {
        return true;
    }

    // 获取手持物品
    ItemStack heldItem = getHeldItem(Hand::MainHand);

    // 检查工具是否能采集
    if (!heldItem.isEmpty()) {
        return heldItem.canHarvestBlock(state);
    }

    // 空手无法采集需要工具的方块
    return false;
}

// ============================================================================
// 受伤/死亡（覆盖 LivingEntity 方法）
// ============================================================================

bool Player::hurt(DamageSource& source, f32 amount)
{
    // 创造模式无敌检查
    if (m_abilities.invulnerable && !source.canDamageCreative()) {
        return false;
    }
    // 调用父类方法处理伤害
    return LivingEntity::hurt(source, amount);
}

bool Player::isInvulnerableTo(DamageSource& source) const
{
    // 先检查基类的免疫判断
    if (LivingEntity::isInvulnerableTo(source)) {
        return true;
    }

    // 玩家专属游戏规则检查：特定伤害类型可被对应游戏规则禁用
    if (m_world != nullptr) {
        const auto& rules = m_world->getGameRules();
        if (source.isDrown() && !rules.getBoolean(world::gamerule::GameRuleKeys::DROWNING_DAMAGE)) {
            return true;
        }
        if (source.isFall() && !rules.getBoolean(world::gamerule::GameRuleKeys::FALL_DAMAGE)) {
            return true;
        }
        if (source.isFire() && !rules.getBoolean(world::gamerule::GameRuleKeys::FIRE_DAMAGE)) {
            return true;
        }
        if (source.isFreezing() && !rules.getBoolean(world::gamerule::GameRuleKeys::FREEZE_DAMAGE)) {
            return true;
        }
    }

    return false;
}

bool Player::canHarmPlayer(const Player& target) const
{
    // 获取攻击者（本玩家）的队伍
    auto* myTeam = getTeam();
    // 获取目标玩家的队伍
    auto* targetTeam = target.getTeam();

    // 攻击者没有队伍，可以伤害
    if (myTeam == nullptr) {
        return true;
    }

    // 两个队伍不是盟友关系，可以伤害
    if (!isAlliedTo(targetTeam)) {
        return true;
    }

    // 同队或盟友关系：取决于队伍是否允许友伤
    return myTeam->getAllowFriendlyFire();
}

void Player::die(DamageSource& cause)
{
    // 调用父类方法处理死亡
    LivingEntity::die(cause);

    // MC Java: Player.die() 中清除火焰状态
    clearFire();

    // 玩家特有：掉落经验
    dropExperience();

    // 记录死亡位置（维度+方块坐标），用于追溯指南针和存档持久化
    m_lastDeathLocation = GlobalPos(m_dimension, onPos());
}

// ============================================================================
// 摔落伤害
// ============================================================================

void Player::handleFallDamage(f32 distance, f32 damageMultiplier)
{
    // 调用父类处理摔落伤害计算（包含摔落音效播放）
    LivingEntity::handleFallDamage(distance, damageMultiplier);
}

void Player::causeFallDamage(f32 distance, f32 damageMultiplier, const DamageSource& source)
{
    // 创造模式飞行玩家免疫坠落伤害
    if (m_abilities.canFly) {
        return;
    }

    // 冲量坠落伤害减免逻辑
    // 当玩家处于冲量免疫状态（重锤砸地攻击或风弹爆炸后），
    // 仅计算冲量冲击点以下部分的坠落伤害
    bool hasImpulseContext = m_currentImpulseImpactPos.has_value() && m_ignoreFallDamageFromCurrentImpulse;

    if (hasImpulseContext) {
        // 计算从冲击位置到当前位置的坠落距离
        // 如果玩家位置在冲击位置上方或相同高度，则坠落距离为 0
        f64 impulseY = static_cast<f64>(m_currentImpulseImpactPos->y);
        f64 playerY = static_cast<f64>(position().y);
        f32 impulseFallDistance = static_cast<f32>(std::min(static_cast<f64>(distance), impulseY - playerY));

        if (impulseFallDistance <= 0.0f) {
            // 玩家在冲击位置上方或相同高度，不受到冲量坠落伤害，立即重置上下文
            resetCurrentImpulseContext();
        } else {
            // 冲量减免了部分坠落伤害，尝试重置上下文（尊重宽限期）
            tryResetCurrentImpulseContext();
        }

        if (impulseFallDistance > 0.0f) {
            // 用减免后的坠落距离计算伤害
            LivingEntity::causeFallDamage(impulseFallDistance, damageMultiplier, source);
            // 受到伤害后重置冲量上下文
            resetCurrentImpulseContext();
        } else {
            // 无有效坠落距离，传播原始坠落距离给乘客
            propagateFallToPassengers(distance, damageMultiplier, source);
        }
    } else {
        // 没有冲量上下文，正常计算坠落伤害
        LivingEntity::causeFallDamage(distance, damageMultiplier, source);
    }
}

// ============================================================================
// 冲量坠落伤害免疫
// ============================================================================

void Player::setIgnoreFallDamageFromCurrentImpulse(bool ignore)
{
    m_ignoreFallDamageFromCurrentImpulse = ignore;
    if (ignore) {
        applyPostImpulseGraceTime(40);
    } else {
        m_currentImpulseContextResetGraceTime = 0;
    }
}

void Player::applyPostImpulseGraceTime(i32 graceTime)
{
    m_currentImpulseContextResetGraceTime = std::max(m_currentImpulseContextResetGraceTime, graceTime);
}

void Player::tryResetCurrentImpulseContext()
{
    if (m_currentImpulseContextResetGraceTime == 0) {
        resetCurrentImpulseContext();
    }
}

void Player::resetCurrentImpulseContext()
{
    m_currentImpulseContextResetGraceTime = 0;
    m_currentExplosionCause = 0;
    m_currentImpulseImpactPos = std::nullopt;
    m_ignoreFallDamageFromCurrentImpulse = false;
}

Vector3 Player::calculateMaceImpactPosition() const
{
    // 如果玩家已有活跃冲量且冲击位置不高于当前位置，保留原有冲击位置
    // 防止连续砸地攻击时"双重获利"
    if (m_ignoreFallDamageFromCurrentImpulse && m_currentImpulseImpactPos.has_value() &&
        m_currentImpulseImpactPos->y <= position().y) {
        return *m_currentImpulseImpactPos;
    }
    return position();
}

void Player::onExplosionHit(Entity* cause)
{
    m_currentImpulseImpactPos = position();
    m_currentExplosionCause = cause != nullptr ? cause->id() : 0;

    // 只有风弹引起的爆炸才启用坠落伤害免疫
    // 对应 MC ServerPlayer.onExplosionHit 中对 WindCharge 实体类型的检查
    bool isWindCharge = cause != nullptr && cause->entityType() == entity::VanillaEntityTypeKeys::WIND_CHARGE;
    setIgnoreFallDamageFromCurrentImpulse(isWindCharge);
}

void Player::_applyWindBurstEffect(i32 windBurstLevel)
{
    // 风爆附魔效果：在重锤砸地攻击命中后触发 TRIGGER 爆炸
    // 参考 MC Java 的 ExplodeEffect + Level.ExplosionInteraction.TRIGGER
    // 不造成伤害、不破坏方块，仅施加定向击退、播放音效和粒子

    if (m_world == nullptr) {
        return;
    }

    // 爆炸参数（对应 MC WindBurstEnchantment 注册数据）
    const f32 radius = item::enchant::WindBurstEnchantment::getExplosionInteractionRange(); // 3.5
    const f32 knockbackMultiplier =
        item::enchant::WindBurstEnchantment::getExplosionKnockbackMultiplier(windBurstLevel);
    const f32 entityRangeMultiplier = game::explosion::ENTITY_RANGE_MULTIPLIER; // 2.0

    // 爆炸中心 = 玩家位置（MC Java: affected=ATTACKER, offset=Vec3.ZERO）
    const Vector3 burstPos = position();
    const f32 range = radius * entityRangeMultiplier;

    // 搜索范围内的所有实体
    AxisAlignedBB searchBox(burstPos.x - range,
        burstPos.y - range,
        burstPos.z - range,
        burstPos.x + range,
        burstPos.y + range,
        burstPos.z + range);
    std::vector<Entity*> entities = m_world->getEntitiesInAABB(searchBox, this);

    // 风爆路径等价 vanilla TRIGGER：不破坏方块，shouldAffectBlocklikeEntities 恒 false，
    // 故掉落物/盔甲架等"方块类实体"在此路径下恒忽略爆炸（不受击退）。
    // 间接源为玩家自身（LivingEntity），供载具判定间接源是否为 Mob。
    const world::explosion::ExplosionImmunityContext immunityCtx{
        .shouldAffectBlocklikeEntities = false,
        .indirectSource = this,
        .directSource = this,
        .mobGriefing = m_world->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING),
    };

    for (Entity* entity : entities) {
        if (entity == nullptr || entity->isRemoved()) {
            continue;
        }

        // 忽略爆炸的实体不受击退
        if (entity->ignoreExplosion(immunityCtx)) {
            continue;
        }

        // 计算方向和距离
        Vector3 entityPos = entity->position();
        f32 dx = entityPos.x - burstPos.x;
        f32 dy = entityPos.y - burstPos.y;
        f32 dz = entityPos.z - burstPos.z;
        f32 distanceSq = dx * dx + dy * dy + dz * dz;
        f32 distance = std::sqrt(distanceSq);
        f32 distanceRatio = distance / range;

        // 超出范围
        if (distanceRatio > 1.0f) {
            continue;
        }

        // 归一化方向向量
        if (distance < 0.001f) {
            // 实体在爆炸中心，随机方向
            dx = m_world->getRandom().nextFloat() * 2.0f - 1.0f;
            dy = m_world->getRandom().nextFloat() * 2.0f - 1.0f;
            dz = m_world->getRandom().nextFloat() * 2.0f - 1.0f;
            distance = std::sqrt(dx * dx + dy * dy + dz * dz);
        }
        dx /= distance;
        dy /= distance;
        dz /= distance;

        // 计算视线遮挡密度
        f32 density = _calculateWindBurstSeenPercent(entity->boundingBox(), burstPos);

        // 击退冲击力 = (1 - 距离比) * 密度
        f32 impact = (1.0f - distanceRatio) * density;

        // 爆炸保护减免击退
        f32 knockbackResistance = 0.0f;
        LivingEntity* living = dynamic_cast<LivingEntity*>(entity);
        if (living != nullptr) {
            i32 blastProtection = item::enchant::EnchantmentHelper::getTotalArmorProtection(
                living->getArmorSlots(), DamageFlags::EXPLOSION);
            if (blastProtection > 0) {
                knockbackResistance = static_cast<f32>(blastProtection) * 0.15f;
            }
        }

        f32 finalImpact = impact * knockbackMultiplier * (1.0f - knockbackResistance);
        if (finalImpact <= 0.0f) {
            continue;
        }

        // 应用击退速度
        entity->addVelocity(dx * finalImpact, dy * finalImpact, dz * finalImpact);
        entity->markHurt();

        // 玩家特殊处理：旁观者和创造模式飞行者不受击退
        Player* player = dynamic_cast<Player*>(entity);
        if (player != nullptr) {
            if (player->isSpectator()) {
                continue;
            }
            const PlayerAbilities& abilities = player->abilities();
            if (player->isCreative() && abilities.flying) {
                continue;
            }
        }

        // 通知实体被爆炸击中（用于冲量坠落伤害免疫等机制）
        // 注意：风爆附魔的爆炸源为 null（attributeToUser=false），与 MC Java 行为一致
        entity->onExplosionHit(nullptr);
    }

    // 风爆附魔扩展冲量宽限期（对应 MC ApplyEntityImpulse 效果）
    applyPostImpulseGraceTime(10);

    // 播放风爆音效
    m_world->playSound(SoundEvents::ENTITY_WIND_CHARGE_WIND_BURST, sound::SoundCategory::Blocks, burstPos, 1.0f, 1.0f);

    // 生成风爆粒子
    m_world->addParticle(particle::ParticleTypeId::GustEmitterSmall, burstPos, Vector3(0.0f, 0.0f, 0.0f));
    m_world->addParticle(particle::ParticleTypeId::GustEmitterLarge, burstPos, Vector3(0.0f, 0.0f, 0.0f));
}

f32 Player::_calculateWindBurstSeenPercent(const AxisAlignedBB& entityBox, const Vector3& center) const
{
    // 参考 MC Explosion.getBlockDensity / WindChargeEntity._calculateSeenPercent
    // 在实体碰撞箱内均匀采样点，射线检测是否有方块遮挡爆炸中心

    if (m_world == nullptr) {
        return 1.0f;
    }

    f32 dx = (entityBox.maxX - entityBox.minX) * 2.0f + 1.0f;
    f32 dy = (entityBox.maxY - entityBox.minY) * 2.0f + 1.0f;
    f32 dz = (entityBox.maxZ - entityBox.minZ) * 2.0f + 1.0f;
    f32 stepX = 1.0f / dx;
    f32 stepY = 1.0f / dy;
    f32 stepZ = 1.0f / dz;

    // 居中采样点偏移
    f32 offsetX = (1.0f - std::floor(1.0f / stepX) * stepX) * 0.5f;
    f32 offsetZ = (1.0f - std::floor(1.0f / stepZ) * stepZ) * 0.5f;

    if (stepX <= 0.0f || stepY <= 0.0f || stepZ <= 0.0f) {
        return 0.0f;
    }

    i32 visible = 0;
    i32 total = 0;

    for (f32 fx = 0.0f; fx <= 1.0f; fx += stepX) {
        for (f32 fy = 0.0f; fy <= 1.0f; fy += stepY) {
            for (f32 fz = 0.0f; fz <= 1.0f; fz += stepZ) {
                Vector3 samplePoint(entityBox.minX + fx * (entityBox.maxX - entityBox.minX) + offsetX,
                    entityBox.minY + fy * (entityBox.maxY - entityBox.minY),
                    entityBox.minZ + fz * (entityBox.maxZ - entityBox.minZ) + offsetZ);

                // 射线从采样点指向爆炸中心
                Ray ray(
                    samplePoint, Vector3(center.x - samplePoint.x, center.y - samplePoint.y, center.z - samplePoint.z));
                f32 rayDistance = (center - samplePoint).length();
                RaycastContext context(ray, rayDistance);

                BlockRaycastResult result = raycastBlocks(context, *m_world);
                if (result.isMiss()) {
                    ++visible;
                }
                ++total;
            }
        }
    }

    return total > 0 ? static_cast<f32>(visible) / static_cast<f32>(total) : 0.0f;
}

// ============================================================================
// 受伤/死亡声音
// ============================================================================

std::optional<ResourceLocation> Player::getHurtSound(DamageSource& source) const
{
    // 根据伤害类型返回不同音效
    if (source.isFire()) {
        return SoundEvents::ENTITY_PLAYER_HURT_ON_FIRE;
    } else if (source.isDrown()) {
        return SoundEvents::ENTITY_PLAYER_HURT_DROWN;
    } else if (source.isSweetBerryBush()) {
        return SoundEvents::ENTITY_PLAYER_HURT_SWEET_BERRY_BUSH;
    }
    return SoundEvents::ENTITY_PLAYER_HURT;
}

std::optional<ResourceLocation> Player::getDeathSound() const
{
    return SoundEvents::ENTITY_PLAYER_DEATH;
}

std::optional<ResourceLocation> Player::getFallSound(i32 fallHeight) const
{
    // 高空摔落 (>4格) 使用 big_fall，否则使用 small_fall
    if (fallHeight > 4) {
        return SoundEvents::ENTITY_PLAYER_BIG_FALL;
    }
    return SoundEvents::ENTITY_PLAYER_SMALL_FALL;
}

ResourceLocation Player::getSplashSound() const
{
    return SoundEvents::ENTITY_PLAYER_SPLASH;
}

ResourceLocation Player::getHighspeedSplashSound() const
{
    return SoundEvents::ENTITY_PLAYER_SPLASH_HIGH_SPEED;
}

void Player::doWaterSplashEffect()
{
    // 观察者模式不产生水花效果
    if (isSpectator()) {
        return;
    }
    // 调用父类方法
    Entity::doWaterSplashEffect();
}

// ============================================================================
// 属性注册（覆盖 LivingEntity 方法）
// ============================================================================

void Player::registerAttributes()
{
    // 先调用父类方法注册基础属性
    LivingEntity::registerAttributes();

    // 注册玩家特有属性
    using namespace entity::attribute;
    attributes().registerAttribute(*Attributes::luck());
    attributes().registerAttribute(*Attributes::attackDamage());
    attributes().registerAttribute(*Attributes::attackSpeed());
    // 注册方块/实体交互距离属性（对应 MC 1.21.11 Player.createAttributes）
    attributes().registerAttribute(*Attributes::blockInteractionRange());
    attributes().registerAttribute(*Attributes::entityInteractionRange());

    // 设置玩家特有属性值
    attributes().setBaseValue(Attributes::MOVEMENT_SPEED, defaults::player::MOVEMENT_SPEED);
    attributes().setBaseValue(Attributes::ATTACK_DAMAGE, defaults::player::ATTACK_DAMAGE);
    attributes().setBaseValue(Attributes::ATTACK_SPEED, defaults::player::ATTACK_SPEED);
    attributes().setBaseValue(Attributes::BLOCK_INTERACTION_RANGE, defaults::player::BLOCK_INTERACTION_RANGE);
    attributes().setBaseValue(Attributes::ENTITY_INTERACTION_RANGE, defaults::player::ENTITY_INTERACTION_RANGE);
    // LUCK 属性默认值为 0.0，无需显式设置
}

// ============================================================================
// 移动物理（覆盖 LivingEntity 方法）
// ============================================================================

void Player::travel(f32 strafing, f32 vertical, f32 forward)
{
    // 飞行模式处理 - Player 特有
    if (m_abilities.flying && !isRiding()) {
        f32 prevJumpFactor = m_jumpMovementFactor;
        m_jumpMovementFactor = m_abilities.flySpeed * (m_isSprinting ? physics::SPRINT_FLY_MULTIPLIER : 1.0f);
        LivingEntity::travel(strafing, vertical, forward);
        m_builtIn.velocity->m_velocity.y *= physics::FLY_VERTICAL_DRAG;
        m_jumpMovementFactor = prevJumpFactor;
        m_builtIn.physicsState->m_fallDistance = 0.0f;
    } else {
        LivingEntity::travel(strafing, vertical, forward);
    }

    updateMoveDistance();
}

void Player::aiStep()
{
    // 玩家不使用 AI 步进，由 handleMovementInput 和 updatePhysics 处理
    // 仅更新跳跃冷却
    if (m_jumpTicks > 0) {
        m_jumpTicks--;
    }
}

// ============================================================================
// 水中物理和游泳实现
// ============================================================================

bool Player::isActualSwimming() const
{
    // 游泳条件：
    // 需要: 眼睛在水中 && 身体在水中 && 不在飞行模式
    return areEyesInWater() && isInWater() && !m_abilities.flying;
}

void Player::updateSwimming()
{
    // 更新游泳动画
    m_prevSwimAnimation = m_swimAnimation;

    bool isSwimmingNow = isActualSwimming();

    // 平滑过渡游泳动画
    if (isSwimmingNow) {
        m_swimAnimation = std::min(1.0f, m_swimAnimation + 0.09f);
    } else {
        m_swimAnimation = std::max(0.0f, m_swimAnimation - 0.09f);
    }

    // 更新游泳状态
    setSwimming(isSwimmingNow);
}

void Player::updatePose()
{
    // 每帧自动判断正确姿态

    // 检查是否有足够的游泳空间（用于姿态切换的后备检查）
    // isPoseClear 在 MC 中检查指定姿态的碰撞箱是否与方块冲突
    auto isPoseClear = [this](EntityPose pose) -> bool { return _canFitPose(pose); };

    // 如果姿态被禁止，不进行自动更新
    // MC: if (this.forcedPose != null) { this.setPose(this.forcedPose); return; }
    // 目前没有实现 forcedPose，直接进行姿态判断

    // 只有在游泳空间足够时才允许姿态切换
    if (!isPoseClear(EntityPose::Swimming)) {
        return;
    }

    // 按优先级判断目标姿态
    EntityPose targetPose = EntityPose::Standing;

    // 检查是否是旁观者模式
    bool isSpectatorMode = isSpectator();
    // 检查是否正在骑乘
    bool isRidingVehicle = isRiding();

    // 1. 鞘翅飞行（优先级最高）
    if (isElytraFlying()) {
        targetPose = EntityPose::FallFlying;
    }
    // 2. 睡眠
    else if (m_isSleeping) {
        targetPose = EntityPose::Sleeping;
    }
    // 3. 游泳
    else if (m_isSwimming) {
        targetPose = EntityPose::Swimming;
    }
    // 4. 三叉戟激流攻击
    else if (isSpinAttacking()) {
        targetPose = EntityPose::SpinAttack;
    }
    // 5. 潜行（非飞行模式）
    else if (m_isSneaking && !m_abilities.flying) {
        targetPose = EntityPose::Crouching;
    }
    // 6. 默认站立
    else {
        targetPose = EntityPose::Standing;
    }

    // 检查目标姿态是否可以容纳
    // MC: if (!this.isSpectator() && !this.isPassenger() && !this.isPoseClear(pose)) { ... }
    if (!isSpectatorMode && !isRidingVehicle && !isPoseClear(targetPose)) {
        // 目标姿态无法容纳，尝试后备姿态
        if (isPoseClear(EntityPose::Crouching)) {
            targetPose = EntityPose::Crouching;
        } else {
            targetPose = EntityPose::Swimming;
        }
    }

    // 设置姿态
    setPose(targetPose);
}

// ============================================================================
// 鞘翅飞行（Elytra Glide）
// ============================================================================

bool Player::canGlide() const
{
    // 对应 MC 1.21.11 Player.canGlide()
    // 创造/旁观飞行模式下禁止滑翔，避免两种飞行模式冲突
    if (m_abilities.flying) {
        return false;
    }
    return LivingEntity::canGlide();
}

bool Player::tryToStartFallFlying()
{
    // 对应 MC 1.21.11 Player.tryToStartFallFlying()
    if (!isElytraFlying() && canGlide() && !isInWater()) {
        startFallFlying();
        return true;
    }
    return false;
}

void Player::startFallFlying()
{
    // 对应 MC 1.21.11 Player.startFallFlying()
    // 设置 FallFlying 标志位（bit 7），数据参数会同步给客户端
    addFlag(EntityFlags::FallFlying);
}

i32 Player::getDepthStriderLevel() const
{
    // 检查靴子上的深度守卫附魔等级
    using namespace item::enchant;
    const ItemStack& boots = m_inventory.getBoots();
    if (boots.isEmpty()) {
        return 0;
    }
    return EnchantmentHelper::getEnchantmentLevel(boots, &AllEnchantments::DEPTH_STRIDER);
}

void Player::swimUp()
{
    // 水中向上游泳
    if (isInWater() && !m_abilities.flying) {
        m_builtIn.velocity->m_velocity.y += physics::SWIM_UP_SPEED;
    }
}

void Player::updateAirSupply()
{
    // 创造模式和旁观者模式下不消耗空气
    if (m_abilities.invulnerable) {
        return;
    }

    // 检测入水/出水状态变化（需要在基类空气处理之前）
    bool inWater = isInWater();
    bool justEnteredWater = inWater && !m_wasInWater;

    // 调用基类方法处理标准空气消耗和溺水逻辑
    LivingEntity::updateAirSupply();

    // 入水溅水效果（玩家特有效果）
    // 客户端本地玩家当前没有 IWorld 适配层，不能直接走实体世界特效出口。
    // 保留空气逻辑，等客户端世界统一接入 IWorld 或专门的客户端特效桥接后再补全本地入水特效。
    if (justEnteredWater && m_world != nullptr) {
        doWaterSplashEffect();
    }

    // 更新上一帧状态
    m_wasInWater = inWater;
}

void Player::updateMoveDistance()
{
    // 保存上一帧的距离
    m_prevMoveDistanceWalked = m_moveDistanceWalked;
    m_prevMoveDistanceSwam = m_moveDistanceSwam;

    // 只累计上次采样后的增量，避免 tick 和物理更新重复统计同一段位移
    f32 dx = m_builtIn.stateVector->m_pos.x - m_moveDistanceSamplePosition.x;
    f32 dy = m_builtIn.stateVector->m_pos.y - m_moveDistanceSamplePosition.y;
    f32 dz = m_builtIn.stateVector->m_pos.z - m_moveDistanceSamplePosition.z;
    f32 distance = std::sqrt(dx * dx + dz * dz); // 水平距离

    // 重置声音触发标志
    m_shouldPlayStepSound = false;
    m_shouldPlaySwimSound = false;

    // 根据 MC 的逻辑：
    // distanceWalkedModified = 水平位移 * 0.6，用于 bobView 相位
    // distanceWalkedOnStepModified = 位移加权值，用于脚步声/游泳声触发
    f32 stepDistance;

    if (isInWater()) {
        // 游泳距离包括垂直移动
        f32 swimDistance = std::sqrt(dx * dx + dy * dy + dz * dz);
        m_moveDistanceSwam += swimDistance;

        // 游泳声音触发
        // MC: distanceWalkedOnStepModified += sqrt(motion.x^2 * 0.2 + motion.y^2 + motion.z^2 * 0.2) * 0.35
        stepDistance = std::sqrt(dx * dx * 0.2f + dy * dy + dz * dz * 0.2f) * 0.35f;
    } else {
        m_moveDistanceWalked += distance * 0.6f;
        // 脚步声距离乘以 0.6
        stepDistance = distance * 0.6f;
    }

    m_distanceWalkedOnStep += stepDistance;

    // 检查是否需要播放脚步声/游泳声
    if (m_distanceWalkedOnStep > m_nextStepDistance && m_builtIn.physicsState->m_onGround) {
        m_nextStepDistance = std::floor(m_distanceWalkedOnStep) + 1.0f;

        if (isInWater()) {
            // 游泳声音量基于速度
            m_swimSoundVolume = std::min(1.0f, stepDistance / 0.35f);
            m_shouldPlaySwimSound = true;
        } else {
            // 记录脚下方块位置（用于获取正确的声音类型）
            m_stepSoundPos = BlockPos(static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.x)),
                static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.y - 0.2f)), // 脚底位置
                static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.z)));
            m_shouldPlayStepSound = true;
        }
    }

    m_moveDistanceSamplePosition = m_builtIn.stateVector->m_pos;
    _updateCameraYaw();

    // 饥饿消耗（基于移动距离）
    // 只有生存模式和冒险模式才消耗饥饿
    if (m_gameMode == GameMode::Survival || m_gameMode == GameMode::Adventure) {
        if (isInWater()) {
            // 游泳消耗：每米 0.01
            f32 swimDistance = std::sqrt(dx * dx + dy * dy + dz * dz);
            if (swimDistance > 0.0f) {
                addExhaustion(EXHAUSTION_SWIM_PER_METER * swimDistance);
            }
        } else if (m_isSprinting && m_builtIn.physicsState->m_onGround) {
            // 疾跑消耗：每米 0.1
            if (distance > 0.0f) {
                addExhaustion(EXHAUSTION_SPRINT_PER_METER * distance);
            }
        }
        // 潜行、普通行走、攀爬不消耗饥饿
    }
}

void Player::_updateCameraYaw()
{
    m_prevCameraYaw = m_cameraYaw;

    if (isRiding()) {
        m_cameraYaw = 0.0f;
        return;
    }

    f32 targetCameraYaw = 0.0f;
    if (m_builtIn.physicsState->m_onGround && !isDead() && !isSwimming()) {
        targetCameraYaw = std::min(0.1f,
            std::sqrt(m_builtIn.velocity->m_velocity.x * m_builtIn.velocity->m_velocity.x +
                m_builtIn.velocity->m_velocity.z * m_builtIn.velocity->m_velocity.z));
    }
    m_cameraYaw += (targetCameraYaw - m_cameraYaw) * 0.4f;
}

void Player::_applyCreativeInteractionRangeModifiers()
{
    // 对应 MC 1.21.11 ServerPlayer.updatePlayerAttributes()：
    // 创造模式为 BLOCK_INTERACTION_RANGE / ENTITY_INTERACTION_RANGE 添加 +0.5 / +2.0 的 Addition 修饰符；
    // 非创造模式移除这两个修饰符。
    //
    // 实现采用「先移除后按需添加」的幂等模式：无论调用多少次，结果都一致。
    // 这样可同时支持：
    //   1. setGameMode() 中切换模式时刷新
    //   2. readAdditionalSaveData() 末尾修正存档中可能残留的旧修饰符
    //      （属性 NBT 在 LivingEntity::readAdditionalSaveData 中加载，修饰符会被持久化到 NBT）
    using namespace entity::attribute;

    // 方块交互距离：创造模式 +0.5
    attributes().removeModifier(Attributes::BLOCK_INTERACTION_RANGE, uuids::CREATIVE_BLOCK_INTERACTION_RANGE_UUID);
    if (isCreative()) {
        attributes().addModifier(Attributes::BLOCK_INTERACTION_RANGE,
            AttributeModifier(uuids::CREATIVE_BLOCK_INTERACTION_RANGE_UUID,
                "Creative Mode Block Interaction Range Boost",
                0.5,
                Operation::Addition));
    }

    // 实体交互距离：创造模式 +2.0
    attributes().removeModifier(Attributes::ENTITY_INTERACTION_RANGE, uuids::CREATIVE_ENTITY_INTERACTION_RANGE_UUID);
    if (isCreative()) {
        attributes().addModifier(Attributes::ENTITY_INTERACTION_RANGE,
            AttributeModifier(uuids::CREATIVE_ENTITY_INTERACTION_RANGE_UUID,
                "Creative Mode Entity Interaction Range Boost",
                2.0,
                Operation::Addition));
    }
}

void Player::playStepSound(const BlockPos& /*pos*/, const BlockState* /*blockState*/)
{
    if (m_world == nullptr) {
        return;
    }

    // 获取脚下方块状态
    const BlockState* blockState = m_world->getBlockState(m_stepSoundPos);
    if (blockState == nullptr || blockState->isAir()) {
        return;
    }

    if (isInWater()) {
        // 水中：播放游泳音效 + 沉闷步声
        playSwimSound(1.0f);
        playMuffledStepSound(*blockState);
    } else {
        // 非水中：调用基类方法，已包含 INSIDE/COMBINATION 步声和紫水晶共振处理
        Entity::playStepSound(m_stepSoundPos, blockState);
    }
}

void Player::playSwimSound(f32 volume)
{
    // 播放游泳声音
    if (m_world == nullptr || isSilent()) {
        return;
    }

    // 使用实体ID和tick计数器生成伪随机音调
    u32 seed = static_cast<u32>(m_id) ^ static_cast<u32>(m_ticksExisted);
    f32 randomValue = static_cast<f32>((seed * 1103515245 + 12345) % 32768) / 32768.0f;
    f32 pitch = 0.8f + randomValue * 0.4f; // 0.8-1.2 范围

    playSound(SoundEvents::ENTITY_PLAYER_SWIM, volume, pitch);
}

void Player::addExhaustion(f32 exhaustion)
{
    // 只有生存模式和冒险模式才消耗饥饿
    if (m_gameMode == GameMode::Survival || m_gameMode == GameMode::Adventure) {
        m_foodStats.addExhaustion(exhaustion);
    }
}

bool Player::canEat(bool ignoreHunger) const
{
    // 创造模式或观察者模式不能进食
    if (isCreative() || isSpectator()) {
        return false;
    }
    // 如果忽略饥饿值检查，返回 true（如金苹果等特殊食物）
    if (ignoreHunger) {
        return true;
    }
    // 否则检查饥饿值是否小于 20
    return m_foodStats.needsFood();
}

// ========== 攻击冷却系统 ==========

f32 Player::getCooledAttackStrength(f32 adjustTicks) const
{
    // 冷却进度 = min(ticksSinceLastAttack + adjustTicks, cooldownPeriod) / cooldownPeriod
    // 冷却周期 = 20 / attackSpeed (ticks)
    f32 attackSpeed = static_cast<f32>(getAttributeValue(entity::attribute::Attributes::ATTACK_SPEED, 4.0));
    if (attackSpeed <= 0.0f) {
        attackSpeed = 4.0f; // 默认攻击速度
    }

    f32 cooldownPeriod = 20.0f / attackSpeed; // 冷却周期（ticks）
    f32 adjustedTicks = static_cast<f32>(m_ticksSinceLastAttack) + adjustTicks;
    f32 progress = adjustedTicks / cooldownPeriod;

    return std::min(progress, 1.0f);
}

void Player::resetCooldown()
{
    m_ticksSinceLastAttack = 0;
}

f32 Player::getItemSwapScale(f32 adjustTicks) const
{
    // 对应 MC Player.getItemSwapScale：clamp((itemSwapTicker + adjust) / currentItemAttackStrengthDelay, 0, 1)。
    // itemSwapTicker 仅在主手物品种类切换时重置，与攻击冷却 ticker 解耦。
    f32 attackSpeed = static_cast<f32>(getAttributeValue(entity::attribute::Attributes::ATTACK_SPEED, 4.0));
    if (attackSpeed <= 0.0f) {
        attackSpeed = 4.0f;
    }

    const f32 cooldownPeriod = 20.0f / attackSpeed;
    const f32 adjustedTicks = static_cast<f32>(m_itemSwapTicker) + adjustTicks;
    return std::min(adjustedTicks / cooldownPeriod, 1.0f);
}

void Player::attack(Entity& target)
{
    // 旁观者模式下攻击实体等同于设置旁观目标
    // setCameraEntityId() 会触发 onCameraEntityChanged()，ServerPlayer 重写该方法
    // 以发送 SetCameraPacket 给客户端并执行传送，确保客户端同步摄像机状态
    if (isSpectator()) {
        setCameraEntityId(target.id());
        return;
    }

    // 2. 只能攻击生物实体
    LivingEntity* livingTarget = dynamic_cast<LivingEntity*>(&target);
    if (!livingTarget) {
        return;
    }

    // 3. 获取基础攻击伤害
    f32 baseDamage = static_cast<f32>(getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE, 1.0));

    // 4. 获取附魔伤害加成
    f32 enchantDamage = 0.0f;
    const ItemStack& mainHand = getMainHandItem();

    // 重锤下落攻击额外伤害
    f32 maceSmashBonus = 0.0f;
    bool isMaceSmashAttack = false;
    const item::MaceItem* maceItem = nullptr;

    if (!mainHand.isEmpty()) {
        enchantDamage = entity::combat::PlayerAttackHelper::getEnchantmentDamageBonus(
            mainHand, livingTarget->getCreatureAttribute());

        // 检查是否为重锤下落攻击
        maceItem = dynamic_cast<const item::MaceItem*>(mainHand.getItem());
        if (maceItem != nullptr && item::MaceItem::canSmashAttack(*this)) {
            isMaceSmashAttack = true;
            maceSmashBonus = item::MaceItem::getSmashAttackDamageBonus(*this, fallDistance(), mainHand);
        }
    }

    // 5. 计算攻击冷却进度
    // 使用 adjustTicks = 0.5F 获取冷却强度
    f32 cooldownProgress = getCooledAttackStrength(0.5f);

    // 6. 应用冷却伤害衰减
    // 基础伤害使用二次冷却系数，附魔伤害使用线性冷却系数
    f32 quadraticCooldown = 0.2f + cooldownProgress * cooldownProgress * 0.8f;
    f32 linearCooldown = cooldownProgress;
    f32 damage = baseDamage * quadraticCooldown;
    enchantDamage *= linearCooldown;

    // 7. 重置攻击冷却
    resetCooldown();

    // 如果伤害为 0，不执行攻击
    if (damage <= 0.0f && enchantDamage <= 0.0f) {
        return;
    }

    // 8. 判断是否是完全冷却攻击
    bool isFullCooldown = cooldownProgress > 0.9f;

    // 9. 计算击退
    i32 knockbackLevel = 0;
    if (!mainHand.isEmpty()) {
        knockbackLevel =
            item::enchant::EnchantmentHelper::getEnchantmentLevel(mainHand, &item::enchant::AllEnchantments::KNOCKBACK);
    }

    // 疾跑额外击退
    bool isSprintKnockback = false;
    if (isSprinting() && isFullCooldown) {
        knockbackLevel++;
        isSprintKnockback = true;
        // 播放击退攻击音效
        playSound(SoundEvents::ENTITY_PLAYER_ATTACK_KNOCKBACK, 1.0f, 1.0f);
    }

    // 10. 暴击判定
    // 重锤下落攻击不触发普通暴击（MC 1.21 规则）
    bool isCritical = !isMaceSmashAttack && entity::combat::PlayerAttackHelper::isCriticalHit(*this);

    // 11. 火焰附加
    i32 fireAspectLevel = 0;
    if (!mainHand.isEmpty()) {
        fireAspectLevel = item::enchant::EnchantmentHelper::getEnchantmentLevel(
            mainHand, &item::enchant::AllEnchantments::FIRE_ASPECT);
    }

    // 攻击前点燃（用于燃烧传递判定）
    bool wasBurning = false;
    if (fireAspectLevel > 0 && !livingTarget->isOnFire()) {
        wasBurning = true;
        livingTarget->igniteForTicks(20); // 1 秒 = 20 ticks
    }

    // 12. 应用暴击倍率
    if (isCritical) {
        damage *= 1.5f; // 暴击倍率 1.5
    }

    // 13. 合并伤害
    f32 totalDamage = damage + enchantDamage;

    // 13.5 重锤下落攻击伤害加成
    // 下落攻击加成不受冷却影响，直接加到总伤害上
    if (isMaceSmashAttack) {
        totalDamage += maceSmashBonus * cooldownProgress;
    }

    // 14. 创建伤害来源并应用伤害
    // 重锤下落攻击使用专属伤害类型 MaceSmash
    EntityDamageSource damageSource =
        isMaceSmashAttack ? DamageSources::maceSmash(this) : DamageSources::playerAttack(this);

    // 保存目标 hurt() 前的速度，用于 causeExtraKnockback 的 ServerPlayer 速度修正
    Vector3 preHurtVelocity = target.velocity();

    bool attacked = livingTarget->hurt(damageSource, totalDamage);

    // 用于跟踪是否播放了特定攻击音效
    bool playedAttackSound = false;

    if (attacked) {
        // 15. 应用额外击退（包含附魔击退和冲刺击退）
        // causeExtraKnockback 会：
        // - 对目标施加击退（方向基于攻击者朝向）
        // - 如果是冲刺击退，减缓攻击者水平速度并停止冲刺
        // - 对 ServerPlayer 目标立即发送速度包并清除 hurtMarked，防止速度重复应用
        causeExtraKnockback(target, static_cast<f32>(knockbackLevel), preHurtVelocity);

        // 16. 横扫攻击（仅当使用剑、冷却>90%、非暴击、非疾跑击退、在地面、且几乎静止时触发）
        // 用于检测玩家是否几乎静止（站立不动才能触发横扫攻击）
        f64 distanceWalkedDelta = static_cast<f64>(m_moveDistanceWalked - m_prevMoveDistanceWalked);
        bool canSweep = isFullCooldown && !isCritical && !isSprintKnockback && isOnGround() &&
            (distanceWalkedDelta < static_cast<f64>(aiMoveSpeed()));
        if (canSweep) {
            // 检查主手是否持有剑
            const item::tool::SwordItem* sword = dynamic_cast<const item::tool::SwordItem*>(mainHand.getItem());
            if (sword != nullptr) {
                f32 sweepRatio = item::enchant::EnchantmentHelper::getSweepingDamageRatio(mainHand);
                if (sweepRatio > 0.0f) {
                    // sweepDamage = 1.0 + sweepRatio * baseDamage
                    // 其中 baseDamage 是冷却调整后的伤害（不含附魔伤害）
                    f32 sweepDamage = 1.0f + sweepRatio * damage;

                    // 扫描目标周围 1x0.25x1 范围内的实体
                    AxisAlignedBB sweepBox = livingTarget->boundingBox().expand(1.0f, 0.25f, 1.0f);
                    std::vector<Entity*> nearbyEntities = world()->getEntitiesInAABB(sweepBox, this);

                    for (Entity* entity : nearbyEntities) {
                        // 排除自身、目标和队友
                        if (entity == this || entity == livingTarget) {
                            continue;
                        }

                        // 只对生物实体生效
                        LivingEntity* nearbyLiving = dynamic_cast<LivingEntity*>(entity);
                        if (!nearbyLiving) {
                            continue;
                        }

                        // 检查距离（最大 3 格）
                        if (distanceSqTo(*entity) > 9.0) { // 3^2 = 9
                            continue;
                        }

                        // 排除标记模式的盔甲架
                        // 标记模式的盔甲架碰撞箱为 0，不应被横扫攻击影响
                        entity::ArmorStandEntity* armorStand = dynamic_cast<entity::ArmorStandEntity*>(entity);
                        if (armorStand != nullptr && armorStand->isMarker()) {
                            continue;
                        }

                        // 排除盟友（友军伤害保护，双向检查）
                        if (isAlliedTo(*entity)) {
                            continue;
                        }

                        // 应用击退并造成伤害
                        // 击退方向基于玩家朝向
                        f32 yawRad = math::toRadians(yaw());
                        f64 knockbackX = static_cast<f64>(std::sin(yawRad));
                        f64 knockbackZ = static_cast<f64>(-std::cos(yawRad));
                        nearbyLiving->applyKnockback(0.4f, knockbackX, knockbackZ);

                        // 造成横扫伤害
                        EntityDamageSource sweepSource = DamageSources::playerAttack(this);
                        nearbyLiving->hurt(sweepSource, sweepDamage);
                    }

                    // 播放横扫攻击音效
                    playSound(SoundEvents::ENTITY_PLAYER_ATTACK_SWEEP, 1.0f, 1.0f);
                }
            }
        }

        // 17. 应用火焰附加
        if (fireAspectLevel > 0) {
            // 火焰附加持续时间 = level * 4 秒
            livingTarget->igniteForSeconds(static_cast<f32>(fireAspectLevel) * 4.0f);
        }

        // 18. 设置最后攻击目标
        setLastHurtTarget(livingTarget);

        // 播放攻击音效
        // 根据攻击类型播放不同音效
        if (isCritical) {
            // 暴击音效
            playSound(SoundEvents::ENTITY_PLAYER_ATTACK_CRIT, 1.0f, 1.0f);
            playedAttackSound = true;

            // 发送暴击动画包，在目标实体周围生成暴击粒子
            if (m_world) {
                m_world->broadcastEntityAnimation(
                    target.id(), static_cast<u8>(network::EntityAnimation::CriticalEffect));
            }
        } else if (canSweep && !playedAttackSound) {
            // 横扫音效已在上面播放
            playedAttackSound = true;
        }

        // 附魔暴击：附魔额外伤害大于 0 时，发送魔法暴击动画包
        if (enchantDamage > 0.0f && m_world) {
            m_world->broadcastEntityAnimation(
                target.id(), static_cast<u8>(network::EntityAnimation::MagicCriticalEffect));
        }

        // 如果没有播放特殊音效，根据冷却强度播放普通攻击音效
        if (!playedAttackSound) {
            if (isFullCooldown) {
                playSound(SoundEvents::ENTITY_PLAYER_ATTACK_STRONG, 1.0f, 1.0f);
            } else {
                playSound(SoundEvents::ENTITY_PLAYER_ATTACK_WEAK, 1.0f, 1.0f);
            }
        }

        // 荆棘附魔反伤处理
        // 攻击成功后，被攻击者的荆棘附魔有概率反伤攻击者
        // 注意：荆棘伤害不会再次触发荆棘，防止无限循环
        std::array<const ItemStack*, 4> armorSlots = livingTarget->getArmorSlots();
        item::enchant::EnchantmentHelper::applyThornsEnchantments(*livingTarget, *this, armorSlots);

        // 19. 武器损耗
        // 攻击成功后消耗武器耐久度
        // 剑消耗 1 点耐久，其他工具消耗 2 点耐久
        if (!mainHand.isEmpty()) {
            Item* item = const_cast<Item*>(mainHand.getItem());
            if (item != nullptr) {
                // 保存物品副本用于创造模式恢复
                ItemStack mainHandCopy = mainHand;

                // 调用物品的 hitEntity 方法，由物品决定耐久消耗
                // 剑 SwordItem::hitEntity() 消耗 1 点
                // 工具 ToolItem::hitEntity() 消耗 2 点
                // 重锤 MaceItem::hitEntity() 消耗 1 点 + 砸地攻击效果
                item->hitEntity(getMutableMainHandItem(), *livingTarget, *this);

                // 调用物品的 postHitEntity 方法（伤害结算后回调）
                // 重锤使用此回调重置攻击者的下落距离
                item->postHitEntity(getMutableMainHandItem(), *livingTarget, *this);

                // 重锤风爆附魔效果：下落攻击命中后产生风爆将攻击者弹起
                if (isMaceSmashAttack && !mainHand.isEmpty()) {
                    i32 windBurstLevel = item::enchant::EnchantmentHelper::getWindBurstLevel(mainHand);
                    if (windBurstLevel > 0) {
                        _applyWindBurstEffect(windBurstLevel);
                    }
                }

                // 派发自定义物品组件回调 - onHitEntity
                auto& itemCompReg = mc::mod::bedrock::addon::ItemComponentRegistry::instance();
                std::string itemTypeId = item->itemLocation().toString();
                if (itemCompReg.hasHitEntityCallback(itemTypeId)) {
                    mc::mod::bedrock::addon::ItemComponentHitEntityEvent event;
                    event.itemTypeId = itemTypeId;
                    event.attackingEntityId = id();
                    event.hitEntityId = livingTarget->id();
                    event.itemStackAmount = mainHand.getCount();
                    event.hadEffect = true;
                    itemCompReg.dispatchHitEntity(itemTypeId, event);
                }
                if (itemCompReg.hasBeforeDurabilityDamageCallback(itemTypeId)) {
                    mc::mod::bedrock::addon::ItemComponentBeforeDurabilityDamageEvent durEvent;
                    durEvent.itemTypeId = itemTypeId;
                    durEvent.attackingEntityId = id();
                    durEvent.hitEntityId = livingTarget->id();
                    durEvent.itemStackAmount = mainHand.getCount();
                    durEvent.durabilityDamage = 1; // 默认耐久消耗
                    itemCompReg.dispatchBeforeDurabilityDamage(itemTypeId, durEvent);
                    // durEvent.durabilityDamage 可能被回调修改
                }

                // 检查物品是否损坏（变空）
                if (mainHand.isEmpty()) {
                    // 物品损坏后清空主手槽位
                    // 创造模式下不需要清空（物品不会损坏）
                    if (!isCreative()) {
                        m_inventory.getSelectedStackRef() = ItemStack();
                    }
                    // 触发 PlayerDestroyItem 事件
                    if (m_world) {
                        m_world->onPlayerDestroyItem(static_cast<PlayerId>(id()),
                            mainHandCopy,
                            0, // 主手槽位
                            Hand::MainHand);
                    }
                }
            }
        }

        // 20. 饱食度消耗
        // 攻击消耗 0.1 饱食度
        addExhaustion(EXHAUSTION_ATTACK);
    } else {
        // 攻击失败（被格挡等）
        // 播放无伤害攻击音效
        playSound(SoundEvents::ENTITY_PLAYER_ATTACK_NODAMAGE, 1.0f, 1.0f);

        if (wasBurning) {
            livingTarget->clearFire(); // 移除之前点燃的火焰
        }
    }
}

void Player::causeExtraKnockback(Entity& target, f32 strength, const Vector3& preHurtVelocity)
{
    // 与 LivingEntity 基类版本相比，添加了冲刺停止逻辑

    if (strength > 0.0f) {
        if (auto* livingTarget = dynamic_cast<LivingEntity*>(&target)) {
            // 击退方向基于攻击者的朝向
            f32 yawRad = math::toRadians(yaw());
            f64 sinYaw = static_cast<f64>(std::sin(yawRad));
            f64 cosYaw = static_cast<f64>(-std::cos(yawRad));
            livingTarget->applyKnockback(strength, sinYaw, cosYaw);
        } else {
            // 非生物实体使用 push
            f32 yawRad = math::toRadians(yaw());
            target.addVelocity(-std::sin(yawRad) * strength, 0.1f, std::cos(yawRad) * strength);
        }

        // 减缓攻击者的水平速度
        Vector3 vel = velocity();
        setVelocity(vel.x * 0.6f, vel.y, vel.z * 0.6f);
        setSprinting(false);
    }

    // ServerPlayer 目标的速度重复应用修复
    // 当疾跑玩家攻击 ServerPlayer 时，hurt() 设置的 hurtMarked 会在 EntityTracker::tick() 中
    // 再次发送 EntityVelocityPacket，导致客户端重复应用击退速度。
    // 修复方法：立即发送速度包给 ServerPlayer，清除 hurtMarked，恢复 hurt 之前的速度。
    // 注意：此逻辑仅对 ServerPlayer 执行（sendVelocityPacket 返回 true 表示实际发送了包），
    // 非 ServerPlayer 的 Player 目标不会执行 clearHurtMarked/setVelocity 修正。
    if (target.isHurtMarked()) {
        Player* targetPlayer = dynamic_cast<Player*>(&target);
        if (targetPlayer != nullptr) {
            // 立即发送当前速度包给被击退的目标玩家
            // ServerPlayer 会实际发送网络包并返回 true，Player 基类返回 false
            bool sent = targetPlayer->sendVelocityPacket();
            if (sent) {
                // 清除 hurtMarked，避免 EntityTracker::tick() 再次发送速度包
                target.clearHurtMarked();

                // 恢复到 hurt() 之前的速度
                // 服务端的物理引擎会在下一 tick 重新计算正确速度
                // 这样 EntityTracker::tick() 就不会发送重复的速度同步
                target.setVelocity(preHurtVelocity);
            }
        }
    }
}

ActionResultType Player::interactOn(Entity& target, Hand hand)
{
    // 1. 旁观者模式：只能打开命名容器
    if (isSpectator()) {
        // 旁观者只能与实现 INamedContainerProvider 的实体交互
        // 例如：村民交易界面、箱子矿车等
        if (auto* provider = dynamic_cast<INamedContainerProvider*>(&target)) {
            if (openContainer(*provider)) {
                return ActionResultType::Success;
            }
        }
        return ActionResultType::Pass;
    }

    // 2. 获取手持物品
    ItemStack itemstack = getHeldItem(hand);
    ItemStack itemstackCopy = itemstack; // 保存副本用于创造模式恢复

    // 3. 先调用实体的 processInitialInteract 方法
    ActionResultType entityResult = target.processInitialInteract(*this, hand);
    if (entityResult == ActionResultType::Success || entityResult == ActionResultType::Consume) {
        // 创造模式恢复物品数量
        if (isCreative() && itemstack.isEmpty()) {
            inventory().setItem(hand == Hand::MainHand ? 0 : 40, itemstackCopy);
        }
        return entityResult;
    }

    // 4. 如果实体不处理，尝试物品的 interactWithEntity
    if (!itemstack.isEmpty()) {
        // 只有生物实体才支持物品交互
        LivingEntity* livingTarget = dynamic_cast<LivingEntity*>(&target);
        if (livingTarget != nullptr) {
            // 创造模式使用物品副本，避免消耗
            if (isCreative()) {
                itemstack = itemstackCopy;
            }

            // 调用物品的 itemInteractionForEntity
            // 注意: ItemStack::getItem() 返回 const Item*，需要转换为非 const
            // 这是安全的，因为 itemInteractionForEntity 可能会修改 ItemStack
            Item* item = const_cast<Item*>(itemstack.getItem());
            if (item != nullptr) {
                bool success = item->itemInteractionForEntity(itemstack, *this, *livingTarget, hand);
                if (success) {
                    // 物品被消耗处理
                    if (!isCreative() && itemstack.isEmpty()) {
                        // 触发 PlayerDestroyItem 事件
                        if (m_world) {
                            m_world->onPlayerDestroyItem(
                                static_cast<PlayerId>(id()), itemstackCopy, hand == Hand::MainHand ? 0 : 40, hand);
                        }
                        inventory().setItem(hand == Hand::MainHand ? 0 : 40, ItemStack());
                    }
                    return ActionResultType::Success;
                }
            }
        }
    }

    return ActionResultType::Pass;
}

// ============================================================================
// 容器交互实现
// ============================================================================

bool Player::openContainer(INamedContainerProvider& provider)
{
    // 通过世界打开实体容器
    if (m_world == nullptr) {
        return false;
    }

    return m_world->openEntityContainer(provider, *this);
}

// ============================================================================
// 注视检测实现
// ============================================================================

Vector3 Player::getLookVector() const
{
    // 根据 yaw 和 pitch 计算视线方向向量
    // MC 坐标系：yaw=0 看向 +Z，yaw=90 看向 -X
    // pitch 正值向下看，负值向上看

    f32 yawRad = math::toRadians(m_builtIn.rotation->m_rot.x);
    f32 pitchRad = math::toRadians(m_builtIn.rotation->m_rot.y);

    // 计算方向向量
    f32 cosYaw = std::cos(yawRad);
    f32 sinYaw = std::sin(yawRad);
    f32 cosPitch = std::cos(pitchRad);
    f32 sinPitch = std::sin(pitchRad);

    // 注意：MC 的 pitch 是负的（向上看时 pitch 为负）
    return Vector3(-sinYaw * cosPitch, -sinPitch, cosYaw * cosPitch).normalized();
}

Vector3 Player::getEyePosition() const
{
    // 眼睛位置 = 实体位置 + 眼睛高度
    return Vector3(x(), static_cast<f32>(getEyeY()), z());
}

// ============================================================================
// 交互范围（对应 MC 1.21.11 Player.blockInteractionRange / entityInteractionRange / isWithin*）
// ============================================================================

f64 Player::blockInteractionRange() const
{
    // 返回 generic.block_interaction_range 属性的计算值
    // 生存/冒险模式默认 4.5，创造模式由修饰符 +0.5 达到 5.0
    return getAttributeValue(entity::attribute::Attributes::BLOCK_INTERACTION_RANGE,
        static_cast<f64>(entity::attribute::defaults::player::BLOCK_INTERACTION_RANGE));
}

f64 Player::entityInteractionRange() const
{
    // 返回 generic.entity_interaction_range 属性的计算值
    // 生存/冒险模式默认 3.0，创造模式由修饰符 +2.0 达到 5.0
    return getAttributeValue(entity::attribute::Attributes::ENTITY_INTERACTION_RANGE,
        static_cast<f64>(entity::attribute::defaults::player::ENTITY_INTERACTION_RANGE));
}

bool Player::isWithinBlockInteractionRange(const BlockPos& pos, f64 padding) const
{
    // 构建 方块对应的 AABB（[x, x+1] × [y, y+1] × [z, z+1]）
    const AxisAlignedBB aabb(static_cast<f32>(pos.x),
        static_cast<f32>(pos.y),
        static_cast<f32>(pos.z),
        static_cast<f32>(pos.x + 1),
        static_cast<f32>(pos.y + 1),
        static_cast<f32>(pos.z + 1));

    // 计算眼睛到 AABB 的最近距离平方
    const Vector3 eye = getEyePosition();
    const f32 dx = (eye.x < aabb.minX) ? (aabb.minX - eye.x) : (eye.x > aabb.maxX) ? (eye.x - aabb.maxX) : 0.0f;
    const f32 dy = (eye.y < aabb.minY) ? (aabb.minY - eye.y) : (eye.y > aabb.maxY) ? (eye.y - aabb.maxY) : 0.0f;
    const f32 dz = (eye.z < aabb.minZ) ? (aabb.minZ - eye.z) : (eye.z > aabb.maxZ) ? (eye.z - aabb.maxZ) : 0.0f;
    const f64 distSq = static_cast<f64>(dx) * dx + static_cast<f64>(dy) * dy + static_cast<f64>(dz) * dz;

    const f64 range = blockInteractionRange() + padding;
    return distSq < range * range;
}

bool Player::isWithinEntityInteractionRange(const Entity& entity, f64 padding) const
{
    // 已移除的实体不可交互
    if (entity.isRemoved()) {
        return false;
    }
    return isWithinEntityInteractionRange(entity.boundingBox(), padding);
}

bool Player::isWithinEntityInteractionRange(const AxisAlignedBB& aabb, f64 padding) const
{
    // 计算眼睛到 AABB 的最近距离平方，与 (range + padding)² 比较
    const Vector3 eye = getEyePosition();
    const f32 dx = (eye.x < aabb.minX) ? (aabb.minX - eye.x) : (eye.x > aabb.maxX) ? (eye.x - aabb.maxX) : 0.0f;
    const f32 dy = (eye.y < aabb.minY) ? (aabb.minY - eye.y) : (eye.y > aabb.maxY) ? (eye.y - aabb.maxY) : 0.0f;
    const f32 dz = (eye.z < aabb.minZ) ? (aabb.minZ - eye.z) : (eye.z > aabb.maxZ) ? (eye.z - aabb.maxZ) : 0.0f;
    const f64 distSq = static_cast<f64>(dx) * dx + static_cast<f64>(dy) * dy + static_cast<f64>(dz) * dz;

    const f64 range = entityInteractionRange() + padding;
    return distSq < range * range;
}

bool Player::isWearingPumpkin() const
{
    // 检查玩家头盔是否为雕刻南瓜或南瓜灯
    // 这两种物品都可以防止末影人被激怒

    const ItemStack& helmet = inventory().getHelmet();

    if (helmet.isEmpty()) {
        return false;
    }

    const Item* item = helmet.getItem();
    if (item == nullptr) {
        return false;
    }

    // 检查是否为雕刻南瓜或南瓜灯
    // 雕刻南瓜物品的 resource location 是 minecraft:carved_pumpkin
    // 南瓜灯物品的 resource location 是 minecraft:jack_o_lantern
    const ResourceLocation& itemId = item->itemLocation();
    return itemId == ResourceLocation("minecraft:carved_pumpkin") ||
        itemId == ResourceLocation("minecraft:jack_o_lantern");
}

bool Player::isLookingAt(const Entity& target) const
{
    // 计算玩家视线方向与玩家到目标向量的点积

    // 1. 获取玩家视线方向
    Vector3 lookVec = getLookVector();

    // 2. 计算玩家眼睛到目标眼睛的向量
    Vector3 eyePos = getEyePosition();
    Vector3 targetEyePos = Vector3(target.x(), target.y() + target.eyeHeight(), target.z());
    Vector3 toTarget = targetEyePos - eyePos;

    // 3. 计算距离
    f32 distance = toTarget.length();
    if (distance < 0.001f) {
        // 距离太近，认为是在看
        return true;
    }

    // 4. 归一化目标向量
    toTarget = toTarget.normalized();

    // 5. 计算点积
    f32 dotProduct = lookVec.dot(toTarget);

    // 6. 根据距离调整阈值
    // 距离越远，阈值越高（更难满足注视条件）
    f32 threshold = 1.0f - 0.025f / distance;

    return dotProduct > threshold;
}

bool Player::isWearingGoldArmor() const
{
    // 检查玩家的四个盔甲槽位是否有金制盔甲
    // 金制盔甲可以使猪灵对玩家保持中立

    // 遍历四个盔甲槽位
    for (i32 slotIndex = static_cast<i32>(EquipmentSlot::Feet); slotIndex <= static_cast<i32>(EquipmentSlot::Head);
        ++slotIndex) {
        const ItemStack& armor = getEquipment(static_cast<EquipmentSlot>(slotIndex));
        if (armor.isEmpty()) {
            continue;
        }

        const Item* item = armor.getItem();
        if (item == nullptr) {
            continue;
        }

        // 检查是否为盔甲物品
        const auto* armorItem = dynamic_cast<const item::items::ArmorItem*>(item);
        if (armorItem == nullptr) {
            continue;
        }

        // 检查材质是否为金
        // 通过比较材质引用来判断
        if (&armorItem->getMaterial() == &item::armor::ArmorMaterials::GOLD) {
            return true;
        }
    }

    return false;
}

// ============================================================================
// NBT 序列化
// ============================================================================

void Player::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    using namespace mc::entity::serialization;
    using namespace mc::entity::serialization::nbt_keys;

    // 先调用基类序列化
    LivingEntity::addAdditionalSaveData(tag);

    // ========== 游戏模式 ==========
    tag.put(PLAYER_GAME_TYPE, static_cast<i32>(m_gameMode));

    // ========== 食物数据 ==========
    tag.put(FOOD_LEVEL, m_foodStats.foodLevel());
    tag.put(FOOD_SATURATION_LEVEL, m_foodStats.saturationLevel());
    tag.put(FOOD_EXHAUSTION_LEVEL, m_foodStats.exhaustionLevel());
    tag.put(FOOD_TICK_TIMER, m_foodStats.foodTimer());

    // ========== 经验 ==========
    tag.put(XP_LEVEL, m_experienceManager->getLevel());
    tag.put(XP_P, m_experienceManager->getProgress());
    tag.put(XP_TOTAL, m_experienceManager->getTotalExperience());
    tag.put(XP_SEED, m_experienceManager->getXpSeed());

    // ========== 玩家能力 ==========
    {
        auto abilities = std::make_unique<nbt::tags::compound_tag>();
        abilities->put(ABILITIES_INVULNERABLE, static_cast<i8>(m_abilities.invulnerable ? 1 : 0));
        abilities->put(ABILITIES_FLYING, static_cast<i8>(m_abilities.flying ? 1 : 0));
        abilities->put(ABILITIES_MAY_FLY, static_cast<i8>(m_abilities.canFly ? 1 : 0));
        abilities->put(ABILITIES_INSTABUILD, static_cast<i8>(m_abilities.creativeMode ? 1 : 0));
        abilities->put(ABILITIES_MAY_BUILD, static_cast<i8>(m_abilities.allowEdit ? 1 : 0));
        abilities->put(ABILITIES_FLY_SPEED, m_abilities.flySpeed);
        abilities->put(ABILITIES_WALK_SPEED, m_abilities.walkSpeed);
        tag.value.emplace(ABILITIES, std::move(abilities));
    }

    // ========== 冲量上下文 ==========
    // MC Java 序列化：current_explosion_impact_pos（可选 Vec3）、
    // ignore_fall_damage_from_current_explosion（bool）、
    // current_impulse_context_reset_grace_time（i32）。
    // 注意：currentExplosionCause 是运行时瞬时引用，不持久化（MC Java 同样不序列化此字段）。
    if (m_currentImpulseImpactPos.has_value()) {
        auto posTag = std::make_unique<nbt::tags::compound_tag>();
        posTag->put("x", static_cast<f64>(m_currentImpulseImpactPos->x));
        posTag->put("y", static_cast<f64>(m_currentImpulseImpactPos->y));
        posTag->put("z", static_cast<f64>(m_currentImpulseImpactPos->z));
        tag.value.emplace(CURRENT_EXPLOSION_IMPACT_POS, std::move(posTag));
    }
    tag.put(IGNORE_FALL_DAMAGE_FROM_CURRENT_EXPLOSION, static_cast<i8>(m_ignoreFallDamageFromCurrentImpulse ? 1 : 0));
    tag.put(CURRENT_IMPULSE_CONTEXT_RESET_GRACE_TIME, m_currentImpulseContextResetGraceTime);

    // ========== 重生点 ==========
    if (m_spawnPoint.has_value()) {
        tag.put(SPAWN_X, m_spawnPoint->x());
        tag.put(SPAWN_Y, m_spawnPoint->y());
        tag.put(SPAWN_Z, m_spawnPoint->z());
        tag.put(SPAWN_DIM, static_cast<i32>(m_spawnPoint->getDimensionId()));
        if (m_spawnForced) {
            tag.put(SPAWN_FORCED, static_cast<i8>(1));
        }
    }

    // ========== 进入下界位置 ==========
    if (m_enteredNetherPosition.has_value()) {
        auto netherTag = std::make_unique<nbt::tags::compound_tag>();
        netherTag->put("x", m_enteredNetherPosition->x);
        netherTag->put("y", m_enteredNetherPosition->y);
        netherTag->put("z", m_enteredNetherPosition->z);
        tag.value.emplace(ENTERED_NETHER_POSITION, std::move(netherTag));
    }

    // ========== 背包和选中槽位 ==========
    m_inventory.toNbt(tag);

    // ========== 末影箱物品栏 ==========
    m_enderChestInventory.toNbt(tag);

    // Score 已迁入按 PlayerScoreComponent 注册的组件序列化器，经 Entity::writeToNBT 的
    // saveAll 写出（批次6 子目标1 Step5），此处不再重复写。

    // ========== 最后死亡位置 ==========
    if (m_lastDeathLocation.has_value()) {
        auto deathTag = std::make_unique<nbt::tags::compound_tag>();
        deathTag->put(
            LAST_DEATH_LOCATION_DIMENSION, std::string(dimensionIdToString(m_lastDeathLocation->getDimensionId())));
        nbt_helper::putIntList(*deathTag,
            LAST_DEATH_LOCATION_POS,
            {m_lastDeathLocation->x(), m_lastDeathLocation->y(), m_lastDeathLocation->z()});
        tag.value.emplace(LAST_DEATH_LOCATION, std::move(deathTag));
    }
}

Result<void> Player::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    using namespace mc::entity::serialization;
    using namespace mc::entity::serialization::nbt_keys;

    // 先调用基类反序列化
    MC_TRY(LivingEntity::readAdditionalSaveData(tag));

    // ========== 游戏模式 ==========
    if (auto val = nbt_helper::tryGetInt(tag, PLAYER_GAME_TYPE)) {
        m_gameMode = static_cast<GameMode>(*val);
    }

    // ========== 食物数据 ==========
    if (auto val = nbt_helper::tryGetInt(tag, FOOD_LEVEL)) {
        m_foodStats.setFoodLevel(*val);
    }
    if (auto val = nbt_helper::tryGetFloat(tag, FOOD_SATURATION_LEVEL)) {
        m_foodStats.setSaturationLevel(*val);
    }
    if (auto val = nbt_helper::tryGetFloat(tag, FOOD_EXHAUSTION_LEVEL)) {
        m_foodStats.setExhaustionLevel(*val);
    }
    if (auto val = nbt_helper::tryGetInt(tag, FOOD_TICK_TIMER)) {
        m_foodStats.setFoodTimer(*val);
    }

    // ========== 经验 ==========
    {
        i32 xpLevel = m_experienceManager->getLevel();
        f32 xpProgress = m_experienceManager->getProgress();
        i32 xpTotal = m_experienceManager->getTotalExperience();
        i32 xpSeed = m_experienceManager->getXpSeed();

        if (auto val = nbt_helper::tryGetInt(tag, XP_LEVEL)) {
            xpLevel = *val;
        }
        if (auto val = nbt_helper::tryGetFloat(tag, XP_P)) {
            xpProgress = *val;
        }
        if (auto val = nbt_helper::tryGetInt(tag, XP_TOTAL)) {
            xpTotal = *val;
        }
        if (auto val = nbt_helper::tryGetInt(tag, XP_SEED)) {
            xpSeed = *val;
        }

        m_experienceManager->setExperience(xpLevel, xpProgress, xpTotal);
        m_experienceManager->setXpSeed(xpSeed);
    }

    // ========== 玩家能力 ==========
    if (auto* abilities = nbt_helper::tryGetCompound(tag, ABILITIES)) {
        if (auto val = nbt_helper::tryGetBool(*abilities, ABILITIES_INVULNERABLE)) {
            m_abilities.invulnerable = *val;
        }
        if (auto val = nbt_helper::tryGetBool(*abilities, ABILITIES_FLYING)) {
            m_abilities.flying = *val;
        }
        if (auto val = nbt_helper::tryGetBool(*abilities, ABILITIES_MAY_FLY)) {
            m_abilities.canFly = *val;
        }
        if (auto val = nbt_helper::tryGetBool(*abilities, ABILITIES_INSTABUILD)) {
            m_abilities.creativeMode = *val;
        }
        if (auto val = nbt_helper::tryGetBool(*abilities, ABILITIES_MAY_BUILD)) {
            m_abilities.allowEdit = *val;
        }
        if (auto val = nbt_helper::tryGetFloat(*abilities, ABILITIES_FLY_SPEED)) {
            m_abilities.flySpeed = *val;
        }
        if (auto val = nbt_helper::tryGetFloat(*abilities, ABILITIES_WALK_SPEED)) {
            m_abilities.walkSpeed = *val;
        }
    }

    // ========== 冲量上下文 ==========
    if (auto* posTag = nbt_helper::tryGetCompound(tag, CURRENT_EXPLOSION_IMPACT_POS)) {
        auto x = nbt_helper::tryGetDouble(*posTag, "x");
        auto y = nbt_helper::tryGetDouble(*posTag, "y");
        auto z = nbt_helper::tryGetDouble(*posTag, "z");
        if (x.has_value() && y.has_value() && z.has_value()) {
            m_currentImpulseImpactPos = Vector3(static_cast<f32>(*x), static_cast<f32>(*y), static_cast<f32>(*z));
        }
    } else {
        m_currentImpulseImpactPos = std::nullopt;
    }
    if (auto val = nbt_helper::tryGetBool(tag, IGNORE_FALL_DAMAGE_FROM_CURRENT_EXPLOSION)) {
        m_ignoreFallDamageFromCurrentImpulse = *val;
    } else {
        // MC Java: getBooleanOr(key, false) — 缺失时重置为 false
        m_ignoreFallDamageFromCurrentImpulse = false;
    }
    if (auto val = nbt_helper::tryGetInt(tag, CURRENT_IMPULSE_CONTEXT_RESET_GRACE_TIME)) {
        m_currentImpulseContextResetGraceTime = *val;
    } else {
        // MC Java: getIntOr(key, 0) — 缺失时重置为 0
        m_currentImpulseContextResetGraceTime = 0;
    }
    // 注意：currentExplosionCause 不从 NBT 读取（运行时瞬时状态，MC Java 同样不持久化）

    // ========== 重生点 ==========
    {
        auto spawnX = nbt_helper::tryGetInt(tag, SPAWN_X);
        auto spawnY = nbt_helper::tryGetInt(tag, SPAWN_Y);
        auto spawnZ = nbt_helper::tryGetInt(tag, SPAWN_Z);
        if (spawnX.has_value() && spawnY.has_value() && spawnZ.has_value()) {
            DimensionId spawnDim = nbt_helper::tryGetInt(tag, SPAWN_DIM).value_or(0);
            m_spawnPoint = GlobalPos(spawnDim, BlockPos(*spawnX, *spawnY, *spawnZ));
            m_spawnForced = nbt_helper::tryGetBool(tag, SPAWN_FORCED).value_or(false);
        } else {
            m_spawnPoint = std::nullopt;
            m_spawnForced = false;
        }
    }

    // ========== 进入下界位置 ==========
    if (auto* netherTag = nbt_helper::tryGetCompound(tag, ENTERED_NETHER_POSITION)) {
        auto x = nbt_helper::tryGetDouble(*netherTag, "x");
        auto y = nbt_helper::tryGetDouble(*netherTag, "y");
        auto z = nbt_helper::tryGetDouble(*netherTag, "z");
        if (x.has_value() && y.has_value() && z.has_value()) {
            m_enteredNetherPosition = Vector3d(*x, *y, *z);
        }
    } else {
        m_enteredNetherPosition = std::nullopt;
    }

    // ========== 背包和选中槽位 ==========
    {
        auto inventoryResult = PlayerInventory::fromNbt(tag);
        if (inventoryResult.success()) {
            m_inventory.copyInventory(inventoryResult.value());
        }
    }

    // ========== 末影箱物品栏 ==========
    m_enderChestInventory.fromNbt(tag);

    // Score 已迁入按 PlayerScoreComponent 注册的组件序列化器，经 Entity::readFromNBT 的
    // loadAll 读回（批次6 子目标1 Step5），此处不再重复读。

    // ========== 最后死亡位置 ==========
    if (auto* deathTag = nbt_helper::tryGetCompound(tag, LAST_DEATH_LOCATION)) {
        auto dimStr = nbt_helper::tryGetString(*deathTag, LAST_DEATH_LOCATION_DIMENSION);
        auto posList = nbt_helper::getIntList(*deathTag, LAST_DEATH_LOCATION_POS);
        if (dimStr.has_value() && posList.size() >= 3) {
            DimensionId dim = dimensionNameToId(*dimStr);
            m_lastDeathLocation = GlobalPos(dim, BlockPos(posList[0], posList[1], posList[2]));
        } else {
            // 兼容整数维度格式
            auto dimInt = nbt_helper::tryGetInt(*deathTag, LAST_DEATH_LOCATION_DIMENSION);
            if (dimInt.has_value() && posList.size() >= 3) {
                m_lastDeathLocation = GlobalPos(*dimInt, BlockPos(posList[0], posList[1], posList[2]));
            } else {
                m_lastDeathLocation = std::nullopt;
            }
        }
    } else {
        m_lastDeathLocation = std::nullopt;
    }

    // 修正创造模式交互距离修饰符
    // 属性 NBT 在 LivingEntity::readAdditionalSaveData 中已加载，若存档来自创造模式玩家，
    // 创造修饰符可能已被持久化到 NBT 中；此调用确保当前修饰符状态与玩家游戏模式严格一致。
    _applyCreativeInteractionRangeModifiers();

    return Result<void>::ok();
}

} // namespace mc
