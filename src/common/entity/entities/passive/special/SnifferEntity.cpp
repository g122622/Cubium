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
#include "../../../../core/Types.hpp"
#include "../../../../item/Items.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../world/block/BlockPos.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/BreedGoal.hpp"
#include "../../../ai/goal/goals/FollowParentGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../ai/goal/goals/PanicGoal.hpp"
#include "../../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/TemptGoal.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityDataManager.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../../serialization/EntityNbtKeys.hpp"
#include "../../../serialization/NbtHelper.hpp"
#include "common/core/Result.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"
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
            // TODO: 对齐 MC Sniffer.onDiggingStart：
            //   1. 设置 DATA_DROP_SEED_AT_TICK = tickCount + 120
            //   2. 广播实体状态 (byte)63 触发客户端挖掘粒子
            //   3. 通过 Brain + MemoryModuleType.SNIFFER_EXPLORED_POSITIONS 管理挖掘位置
            // 当前项目无 Brain 系统且挖掘 AI 未实现，此处仅切换状态。
            // 后续实现挖掘 Goal 时需在此补充掉落种子时序逻辑。
            setState(State::Digging);
            break;
        case State::Rising:
            playSound(SoundEvents::SNIFFER_DIGGING_STOP, 1.0f, 1.0f);
            setState(State::Rising);
            break;
    }
}

// ========== 繁殖 ==========

bool SnifferEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // 对齐 MC Sniffer.isFood：使用 ItemTags.SNIFFER_FOOD 标签。
    // 当前项目无物品标签系统，直接判断 TORCHFLOWER_SEEDS 或 PITCHER_POD。
    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return false;
    }
    // Items 可能在初始化期间为 nullptr，需防御性检查
    if (Items::TORCHFLOWER_SEEDS != nullptr && item == Items::TORCHFLOWER_SEEDS) {
        return true;
    }
    if (Items::PITCHER_POD != nullptr && item == Items::PITCHER_POD) {
        return true;
    }
    return false;
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
    auto isMatingState = [](State s) { return s == State::Idling || s == State::Scenting || s == State::FeelingHappy; };
    if (!isMatingState(getState())) {
        return false;
    }
    const auto* otherSniffer = dynamic_cast<const SnifferEntity*>(&other);
    if (otherSniffer == nullptr || !isMatingState(otherSniffer->getState())) {
        return false;
    }
    return AnimalEntity::canMateWith(other);
}

std::unique_ptr<AnimalEntity> SnifferEntity::spawnBaby(AnimalEntity& /*partner*/)
{
    // ECS 迁移：实体构造需要 registry 句柄，ClientWorld 返回 nullptr 表客户端不接入 ECS
    auto* registry = &ecsRegistry();
    if (registry == nullptr) {
        return nullptr;
    }

    // 创建幼体嗅探兽
    auto baby = std::make_unique<SnifferEntity>(0, *registry);
    baby->setTypeId(entity::EntityTypeKeys::SNIFFER); // 工厂绕过补救：直接构造缺 typeId
    // 设置为幼体（-48000 tick，40 分钟）
    baby->setChild(true);
    // 设置位置（在父体位置附近）
    baby->setPosition(x(), y(), z());
    return baby;
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
    // - Digging 状态：emitDiggingParticles + dropSeed（当前未实现）
    if (getState() == State::Searching) {
        // 对齐 MC Sniffer.playSearchingSound：仅客户端 + tickCount % 20 == 0
        if (m_world != nullptr && m_world->isClientSide() && ticksExisted() % 20 == 0) {
            // 客户端本地音效，这里通过 playSound 播放
            playSound(SoundEvents::SNIFFER_SEARCHING, 1.0f, 1.0f);
        }
    }
    // TODO: Digging 状态的 emitDiggingParticles + dropSeed 逻辑待挖掘 AI 实现后补充。

    AnimalEntity::tick();
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

    // 优先级 1: 恐慌逃跑
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::PanicGoal>(this, 2.0));

    // 优先级 2: 繁殖
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::BreedGoal>(this, 1.0));

    // 优先级 3: 食物诱惑（火把花种子、瓶草荚果）
    auto* temptGoal = new entity::ai::goal::TemptGoal(
        this,
        1.25,
        [](const ItemStack& stack) -> bool {
            const Item* item = stack.getItem();
            if (item == nullptr) return false;
            if (Items::TORCHFLOWER_SEEDS != nullptr && item == Items::TORCHFLOWER_SEEDS) {
                return true;
            }
            if (Items::PITCHER_POD != nullptr && item == Items::PITCHER_POD) {
                return true;
            }
            return false;
        },
        false);
    m_goalSelector.addGoal(3, temptGoal);

    // 优先级 4: 跟随父母（幼年嗅探兽）
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::FollowParentGoal>(this, 1.1));

    // 优先级 5: 随机漫步
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::RandomWalkingGoal>(this, 1.0, 60));

    // 优先级 6: 看向玩家
    m_goalSelector.addGoal(6, std::make_unique<entity::ai::goal::LookAtGoal>(this, 8.0f));

    // 优先级 7: 随机看向
    m_goalSelector.addGoal(7, std::make_unique<entity::ai::goal::LookRandomlyGoal>(this));

    // TODO: 待 Brain 系统或等效 Goal 系统实现后，补充以下嗅探兽特有 AI：
    //   - ScentingGoal（闻气味，对齐 SnifferAi.Scenting）
    //   - SniffingGoal（嗅探，对齐 SnifferAi.Sniffing）
    //   - SearchingGoal（搜索挖掘点，对齐 SnifferAi.Searching）
    //   - DiggingGoal（挖掘并掉落种子，对齐 SnifferAi.DiggingAndRise）
    //   - RisingGoal（挖掘结束抬头，对齐 SnifferAi.DiggingAndRise.Rising）
    //   - FeelingHappyGoal（繁殖后开心，对齐 SnifferAi.FeelingHappy）
    // 当前实现仅提供基础动物 AI，保证嗅探兽可被繁殖、跟随父母、随机漫步。
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
