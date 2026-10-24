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

#include "ChickenEntity.hpp"

#include "common/core/Types.hpp"
#include "common/entity/ai/goal/goals/BreedGoal.hpp"
#include "common/entity/ai/goal/goals/FollowParentGoal.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/PanicGoal.hpp"
#include "common/entity/ai/goal/goals/RandomWalkingGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/TemptGoal.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/AgeableEntity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/IChunkManager.hpp"
#include "common/world/chunk/data/ChunkData.hpp"

#include <memory>
#include <optional>
#include <utility>

namespace mc {

entity::DataParameter<entity::HolderVariantValue> ChickenEntity::DATA_VARIANT_ID_PARAM =
    entity::EntityDataManager::createKey<entity::HolderVariantValue>();

const entity::EntityClassInfo& ChickenEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"ChickenEntity", &AnimalEntity::classInfo()};
    return s_classInfo;
}

void ChickenEntity::registerData()
{
    // 先注册父类字段，保证 DATA_VARIANT_ID 的 id 续接 AnimalEntity 继承链。
    AnimalEntity::registerData();

    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    // 对齐 vanilla Chicken.defineSynchedData：
    //   DATA_VARIANT_ID = CHICKEN_VARIANT（serializerId 27），默认 TEMPERATE(0)。
    m_dataManager.registerParam(DATA_VARIANT_ID_PARAM,
        entity::HolderVariantValue{
            static_cast<i32>(VANILLA_CHICKEN_VARIANT_SERIALIZER_ID), static_cast<i32>(DEFAULT_VARIANT)});
}

namespace {
/**
 * @brief 按群系判定是否为农畜暖/寒变种群系（数据包标签未就绪时的回退）
 *
 * 对齐数据包标签内容：
 *   #spawns_warm_variant_farm_animals = 沙漠 / 暖水海洋 / #is_jungle / #is_savanna / 下界 /
 *                                       #is_badlands / 红树林沼泽 / 温和海洋
 *   #spawns_cold_variant_farm_animals = 雪原 / 冰刺 / 冻峰 / 尖峭山峰 / 雪坡 / 冻洋 / 深冻洋 /
 *                                       树林 / 深暗之域 / 冻河 / 积雪针叶林 / 雪滩 / 末地 / 冷水海洋 /
 *                                       深冷水海洋 / 原始松林 / 原始云杉林 / 针叶林 / 风袭森林 /
 *                                       风袭砂砾丘陵 / 风袭丘陵 / 石峰
 */
bool _isWarmFarmAnimalBiome(BiomeId biomeId)
{
    using namespace world::biome;
    return biomeId == Biomes::Desert || biomeId == Biomes::WarmOcean || biomeId == Biomes::DeepWarmOcean ||
        biomeId == Biomes::LukewarmOcean || biomeId == Biomes::DeepLukewarmOcean || biomeId == Biomes::MangroveSwamp ||
        biomeId == Biomes::Jungle || biomeId == Biomes::JungleHills || biomeId == Biomes::JungleEdge ||
        biomeId == Biomes::ModifiedJungle || biomeId == Biomes::ModifiedJungleEdge || biomeId == Biomes::BambooJungle ||
        biomeId == Biomes::BambooJungleHills || biomeId == Biomes::Savanna || biomeId == Biomes::SavannaPlateau ||
        biomeId == Biomes::ShatteredSavanna || biomeId == Biomes::ShatteredSavannaPlateau ||
        biomeId == Biomes::Badlands || biomeId == Biomes::WoodedBadlandsPlateau || biomeId == Biomes::BadlandsPlateau ||
        biomeId == Biomes::ModifiedWoodedBadlandsPlateau || biomeId == Biomes::ModifiedBadlandsPlateau ||
        biomeId == Biomes::ErodedBadlands;
}

bool _isColdFarmAnimalBiome(BiomeId biomeId)
{
    using namespace world::biome;
    return biomeId == Biomes::SnowyPlains || biomeId == Biomes::IceSpikes || biomeId == Biomes::FrozenPeaks ||
        biomeId == Biomes::JaggedPeaks || biomeId == Biomes::SnowySlopes || biomeId == Biomes::FrozenOcean ||
        biomeId == Biomes::DeepFrozenOcean || biomeId == Biomes::Grove || biomeId == Biomes::DeepDark ||
        biomeId == Biomes::FrozenRiver || biomeId == Biomes::SnowyTaiga || biomeId == Biomes::SnowyTaigaHills ||
        biomeId == Biomes::SnowyTaigaMountains || biomeId == Biomes::SnowyBeach || biomeId == Biomes::ColdOcean ||
        biomeId == Biomes::DeepColdOcean || biomeId == Biomes::OldGrowthPineTaiga ||
        biomeId == Biomes::OldGrowthSpruceTaiga || biomeId == Biomes::Taiga || biomeId == Biomes::TaigaHills ||
        biomeId == Biomes::TaigaMountains || biomeId == Biomes::WindsweptForest ||
        biomeId == Biomes::WindsweptGravellyHills || biomeId == Biomes::WindsweptHills || biomeId == Biomes::StonyPeaks;
}
} // namespace

std::unique_ptr<Entity> ChickenEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    // 使用临时ID 0，实际ID由 EntityManager 分配
    // 注意：不要使用静态计数器，以避免线程安全问题和ID冲突
    return std::make_unique<ChickenEntity>(0, registry);
}

ChickenEntity::ChickenEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AnimalEntity(id, registry)
{
    // 注册同步数据参数（DATA_VARIANT_ID）。须先于任何 setVariant 调用。
    registerData();
    // 注册属性
    registerAttributes();
    // 注册 AI 目标
    registerGoals();
    // 初始化下蛋计时器
    resetEggTimer();
}

void ChickenEntity::setVariant(ChickenVariant variant)
{
    // 对齐 vanilla Chicken.setVariant：值直接写入 DATA_VARIANT_ID（Holder<ChickenVariant>）。
    m_dataManager.set(DATA_VARIANT_ID_PARAM,
        entity::HolderVariantValue{static_cast<i32>(VANILLA_CHICKEN_VARIANT_SERIALIZER_ID), static_cast<i32>(variant)});
}

void ChickenEntity::finalizeSpawn(
    IWorld& world, const entity::combat::DifficultyInstance& difficulty, world::spawn::SpawnReason spawnReason)
{
    // 对齐 vanilla Chicken.finalizeSpawn：先按生成位置选择变种，再执行通用初始化。
    setVariant(selectVariantForPosition(&world, onPos()));
    AnimalEntity::finalizeSpawn(world, difficulty, spawnReason);
}

ChickenVariant ChickenEntity::selectVariantForPosition(const IWorld* world, const BlockPos& pos)
{
    // 对齐 vanilla VariantUtils.selectVariantToSpawn(SpawnContext, Registries.CHICKEN_VARIANT)：
    // 按群系 spawn_conditions 选中变种。数据包 warm/cold 变种均为 priority 1 的群系条件，
    // temperate 为 priority 0 的兜底项。
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

    if (world::biome::BiomeTags::SPAWNS_WARM_VARIANT_FARM_ANIMALS().contains(biomeId)) {
        return ChickenVariant::Warm;
    }
    if (world::biome::BiomeTags::SPAWNS_COLD_VARIANT_FARM_ANIMALS().contains(biomeId)) {
        return ChickenVariant::Cold;
    }
    // 数据包未提供农畜变种标签时，按原版标签内容回退为群系直接判定（与青蛙变种选择同一范式）。
    if (_isWarmFarmAnimalBiome(biomeId)) {
        return ChickenVariant::Warm;
    }
    if (_isColdFarmAnimalBiome(biomeId)) {
        return ChickenVariant::Cold;
    }
    return DEFAULT_VARIANT;
}

ItemStack ChickenEntity::getEggItemStack() const
{
    const Item* eggItem = eggItemForVariant(getVariant());
    // 物品注册未完成时（单元测试/早期初始化）回退到白蛋，避免空堆静默丢蛋。
    if (eggItem == nullptr) {
        eggItem = Items::EGG;
    }
    return eggItem != nullptr ? ItemStack(*eggItem, 1) : ItemStack();
}

const Item* ChickenEntity::eggItemForVariant(ChickenVariant variant)
{
    // 对齐数据包 gameplay/chicken_lay：temperate → egg，warm → brown_egg，cold → blue_egg。
    switch (variant) {
        case ChickenVariant::Warm:
            return Items::BROWN_EGG;
        case ChickenVariant::Cold:
            return Items::BLUE_EGG;
        case ChickenVariant::Temperate:
        default:
            return Items::EGG;
    }
}

void ChickenEntity::resetEggTimer()
{
    // 下蛋时间：6000-12000 ticks = 5-10 分钟
    m_eggTimer = EGG_TIME_MIN + getRandom().nextInt(EGG_TIME_MAX - EGG_TIME_MIN);
}

void ChickenEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    AnimalEntity::addAdditionalSaveData(tag);
    tag.put(entity::serialization::nbt_keys::EGG_LAY_TIME, m_eggTimer);
    tag.put(entity::serialization::nbt_keys::IS_CHICKEN_JOCKEY, static_cast<i8>(m_chickenJockey));
    // 变种对齐 vanilla VariantUtils.writeVariant：存注册表 id（非资源位置）。
    tag.put(entity::serialization::nbt_keys::CHICKEN_VARIANT, static_cast<i32>(getVariant()));
}

Result<void> ChickenEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    using namespace entity::serialization;
    MC_TRY(AnimalEntity::readAdditionalSaveData(tag));
    m_chickenJockey = nbt_helper::tryGetBool(tag, nbt_keys::IS_CHICKEN_JOCKEY).value_or(false);
    if (const auto timer = nbt_helper::tryGetInt(tag, nbt_keys::EGG_LAY_TIME)) {
        m_eggTimer = *timer;
    }
    if (const auto variant = nbt_helper::tryGetInt(tag, nbt_keys::CHICKEN_VARIANT)) {
        // 越界/未知 id 回退默认变种（对齐 vanilla readVariant 解不出时保持原值）。
        if (*variant >= static_cast<i32>(ChickenVariant::Temperate) &&
            *variant <= static_cast<i32>(ChickenVariant::Cold)) {
            setVariant(static_cast<ChickenVariant>(*variant));
        }
    }
    return Result<void>::ok();
}

std::optional<ResourceLocation> ChickenEntity::getAmbientSound() const
{
    return makeSoundEventId("ambient");
}

std::optional<ResourceLocation> ChickenEntity::getHurtSound(DamageSource& /*source*/) const
{
    return makeSoundEventId("hurt");
}

std::optional<ResourceLocation> ChickenEntity::getDeathSound() const
{
    return makeSoundEventId("death");
}

std::optional<ResourceLocation> ChickenEntity::getStepSound() const
{
    return makeSoundEventId("step");
}

void ChickenEntity::playStepSound(const BlockPos& /*pos*/, const BlockState* /*blockState*/)
{
    // 鸡播放固定的脚步声，忽略脚下方块类型
    auto sound = getStepSound();
    if (sound) {
        playSound(*sound, 0.15f, 1.0f);
    }
}

bool ChickenEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // 鸡用种子繁殖
    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return false;
    }
    return item == Items::WHEAT_SEEDS || item == Items::PUMPKIN_SEEDS || item == Items::MELON_SEEDS ||
        item == Items::BEETROOT_SEEDS;
}

bool ChickenEntity::canMateWith(const AnimalEntity& other) const
{
    return AnimalEntity::canMateWith(other);
}

std::unique_ptr<AnimalEntity> ChickenEntity::spawnBaby(AnimalEntity& partner)
{
    // ECS 迁移：实体构造需要 registry 句柄，ClientWorld 返回 nullptr 表客户端不接入 ECS
    auto* registry = &ecsRegistry();
    if (registry == nullptr) {
        return nullptr;
    }

    // 创建小鸡
    auto baby = std::make_unique<ChickenEntity>(0, *registry);

    // 对齐 vanilla Chicken.getBreedOffspring：一半概率继承本鸡变种，另一半取配偶变种。
    const auto* partnerChicken = dynamic_cast<const ChickenEntity*>(&partner);
    const ChickenVariant partnerVariant = partnerChicken != nullptr ? partnerChicken->getVariant() : DEFAULT_VARIANT;
    baby->setVariant(getRandom().nextBoolean() ? getVariant() : partnerVariant);

    // 设置为幼体
    baby->setChild(true);

    // 设置位置（在父体位置附近）
    baby->setPosition(x(), y(), z());

    return baby;
}

void ChickenEntity::registerGoals()
{
    // 调用父类方法（AgeableEntity 会调用 AnimalEntity，现在 AnimalEntity 不注册任何目标）
    AgeableEntity::registerGoals();

    // 注意：AnimalEntity 基类不注册任何 goal，所以这里需要注册完整的 AI 目标列表

    // 优先级 0: 游泳（最高优先级）
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::SwimGoal>(this));

    // 优先级 1: 恐慌逃跑
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::PanicGoal>(this, 1.4));

    // 优先级 2: 繁殖
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::BreedGoal>(this, 1.0));

    // 优先级 3: 种子诱惑
    m_goalSelector.addGoal(3,
        std::make_unique<::mc::entity::ai::goal::TemptGoal>(
            this,
            1.0,
            [](const ItemStack& stack) -> bool {
                const Item* item = stack.getItem();
                return item != nullptr &&
                    (item == Items::WHEAT_SEEDS || item == Items::PUMPKIN_SEEDS || item == Items::MELON_SEEDS ||
                        item == Items::BEETROOT_SEEDS);
            },
            false)); // scaredByMovement = false

    // 优先级 4: 跟随父母
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::FollowParentGoal>(this, 1.1));

    // 优先级 5: 随机漫步
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::RandomWalkingGoal>(this, 1.0));

    // 优先级 6: 看向玩家
    m_goalSelector.addGoal(6, std::make_unique<entity::ai::goal::LookAtGoal>(this, 6.0f));

    // 优先级 7: 随机看向
    m_goalSelector.addGoal(7, std::make_unique<entity::ai::goal::LookRandomlyGoal>(this));
}

void ChickenEntity::registerAttributes()
{
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // 鸡的属性
    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 4.0);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
}

void ChickenEntity::tick()
{
    // 保存上一帧翅膀角度
    m_prevWingRotation = m_wingRotation;

    AnimalEntity::tick();

    // 翅膀动画
    constexpr f32 WING_FLAP_SPEED = 1.0f;
    constexpr f32 WING_DAMPING = 0.9f;
    m_wingRotDelta += WING_FLAP_SPEED;
    m_wingRotation += m_wingRotDelta;
    m_wingRotDelta *= WING_DAMPING;

    // 下蛋逻辑（仅服务端、仅成体、非鸡骑士）
    if (world() != nullptr && !world()->isClientSide() && !isChild() && !m_chickenJockey) {
        --m_eggTimer;

        if (m_eggTimer <= 0) {
            // ECS 迁移：ItemEntity 构造需要 registry 句柄
            auto* registry = &ecsRegistry();

            // 通过 EntityType 工厂创建 ItemEntity（而非直接 make_unique）：工厂在创建后会调
            // setTypeId("minecraft:item")，使 getTypeId()/entityType() 返回正确类型标识。
            // 直接 make_unique<ItemEntity> 绕过工厂会导致 m_typeId 为空，JS 侧按 typeId 过滤
            // 实体的断言（assertEntityInVolume "item"）与 C++ 侧 getTypeId()=="minecraft:item"
            // 判定全部失效（系统性问题：所有 make_unique 直接构造的实体均缺 typeId，此处先修鸡下蛋）。
            const entity::EntityType* itemType =
                entity::EntityRegistry::instance().getType(entity::EntityTypeKeys::ITEM);
            if (itemType == nullptr) {
                resetEggTimer();
                return;
            }
            auto egg = itemType->create(world(), *registry);
            if (egg == nullptr) {
                resetEggTimer();
                return;
            }
            // 工厂创建的 ItemEntity 在原点、持空 stack；补齐下蛋位置与物品。
            auto* eggEntity = dynamic_cast<ItemEntity*>(egg.get());
            if (eggEntity == nullptr) {
                resetEggTimer();
                return;
            }
            // 对齐数据包 gameplay/chicken_lay：变种决定产蛋种类。
            eggEntity->setItemStack(getEggItemStack());
            eggEntity->setPosition(x(), y() + 0.2f, z());

            // 播放下蛋音效
            playSound(
                *makeSoundEventId("egg"), 1.0f, (getRandom().nextFloat() - getRandom().nextFloat()) * 0.2f + 1.0f);

            world()->spawnEntity(std::move(egg));
            resetEggTimer();
        }
    }
}

} // namespace mc
