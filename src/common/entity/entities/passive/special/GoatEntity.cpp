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

#include "GoatEntity.hpp"

#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/BreedGoal.hpp"
#include "common/entity/ai/goal/goals/FollowParentGoal.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/PanicGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/TemptGoal.hpp"
#include "common/entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "common/entity/ai/goal/goals/special/GoatLongJumpGoal.hpp"
#include "common/entity/ai/goal/goals/special/GoatRamGoal.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/AgeableEntity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/instrument/InstrumentRegistry.hpp"
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>

namespace mc {

entity::DataParameter<bool> GoatEntity::DATA_IS_SCREAMING_GOAT_PARAM = entity::EntityDataManager::createKey<bool>();
entity::DataParameter<bool> GoatEntity::DATA_HAS_LEFT_HORN_PARAM = entity::EntityDataManager::createKey<bool>();
entity::DataParameter<bool> GoatEntity::DATA_HAS_RIGHT_HORN_PARAM = entity::EntityDataManager::createKey<bool>();

const entity::EntityClassInfo& GoatEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"GoatEntity", &AnimalEntity::classInfo()};
    return s_classInfo;
}

void GoatEntity::registerData()
{
    AnimalEntity::registerData();

    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    // 对齐 vanilla Goat.defineSynchedData：三个布尔字段，默认 false / true / true。
    m_dataManager.registerParam(DATA_IS_SCREAMING_GOAT_PARAM, false);
    m_dataManager.registerParam(DATA_HAS_LEFT_HORN_PARAM, true);
    m_dataManager.registerParam(DATA_HAS_RIGHT_HORN_PARAM, true);
}

GoatEntity::GoatEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AnimalEntity(id, registry)
{
    registerData();
    registerAttributes();
    registerGoals();
    // 对齐 vanilla GoatAi.initMemories：创建时确定首次冲撞冷却。
    m_ramCooldown = pickNextRamCooldown();
}

std::unique_ptr<Entity> GoatEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<GoatEntity>(0, registry);
}

void GoatEntity::registerAttributes()
{
    // 对齐 vanilla Goat.createAttributes：Animal 默认 + 生命 10 + 移速 0.2 + 攻击 2 + 护甲 0。
    AnimalEntity::registerAttributes();
    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.2);
    attributes().registerAttribute(*entity::attribute::Attributes::attackDamage());
    attributes().setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 2.0);
    setHealth(maxHealth());
}

void GoatEntity::registerGoals()
{
    AnimalEntity::registerGoals();

    // 优先级 0：游泳（对齐 GoatAi CORE 活动的 Swim(0.8)）
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::SwimGoal>(this));

    // 优先级 1：恐慌（对齐 AnimalPanic(2.0)）
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::PanicGoal>(this, 2.0));

    // 优先级 2：冲撞（对齐 GoatAi RAM 活动：准备 → 冲撞 → 冷却）
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::GoatRamGoal>(this));

    // 优先级 3：繁殖（对齐 AnimalMakeLove）
    m_goalSelector.addGoal(3, std::make_unique<entity::ai::goal::BreedGoal>(this, 1.0));

    // 优先级 4：诱惑（对齐 FollowTemptation，速度倍率 1.25；食物 = #goat_food 即小麦）
    m_goalSelector.addGoal(4,
        std::make_unique<entity::ai::goal::TemptGoal>(
            this,
            1.25,
            [](const ItemStack& stack) {
                return stack.getItem() == Items::WHEAT;
            },
            false));

    // 优先级 5：幼体跟随成体（对齐 BabyFollowAdult，速度倍率 1.25）
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::FollowParentGoal>(this, 1.25));

    // 优先级 6：随机漫步（对齐 RandomStroll.stroll(1.0)）
    m_goalSelector.addGoal(6, std::make_unique<entity::ai::goal::WaterAvoidingRandomWalkingGoal>(this, 1.0));

    // 优先级 7：看向玩家（对齐 SetEntityLookTargetSometimes：6 格）
    m_goalSelector.addGoal(7, std::make_unique<entity::ai::goal::LookAtGoal>(this, 6.0f));

    // 优先级 8：随机看向
    m_goalSelector.addGoal(8, std::make_unique<entity::ai::goal::LookRandomlyGoal>(this));

    // 优先级 9：随机长跳（对齐 GoatAi LONG_JUMP 活动的 LongJumpToRandomPos + LongJumpMidJump；
    // 优先级最低，不影响冲撞/繁殖/漫步等既有行为）
    m_goalSelector.addGoal(9, std::make_unique<entity::ai::goal::GoatLongJumpGoal>(this));
}

entity::EntitySize GoatEntity::getDimensions(EntityPose pose) const
{
    // 长跳中碰撞箱缩放 0.7（vanilla LONG_JUMPING_DIMENSIONS.scale(getAgeScale())），
    // 幼体缩放由 width()/height() 内部承担。
    const entity::EntitySize base = entity::EntitySize(width(), height(), eyeHeight(), false);
    if (pose == EntityPose::LongJumping) {
        return base.scale(LONG_JUMPING_SCALE, LONG_JUMPING_SCALE);
    }
    return base;
}

// ============================================================================
// 尖叫山羊 / 角
// ============================================================================

bool GoatEntity::isScreamingGoat() const
{
    return m_dataManager.get(DATA_IS_SCREAMING_GOAT_PARAM);
}

void GoatEntity::setScreamingGoat(bool screaming)
{
    m_dataManager.set(DATA_IS_SCREAMING_GOAT_PARAM, screaming);
}

bool GoatEntity::hasLeftHorn() const
{
    return m_dataManager.get(DATA_HAS_LEFT_HORN_PARAM);
}

bool GoatEntity::hasRightHorn() const
{
    return m_dataManager.get(DATA_HAS_RIGHT_HORN_PARAM);
}

void GoatEntity::addHorns()
{
    m_dataManager.set(DATA_HAS_LEFT_HORN_PARAM, true);
    m_dataManager.set(DATA_HAS_RIGHT_HORN_PARAM, true);
}

void GoatEntity::removeHorns()
{
    m_dataManager.set(DATA_HAS_LEFT_HORN_PARAM, false);
    m_dataManager.set(DATA_HAS_RIGHT_HORN_PARAM, false);
}

ItemStack GoatEntity::createHorn() const
{
    // 对齐 vanilla Goat.createHorn：
    //   RandomSource.create(uuid.hashCode()) 后从
    //   SCREAMING_GOAT_HORNS / REGULAR_GOAT_HORNS 标签中随机取一个乐器，
    //   并写入山羊角的 instrument 数据组件。
    if (Items::GOAT_HORN == nullptr) {
        return ItemStack();
    }
    ItemStack horn(*Items::GOAT_HORN, 1);
    const ResourceLocation instrumentId = pickHornInstrument();
    if (!instrumentId.toString().empty()) {
        horn.setInstrument(instrumentId.toString());
    }
    return horn;
}

ResourceLocation GoatEntity::pickHornInstrument() const
{
    // 乐器注册表须先初始化（幂等，重复调用无副作用）。
    item::instrument::InstrumentRegistry& registry = item::instrument::InstrumentRegistry::instance();
    registry.initialize();

    // 乐器标签（对齐 InstrumentTagsProvider.REGULAR_GOAT_HORNS / SCREAMING_GOAT_HORNS）。
    static const std::array<const char*, 4> REGULAR = {
        "minecraft:ponder_goat_horn",
        "minecraft:sing_goat_horn",
        "minecraft:seek_goat_horn",
        "minecraft:feel_goat_horn",
    };
    static const std::array<const char*, 4> SCREAMING = {
        "minecraft:admire_goat_horn",
        "minecraft:call_goat_horn",
        "minecraft:yearn_goat_horn",
        "minecraft:dream_goat_horn",
    };
    const auto& candidates = isScreamingGoat() ? SCREAMING : REGULAR;

    // 种子 = UUID 字符串的 Java String.hashCode（对齐 vanilla RandomSource.create(uuid.hashCode())）。
    math::JavaLegacyRandom random;
    random.setSeed(static_cast<u64>(static_cast<u32>(_javaStringHashCode(uuid()))));
    const i32 index = random.nextInt(static_cast<i32>(candidates.size()));
    const ResourceLocation candidate(candidates[static_cast<std::size_t>(index)]);
    if (registry.get(candidate) == nullptr) {
        return ResourceLocation();
    }
    return candidate;
}

i32 GoatEntity::_javaStringHashCode(const std::string& text)
{
    i32 hash = 0;
    for (const char ch : text) {
        hash = 31 * hash + static_cast<u8>(ch);
    }
    return hash;
}

bool GoatEntity::dropHorn()
{
    // 对齐 vanilla dropHorn：优先掉仍存在的那只角，两只都在时随机。
    const bool leftHorn = hasLeftHorn();
    const bool rightHorn = hasRightHorn();
    if (!leftHorn && !rightHorn) {
        return false;
    }
    const bool dropLeft = leftHorn != rightHorn ? leftHorn : getRandom().nextBoolean();
    m_dataManager.set(dropLeft ? DATA_HAS_LEFT_HORN_PARAM : DATA_HAS_RIGHT_HORN_PARAM, false);

    const ItemStack horn = createHorn();
    if (horn.isEmpty()) {
        return false;
    }
    if (m_world != nullptr && !m_world->isClientSide()) {
        // 掉落速度对齐 vanilla：水平 ±0.2、垂直 0.3~0.7。
        ItemEntity* item = ItemDropHelper::spawnItemEntity(
            m_world, horn, x(), y(), z(), m_world->getRandom(), ItemDropHelper::DEFAULT_PICKUP_DELAY);
        if (item != nullptr) {
            const f32 vx = static_cast<f32>(getRandom().nextDouble()) * 0.4f - 0.2f;
            const f32 vy = static_cast<f32>(getRandom().nextDouble()) * 0.4f + 0.3f;
            const f32 vz = static_cast<f32>(getRandom().nextDouble()) * 0.4f - 0.2f;
            item->setVelocity(Vector3(vx, vy, vz));
        }
    }
    return true;
}

// ============================================================================
// 冲撞状态
// ============================================================================

void GoatEntity::setRamCooldown(i32 ticks)
{
    m_ramCooldown = ticks;
}

i32 GoatEntity::pickNextRamCooldown() const
{
    // 对齐 vanilla GoatAi：尖叫山羊 100~300 tick，普通山羊 600~6000 tick。
    if (isScreamingGoat()) {
        return RAM_COOLDOWN_SCREAMER_MIN +
            getRandom().nextInt(RAM_COOLDOWN_SCREAMER_MAX - RAM_COOLDOWN_SCREAMER_MIN + 1);
    }
    return RAM_COOLDOWN_MIN + getRandom().nextInt(RAM_COOLDOWN_MAX - RAM_COOLDOWN_MIN + 1);
}

void GoatEntity::setLoweringHead(bool lowering)
{
    m_isLoweringHead = lowering;
}

// ============================================================================
// 交互
// ============================================================================

ActionResultType GoatEntity::interactMob(Player& player, Hand hand)
{
    // 对齐 vanilla Goat.mobInteract：空桶 + 成体 → 挤奶并把手持物换成牛奶桶。
    ItemStack& heldStack = player.getHeldItem(hand);
    if (heldStack.getItem() == Items::BUCKET && !isChild()) {
        _playSound(isScreamingGoat() ? "screaming_milk" : "milk", 1.0f);
        if (!player.isCreative() && Items::MILK_BUCKET != nullptr) {
            heldStack.shrink(1);
            if (heldStack.isEmpty()) {
                heldStack = ItemStack(*Items::MILK_BUCKET, 1);
            } else {
                ItemStack milkBucket(*Items::MILK_BUCKET, 1);
                const i32 remaining = player.inventory().add(milkBucket);
                if (remaining > 0) {
                    ItemDropHelper::spawnItemEntity(
                        m_world, milkBucket, x(), y(), z(), m_world->getRandom(), ItemDropHelper::DEFAULT_PICKUP_DELAY);
                }
            }
        }
        return ActionResultType::Success;
    }

    const ActionResultType result = AnimalEntity::interactMob(player, hand);
    if (result == ActionResultType::Success && isBreedingItem(heldStack)) {
        _playEatingSound();
    }
    return result;
}

void GoatEntity::_playEatingSound()
{
    // 对齐 vanilla playEatingSound：随机音高 0.8~1.2。
    _playSound(isScreamingGoat() ? "screaming_eat" : "eat", 0.8f + static_cast<f32>(getRandom().nextDouble()) * 0.4f);
}

void GoatEntity::_playSound(const char* suffix, f32 pitch)
{
    // vanilla 的 SoundEvents.GOAT_XXX 由 makeSoundEventId 统一拼接；
    // 查不到对应音效标识时静默跳过（音效资源未就绪的测试夹具）。
    const std::optional<ResourceLocation> sound = makeSoundEventId(suffix);
    if (sound.has_value()) {
        playSound(*sound, getSoundVolume(), pitch);
    }
}

// ============================================================================
// 声音
// ============================================================================

std::optional<ResourceLocation> GoatEntity::getAmbientSound() const
{
    return makeSoundEventId(isScreamingGoat() ? "screaming_ambient" : "ambient");
}

std::optional<ResourceLocation> GoatEntity::getHurtSound(DamageSource& /*source*/) const
{
    return makeSoundEventId(isScreamingGoat() ? "screaming_hurt" : "hurt");
}

std::optional<ResourceLocation> GoatEntity::getDeathSound() const
{
    return makeSoundEventId(isScreamingGoat() ? "screaming_death" : "death");
}

std::optional<ResourceLocation> GoatEntity::getStepSound() const
{
    return makeSoundEventId("step");
}

void GoatEntity::playStepSound(const BlockPos& /*pos*/, const BlockState* /*blockState*/)
{
    // 对齐 vanilla：山羊播放固定脚步声（音量 0.15）。
    if (const auto sound = getStepSound()) {
        playSound(*sound, 0.15f, 1.0f);
    }
}

// ============================================================================
// 繁殖 / 食物
// ============================================================================

bool GoatEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // 对齐 vanilla Goat.isFood：ItemTags.GOAT_FOOD（仅小麦）。
    return itemStack.getItem() == Items::WHEAT && !itemStack.isEmpty();
}

std::unique_ptr<AnimalEntity> GoatEntity::spawnBaby(AnimalEntity& partner)
{
    auto* registry = &ecsRegistry();
    if (registry == nullptr) {
        return nullptr;
    }
    auto baby = std::make_unique<GoatEntity>(0, *registry);
    baby->setTypeId(entity::EntityTypeKeys::GOAT); // 工厂绕过补救：直接构造缺 typeId

    // 对齐 vanilla Goat.getBreedOffspring：是否尖叫取自双亲之一，另有 2% 概率自然尖叫。
    const auto* partnerGoat = dynamic_cast<const GoatEntity*>(&partner);
    const GoatEntity* chosen = getRandom().nextBoolean() ? this : partnerGoat;
    const bool screaming =
        (chosen != nullptr && chosen->isScreamingGoat()) || getRandom().nextDouble() < GOAT_SCREAMING_CHANCE;
    baby->setScreamingGoat(screaming);
    baby->setChild(true);
    baby->setPosition(x(), y(), z());
    return baby;
}

void GoatEntity::finalizeSpawn(
    IWorld& world, const entity::combat::DifficultyInstance& difficulty, world::spawn::SpawnReason spawnReason)
{
    // 对齐 vanilla Goat.finalizeSpawn：2% 尖叫山羊；成体 10% 概率缺一只角。
    setScreamingGoat(getRandom().nextDouble() < GOAT_SCREAMING_CHANCE);
    if (!isChild() && getRandom().nextFloat() < UNIHORN_CHANCE) {
        m_dataManager.set(getRandom().nextBoolean() ? DATA_HAS_LEFT_HORN_PARAM : DATA_HAS_RIGHT_HORN_PARAM, false);
    }
    m_ramCooldown = pickNextRamCooldown();
    AnimalEntity::finalizeSpawn(world, difficulty, spawnReason);
}

// ============================================================================
// 生命周期
// ============================================================================

void GoatEntity::tick()
{
    // 对齐 vanilla Goat.aiStep：低头计时 0..20（低头 +1，否则 -2）。
    if (m_isLoweringHead) {
        ++m_lowerHeadTick;
    } else {
        m_lowerHeadTick -= 2;
    }
    m_lowerHeadTick = std::clamp(m_lowerHeadTick, 0, RAM_PREPARE_TIME);

    AnimalEntity::tick();
}

void GoatEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    AnimalEntity::addAdditionalSaveData(tag);
    tag.put(entity::serialization::nbt_keys::IS_SCREAMING_GOAT, static_cast<i8>(isScreamingGoat()));
    tag.put(entity::serialization::nbt_keys::HAS_LEFT_HORN, static_cast<i8>(hasLeftHorn()));
    tag.put(entity::serialization::nbt_keys::HAS_RIGHT_HORN, static_cast<i8>(hasRightHorn()));
}

Result<void> GoatEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    using namespace entity::serialization;
    MC_TRY(AnimalEntity::readAdditionalSaveData(tag));
    setScreamingGoat(nbt_helper::tryGetBool(tag, nbt_keys::IS_SCREAMING_GOAT).value_or(false));
    m_dataManager.set(DATA_HAS_LEFT_HORN_PARAM, nbt_helper::tryGetBool(tag, nbt_keys::HAS_LEFT_HORN).value_or(true));
    m_dataManager.set(DATA_HAS_RIGHT_HORN_PARAM, nbt_helper::tryGetBool(tag, nbt_keys::HAS_RIGHT_HORN).value_or(true));
    return Result<void>::ok();
}

} // namespace mc