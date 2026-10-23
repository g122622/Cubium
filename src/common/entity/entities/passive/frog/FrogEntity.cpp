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

#include "FrogEntity.hpp"

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
#include "common/entity/ai/goal/goals/special/FrogGoals.hpp"
#include "common/entity/ai/goal/goals/special/FrogLongJumpGoal.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/monster/basic/SlimeEntity.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/chunk/IChunkManager.hpp"
#include "common/world/chunk/data/ChunkData.hpp"

#include <algorithm>
#include <memory>
#include <optional>

namespace mc {

// ========== DataParameter 静态定义 ==========

/// 对齐 vanilla Frog.DATA_VARIANT_ID（FROG_VARIANT 序列化器，Holder<FrogVariant>）
entity::DataParameter<entity::HolderVariantValue> FrogEntity::DATA_VARIANT_ID_PARAM =
    entity::EntityDataManager::createKey<entity::HolderVariantValue>();

/// 对齐 vanilla Frog.DATA_TONGUE_TARGET_ID（OPTIONAL_UNSIGNED_INT 序列化器）
entity::DataParameter<entity::OptionalUnsignedIntValue> FrogEntity::DATA_TONGUE_TARGET_ID_PARAM =
    entity::EntityDataManager::createKey<entity::OptionalUnsignedIntValue>();

const entity::EntityClassInfo& FrogEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"FrogEntity", &AnimalEntity::classInfo()};
    return s_classInfo;
}

// ========== 常量 ==========

namespace {

/// 移动速度（对齐 MC Frog.createAttributes: MOVEMENT_SPEED = 1.0）
constexpr f32 FROG_MOVEMENT_SPEED = 1.0f;
/// 最大生命值（对齐 MC Frog.createAttributes: MAX_HEALTH = 10.0）
constexpr f32 FROG_MAX_HEALTH = 10.0f;
/// 攻击伤害（对齐 MC Frog.createAttributes: ATTACK_DAMAGE = 10.0，供舌击捕食使用）
constexpr f32 FROG_ATTACK_DAMAGE = 10.0f;
/// 跨步高度（对齐 MC Frog.createAttributes: STEP_HEIGHT = 1.0）
constexpr f32 FROG_STEP_HEIGHT = 1.0f;
/// 跟随范围（Animal.createAnimalAttributes 默认值 16.0F）
constexpr f32 FROG_FOLLOW_RANGE = 16.0f;
/// 脚步声量（对齐 MC Frog.playStepSound: 0.15F）
constexpr f32 FROG_STEP_VOLUME = 0.15f;

/// NBT 键：变体（对齐 vanilla VariantUtils.writeVariant 写入的 "variant"）
constexpr const char* NBT_VARIANT = "variant";

} // namespace

// ========== 构造 ==========

FrogEntity::FrogEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AnimalEntity(id, registry)
{
    // 基类构造期虚函数不派发，须在派生类构造显式补调。
    registerGoals();
    registerAttributes();
    registerData();
}

std::unique_ptr<Entity> FrogEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<FrogEntity>(0, registry);
}

// ========== 变体 ==========

FrogEntity::Variant FrogEntity::getVariant() const
{
    return static_cast<Variant>(m_dataManager.get(DATA_VARIANT_ID_PARAM).registryId);
}

void FrogEntity::setVariant(Variant variant)
{
    // serializerId 25 = FROG_VARIANT（vanilla EntityDataSerializers 注册顺序）。
    m_dataManager.set(DATA_VARIANT_ID_PARAM,
        entity::HolderVariantValue{static_cast<i32>(VANILLA_FROG_VARIANT_SERIALIZER_ID), static_cast<i32>(variant)});
}

FrogEntity::Variant FrogEntity::selectVariantForPosition(const IWorld* world, const BlockPos& pos)
{
    // 对齐 vanilla VariantUtils.selectVariantToSpawn：
    //   群系属于 SPAWNS_WARM_VARIANT_FROGS → warm；属于 SPAWNS_COLD_VARIANT_FROGS → cold；否则 temperate。
    if (world == nullptr) {
        return DEFAULT_VARIANT;
    }
    const auto* chunkManager = world->chunkManager();
    if (chunkManager == nullptr) {
        return DEFAULT_VARIANT;
    }
    const ChunkData* chunk = chunkManager->tryToGetChunkInMem(pos.chunkX(), pos.chunkZ());
    if (chunk == nullptr) {
        return DEFAULT_VARIANT;
    }
    const BiomeId biomeId = chunk->getBiomeAtBlock(pos.localX(), pos.y, pos.localZ());

    if (world::biome::BiomeTags::SPAWNS_WARM_VARIANT_FROGS().contains(biomeId)) {
        return Variant::Warm;
    }
    if (world::biome::BiomeTags::SPAWNS_COLD_VARIANT_FROGS().contains(biomeId)) {
        return Variant::Cold;
    }
    // 数据包未提供变体标签时，按原版标签内容回退为群系直接判定：
    //   #spawns_warm_variant_frogs = 热带草原 / 恶地 / 沙漠 / 暖水海洋
    //   #spawns_cold_variant_frogs = 雪原 / 冰雪针叶林 / 针叶林 / 原始松林 / 沼泽
    if (biomeId == Biomes::WarmOcean || biomeId == Biomes::DeepWarmOcean || biomeId == Biomes::Badlands ||
        biomeId == Biomes::BadlandsPlateau || biomeId == Biomes::ModifiedBadlandsPlateau ||
        biomeId == Biomes::WoodedBadlandsPlateau || biomeId == Biomes::ModifiedWoodedBadlandsPlateau ||
        biomeId == Biomes::ErodedBadlands || biomeId == Biomes::Desert || biomeId == Biomes::DesertHills ||
        biomeId == Biomes::DesertLakes || biomeId == Biomes::Savanna || biomeId == Biomes::SavannaPlateau) {
        return Variant::Warm;
    }
    if (biomeId == Biomes::Taiga || biomeId == Biomes::TaigaHills || biomeId == Biomes::TaigaMountains ||
        biomeId == Biomes::SnowyTaiga || biomeId == Biomes::SnowyTaigaHills || biomeId == Biomes::SnowyTaigaMountains ||
        biomeId == Biomes::GiantTreeTaiga || biomeId == Biomes::GiantTreeTaigaHills ||
        biomeId == Biomes::GiantSpruceTaiga || biomeId == Biomes::GiantSpruceTaigaHills || biomeId == Biomes::Swamp ||
        biomeId == Biomes::SwampHills) {
        return Variant::Cold;
    }
    return DEFAULT_VARIANT;
}

// ========== 舌击目标 ==========

void FrogEntity::setTongueTarget(EntityInstanceId target)
{
    m_dataManager.set(DATA_TONGUE_TARGET_ID_PARAM, entity::OptionalUnsignedIntValue{true, static_cast<i32>(target)});
}

void FrogEntity::eraseTongueTarget()
{
    m_dataManager.set(DATA_TONGUE_TARGET_ID_PARAM, entity::OptionalUnsignedIntValue{false, 0});
}

std::optional<EntityInstanceId> FrogEntity::tongueTargetId() const
{
    const auto value = m_dataManager.get(DATA_TONGUE_TARGET_ID_PARAM);
    if (!value.present) {
        return std::nullopt;
    }
    return EntityInstanceId(static_cast<u64>(value.value));
}

bool FrogEntity::canEat(const LivingEntity& entity)
{
    // 对齐 vanilla Frog.canEat：!(entity instanceof Slime slime && slime.getSize() != 1) && type.is(FROG_FOOD)
    if (auto* slime = dynamic_cast<const SlimeEntity*>(&entity); slime != nullptr && slime->getSlimeSize() != 1) {
        return false;
    }
    if (!EntityTypeTags::isInitialized()) {
        // 标签未初始化（测试夹具/早期生成）：按 vanilla 默认成员直接判定
        const std::string typeId = entity.getTypeId();
        return typeId == "minecraft:slime" || typeId == "minecraft:magma_cube";
    }
    return EntityTypeTags::FROG_FOOD().contains(ResourceLocation(entity.getTypeId()));
}

void FrogEntity::eatTongueTarget()
{
    if (m_world == nullptr) {
        return;
    }
    playSound(SoundEvents::FROG_EAT, 2.0f, 1.0f);

    const std::optional<EntityInstanceId> targetId = tongueTargetId();
    if (!targetId.has_value()) {
        return;
    }
    auto* target = dynamic_cast<LivingEntity*>(m_world->getEntity(*targetId));
    if (target == nullptr || !target->isAlive()) {
        return;
    }
    if (attackEntityAsMob(*target) && !target->isAlive()) {
        target->remove(RemovalReason::Killed);
    }
    eraseTongueTarget();
}

// ========== 繁殖 ==========

bool FrogEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // 对齐 vanilla Frog.isFood：ItemStack.is(ItemTags.FROG_FOOD)（黏液球）。
    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return false;
    }
    return item->isIn(item::tag::ItemTags::FROG_FOOD());
}

std::unique_ptr<AnimalEntity> FrogEntity::spawnBaby(AnimalEntity& /*partner*/)
{
    // 对齐 vanilla Frog.getBreedOffspring：后代即青蛙本身（青蛙没有幼体形态）。
    auto* registry = &ecsRegistry();
    auto baby = std::make_unique<FrogEntity>(0, *registry);
    baby->setTypeId(entity::EntityTypeKeys::FROG);
    baby->setVariant(getVariant());
    baby->setPosition(x(), y(), z());
    return baby;
}

// ========== 生命周期 ==========

void FrogEntity::finalizeSpawn(
    IWorld& world, const entity::combat::DifficultyInstance& difficulty, world::spawn::SpawnReason spawnReason)
{
    // 对齐 vanilla Frog.finalizeSpawn：先按生成位置选择变体，再执行通用初始化。
    setVariant(selectVariantForPosition(&world, onPos()));
    AnimalEntity::finalizeSpawn(world, difficulty, spawnReason);
}

void FrogEntity::die(DamageSource& source)
{
    // 对齐 vanilla：死亡时舌击目标失去意义，清理舌击目标同步值并结束突舌状态
    // （攻击目标、USING_TONGUE 姿态由 FrogTongueGoal::resetTask 复位；此处直接兜底复位）。
    eraseTongueTarget();
    setAttackTarget(nullptr);
    setPose(EntityPose::Standing);
    AnimalEntity::die(source);
}

// ========== 摔落伤害 ==========

void FrogEntity::causeFallDamage(f32 distance, f32 damageMultiplier, const DamageSource& source)
{
    // 对齐 vanilla Frog.calculateFallDamage：super.calculateFallDamage(...) - FROG_FALL_DAMAGE_REDUCTION。
    // 基类公式已扣除 5 点减免，此处按比例折算回"等效摔落距离"，使基类按减免后的距离结算，
    // 避免先把伤害施加到实体再补一次负伤害（负伤害会被当成治疗并触发伤害反馈）。
    const f32 safeFallDistance =
        static_cast<f32>(attributes().getValue(entity::attribute::Attributes::SAFE_FALL_DISTANCE, 3.0));
    const f32 fallDamageMultiplier =
        static_cast<f32>(attributes().getValue(entity::attribute::Attributes::FALL_DAMAGE_MULTIPLIER, 1.0));
    const f32 scale = damageMultiplier * fallDamageMultiplier;
    f32 reducedDistance = distance;
    if (scale > 0.0f) {
        // 每格摔落伤害对应 1/scale 格距离；减免 5 点伤害等价于减免 5/scale 格。
        reducedDistance = std::max(safeFallDistance, distance - static_cast<f32>(FROG_FALL_DAMAGE_REDUCTION) / scale);
    }
    AnimalEntity::causeFallDamage(reducedDistance, damageMultiplier, source);
}

// ========== 音效 ==========

std::optional<ResourceLocation> FrogEntity::getAmbientSound() const
{
    return SoundEvents::FROG_AMBIENT;
}

std::optional<ResourceLocation> FrogEntity::getHurtSound(DamageSource& /*source*/) const
{
    return SoundEvents::FROG_HURT;
}

std::optional<ResourceLocation> FrogEntity::getDeathSound() const
{
    return SoundEvents::FROG_DEATH;
}

void FrogEntity::playStepSound(const BlockPos& /*pos*/, const BlockState* /*blockState*/)
{
    // 对齐 MC Frog.playStepSound：音量 0.15F，音高 1.0F
    playSound(SoundEvents::FROG_STEP, FROG_STEP_VOLUME, 1.0f);
}

// ========== AI 目标注册 ==========

void FrogEntity::registerGoals()
{
    // 调用父类方法（基类当前为空实现，保留调用以兼容未来扩展）
    AnimalEntity::registerGoals();

    // 青蛙 AI 优先级（参考 MC FrogAi.makeBrain，因项目无 Brain 系统改用 GoalSelector）：
    //   突舌捕食（IDLE/SWIM 活动的 StartAttacking + TONGUE 活动的 ShootTongue）已以
    //   FrogAttackTargetGoal + FrogTongueGoal 等效实现。
    //   长跳（LONG_JUMP）由 FrogLongJumpGoal 等效实现；产卵（LAY_SPAWN → Frogspawn）
    //   仍依赖 Brain 记忆，待 Brain 版青蛙 AI 落地时补齐。

    // 优先级 0: 游泳（最高优先级，防止溺水）
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::SwimGoal>(this));

    // 优先级 1: 恐慌逃跑
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::PanicGoal>(this, 2.0));

    // 优先级 2: 繁殖
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::BreedGoal>(this, 1.0));

    // 优先级 3: 食物诱惑（FROG_FOOD 标签：黏液球）
    m_goalSelector.addGoal(3,
        std::make_unique<entity::ai::goal::TemptGoal>(
            this,
            1.25,
            [](const ItemStack& stack) -> bool {
                const Item* item = stack.getItem();
                return item != nullptr && item->isIn(item::tag::ItemTags::FROG_FOOD());
            },
            false));

    // 优先级 4: 跟随父母
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::FollowParentGoal>(this, 1.1));

    // 优先级 5: 随机漫步
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::RandomWalkingGoal>(this, 1.0));

    // 优先级 6: 看向最近玩家
    m_goalSelector.addGoal(6, std::make_unique<entity::ai::goal::LookAtGoal>(this, 6.0f));

    // 优先级 7: 随机看向
    m_goalSelector.addGoal(7, std::make_unique<entity::ai::goal::LookRandomlyGoal>(this));

    // 优先级 3: 突舌捕食（对齐 vanilla ShootTongue：Move + Look，消费 attackTarget）
    m_goalSelector.addGoal(3, std::make_unique<entity::ai::goal::FrogTongueGoal>(this));

    // 优先级 4: 寻找可捕食目标（对齐 vanilla StartAttacking + NEAREST_ATTACKABLE）：
    //   范围内最近的 FROG_FOOD 实体（默认 5% 概率/次评估搜索）。
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::FrogAttackTargetGoal>(this, FROG_FOLLOW_RANGE, 0.05f));

    // 优先级 8: 随机长跳（对齐 FrogAi LONG_JUMP 活动的 LongJumpToPreferredBlock +
    //   LongJumpMidJump；优先级最低，不抢占捕食/繁殖/漫步）
    m_goalSelector.addGoal(8, std::make_unique<entity::ai::goal::FrogLongJumpGoal>(this));
}

void FrogEntity::registerAttributes()
{
    AnimalEntity::registerAttributes();

    // 对齐 vanilla Frog.createAttributes
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, FROG_MOVEMENT_SPEED);
    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, FROG_MAX_HEALTH);
    attributes().registerAttribute(*entity::attribute::Attributes::attackDamage());
    attributes().setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, FROG_ATTACK_DAMAGE);
    // STEP_HEIGHT 在本项目是实体字段而非属性（vanilla 1.21 起才有该属性），直接设置。
    setStepHeight(FROG_STEP_HEIGHT);
    attributes().setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, FROG_FOLLOW_RANGE);
}

void FrogEntity::registerData()
{
    AnimalEntity::registerData();

    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    // 对齐 vanilla Frog.defineSynchedData：
    //   DATA_VARIANT_ID = 可用变体中的默认值（此处 TEMPERATE）
    //   DATA_TONGUE_TARGET_ID = OptionalInt.empty()
    m_dataManager.registerParam(DATA_VARIANT_ID_PARAM,
        entity::HolderVariantValue{
            static_cast<i32>(VANILLA_FROG_VARIANT_SERIALIZER_ID), static_cast<i32>(DEFAULT_VARIANT)});
    m_dataManager.registerParam(DATA_TONGUE_TARGET_ID_PARAM, entity::OptionalUnsignedIntValue{false, 0});
}

// ========== NBT ==========

void FrogEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    AnimalEntity::addAdditionalSaveData(tag);
    // 对齐 vanilla VariantUtils.writeVariant：写变体 id（temperate/warm/cold）。
    tag.put(NBT_VARIANT, std::string(variantIdName(getVariant())));
}

Result<void> FrogEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    using namespace entity::serialization;
    MC_TRY(AnimalEntity::readAdditionalSaveData(tag));
    if (const auto variantName = nbt_helper::tryGetString(tag, NBT_VARIANT)) {
        if (const auto parsed = variantFromName(*variantName)) {
            setVariant(*parsed);
        }
    }
    return Result<void>::ok();
}

// ========== 变体名称映射 ==========

const char* FrogEntity::variantIdName(Variant variant)
{
    switch (variant) {
        case Variant::Warm:
            return "minecraft:warm";
        case Variant::Cold:
            return "minecraft:cold";
        case Variant::Temperate:
        default:
            return "minecraft:temperate";
    }
}

std::optional<FrogEntity::Variant> FrogEntity::variantFromName(const std::string& name)
{
    if (name == "minecraft:warm" || name == "warm") {
        return Variant::Warm;
    }
    if (name == "minecraft:cold" || name == "cold") {
        return Variant::Cold;
    }
    if (name == "minecraft:temperate" || name == "temperate") {
        return Variant::Temperate;
    }
    return std::nullopt;
}

} // namespace mc