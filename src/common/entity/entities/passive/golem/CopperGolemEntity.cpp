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

#include "CopperGolemEntity.hpp"

#include "CopperGolemTypes.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/RandomWalkingGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/special/CopperGolemGoals.hpp"
#include "common/entity/ai/pathfinding/PathNodeType.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/passive/golem/GolemEntity.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/blocks/ChestBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/interactive/CopperGolemStatueBlockEntity.hpp"
#include "common/world/blockentity/storage/ChestEntity.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include <cmath>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace mc {

// ============================================================================
// CopperGolemEntity 实现
// ============================================================================

CopperGolemEntity::CopperGolemEntity(EntityInstanceId id)
    : GolemEntity(id)
{
    // 对应 MC 1.21.11 CopperGolem 构造函数：
    //   setPersistenceRequired();
    //   setState(IDLE);
    //   setPathfindingMalus(DANGER_FIRE, 16.0F);
    //   setPathfindingMalus(DANGER_OTHER, 16.0F);
    //   setPathfindingMalus(DAMAGE_FIRE, -1.0F);
    // DANGER_FIRE/DANGER_OTHER 设为 16.0F：高代价但可通行（铜傀儡会避开火焰周边）
    // DAMAGE_FIRE 设为 -1.0F：禁止踏入火焰方块本身
    setPathfindingMalus(entity::ai::pathfinding::PathNodeType::DangerFire, 16.0f);
    setPathfindingMalus(entity::ai::pathfinding::PathNodeType::DangerOther, 16.0f);
    setPathfindingMalus(entity::ai::pathfinding::PathNodeType::DamageFire, -1.0f);

    // 铜傀儡可以走上1格高的方块（MC: STEP_HEIGHT=1.0）
    setStepHeight(1.0f);

    // 标记为持久化（生成后不会自然消失）
    enablePersistence();

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> CopperGolemEntity::create(IWorld* /*world*/)
{
    return std::make_unique<CopperGolemEntity>(0);
}

void CopperGolemEntity::spawnFromStatue(entity::CopperGolemWeatherState weatherState)
{
    // 对应 MC Java: CopperGolem.spawn(WeatherState) - 设置氧化等级并播放生成音效
    setWeatherState(weatherState);
    playSpawnSound();
}

void CopperGolemEntity::playSpawnSound()
{
    // 对应 MC Java: CopperGolem.playSpawnSound() -> playSound(SoundEvents.COPPER_GOLEM_SPAWN)
    playSound(SoundEvents::ENTITY_COPPER_GOLEM_SPAWN, 1.0f, 1.0f);
}

// ========== ContainerUser 接口实现 ==========

bool CopperGolemEntity::hasContainerOpen(const BlockPos& pos) const
{
    // 对应 MC 1.21.11 CopperGolem.hasContainerOpen(ContainerOpenersCounter, BlockPos):
    //   if (this.openedChestPos == null) return false;
    //   BlockState blockstate = this.level().getBlockState(this.openedChestPos);
    //   return this.openedChestPos.equals(p_482140_)
    //       || blockstate.getBlock() instanceof ChestBlock
    //           && blockstate.getValue(ChestBlock.TYPE) != ChestType.SINGLE
    //           && ChestBlock.getConnectedBlockPos(this.openedChestPos, blockstate).equals(p_482140_);
    if (!m_openedChestPos.has_value()) {
        return false;
    }

    const BlockPos& openedPos = m_openedChestPos.value();
    if (openedPos == pos) {
        return true;
    }

    // 检查是否为双箱另一半场景
    const IWorld* w = world();
    if (w == nullptr) {
        return false;
    }
    const BlockState* state = w->getBlockState(openedPos);
    if (state == nullptr) {
        return false;
    }

    // 方块必须是 ChestBlock（含子类 CopperChestBlock 等）
    const auto* chestBlock = dynamic_cast<const blocks::ChestBlock*>(&state->getBlock());
    if (chestBlock == nullptr) {
        return false;
    }

    // 检查 CHEST_TYPE 属性：必须非 Single 才有连通的另一半
    BlockStateProperties::ChestType chestType = state->get(BlockStateProperties::CHEST_TYPE());
    if (chestType == BlockStateProperties::ChestType::Single) {
        return false;
    }

    // 计算双箱连通位置：
    // ChestBlock::getConnectedDirection(state) 返回连通方向，offset 后得到另一半位置
    // 对应 MC ChestBlock.getConnectedBlockPos(pos, state)
    Direction connectedDir = blocks::ChestBlock::getConnectedDirection(*state);
    if (connectedDir == Direction::None) {
        return false;
    }
    BlockPos connectedPos = openedPos.offset(connectedDir);
    return connectedPos == pos;
}

LivingEntity* CopperGolemEntity::getLivingEntity()
{
    // CopperGolemEntity IS-A LivingEntity（继承链：CopperGolemEntity → GolemEntity →
    // CreatureEntity → MobEntity → LivingEntity），直接返回 this。
    return this;
}

const LivingEntity* CopperGolemEntity::getLivingEntity() const
{
    return this;
}

bool CopperGolemEntity::isShearable() const
{
    // 对应 MC Java: CopperGolem.readyForShearing()
    //   return this.isAlive() && this.getItemBySlot(EQUIPMENT_SLOT_ANTENNA)
    //                                 .is(ItemTags.SHEARABLE_FROM_COPPER_GOLEM)
    if (!isAlive()) {
        return false;
    }
    const ItemStack& antenna = getEquipment(EQUIPMENT_SLOT_ANTENNA);
    if (antenna.isEmpty()) {
        return false;
    }
    const Item* item = antenna.getItem();
    return item != nullptr && item->isIn(item::tag::ItemTags::SHEARABLE_FROM_COPPER_GOLEM());
}

std::vector<ItemStack> CopperGolemEntity::shear(Player* /*player*/)
{
    // 对应 MC Java: CopperGolem.shear(ServerLevel, SoundSource, ItemStack)
    //   - playSound(COPPER_GOLEM_SHEAR)
    //   - itemstack = getItemBySlot(EQUIPMENT_SLOT_ANTENNA)
    //   - setItemSlot(EQUIPMENT_SLOT_ANTENNA, ItemStack.EMPTY)
    //   - spawnAtLocation(serverlevel, itemstack, 1.5F)
    // 本项目的 IShearable 接口由 ShearsItem::itemInteractionForEntity 统一调用
    // ItemDropHelper::spawnItemEntity 在世界生成 ItemEntity，因此这里只返回掉落物列表。
    std::vector<ItemStack> drops;

    IWorld* w = world();
    if (w != nullptr) {
        // 在实体位置播放剪切音效
        Vector3 soundPos(x(), y(), z());
        w->playSound(SoundEvents::ENTITY_COPPER_GOLEM_SHEAR, sound::SoundCategory::Neutral, soundPos, 1.0f, 1.0f);
    }

    // 触发 SHEAR 游戏事件
    if (w != nullptr) {
        BlockPos soundBlockPos(
            static_cast<i32>(std::floor(x())), static_cast<i32>(std::floor(y())), static_cast<i32>(std::floor(z())));
        w->gameEvent(gameevent::GameEvents::SHEAR, soundBlockPos, nullptr);
    }

    // 取出天线槽物品并清空槽位
    // 对应 MC: itemstack = getItemBySlot(EQUIPMENT_SLOT_ANTENNA); setItemSlot(..., EMPTY)
    ItemStack antenna = getEquipment(EQUIPMENT_SLOT_ANTENNA);
    setEquipment(EQUIPMENT_SLOT_ANTENNA, ItemStack{});

    if (!antenna.isEmpty()) {
        drops.push_back(std::move(antenna));
    }
    return drops;
}

std::optional<ResourceLocation> CopperGolemEntity::getHurtSound(DamageSource& /*source*/) const
{
    // 对应 MC Java: CopperGolem.getHurtSound() -> CopperGolemOxidationLevels.getOxidationLevel(...).hurtSound()
    // Unaffected/Exposed 等级使用基础音效，Weathered 使用锈蚀音效，Oxidized 使用氧化音效
    switch (m_weatherState) {
        case entity::CopperGolemWeatherState::Unaffected:
        case entity::CopperGolemWeatherState::Exposed:
            return SoundEvents::ENTITY_COPPER_GOLEM_HURT;
        case entity::CopperGolemWeatherState::Weathered:
            return SoundEvents::ENTITY_COPPER_GOLEM_WEATHERED_HURT;
        case entity::CopperGolemWeatherState::Oxidized:
            return SoundEvents::ENTITY_COPPER_GOLEM_OXIDIZED_HURT;
    }
    return SoundEvents::ENTITY_COPPER_GOLEM_HURT;
}

std::optional<ResourceLocation> CopperGolemEntity::getDeathSound() const
{
    // 对应 MC Java: CopperGolem.getDeathSound() -> CopperGolemOxidationLevels.getOxidationLevel(...).deathSound()
    switch (m_weatherState) {
        case entity::CopperGolemWeatherState::Unaffected:
        case entity::CopperGolemWeatherState::Exposed:
            return SoundEvents::ENTITY_COPPER_GOLEM_DEATH;
        case entity::CopperGolemWeatherState::Weathered:
            return SoundEvents::ENTITY_COPPER_GOLEM_WEATHERED_DEATH;
        case entity::CopperGolemWeatherState::Oxidized:
            return SoundEvents::ENTITY_COPPER_GOLEM_OXIDIZED_DEATH;
    }
    return SoundEvents::ENTITY_COPPER_GOLEM_DEATH;
}

void CopperGolemEntity::playStepSound(const BlockPos& /*pos*/, const BlockState* /*blockState*/)
{
    // 对应 MC Java: CopperGolem.playStepSound() -> CopperGolemOxidationLevels.getOxidationLevel(...).stepSound()
    ResourceLocation sound = SoundEvents::ENTITY_COPPER_GOLEM_STEP;
    switch (m_weatherState) {
        case entity::CopperGolemWeatherState::Unaffected:
        case entity::CopperGolemWeatherState::Exposed:
            sound = SoundEvents::ENTITY_COPPER_GOLEM_STEP;
            break;
        case entity::CopperGolemWeatherState::Weathered:
            sound = SoundEvents::ENTITY_COPPER_GOLEM_WEATHERED_STEP;
            break;
        case entity::CopperGolemWeatherState::Oxidized:
            sound = SoundEvents::ENTITY_COPPER_GOLEM_OXIDIZED_STEP;
            break;
    }
    playSound(sound, 1.0f, 1.0f);
}

void CopperGolemEntity::tick()
{
    GolemEntity::tick();

    IWorld* w = world();
    if (w == nullptr) {
        return;
    }

    if (w->isClientSide()) {
        // 客户端：动画状态机更新（setupAnimationStates）
        // TODO: 实现客户端动画状态机（idleAnimationState、interactionGetItemAnimationState 等）
        // 当前仅服务端逻辑实现完整，客户端动画留作后续渲染系统补充
        return;
    }

    // 服务端：更新氧化状态
    // 对应 MC Java: CopperGolem.tick() -> updateWeathering(serverLevel, random, gameTime)
    updateWeathering(static_cast<i64>(w->getGameTime()));
}

void CopperGolemEntity::registerGoals()
{
    // 调用父类方法
    GolemEntity::registerGoals();

    // ========== AI 目标注册 ==========
    //
    // MC 1.21.11 原版 CopperGolem 使用 Brain 系统（CopperGolemAi）实现：
    //   - AnimalPanic（恐慌逃跑）
    //   - LookAtTargetSink / MoveToTargetSink（核心 AI 流程）
    //   - InteractWithDoor（开门）
    //   - TransportItemsBetweenContainers（在铜箱子与普通箱子间运输物品，核心行为）
    //   - SetEntityLookTargetSometimes<Player>（偶尔看向玩家）
    //   - RandomStroll（随机漫步）
    //   - DoNothing（偶尔发呆）
    //
    // 本项目使用 GoalSelector AI 系统替代 Brain，通过以下 Goal 复刻核心行为：
    //   - SwimGoal（游泳）
    //   - TransportItemsBetweenContainersGoal（物品运输，核心行为）
    //   - RandomWalkingGoal（随机漫步）
    //   - LookAtGoal（看向玩家）
    //   - LookRandomlyGoal（随机看向）
    //
    // 优先级参考 MC CopperGolemAi 中 TransportItemsBetweenContainers 在 Idle Activity
    // 中的 Pair.of(0, ...)，即运输目标在空闲行为中优先级最高。

    // 优先级 0: 游泳目标（在水中时上浮）
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::SwimGoal>(this));

    // 优先级 1: 物品运输目标（在铜箱子与普通箱子/陷阱箱之间运输物品）
    // 对应 MC CopperGolemAi.getIdleActivity 的 Pair.of(0, new TransportItemsBetweenContainers(...))
    // 速度倍率 1.0F 对应 MC 构造参数 speedMultiplier
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::TransportItemsBetweenContainersGoal>(this, 1.0));

    // 优先级 2: 随机漫步
    // 对应 MC RandomStroll.stroll(1.0F, 2, 2)
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::RandomWalkingGoal>(this, 1.0, 60));

    // 优先级 6: 看向玩家（对应 MC SetEntityLookTargetSometimes<Player, 6.0F>）
    m_goalSelector.addGoal(6, std::make_unique<entity::ai::goal::LookAtGoal>(this, 6.0f, 0.02f));

    // 优先级 7: 随机看向（对应 MC 默认 LookRandomlyGoal）
    m_goalSelector.addGoal(7, std::make_unique<entity::ai::goal::LookRandomlyGoal>(this));
}

void CopperGolemEntity::registerAttributes()
{
    // 调用父类方法
    GolemEntity::registerAttributes();

    // 对应 MC 1.21.11 CopperGolem.createAttributes():
    //   Mob.createMobAttributes()
    //       .add(Attributes.MOVEMENT_SPEED, 0.2F)
    //       .add(Attributes.STEP_HEIGHT, 1.0)
    //       .add(Attributes.MAX_HEALTH, 12.0)
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 12.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.2);
    // STEP_HEIGHT 在构造函数中通过 setStepHeight(1.0f) 设置
}

std::optional<ResourceLocation> CopperGolemEntity::getAmbientSound() const
{
    // 铜傀儡无环境音，对齐原版 AbstractGolem.getAmbientSound 返回 null。
    // sounds.json 中无 entity.copper_golem.ambient。
    return std::nullopt;
}

void CopperGolemEntity::updateWeathering(i64 currentGameTime)
{
    // 对应 MC Java: CopperGolem.updateWeathering(ServerLevel, RandomSource, long)
    //
    // 氧化逻辑：
    // - m_nextWeatheringTick == -2 表示已涂蜡，不氧化
    // - m_nextWeatheringTick == -1 表示需要初始化下一次氧化时间
    // - 否则：达到时间后氧化到下一等级，达到 Oxidized 后有概率转化为雕像
    if (m_nextWeatheringTick == IGNORE_WEATHERING_TICK) {
        // 已涂蜡，不氧化
        return;
    }

    if (m_nextWeatheringTick == UNSET_WEATHERING_TICK) {
        // 初始化下一次氧化时间
        // 对应 MC: this.nextWeatheringTick = gameTime + random.nextIntBetweenInclusive(504000, 552000)
        math::IRandom& rng = getRandom();
        m_nextWeatheringTick =
            currentGameTime + static_cast<i64>(rng.nextInt(WEATHERING_TICK_FROM, WEATHERING_TICK_TO));
        return;
    }

    // 检查是否达到氧化时间
    bool isOxidized = (m_weatherState == entity::CopperGolemWeatherState::Oxidized);
    if (currentGameTime >= m_nextWeatheringTick && !isOxidized) {
        // 氧化到下一等级
        auto next = entity::CopperGolemOxidationUtils::next(m_weatherState);
        if (next.has_value()) {
            setWeatherState(next.value());
            // 重新计算下一次氧化时间
            bool nextIsOxidized = (next.value() == entity::CopperGolemWeatherState::Oxidized);
            math::IRandom& rng = getRandom();
            m_nextWeatheringTick = nextIsOxidized
                ? 0L
                : m_nextWeatheringTick + static_cast<i64>(rng.nextInt(WEATHERING_TICK_FROM, WEATHERING_TICK_TO));
        }
    }

    // 已氧化且满足条件时转化为雕像
    if (isOxidized && canTurnToStatue()) {
        turnToStatue();
    }
}

bool CopperGolemEntity::canTurnToStatue() const
{
    // 对应 MC Java: CopperGolem.canTurnToStatue(Level)
    //   return level.getBlockState(this.blockPosition()).isAir() && level.random.nextFloat() <= 0.0058F
    const IWorld* w = world();
    if (w == nullptr) {
        return false;
    }
    // MC 的 blockPosition() = BlockPos(floor(x), floor(y), floor(z))
    BlockPos currentPos(
        static_cast<i32>(std::floor(x())), static_cast<i32>(std::floor(y())), static_cast<i32>(std::floor(z())));
    const BlockState* state = w->getBlockState(currentPos);
    if (state == nullptr || !state->isAir()) {
        return false;
    }
    math::IRandom& rng = const_cast<CopperGolemEntity*>(this)->getRandom();
    return rng.nextFloat() <= TURN_TO_STATUE_CHANCE;
}

void CopperGolemEntity::turnToStatue()
{
    // 对应 MC Java: CopperGolem.turnToStatue(ServerLevel)
    //   BlockPos blockpos = this.blockPosition();
    //   level.setBlock(blockpos, Blocks.OXIDIZED_COPPER_GOLEM_STATUE.defaultBlockState()
    //       .setValue(POSE, Pose.values()[random.nextInt(0, Pose.values().length)])
    //       .setValue(FACING, Direction.fromYRot(this.getYRot())), 3);
    //   if (level.getBlockEntity(blockpos) instanceof CopperGolemStatueBlockEntity be) {
    //       be.createStatue(this);
    //       this.dropPreservedEquipment(level);
    //       this.discard();
    //       this.playSound(SoundEvents.COPPER_GOLEM_BECOME_STATUE);
    //       if (this.isLeashed()) { ... }
    //   }
    IWorld* w = world();
    if (w == nullptr) {
        return;
    }

    BlockPos blockPos(
        static_cast<i32>(std::floor(x())), static_cast<i32>(std::floor(y())), static_cast<i32>(std::floor(z())));

    // 获取 Oxidized 铜傀儡雕像方块
    Block* statueBlock = VanillaBlocks::OXIDIZED_COPPER_GOLEM_STATUE;
    if (statueBlock == nullptr) {
        // 方块未注册（理论上不应发生，CopperBlocks::initialize 已注册）
        return;
    }

    // 随机选择姿态
    // 对应 MC: Pose.values()[random.nextInt(0, Pose.values().length)]
    math::IRandom& rng = getRandom();
    i32 poseIndex = rng.nextInt(0, 3); // [0, 3] 共 4 种姿态
    using Pose = BlockStateProperties::CopperGolemPose;
    Pose pose = Pose::Standing;
    switch (poseIndex) {
        case 0:
            pose = Pose::Standing;
            break;
        case 1:
            pose = Pose::Sitting;
            break;
        case 2:
            pose = Pose::Running;
            break;
        case 3:
            pose = Pose::Star;
            break;
        default:
            pose = Pose::Standing;
            break;
    }

    // 根据当前 Y 旋转计算朝向
    // MC: Direction.fromYRot(this.getYRot()) - 0=South, 90=West, 180=North, 270=East
    f32 yawValue = yaw();
    Direction facing = Direction::North; // 默认北
    // 将 yaw 规范化到 [0, 360)
    while (yawValue < 0.0f)
        yawValue += 360.0f;
    while (yawValue >= 360.0f)
        yawValue -= 360.0f;
    if (yawValue >= 45.0f && yawValue < 135.0f) {
        facing = Direction::West;
    } else if (yawValue >= 135.0f && yawValue < 225.0f) {
        facing = Direction::North;
    } else if (yawValue >= 225.0f && yawValue < 315.0f) {
        facing = Direction::East;
    } else {
        facing = Direction::South;
    }

    // 构造新状态：默认状态 + POSE + FACING
    const BlockState& newState = statueBlock->defaultState()
                                     .with(BlockStateProperties::COPPER_GOLEM_POSE(), pose)
                                     .with(BlockStateProperties::HORIZONTAL_FACING(), facing);

    // 放置方块
    w->setBlockState(blockPos, &newState, 3);

    // 获取方块实体并保存自定义名称
    BlockEntity* be = w->getBlockEntity(blockPos);
    if (be != nullptr) {
        auto* statueBe = dynamic_cast<blockentity::CopperGolemStatueBlockEntity*>(be);
        if (statueBe != nullptr) {
            // 转移自定义名称到雕像方块实体
            // 对应 MC Java: CopperGolemStatueBlockEntity.createStatue(CopperGolem)
            //   - 保存 CUSTOM_NAME 组件
            if (hasCustomName()) {
                statueBe->setCustomName(customNameText());
            }
        }
    }

    // 丢弃保存的装备
    // 对应 MC Java: this.dropPreservedEquipment(serverLevel)
    // MobEntity::dropPreservedEquipment() 已实现：遍历所有装备槽（含 Saddle/天线槽），
    // 掉落标记为保留（掉落概率 > 1.0，由 setGuaranteedDrop 设置）的物品。
    // 铜傀儡天线槽（EQUIPMENT_SLOT_ANTENNA = Saddle）由铁傀儡 OfferFlowerGoal
    // 装备罂粟花时调用 setGuaranteedDrop 标记保留，转雕像时此处自动掉落。
    // 返回值为保留在实体上的装备槽位集合（谓词返回 false 的槽位，如绑定诅咒）。
    (void)dropPreservedEquipment();

    // 播放变雕像音效
    // 对应 MC Java: this.playSound(SoundEvents.COPPER_GOLEM_BECOME_STATUE)
    playSound(SoundEvents::BLOCK_COPPER_GOLEM_BECOME_STATUE, 1.0f, 1.0f);

    // 处理拴绳掉落
    // 对应 MC Java: if (this.isLeashed()) { if (gameRules.ENTITY_DROPS) dropLeash(); else removeLeash(); }
    // MobEntity 已提供 isLeashed() / dropLeash() / clearLeash() 接口：
    //   - dropLeash() 内部已检查 DO_ENTITY_DROPS 规则：规则为 true 时掉落拴绳物品，false 时仅清除状态。
    //   - clearLeash() 仅清除拴绳状态（不掉落物品），对应 MC Java 的 removeLeash()。
    // 这里采用显式 if/else 分支以 1:1 对应 MC 原版控制流，便于源码对照与未来维护。
    if (isLeashed()) {
        if (w->getGameRules().getBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS)) {
            dropLeash();
        } else {
            clearLeash();
        }
    }

    // 移除实体
    // 对应 MC Java: this.discard()
    discard();
}

// ============================================================================
// NBT 序列化
// ============================================================================

void CopperGolemEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    // 对应 MC Java: CopperGolem.addAdditionalSaveData(ValueOutput)
    //   super.addAdditionalSaveData(p_480213_);
    //   p_480213_.putLong("next_weather_age", this.nextWeatheringTick);
    //   p_480213_.store("weather_state", WeatheringCopper.WeatherState.CODEC, this.getWeatherState());
    GolemEntity::addAdditionalSaveData(tag);
    using namespace mc::entity::serialization;
    tag.put(nbt_keys::NEXT_WEATHER_AGE, static_cast<i64>(m_nextWeatheringTick));
    tag.put(nbt_keys::WEATHER_STATE, entity::CopperGolemOxidationUtils::toString(m_weatherState));
}

Result<void> CopperGolemEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    // 对应 MC Java: CopperGolem.readAdditionalSaveData(ValueInput)
    //   super.readAdditionalSaveData(p_481630_);
    //   this.nextWeatheringTick = p_481630_.getLongOr("next_weather_age", -1L);
    //   this.setWeatherState(p_481630_.read("weather_state", WeatheringCopper.WeatherState.CODEC)
    //       .orElse(WeatheringCopper.WeatherState.UNAFFECTED));
    MC_TRY(GolemEntity::readAdditionalSaveData(tag));
    using namespace mc::entity::serialization;

    if (auto val = nbt_helper::tryGetLong(tag, nbt_keys::NEXT_WEATHER_AGE)) {
        m_nextWeatheringTick = *val;
    } else {
        m_nextWeatheringTick = UNSET_WEATHERING_TICK; // -1，MC 默认值
    }

    if (auto val = nbt_helper::tryGetString(tag, nbt_keys::WEATHER_STATE)) {
        m_weatherState = entity::CopperGolemOxidationUtils::fromString(*val);
    } else {
        m_weatherState = entity::CopperGolemWeatherState::Unaffected; // MC 默认值
    }

    // behaviorState 不持久化，重置为 Idle（与 MC 一致）
    m_behaviorState = entity::CopperGolemState::Idle;

    return Result<void>::ok();
}

} // namespace mc
