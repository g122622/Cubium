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

#include "ArmadilloEntity.hpp"

#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/BreedGoal.hpp"
#include "common/entity/ai/goal/goals/FollowParentGoal.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/PanicGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/TemptGoal.hpp"
#include "common/entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/AgeableEntity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/damage/tag/DamageTypeTags.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/attribute/EnvironmentAttributes.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/chunk/IChunkManager.hpp"
#include "common/world/chunk/data/ChunkData.hpp"

#include <cmath>
#include <memory>
#include <utility>

namespace mc {

entity::DataParameter<i32> ArmadilloEntity::DATA_ID_STATE_PARAM = entity::EntityDataManager::createKey<i32>();

namespace {

/// 自然掉落鳞甲的基准间隔（vanilla Armadillo.pickNextScuteDropTime：5 分钟）
constexpr i32 SCUTE_DROP_BASE_TICKS = 20 * 60 * 5;
/// 各状态动画时长（vanilla ArmadilloState.animationDuration）
constexpr i64 ROLLING_ANIMATION_DURATION = 10;
constexpr i64 SCARED_ANIMATION_DURATION = 50;
constexpr i64 UNROLLING_ANIMATION_DURATION = 30;

/// 各状态"是否藏在壳里"（vanilla ArmadilloState.shouldHideInShell 的四个实现）
bool _shouldHideInShell(ArmadilloState state, i64 inStateTicks)
{
    switch (state) {
        case ArmadilloState::Idle:
            return false;
        case ArmadilloState::Rolling:
            return inStateTicks > 5;
        case ArmadilloState::Scared:
            return true;
        case ArmadilloState::Unrolling:
            return inStateTicks < 26;
    }
    return false;
}

/// 状态动画时长（vanilla ArmadilloState.animationDuration）
i64 _animationDuration(ArmadilloState state)
{
    switch (state) {
        case ArmadilloState::Rolling:
            return ROLLING_ANIMATION_DURATION;
        case ArmadilloState::Scared:
            return SCARED_ANIMATION_DURATION;
        case ArmadilloState::Unrolling:
            return UNROLLING_ANIMATION_DURATION;
        case ArmadilloState::Idle:
            break;
    }
    return 0;
}

/// 状态名（NBT "state" 字符串值，对齐 vanilla ArmadilloState 的 StringRepresentable）
const char* _stateName(ArmadilloState state)
{
    switch (state) {
        case ArmadilloState::Rolling:
            return "rolling";
        case ArmadilloState::Scared:
            return "scared";
        case ArmadilloState::Unrolling:
            return "unrolling";
        case ArmadilloState::Idle:
            break;
    }
    return "idle";
}

bool _stateFromName(const std::string& name, ArmadilloState& out)
{
    if (name == "idle") {
        out = ArmadilloState::Idle;
        return true;
    }
    if (name == "rolling") {
        out = ArmadilloState::Rolling;
        return true;
    }
    if (name == "scared") {
        out = ArmadilloState::Scared;
        return true;
    }
    if (name == "unrolling") {
        out = ArmadilloState::Unrolling;
        return true;
    }
    return false;
}

} // namespace

// ============================================================================
// 继承链标识与同步数据
// ============================================================================

const entity::EntityClassInfo& ArmadilloEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"ArmadilloEntity", &AnimalEntity::classInfo()};
    return s_classInfo;
}

void ArmadilloEntity::registerData()
{
    // 先注册父类字段，保证 DATA_ID_STATE 的 id 续接 AnimalEntity 继承链。
    AnimalEntity::registerData();

    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    // 对齐 vanilla Armadillo.defineSynchedData：
    //   ARMADILLO_STATE（serializerId 32）默认 IDLE(0)。
    // 注：i32 承载走 VarInt 编码，与 vanilla ARMADILLO_STATE 的 idMapper(ByteBufCodecs.idMapper) 一致。
    m_dataManager.registerParam(DATA_ID_STATE_PARAM, static_cast<i32>(ArmadilloState::Idle));
}

// ============================================================================
// 构造与 AI
// ============================================================================

ArmadilloEntity::ArmadilloEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AnimalEntity(id, registry)
{
    // 注册同步数据（须先于任何 switchToState）。
    registerData();
    // 注册属性与 AI 目标。
    registerAttributes();
    registerGoals();
    // 对齐 vanilla 构造：scuteTime 在创建时随机确定。
    resetScuteTime();
}

std::unique_ptr<Entity> ArmadilloEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<ArmadilloEntity>(0, registry);
}

void ArmadilloEntity::registerGoals()
{
    AnimalEntity::registerGoals();

    // 优先级 0：游泳（对齐 ArmadilloAi CORE 活动的 Swim）
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::SwimGoal>(this));

    // 优先级 1：环境伤害恐慌（对齐 ArmadilloAi.ArmadilloPanic：仅 PANIC_ENVIRONMENTAL_CAUSES）
    auto panicGoal =
        std::make_unique<entity::ai::goal::PanicGoal>(this, 2.0, DamageTypeTags::PANIC_ENVIRONMENTAL_CAUSES());
    m_panicGoal = panicGoal.get();
    m_goalSelector.addGoal(1, std::move(panicGoal));

    // 优先级 2：繁殖（对齐 ArmadilloAi IDLE 活动的 AnimalMakeLove）
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::BreedGoal>(this, 1.0));

    // 优先级 3：诱惑（对齐 ArmadilloAi FollowTemptation，速度倍率 1.25；食物 = 蜘蛛眼）
    m_goalSelector.addGoal(3,
        std::make_unique<entity::ai::goal::TemptGoal>(
            this, 1.25, [](const ItemStack& stack) { return stack.getItem() == Items::SPIDER_EYE; }, false));

    // 优先级 4：幼体跟随成体（对齐 ArmadilloAi BabyFollowAdult）
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::FollowParentGoal>(this, 1.25));

    // 优先级 5：随机漫步
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::WaterAvoidingRandomWalkingGoal>(this, 1.0));

    // 优先级 6：看向玩家（对齐 ArmadilloAi SetEntityLookTargetSometimes：6 格）
    m_goalSelector.addGoal(6, std::make_unique<entity::ai::goal::LookAtGoal>(this, 6.0f));

    // 优先级 7：随机看向
    m_goalSelector.addGoal(7, std::make_unique<entity::ai::goal::LookRandomlyGoal>(this));
}

void ArmadilloEntity::registerAttributes()
{
    // 对齐 vanilla Armadillo.createAttributes：Animal 默认属性 + 生命 12 + 移速 0.14。
    AnimalEntity::registerAttributes();
    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 12.0);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.14);
    setHealth(maxHealth());
}

// ============================================================================
// 状态
// ============================================================================

ArmadilloState ArmadilloEntity::getState() const
{
    return static_cast<ArmadilloState>(m_dataManager.get(DATA_ID_STATE_PARAM));
}

void ArmadilloEntity::switchToState(ArmadilloState state)
{
    // 对齐 vanilla onSyncedDataUpdated：状态变化本身即把 inStateTicks 归零。
    if (getState() != state) {
        m_inStateTicks = 0;
    }
    m_dataManager.set(DATA_ID_STATE_PARAM, static_cast<i32>(state));
}

bool ArmadilloEntity::shouldHideInShell() const
{
    return _shouldHideInShell(getState(), m_inStateTicks);
}

bool ArmadilloEntity::shouldSwitchToScaredState() const
{
    return getState() == ArmadilloState::Rolling && m_inStateTicks > _animationDuration(ArmadilloState::Rolling);
}

bool ArmadilloEntity::canStayRolledUp() const
{
    // 对齐 vanilla canStayRolledUp：!isPanicking && !isInLiquid && !isLeashed && !isPassenger && !isVehicle。
    return !isPanicking() && !isInWater() && !isLeashed() && getVehicle() == INVALID_ENTITY_ID &&
        getPassengers().empty();
}

bool ArmadilloEntity::isPanicking() const
{
    return m_panicGoal != nullptr && m_panicGoal->isRunning();
}

bool ArmadilloEntity::isScaredBy(const LivingEntity& other) const
{
    // 对齐 vanilla Armadillo.isScaredBy：膨胀框 (7, 2, 7) 内，且满足任一条件。
    const AxisAlignedBB scareBox = boundingBox().expand(static_cast<f32>(SCARE_DISTANCE_HORIZONTAL),
        static_cast<f32>(SCARE_DISTANCE_VERTICAL),
        static_cast<f32>(SCARE_DISTANCE_HORIZONTAL));
    if (!scareBox.intersects(other.boundingBox())) {
        return false;
    }
    if (EntityTypeTags::UNDEAD().contains(other.getTypeId())) {
        return true;
    }
    if (getLastHurtBy() == &other) {
        return true;
    }
    if (const auto* player = dynamic_cast<const Player*>(&other)) {
        return player->isSprinting() || player->getVehicle() != INVALID_ENTITY_ID;
    }
    return false;
}

// ============================================================================
// 动作
// ============================================================================

void ArmadilloEntity::rollUp()
{
    // 对齐 vanilla rollUp：已受惊则忽略。
    if (isScared()) {
        return;
    }
    // 对齐 vanilla stopInPlace：清零速度并中断导航。
    setVelocity(Vector3(0.0, 0.0, 0.0));
    clearNavigation();
    resetLove();
    _playSoundOrNothing("roll");
    switchToState(ArmadilloState::Rolling);
}

void ArmadilloEntity::rollOut()
{
    // 对齐 vanilla rollOut：仅在受惊状态下展开。
    if (!isScared()) {
        return;
    }
    _playSoundOrNothing("unroll_finish");
    switchToState(ArmadilloState::Idle);
}

bool ArmadilloEntity::brushOffScute(Player* player)
{
    // 对齐 vanilla brushOffScute：幼年不刷；否则掉落 1 个鳞甲 + 音效。
    if (isChild()) {
        return false;
    }
    if (m_world != nullptr && !m_world->isClientSide() && Items::ARMADILLO_SCUTE != nullptr) {
        ItemStack scuteStack(*Items::ARMADILLO_SCUTE, 1);
        (void)ItemDropHelper::spawnItemEntity(
            m_world, scuteStack, x(), y() + 0.5, z(), m_world->getRandom(), ItemDropHelper::DEFAULT_PICKUP_DELAY);
        _playSoundOrNothing("brush");
    }
    MC_UNUSED(player);
    return true;
}

void ArmadilloEntity::resetScuteTime()
{
    // 对齐 vanilla pickNextScuteDropTime：base + rand(0, base)，base 取自环境属性
    // ARMADILLO_SCUTE_DROP_INTERVAL（单位秒，默认 1200 秒）；属性系统未就绪时用 5 分钟常量。
    i32 baseTicks = SCUTE_DROP_BASE_TICKS;
    const IWorld* worldPtr = world();
    if (worldPtr != nullptr) {
        const world::attribute::EnvironmentAttributeSystem& attributes = worldPtr->environmentAttributes();
        if (attributes.attributeCount() != 0) {
            const math::Vector3 queryPos(static_cast<f32>(x()), static_cast<f32>(y()), static_cast<f32>(z()));
            const f32 intervalSeconds =
                attributes.getValue(world::attribute::EnvironmentAttributes::ARMADILLO_SCUTE_DROP_INTERVAL(), queryPos);
            if (intervalSeconds > 0.0f) {
                baseTicks = static_cast<i32>(intervalSeconds * 20.0f);
            }
        }
    }
    m_scuteTime = baseTicks + getRandom().nextInt(baseTicks);
}

// ============================================================================
// 交互
// ============================================================================

ActionResultType ArmadilloEntity::interactMob(Player& player, Hand hand)
{
    // 对齐 vanilla Armadillo.mobInteract：
    //   手持刷子且刷掉鳞甲成功 → 消耗 16 点刷子耐久，返回 Success；
    //   受惊 → 拒绝交互（Fail）；
    //   其余交给基类（食物繁殖等）。
    ItemStack& heldStack = player.getHeldItem(hand);
    const Item* item = heldStack.getItem();
    if (item == Items::BRUSH && !heldStack.isEmpty() && brushOffScute(&player)) {
        (void)LivingEntity::hurtAndBreak(
            heldStack, BRUSH_DURABILITY_COST, &player, LivingEntity::handToEquipmentSlot(hand));
        return ActionResultType::Success;
    }
    if (isScared()) {
        return ActionResultType::Fail;
    }
    return AnimalEntity::interactMob(player, hand);
}

// ============================================================================
// 伤害
// ============================================================================

bool ArmadilloEntity::hurt(DamageSource& source, f32 amount)
{
    // 对齐 vanilla hurtServer：受惊（卷球）状态下伤害减半为 (amount-1)/2。
    f32 effectiveAmount = amount;
    if (isScared()) {
        effectiveAmount = (amount - 1.0f) / 2.0f;
    }
    return LivingEntity::hurt(source, effectiveAmount);
}

void ArmadilloEntity::actuallyHurt(DamageSource& source, f32 amount)
{
    LivingEntity::actuallyHurt(source, amount);

    if (isNoAi() || isDead() || health() <= 0.0f) {
        return;
    }
    const bool environmentalPanic = source.is(DamageTypeTags::PANIC_ENVIRONMENTAL_CAUSES());
    _onHurtByEntity(source, environmentalPanic);
}

void ArmadilloEntity::_onHurtByEntity(const DamageSource& source, bool isEnvironmentalPanicCause)
{
    // 对齐 vanilla actuallyHurt：
    //   攻击者为生物 → 记录危险记忆（80 tick），可卷球时立即卷球；
    //   否则环境恐慌伤害 → 展开（不再躲藏）。
    if (source.getEntity() != nullptr) {
        m_dangerTicks = DANGER_MEMORY_DURATION;
        if (canStayRolledUp()) {
            rollUp();
        }
        return;
    }
    if (isEnvironmentalPanicCause) {
        rollOut();
    }
}

f32 ArmadilloEntity::eyeHeight() const
{
    // 卷球时头部隐藏，视线点降低到近中心位置，避免隔着壳看到外部。
    return shouldHideInShell() ? 0.4f : 0.7f;
}

void ArmadilloEntity::updateMovementGoalFlags()
{
    // 对齐 vanilla：卷球期间不寻路（ArmadilloAi 的 MoveToTargetSink 在 isScared 时拒绝启动，
    // 速度倍率随之失效）。关闭 MOVE flag 使一切带 Move 的 goal（漫步/诱惑/繁殖/恐慌）停摆。
    MobEntity::updateMovementGoalFlags();
    if (isScared()) {
        m_goalSelector.disableFlag(entity::ai::GoalFlag::Move);
        m_goalSelector.disableFlag(entity::ai::GoalFlag::Jump);
    }
}

void ArmadilloEntity::_playSoundOrNothing(std::string_view suffix)
{
    // vanilla 的 SoundEvents.ARMADILLO_XXX 由 makeSoundEventId 统一拼接；
    // 查不到对应音效标识时静默跳过（音效资源未就绪的测试夹具）。
    const std::optional<ResourceLocation> sound = makeSoundEventId(suffix);
    if (sound.has_value()) {
        playSound(*sound, getSoundVolume(), 1.0f);
    }
}

// ============================================================================
// 声音
// ============================================================================

std::optional<ResourceLocation> ArmadilloEntity::getAmbientSound() const
{
    // 对齐 vanilla：受惊时不播放环境音。
    if (isScared()) {
        return std::nullopt;
    }
    return makeSoundEventId("ambient");
}

std::optional<ResourceLocation> ArmadilloEntity::getHurtSound(DamageSource& /*source*/) const
{
    // 对齐 vanilla：受惊时使用音量更低的受伤音效。
    return isScared() ? makeSoundEventId("hurt_reduced") : makeSoundEventId("hurt");
}

std::optional<ResourceLocation> ArmadilloEntity::getDeathSound() const
{
    return makeSoundEventId("death");
}

std::optional<ResourceLocation> ArmadilloEntity::getStepSound() const
{
    return makeSoundEventId("step");
}

// ============================================================================
// 繁殖
// ============================================================================

bool ArmadilloEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // 对齐 vanilla Armadillo.isFood：ItemTags.ARMADILLO_FOOD（蜘蛛眼）。
    const Item* item = itemStack.getItem();
    if (item == nullptr || itemStack.isEmpty()) {
        return false;
    }
    return item == Items::SPIDER_EYE;
}

std::unique_ptr<AnimalEntity> ArmadilloEntity::spawnBaby(AnimalEntity& /*partner*/)
{
    auto* registry = &ecsRegistry();
    if (registry == nullptr) {
        return nullptr;
    }
    auto baby = std::make_unique<ArmadilloEntity>(0, *registry);
    baby->setChild(true);
    baby->setPosition(x(), y(), z());
    return baby;
}

// ============================================================================
// 生命周期
// ============================================================================

void ArmadilloEntity::_tickDangerMemory()
{
    if (m_dangerTicks > 0) {
        --m_dangerTicks;
    }

    // 对齐 ArmadilloBallUp.tick：以"危险剩余时间 > 75"作为危险仍在身边的判定。
    const bool dangerAround = m_dangerTicks > DANGER_RECENTLY_THRESHOLD;
    if (dangerAround != m_dangerWasAround) {
        m_nextPeekTimer = static_cast<i32>(SCARED_ANIMATION_DURATION) + getRandom().nextInt(100, 400);
    }
    m_dangerWasAround = dangerAround;

    if (m_nextPeekTimer > 0) {
        --m_nextPeekTimer;
    }
}

void ArmadilloEntity::tick()
{
    AnimalEntity::tick();

    // inStateTicks 随 tick 递增（对齐 vanilla Armadillo.tick 末尾的 ++inStateTicks）。
    ++m_inStateTicks;

    if (m_world == nullptr || m_world->isClientSide()) {
        return;
    }

    _tickDangerMemory();

    // 自然掉落犰狳鳞甲（对齐 vanilla customServerAiStep：存活、非幼年、倒计时到 0）。
    if (isAlive() && !isChild() && --m_scuteTime <= 0) {
        if (Items::ARMADILLO_SCUTE != nullptr) {
            ItemStack scuteStack(*Items::ARMADILLO_SCUTE, 1);
            (void)ItemDropHelper::spawnItemEntity(
                m_world, scuteStack, x(), y() + 0.5, z(), m_world->getRandom(), ItemDropHelper::DEFAULT_PICKUP_DELAY);
            _playSoundOrNothing("scute_drop");
        }
        resetScuteTime();
    }

    // 惊吓检测（对齐 ArmadilloAi.ARMADILLO_SCARE_DETECTED 传感器 + SCARE_CHECK_INTERVAL 节流）。
    const i64 currentTick = ticksExisted();
    if (m_lastScareCheckTick < 0 || currentTick - m_lastScareCheckTick >= SCARE_CHECK_INTERVAL) {
        m_lastScareCheckTick = currentTick;
        const AxisAlignedBB searchBox = boundingBox().expand(static_cast<f32>(SCARE_DISTANCE_HORIZONTAL),
            static_cast<f32>(SCARE_DISTANCE_VERTICAL),
            static_cast<f32>(SCARE_DISTANCE_HORIZONTAL));
        const std::vector<Entity*> nearby = m_world->getEntitiesInAABB(searchBox, this);
        for (Entity* candidate : nearby) {
            auto* living = dynamic_cast<LivingEntity*>(candidate);
            if (living == nullptr || !living->isAlive()) {
                continue;
            }
            if (isScaredBy(*living)) {
                m_dangerTicks = DANGER_MEMORY_DURATION;
                break;
            }
        }
    }

    // 卷球/探头/展开时序（对齐 ArmadilloBallUp.tick 的状态机）。
    const ArmadilloState state = getState();
    if (state == ArmadilloState::Idle) {
        return;
    }
    if (shouldSwitchToScaredState()) {
        // ROLLING 动画结束 → SCARED
        switchToState(ArmadilloState::Scared);
        if (onGround()) {
            _playSoundOrNothing("land");
        }
        return;
    }
    if (state == ArmadilloState::Scared) {
        if (m_dangerTicks < static_cast<i32>(UNROLLING_ANIMATION_DURATION)) {
            // 危险即将消失 → 开始展开
            _playSoundOrNothing("unroll_start");
            switchToState(ArmadilloState::Unrolling);
        }
        return;
    }
    if (state == ArmadilloState::Unrolling && m_dangerTicks > static_cast<i32>(UNROLLING_ANIMATION_DURATION)) {
        // 危险重新出现 → 回到受惊
        switchToState(ArmadilloState::Scared);
        return;
    }
    if (state == ArmadilloState::Unrolling && m_dangerTicks <= 0) {
        // 展开动画结束且危险消失 → 恢复常态（vanilla 由 PANIC 活动退出时 rollOut）
        rollOut();
    }
}

void ArmadilloEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    AnimalEntity::addAdditionalSaveData(tag);
    // 对齐 vanilla：存状态的字符串名与 scute_time。
    tag.put(entity::serialization::nbt_keys::ARMADILLO_STATE, std::string(_stateName(getState())));
    tag.put(entity::serialization::nbt_keys::SCUTE_TIME, m_scuteTime);
}

Result<void> ArmadilloEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    using namespace entity::serialization;
    MC_TRY(AnimalEntity::readAdditionalSaveData(tag));

    if (const auto stateName = nbt_helper::tryGetString(tag, nbt_keys::ARMADILLO_STATE)) {
        ArmadilloState state = ArmadilloState::Idle;
        if (_stateFromName(*stateName, state)) {
            switchToState(state);
        }
    }
    if (const auto scute = nbt_helper::tryGetInt(tag, nbt_keys::SCUTE_TIME)) {
        m_scuteTime = *scute;
    }
    return Result<void>::ok();
}

} // namespace mc