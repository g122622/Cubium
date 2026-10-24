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

#include "SnifferEntity.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/BreedGoal.hpp"
#include "common/entity/ai/goal/goals/FollowParentGoal.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/PanicGoal.hpp"
#include "common/entity/ai/goal/goals/RandomWalkingGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/TemptGoal.hpp"
#include "common/entity/ai/goal/goals/special/SnifferGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/LootTableManager.hpp"
#include "common/item/loot/context/LootContextBuilder.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockSoundType.hpp"
#include "common/world/block/BlockState.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>

namespace mc {

// ========== DataParameter 静态定义 ==========

/// 对齐 MC Sniffer.DATA_STATE（SNIFFER_STATE 序列化器 id=31, wire=VarInt(State.id)）。
entity::DataParameter<entity::SnifferStateValue> SnifferEntity::DATA_STATE_PARAM =
    entity::EntityDataManager::createKey<entity::SnifferStateValue>();

/// 对齐 MC Sniffer.DATA_DROP_SEED_AT_TICK（INT 序列化器）。
entity::DataParameter<i32> SnifferEntity::DATA_DROP_SEED_AT_TICK_PARAM = entity::EntityDataManager::createKey<i32>();

const entity::EntityClassInfo& SnifferEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"SnifferEntity", &AnimalEntity::classInfo()};
    return s_classInfo;
}

// ========== 常量 ==========

namespace {

/// 嗅探兽基础移动速度（对齐 MC Sniffer.createAttributes: MOVEMENT_SPEED = 0.1F）
constexpr f32 SNIFFER_MOVEMENT_SPEED = 0.1f;
/// 嗅探兽最大生命值（对齐 MC Sniffer.createAttributes: MAX_HEALTH = 14.0）
constexpr f32 SNIFFER_MAX_HEALTH = 14.0f;
/// 嗅探兽跟随范围（对齐 Animal.createAnimalAttributes 默认值 16.0F）
constexpr f32 SNIFFER_FOLLOW_RANGE = 16.0f;
/// 脚步声音量（对齐 MC Sniffer.playStepSound: 0.15F）
constexpr f32 SNIFFER_STEP_VOLUME = 0.15f;

} // namespace

// ========== 构造函数 ==========

SnifferEntity::SnifferEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AnimalEntity(id, registry)
{
    // 注册 AI 目标
    registerGoals();

    // 补调 registerAttributes / registerData：AnimalEntity 构造只调 registerAttributes 且 vtable
    // 在基类构造期间指向 AnimalEntity，派生 override 永不执行，须在派生类构造显式调用。
    // 此前注释误称"registerData 由 AnimalEntity 构造链调用"——实际 AnimalEntity 构造不调
    // registerData，故 Sniffer 的 registerData（注册 DATA_STATE / DATA_DROP_SEED_AT_TICK）永不执行。
    registerAttributes();
    registerData();
}

std::unique_ptr<Entity> SnifferEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<SnifferEntity>(0, registry);
}

// ========== 幼体设置 ==========

void SnifferEntity::setChild(bool baby)
{
    // 对齐 MC Sniffer.setBaby：嗅探兽幼年期为 48000 tick（40 分钟），
    // 是普通动物（AgeableEntity::BABY_AGE = -24000）的两倍。
    setGrowingAge(baby ? -SNIFFER_BABY_AGE_TICKS : 0);
}

// ========== 状态机 ==========

void SnifferEntity::transitionTo(State state)
{
    // 对齐 vanilla Sniffer 的 transitionTo：切换状态时一并处理该状态的伴随语义。
    // 进入 Digging 表示嗅探目标已被挖掘点兑现，标记随之消费（对齐 Brain 在 FinishedDigging 擦除
    // SNIFFER_DIGGING 记忆；此处提前到进入挖掘时，避免挖掘中途被打断后重复使用同一目标）。
    if (state == State::Digging) {
        m_hasSniffingTarget = false;
    }
    // 对齐 MC Sniffer.transitionTo：根据目标状态播放对应声音。
    switch (state) {
        case State::Idling:
            setState(State::Idling);
            break;
        case State::FeelingHappy:
            playSound(SoundEvents::SNIFFER_HAPPY, 1.0f, 1.0f);
            setState(State::FeelingHappy);
            break;
        case State::Scenting:
            setState(State::Scenting);
            // 对齐 MC Sniffer.onScentingStart：幼体音调 1.3F
            playSound(SoundEvents::SNIFFER_SCENTING, 1.0f, isChild() ? 1.3f : 1.0f);
            break;
        case State::Sniffing:
            playSound(SoundEvents::SNIFFER_SNIFFING, 1.0f, 1.0f);
            setState(State::Sniffing);
            break;
        case State::Searching:
            setState(State::Searching);
            break;
        case State::Digging:
            // 对齐 MC Sniffer.transitionTo(DIGGING)：先切状态，再执行 onDiggingStart
            // （记录掉种时刻与广播实体状态 63）。挖掘过程中的方块粒子与击打音效由
            // tick() 内的 emitDiggingParticles 负责；SNIFFER_DIGGING 循环音效由客户端
            // 收到实体状态 63 后播放（对齐 ClientPacketListener case 63 → SnifferSoundInstance）。
            setState(State::Digging);
            _onDiggingStart();
            refreshDimensions();
            break;
        case State::Rising:
            playSound(SoundEvents::SNIFFER_DIGGING_STOP, 1.0f, 1.0f);
            setState(State::Rising);
            refreshDimensions();
            break;
    }
}

void SnifferEntity::_onDiggingStart()
{
    // 对齐 MC Sniffer.onDiggingStart：DATA_DROP_SEED_AT_TICK = tickCount + 120，
    // 并向追踪玩家广播实体状态 63（客户端据此播放 SNIFFER_DIGGING 循环音效）。
    m_dataManager.set(DATA_DROP_SEED_AT_TICK_PARAM, static_cast<i32>(ticksExisted()) + DIGGING_DROP_SEED_OFFSET_TICKS);
    if (m_world != nullptr && !m_world->isClientSide()) {
        m_world->broadcastEntityStatus(m_id, DIGGING_ENTITY_STATUS);
    }
}

entity::EntitySize SnifferEntity::getDimensions(EntityPose pose) const
{
    // 对齐 MC Sniffer.getDefaultDimensions：挖掘状态下使用 DIGGING_DIMENSIONS
    // （宽不变、高 −0.4、眼高 0.81），并按幼体缩放。
    if (getState() == State::Digging) {
        const entity::EntitySize digging(
            getBaseWidth(), getBaseHeight() - DIGGING_HEIGHT_REDUCTION, DIGGING_EYE_HEIGHT, false);
        const f32 ageScale = isChild() ? babyScale() : 1.0f;
        return ageScale == 1.0f ? digging : digging.scale(ageScale);
    }
    return AnimalEntity::getDimensions(pose);
}

// ========== 嗅探状态机 ==========

bool SnifferEntity::canSniff() const
{
    // 对齐 vanilla Sniffer.canSniff：
    //   !isTempted() && !isPanicking() && !isInWater() && !isInLove() && onGround()
    //   && !isPassenger() && !isLeashed()
    auto* self = const_cast<SnifferEntity*>(this);
    return !entity::ai::goal::sniffer::isTempted(self) && !entity::ai::goal::sniffer::isPanicking(self) &&
        !isInWater() && !isInLove() && onGround() && getVehicle() == 0 && !self->isLeashed();
}

bool SnifferEntity::canDig() const
{
    // 对齐 vanilla Sniffer.canDig 的非方块部分（方块标签与已探索位置由 SnifferDigGoal::shouldMoveTo 判定）。
    auto* self = const_cast<SnifferEntity*>(this);
    return !entity::ai::goal::sniffer::isPanicking(self) && !entity::ai::goal::sniffer::isTempted(self) && !isChild() &&
        !isInWater() && onGround() && getVehicle() == 0;
}

bool SnifferEntity::consumeShouldDropSeed()
{
    const bool value = m_shouldDropSeed;
    m_shouldDropSeed = false;
    // 直接掉种时绕过 DATA_DROP_SEED_AT_TICK 的时序门控（该字段仅表达挖掘链路的掉种时刻）。
    m_dropSeedPending = value;
    return value;
}

void SnifferEntity::setSniffCooldownTicks(u64 ticks)
{
    m_sniffCooldownUntil = static_cast<u64>(ticksExisted()) + ticks;
    m_hasSniffingTarget = false;
}

bool SnifferEntity::isSniffCoolingDown() const noexcept
{
    return static_cast<u64>(ticksExisted()) < m_sniffCooldownUntil;
}

bool SnifferEntity::isExploredPosition(const BlockPos& pos) const
{
    return std::find(m_exploredPositions.begin(), m_exploredPositions.end(), pos) != m_exploredPositions.end();
}

void SnifferEntity::storeExploredPosition(const BlockPos& pos)
{
    // 对齐 vanilla Sniffer.storeExploredPosition：最新在前，最多保留 20 个。
    const auto existing = std::find(m_exploredPositions.begin(), m_exploredPositions.end(), pos);
    if (existing != m_exploredPositions.end()) {
        m_exploredPositions.erase(existing);
    }
    m_exploredPositions.insert(m_exploredPositions.begin(), pos);
    if (m_exploredPositions.size() > MAX_EXPLORED_POSITIONS) {
        m_exploredPositions.resize(MAX_EXPLORED_POSITIONS);
    }
}

// ========== 繁殖 ==========

bool SnifferEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // 对齐 MC Sniffer.isFood：ItemStack.is(ItemTags.SNIFFER_FOOD)。
    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return false;
    }
    // 数据包可覆写该标签，故必须经标签系统查询而非硬编码物品常量。
    return item->isIn(item::tag::ItemTags::SNIFFER_FOOD());
}

bool SnifferEntity::canMateWith(const AnimalEntity& other) const
{
    // 对齐 MC Sniffer.canMate：
    // 1. 双方必须是 Sniffer
    // 2. 双方状态必须在 {IDLING, SCENTING, FEELING_HAPPY} 集合内
    // 3. 调用父类 canMate 检查爱心状态
    if (this == &other) {
        return false;
    }
    if (entityType() != other.entityType()) {
        return false;
    }
    // 检查双方状态是否允许繁殖
    auto isMatingState = [](State s) {
        return s == State::Idling || s == State::Scenting || s == State::FeelingHappy;
    };
    if (!isMatingState(getState())) {
        return false;
    }
    const auto* otherSniffer = dynamic_cast<const SnifferEntity*>(&other);
    if (otherSniffer == nullptr || !isMatingState(otherSniffer->getState())) {
        return false;
    }
    return AnimalEntity::canMateWith(other);
}

AnimalEntity* SnifferEntity::spawnChildFromBreeding(AnimalEntity& /*partner*/)
{
    // 对齐 MC Sniffer.spawnChildFromBreeding（Sniffer.java:337-344）：
    //   ItemStack itemstack = new ItemStack(Items.SNIFFER_EGG);
    //   ItemEntity itementity = new ItemEntity(level, getX(), getY(), getZ(), itemstack);
    //   itementity.setDefaultPickUpDelay();
    //   finalizeSpawnChildFromBreeding(level, partner, null);
    //   playSound(SNIFFER_EGG_PLOP, 1.0F, (random.nextFloat() - random.nextFloat()) * 0.2F + 0.5F);
    //   level.addFreshEntity(itementity);
    // finalizeSpawnChildFromBreeding 中与产物无关的部分（发情重置、繁殖冷却、繁殖事件、
    // 爱心粒子、经验球）由 BreedGoal::spawnBaby() 在本方法返回后统一执行。
    IWorld* world = m_world;
    if (world == nullptr || world->isClientSide()) {
        // 繁殖产物只在服务端生成（vanilla spawnChildFromBreeding 仅经 ServerLevel 调用）
        return nullptr;
    }
    auto* registry = world->entityRegistry();
    if (registry == nullptr) {
        return nullptr;
    }

    // 物品实体使用原版三参构造的默认动量 (rand*0.2-0.1, 0.2, rand*0.2-0.1)。
    // ItemEntity 构造函数已把拾取延迟初始化为 DEFAULT_PICKUP_DELAY(10)，
    // 等价 vanilla setDefaultPickUpDelay()。
    math::IRandom& rng = getRandom();
    auto itemEntity = std::make_unique<ItemEntity>(EntityInstanceId(0),
        ItemStack(Items::SNIFFER_EGG),
        x(),
        y(),
        z(),
        static_cast<f32>(rng.nextDouble() * 0.2 - 0.1),
        0.2f,
        static_cast<f32>(rng.nextDouble() * 0.2 - 0.1),
        *registry);
    // 直接构造的实体需要显式设置 typeId（注册表路径会自动设置）
    itemEntity->setTypeId(entity::EntityTypeKeys::ITEM);

    playSound(SoundEvents::SNIFFER_EGG_PLOP, 1.0f, (rng.nextFloat() - rng.nextFloat()) * 0.2f + 0.5f);
    world->spawnEntity(std::move(itemEntity));
    return nullptr;
}

std::unique_ptr<AnimalEntity> SnifferEntity::spawnBaby(AnimalEntity& /*partner*/)
{
    // 嗅探兽繁殖产出为掉蛋（见 spawnChildFromBreeding），不生成幼体。
    return nullptr;
}

// ========== 声音 ==========

std::optional<ResourceLocation> SnifferEntity::getAmbientSound() const
{
    // 对齐 MC Sniffer.getAmbientSound：Digging / Searching 状态不播放环境音
    State s = getState();
    if (s == State::Digging || s == State::Searching) {
        return std::nullopt;
    }
    return SoundEvents::SNIFFER_IDLE;
}

std::optional<ResourceLocation> SnifferEntity::getHurtSound(DamageSource& /*source*/) const
{
    return SoundEvents::SNIFFER_HURT;
}

std::optional<ResourceLocation> SnifferEntity::getDeathSound() const
{
    return SoundEvents::SNIFFER_DEATH;
}

// ========== 生命周期 ==========

void SnifferEntity::tick()
{
    // 对齐 MC Sniffer.tick：
    // - Searching 状态：客户端每 20 tick 播放 SNIFFER_SEARCHING 音效
    // - Digging 状态：emitDiggingParticles + dropSeed
    // - 嗅探（Sniffing）自然结束后由 SnifferSniffGoal 置位掉种标记，于本 tick 消耗并触发一次
    //   dropSeed（对齐 vanilla Sniffing.stop 在 timedOut 时设置嗅探目标的行为效果）。
    if (consumeShouldDropSeed()) {
        dropSeed();
    }
    if (getState() == State::Searching) {
        // 对齐 MC Sniffer.playSearchingSound：仅客户端 + tickCount % 20 == 0
        if (m_world != nullptr && m_world->isClientSide() && ticksExisted() % 20 == 0) {
            // 客户端本地音效，这里通过 playSound 播放
            playSound(SoundEvents::SNIFFER_SEARCHING, 1.0f, 1.0f);
        }
    }
    // 对齐 MC Sniffer.tick：挖掘期间喷出方块粒子，到 DATA_DROP_SEED_AT_TICK 时刻掉出种子。
    if (getState() == State::Digging) {
        const i32 dropAtTick = m_dataManager.get(DATA_DROP_SEED_AT_TICK_PARAM);
        const i32 diggingTicks = DIGGING_PARTICLES_DURATION_TICKS - (dropAtTick - static_cast<i32>(ticksExisted()));
        emitDiggingParticles(diggingTicks);
        dropSeed();
    }

    AnimalEntity::tick();
}

void SnifferEntity::emitDiggingParticles(i32 diggingTicks)
{
    // 对齐 vanilla Sniffer.emitDiggingParticles：仅在 1700~6000 毫秒（挖掘动画中段）喷粒子。
    if (m_world == nullptr || diggingTicks <= DIGGING_PARTICLES_DELAY_TICKS ||
        diggingTicks >= DIGGING_PARTICLES_DURATION_TICKS) {
        return;
    }

    // 头部方块 = 嗅探兽脚下方块；粒子从方块中心略下方喷出。
    const BlockPos headPos(
        static_cast<i32>(std::floor(x())), static_cast<i32>(std::floor(y())) - 1, static_cast<i32>(std::floor(z())));
    const BlockState* state = m_world->getBlockState(headPos);
    if (state == nullptr) {
        return;
    }

    const Vector3 center(
        static_cast<f64>(headPos.x) + 0.5, static_cast<f64>(headPos.y) + 0.35, static_cast<f64>(headPos.z) + 0.5);
    for (i32 i = 0; i < DIGGING_PARTICLES_AMOUNT; ++i) {
        m_world->addBlockParticle(particle::ParticleTypeId::Block, center, Vector3(0.0, 0.0, 0.0), *state);
    }

    // 每 10 tick 播放一次方块击打音效（对齐 vanilla playLocalSound(..., 0.5F, 0.5F)）。
    if (ticksExisted() % 10 == 0) {
        playSound(state->getSoundType().getHitSound(), 0.5f, 0.5f);
    }
}

i32 SnifferEntity::ticksUntilDropSeed() const
{
    return m_dataManager.get(DATA_DROP_SEED_AT_TICK_PARAM) - static_cast<i32>(ticksExisted());
}

void SnifferEntity::dropSeed()
{
    // 对齐 MC Sniffer.dropSeed：仅服务端生效。
    if (m_world == nullptr || m_world->isClientSide()) {
        return;
    }
    // 挖掘链路：掉种时刻必须恰好为当前 tick（DATA_DROP_SEED_AT_TICK 由 transitionTo(Digging) 设置）；
    // 嗅探链路：SnifferSniffGoal 自然结束时直接置位掉种标记，由 tick() 消费后调用本方法。
    const bool diggingDropDue = m_dataManager.get(DATA_DROP_SEED_AT_TICK_PARAM) == static_cast<i32>(ticksExisted());
    if (!diggingDropDue && !m_dropSeedPending) {
        return;
    }
    m_dropSeedPending = false;

    const loot::LootTableManager* lootTables = m_world->lootTableManager();
    if (lootTables == nullptr) {
        return;
    }
    const loot::LootTable* table = lootTables->getTable("minecraft:gameplay/sniffer_digging");
    if (table == nullptr) {
        return;
    }

    // 掉落在头部方块（嗅探兽正下方的可挖掘方块中心），对齐 vanilla getHeadBlock()。
    const BlockPos dropPos(
        static_cast<i32>(std::floor(x())), static_cast<i32>(std::floor(y())) - 1, static_cast<i32>(std::floor(z())));

    auto context = loot::LootContextBuilder(*m_world)
                       .withRandom(getRandom())
                       .withParameter(loot::LootParams::THIS_ENTITY, static_cast<Entity*>(this))
                       .withOwnedValue(loot::LootParams::BLOCK_POS, dropPos)
                       .withLootTableResolver([lootTables](const std::string& id) -> const loot::LootTable* {
                           return lootTables->getTable(id);
                       })
                       .withPredicateResolver([lootTables](const std::string& id) -> const loot::LootCondition* {
                           return lootTables->getPredicate(id);
                       })
                       .build(loot::LootParameterSets::gift());
    if (context == nullptr) {
        return;
    }

    for (const auto& stack : table->generate(*context)) {
        if (!stack.isEmpty()) {
            ItemDropHelper::spawnItemEntity(m_world, stack, x(), y(), z(), getRandom());
        }
    }
    playSound(SoundEvents::SNIFFER_DROP_SEED, 1.0f, 1.0f);
}

// ========== 死亡 ==========

void SnifferEntity::die(DamageSource& source)
{
    // 对齐 MC Java 1.21.11 Sniffer.die（Sniffer.java:347-350）：
    //   this.transitionTo(Sniffer.State.IDLING);
    //   super.die(p_277689_);
    // 死亡时将状态机重置为 Idling，再委托父类执行通用死亡逻辑。
    transitionTo(State::Idling);
    AnimalEntity::die(source);
}

// ========== AI 目标注册 ==========

void SnifferEntity::registerGoals()
{
    // 调用父类方法注册基础动物 AI（基类当前为空实现，保留调用以兼容未来扩展）
    AnimalEntity::registerGoals();

    // 嗅探兽 AI 目标优先级（参考 MC SnifferAi.makeBrain，但因项目无 Brain 系统改用 GoalSelector）：
    //
    // 优先级 0: 游泳（最高优先级，防止溺水）
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::SwimGoal>(this));

    // 优先级 1: 恐慌逃跑（中断挖掘时需擦除嗅探目标，对齐 vanilla Digging.stop 的 resetSniffing）
    auto panicGoal = std::make_unique<entity::ai::goal::PanicGoal>(this, 2.0);
    m_panicGoal = panicGoal.get();
    m_goalSelector.addGoal(1, std::move(panicGoal));

    // 优先级 2: 繁殖
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::BreedGoal>(this, 1.0));

    // 优先级 3: 食物诱惑（火把花种子、瓶草荚果）
    auto temptGoal = std::make_unique<entity::ai::goal::TemptGoal>(
        this,
        1.25,
        [](const ItemStack& stack) -> bool {
            const Item* item = stack.getItem();
            if (item == nullptr) {
                return false;
            }
            if (Items::TORCHFLOWER_SEEDS != nullptr && item == Items::TORCHFLOWER_SEEDS) {
                return true;
            }
            if (Items::PITCHER_POD != nullptr && item == Items::PITCHER_POD) {
                return true;
            }
            return false;
        },
        false);
    m_temptGoal = temptGoal.get();
    m_goalSelector.addGoal(3, std::move(temptGoal));

    // 优先级 4: 跟随父母（幼年嗅探兽）
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::FollowParentGoal>(this, 1.1));

    // 优先级 5: 随机漫步
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::RandomWalkingGoal>(this, 1.0, 60));

    // 优先级 6: 看向玩家
    m_goalSelector.addGoal(6, std::make_unique<entity::ai::goal::LookAtGoal>(this, 8.0f));

    // 优先级 7: 随机看向
    m_goalSelector.addGoal(7, std::make_unique<entity::ai::goal::LookRandomlyGoal>(this));

    // 嗅探兽特有行为（对齐 MC SnifferAi.makeBrain 的 IDLE / SNIFF / DIG 活动，因项目无 Brain
    // 系统改用 GoalSelector）。优先级顺序对应 vanilla 活动的行为顺序：
    //   DIG   : Digging / FinishedDigging
    //   SNIFF : Searching
    //   IDLE  : FeelingHappy → Scenting / Sniffing
    // 优先级 3: 挖掘（对齐 SnifferAi.Digging + FinishedDigging），依赖嗅探目标已完成搜索。
    m_goalSelector.addGoal(3, std::make_unique<entity::ai::goal::SnifferDigGoal>(this, 1.0));
    // 优先级 3: 搜索挖掘点（对齐 SnifferAi.Searching），找到后交接给挖掘目标。
    m_goalSelector.addGoal(3, std::make_unique<entity::ai::goal::SnifferSearchGoal>(this));
    // 优先级 3: 开心（对齐 SnifferAi.FeelingHappy），挖掘完成或繁殖后执行一次。
    m_goalSelector.addGoal(3, std::make_unique<entity::ai::goal::SnifferHappyGoal>(this));
    // 优先级 3: 闻气味 / 嗅探（对齐 SnifferAi.Scenting 与 Sniffing）。
    m_goalSelector.addGoal(3, std::make_unique<entity::ai::goal::SnifferScentGoal>(this));
    m_goalSelector.addGoal(3, std::make_unique<entity::ai::goal::SnifferSniffGoal>(this));
}

// ========== 属性注册 ==========

void SnifferEntity::registerAttributes()
{
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // 对齐 MC Sniffer.createAttributes：
    //   Animal.createAnimalAttributes() + MOVEMENT_SPEED=0.1 + MAX_HEALTH=14.0
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, SNIFFER_MOVEMENT_SPEED);
    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, SNIFFER_MAX_HEALTH);
    attributes().setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, SNIFFER_FOLLOW_RANGE);
}

// ========== 数据同步 ==========

void SnifferEntity::registerData()
{
    AnimalEntity::registerData();

    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    // 对齐 MC Sniffer.defineSynchedData：
    //   DATA_STATE = IDLING (0)  —— wire 走 SNIFFER_STATE 序列化器(id=31, VarInt)
    //   DATA_DROP_SEED_AT_TICK = 0
    m_dataManager.registerParam(DATA_STATE_PARAM, entity::SnifferStateValue{static_cast<i32>(State::Idling)});
    m_dataManager.registerParam(DATA_DROP_SEED_AT_TICK_PARAM, 0);
}

// ========== 脚步声 ==========

void SnifferEntity::playStepSound(const BlockPos& /*pos*/, const BlockState* /*blockState*/)
{
    // 对齐 MC Sniffer.playStepSound：音量 0.15F，音高 1.0F
    playSound(SoundEvents::SNIFFER_STEP, SNIFFER_STEP_VOLUME, 1.0f);
}

// ========== NBT 序列化 ==========

void SnifferEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    using namespace mc::entity::serialization;

    // 先调用基类实现
    AnimalEntity::addAdditionalSaveData(tag);

    // 对齐 MC Sniffer.addAdditionalSaveData：
    //   - "state" (Byte): 当前状态枚举值
    //   - "drop_seed_at_tick" (Int): 挖掘掉落种子的 tick
    tag.put(nbt_keys::SNIFFER_STATE, static_cast<i8>(getState()));
    tag.put(nbt_keys::SNIFFER_DROP_SEED_AT_TICK, m_dataManager.get(DATA_DROP_SEED_AT_TICK_PARAM));
}

Result<void> SnifferEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    using namespace mc::entity::serialization;
    MC_TRY(AnimalEntity::readAdditionalSaveData(tag));

    // 读取状态
    if (auto stateVal = nbt_helper::tryGetByte(tag, nbt_keys::SNIFFER_STATE)) {
        i8 raw = *stateVal;
        // 对齐 MC Sniffer.State.BY_ID：超出范围时回落到 IDLING(0)
        if (raw < 0 || raw > 6) {
            raw = static_cast<i8>(State::Idling);
        }
        setState(static_cast<State>(raw));
    }

    // 读取挖掘掉落种子 tick
    if (auto dropTick = nbt_helper::tryGetInt(tag, nbt_keys::SNIFFER_DROP_SEED_AT_TICK)) {
        m_dataManager.set(DATA_DROP_SEED_AT_TICK_PARAM, *dropTick);
    }

    return Result<void>::ok();
}

} // namespace mc
