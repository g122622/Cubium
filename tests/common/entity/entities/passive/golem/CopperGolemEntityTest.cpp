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

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/passive/golem/CopperGolemEntity.hpp"
#include "common/entity/entities/passive/golem/CopperGolemTypes.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gameevent/GameEvents.hpp"

#include <memory>
#include <vector>

using namespace mc;
using namespace mc::entity;
using namespace mc::entity::serialization;
using mc::item::tag::ItemTags;

namespace {

// ============================================================================
// 测试用世界 - 支持可控游戏时间、playSound、gameEvent、spawnEntity
// ============================================================================

class CopperGolemTestWorld final : public mc::test::BaseTestWorld {
public:
    CopperGolemTestWorld() = default;

    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }
    [[nodiscard]] bool isClientSide() const override { return false; }

    void playSound(const ResourceLocation& sound,
        sound::SoundCategory category,
        const Vector3& pos,
        f32 volume,
        f32 pitch) override
    {
        m_sounds.push_back({sound, category, pos, volume, pitch});
    }

    void gameEvent(
        const gameevent::GameEvent& event, const BlockPos& pos, const gameevent::GameEvent::Context& context) override
    {
        m_gameEvents.push_back({&event, pos, context});
    }

    [[nodiscard]] EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size());
    }

    [[nodiscard]] const BlockState* getBlockState(i32, i32, i32) const override
    {
        return &VanillaBlocks::AIR->defaultState();
    }

    // 测试辅助
    void setTick(u64 tick) { m_currentTick = tick; }
    void advanceTick() { m_currentTick++; }

    struct SoundRecord {
        ResourceLocation sound;
        sound::SoundCategory category;
        Vector3 pos;
        f32 volume;
        f32 pitch;
    };

    struct GameEventRecord {
        const gameevent::GameEvent* event;
        BlockPos pos;
        gameevent::GameEvent::Context context;
    };

    [[nodiscard]] const std::vector<SoundRecord>& sounds() const { return m_sounds; }
    [[nodiscard]] const std::vector<std::unique_ptr<Entity>>& spawnedEntities() const { return m_spawnedEntities; }
    [[nodiscard]] const std::vector<GameEventRecord>& gameEvents() const { return m_gameEvents; }
    void clearSounds() { m_sounds.clear(); }

private:
    u64 m_currentTick = 0;
    std::vector<SoundRecord> m_sounds;
    std::vector<GameEventRecord> m_gameEvents;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
};

} // namespace

// ============================================================================
// 基础属性与状态测试
// ============================================================================

class CopperGolemEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        VanillaEntities::registerAll();
        golem_ = std::make_unique<CopperGolemEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    }

    std::unique_ptr<CopperGolemEntity> golem_;
};

TEST_F(CopperGolemEntityTest, Default_WeatherStateIsUnaffected)
{
    EXPECT_EQ(golem_->getWeatherState(), CopperGolemWeatherState::Unaffected);
}

TEST_F(CopperGolemEntityTest, Default_BehaviorStateIsIdle)
{
    EXPECT_EQ(golem_->getBehaviorState(), CopperGolemState::Idle);
}

TEST_F(CopperGolemEntityTest, SetWeatherState_StoresValue)
{
    golem_->setWeatherState(CopperGolemWeatherState::Exposed);
    EXPECT_EQ(golem_->getWeatherState(), CopperGolemWeatherState::Exposed);

    golem_->setWeatherState(CopperGolemWeatherState::Weathered);
    EXPECT_EQ(golem_->getWeatherState(), CopperGolemWeatherState::Weathered);

    golem_->setWeatherState(CopperGolemWeatherState::Oxidized);
    EXPECT_EQ(golem_->getWeatherState(), CopperGolemWeatherState::Oxidized);

    golem_->setWeatherState(CopperGolemWeatherState::Unaffected);
    EXPECT_EQ(golem_->getWeatherState(), CopperGolemWeatherState::Unaffected);
}

TEST_F(CopperGolemEntityTest, SetBehaviorState_StoresValue)
{
    golem_->setBehaviorState(CopperGolemState::GettingItem);
    EXPECT_EQ(golem_->getBehaviorState(), CopperGolemState::GettingItem);

    golem_->setBehaviorState(CopperGolemState::DroppingItem);
    EXPECT_EQ(golem_->getBehaviorState(), CopperGolemState::DroppingItem);

    golem_->setBehaviorState(CopperGolemState::Idle);
    EXPECT_EQ(golem_->getBehaviorState(), CopperGolemState::Idle);
}

// ============================================================================
// 尺寸测试
// ============================================================================

TEST_F(CopperGolemEntityTest, Dimensions_CorrectValues)
{
    // MC 1.21.11: 宽 0.49, 高 0.98
    EXPECT_FLOAT_EQ(golem_->width(), 0.49f);
    EXPECT_FLOAT_EQ(golem_->height(), 0.98f);
    EXPECT_FLOAT_EQ(golem_->eyeHeight(), 0.45f);
}

// ============================================================================
// 属性测试
// ============================================================================

TEST_F(CopperGolemEntityTest, Attributes_HasCorrectBaseValues)
{
    // MC 1.21.11: MAX_HEALTH = 12.0
    EXPECT_FLOAT_EQ(golem_->maxHealth(), 12.0f);

    // MC 1.21.11: MOVEMENT_SPEED = 0.2
    EXPECT_FLOAT_EQ(static_cast<f32>(golem_->getAttributeValue(attribute::Attributes::MOVEMENT_SPEED, 0.0)), 0.2f);
}

// ============================================================================
// 工厂方法测试
// ============================================================================

TEST_F(CopperGolemEntityTest, Create_ReturnsValidEntity)
{
    CopperGolemTestWorld world;
    auto entity = CopperGolemEntity::create(&world, mc::test::testEcsRegistry());
    ASSERT_NE(entity, nullptr);

    auto* golem = dynamic_cast<CopperGolemEntity*>(entity.get());
    EXPECT_NE(golem, nullptr);
}

// ============================================================================
// 继承测试
// ============================================================================

TEST_F(CopperGolemEntityTest, Inheritance_IsGolemEntity)
{
    GolemEntity* golemBase = dynamic_cast<GolemEntity*>(golem_.get());
    EXPECT_NE(golemBase, nullptr);
}

TEST_F(CopperGolemEntityTest, Inheritance_IsShearable)
{
    IShearable* shearable = dynamic_cast<IShearable*>(golem_.get());
    EXPECT_NE(shearable, nullptr);
}

// ============================================================================
// IShearable 接口测试
// ============================================================================

TEST_F(CopperGolemEntityTest, IsShearable_FalseWhenAntennaSlotEmpty)
{
    // 铜傀儡天线槽（Saddle）默认为空，不可剪切
    // 对应 MC: readyForShearing() 要求天线槽物品 ∈ SHEARABLE_FROM_COPPER_GOLEM
    EXPECT_FALSE(golem_->isShearable());
}

TEST_F(CopperGolemEntityTest, IsShearable_TrueWhenAntennaSlotHasPoppy)
{
    // 初始化顺序必须对齐生产(MinecraftServer::initializeRegistries / ClientApplicationBootstrap):
    // VanillaBlocks → Items → BlockItemRegistry → ItemTags。ItemTags::initialize 内部会从
    // ItemRegistry 取 poppy 指针快照加入 SHEARABLE_FROM_COPPER_GOLEM 标签(ItemTags.cpp:1016),
    // 若早于 Items::initialize 调用,getItem 返回 nullptr,add 静默丢弃(ItemTag.cpp:41),
    // 标签永久为空且 s_initialized 幂等锁死,isShearable 必返回 false。原顺序写反致本用例失败。
    VanillaBlocks::initialize();
    Items::initialize();
    BlockItemRegistry::instance().initializeVanillaBlockItems();
    ItemTags::initialize();

    ASSERT_NE(Items::POPPY, nullptr);
    golem_->setEquipment(CopperGolemEntity::EQUIPMENT_SLOT_ANTENNA, ItemStack(Items::POPPY, 1));
    EXPECT_TRUE(golem_->isShearable());
}

TEST_F(CopperGolemEntityTest, Shear_PlaysShearSoundAndDropsAntenna)
{
    // 同 IsShearable_TrueWhenAntennaSlotHasPoppy:ItemTags 必须在 Items 之后初始化。
    VanillaBlocks::initialize();
    Items::initialize();
    BlockItemRegistry::instance().initializeVanillaBlockItems();
    ItemTags::initialize();

    CopperGolemTestWorld world;
    golem_->setWorld(&world);
    golem_->setPosition(0.0f, 64.0f, 0.0f);

    // 装备罂粟花到天线槽
    ASSERT_NE(Items::POPPY, nullptr);
    golem_->setEquipment(CopperGolemEntity::EQUIPMENT_SLOT_ANTENNA, ItemStack(Items::POPPY, 1));
    ASSERT_TRUE(golem_->isShearable());

    world.clearSounds();
    auto drops = golem_->shear(nullptr);

    // 应播放剪切音效
    EXPECT_FALSE(world.sounds().empty());
    bool foundShearSound = false;
    for (const auto& s : world.sounds()) {
        if (s.sound == SoundEvents::ENTITY_COPPER_GOLEM_SHEAR) {
            foundShearSound = true;
            break;
        }
    }
    EXPECT_TRUE(foundShearSound);

    // 应掉落 1 个罂粟花（天线槽物品）
    ASSERT_EQ(drops.size(), 1u);
    EXPECT_EQ(drops[0].getItem(), Items::POPPY);
    EXPECT_EQ(drops[0].getCount(), 1);

    // 剪后天线槽应清空，isShearable 应为 false
    EXPECT_TRUE(golem_->getEquipment(CopperGolemEntity::EQUIPMENT_SLOT_ANTENNA).isEmpty());
    EXPECT_FALSE(golem_->isShearable());
}

TEST_F(CopperGolemEntityTest, Shear_NoDropWhenAntennaSlotEmpty)
{
    CopperGolemTestWorld world;
    golem_->setWorld(&world);
    golem_->setPosition(0.0f, 64.0f, 0.0f);

    // 天线槽为空时直接调用 shear（绕过 isShearable 检查）
    world.clearSounds();
    auto drops = golem_->shear(nullptr);

    // 仍应播放剪切音效
    bool foundShearSound = false;
    for (const auto& s : world.sounds()) {
        if (s.sound == SoundEvents::ENTITY_COPPER_GOLEM_SHEAR) {
            foundShearSound = true;
            break;
        }
    }
    EXPECT_TRUE(foundShearSound);

    // 天线槽为空，掉落应为空
    EXPECT_TRUE(drops.empty());
}

// ============================================================================
// 声音测试
// ============================================================================

TEST_F(CopperGolemEntityTest, GetHurtSound_UnaffectedReturnsBaseSound)
{
    golem_->setWeatherState(CopperGolemWeatherState::Unaffected);
    auto source = DamageSources::fall();
    auto sound = golem_->getHurtSound(source);
    EXPECT_TRUE(sound.has_value());
    EXPECT_EQ(*sound, SoundEvents::ENTITY_COPPER_GOLEM_HURT);
}

TEST_F(CopperGolemEntityTest, GetHurtSound_ExposedReturnsBaseSound)
{
    golem_->setWeatherState(CopperGolemWeatherState::Exposed);
    auto source = DamageSources::fall();
    auto sound = golem_->getHurtSound(source);
    EXPECT_TRUE(sound.has_value());
    EXPECT_EQ(*sound, SoundEvents::ENTITY_COPPER_GOLEM_HURT);
}

TEST_F(CopperGolemEntityTest, GetHurtSound_WeatheredReturnsWeatheredSound)
{
    golem_->setWeatherState(CopperGolemWeatherState::Weathered);
    auto source = DamageSources::fall();
    auto sound = golem_->getHurtSound(source);
    EXPECT_TRUE(sound.has_value());
    EXPECT_EQ(*sound, SoundEvents::ENTITY_COPPER_GOLEM_WEATHERED_HURT);
}

TEST_F(CopperGolemEntityTest, GetHurtSound_OxidizedReturnsOxidizedSound)
{
    golem_->setWeatherState(CopperGolemWeatherState::Oxidized);
    auto source = DamageSources::fall();
    auto sound = golem_->getHurtSound(source);
    EXPECT_TRUE(sound.has_value());
    EXPECT_EQ(*sound, SoundEvents::ENTITY_COPPER_GOLEM_OXIDIZED_HURT);
}

TEST_F(CopperGolemEntityTest, GetDeathSound_UnaffectedReturnsBaseSound)
{
    golem_->setWeatherState(CopperGolemWeatherState::Unaffected);
    auto sound = golem_->getDeathSound();
    EXPECT_TRUE(sound.has_value());
    EXPECT_EQ(*sound, SoundEvents::ENTITY_COPPER_GOLEM_DEATH);
}

TEST_F(CopperGolemEntityTest, GetDeathSound_WeatheredReturnsWeatheredSound)
{
    golem_->setWeatherState(CopperGolemWeatherState::Weathered);
    auto sound = golem_->getDeathSound();
    EXPECT_TRUE(sound.has_value());
    EXPECT_EQ(*sound, SoundEvents::ENTITY_COPPER_GOLEM_WEATHERED_DEATH);
}

TEST_F(CopperGolemEntityTest, GetDeathSound_OxidizedReturnsOxidizedSound)
{
    golem_->setWeatherState(CopperGolemWeatherState::Oxidized);
    auto sound = golem_->getDeathSound();
    EXPECT_TRUE(sound.has_value());
    EXPECT_EQ(*sound, SoundEvents::ENTITY_COPPER_GOLEM_OXIDIZED_DEATH);
}

// ============================================================================
// spawnFromStatue 测试
// ============================================================================

TEST_F(CopperGolemEntityTest, SpawnFromStatue_SetsUnaffectedAndPlaysSound)
{
    CopperGolemTestWorld world;
    golem_->setWorld(&world);
    golem_->setWeatherState(CopperGolemWeatherState::Oxidized);

    world.clearSounds();
    golem_->spawnFromStatue(CopperGolemWeatherState::Unaffected);

    // 应设置为 Unaffected
    EXPECT_EQ(golem_->getWeatherState(), CopperGolemWeatherState::Unaffected);

    // 应播放生成音效
    EXPECT_FALSE(world.sounds().empty());
    bool foundSpawnSound = false;
    for (const auto& s : world.sounds()) {
        if (s.sound == SoundEvents::ENTITY_COPPER_GOLEM_SPAWN) {
            foundSpawnSound = true;
            break;
        }
    }
    EXPECT_TRUE(foundSpawnSound);
}

TEST_F(CopperGolemEntityTest, PlaySpawnSound_PlaysCorrectSound)
{
    CopperGolemTestWorld world;
    golem_->setWorld(&world);

    world.clearSounds();
    golem_->playSpawnSound();

    EXPECT_FALSE(world.sounds().empty());
    EXPECT_EQ(world.sounds()[0].sound, SoundEvents::ENTITY_COPPER_GOLEM_SPAWN);
}

// ============================================================================
// NBT 序列化测试
// ============================================================================
//
// 对应 MC 1.21.11: CopperGolem.addAdditionalSaveData / readAdditionalSaveData
// 持久化字段：next_weather_age (i64)、weather_state (string)

TEST_F(CopperGolemEntityTest, NBT_SaveWritesWeatherState)
{
    golem_->setWeatherState(CopperGolemWeatherState::Weathered);

    nbt::tags::compound_tag tag;
    golem_->addAdditionalSaveData(tag);

    auto val = nbt_helper::tryGetString(tag, nbt_keys::WEATHER_STATE);
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, "weathered");
}

TEST_F(CopperGolemEntityTest, NBT_SaveWritesNextWeatherAge)
{
    // 默认 m_nextWeatheringTick = -1（UNSET_WEATHERING_TICK）
    nbt::tags::compound_tag tag;
    golem_->addAdditionalSaveData(tag);

    auto val = nbt_helper::tryGetLong(tag, nbt_keys::NEXT_WEATHER_AGE);
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, -1);
}

TEST_F(CopperGolemEntityTest, NBT_ReadWeatherState)
{
    nbt::tags::compound_tag tag;
    tag.put(nbt_keys::WEATHER_STATE, std::string("oxidized"));

    auto result = golem_->readAdditionalSaveData(tag);
    EXPECT_TRUE(static_cast<bool>(result));

    EXPECT_EQ(golem_->getWeatherState(), CopperGolemWeatherState::Oxidized);
}

TEST_F(CopperGolemEntityTest, NBT_ReadNextWeatherAge)
{
    nbt::tags::compound_tag tag;
    tag.put(nbt_keys::NEXT_WEATHER_AGE, static_cast<i64>(1000000));

    auto result = golem_->readAdditionalSaveData(tag);
    EXPECT_TRUE(static_cast<bool>(result));

    // 重新保存应能读回该值
    nbt::tags::compound_tag tag2;
    golem_->addAdditionalSaveData(tag2);
    auto val = nbt_helper::tryGetLong(tag2, nbt_keys::NEXT_WEATHER_AGE);
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, 1000000);
}

TEST_F(CopperGolemEntityTest, NBT_MissingWeatherState_DefaultsToUnaffected)
{
    nbt::tags::compound_tag tag;

    auto result = golem_->readAdditionalSaveData(tag);
    EXPECT_TRUE(static_cast<bool>(result));

    EXPECT_EQ(golem_->getWeatherState(), CopperGolemWeatherState::Unaffected);
}

TEST_F(CopperGolemEntityTest, NBT_MissingNextWeatherAge_DefaultsToUnset)
{
    nbt::tags::compound_tag tag;

    auto result = golem_->readAdditionalSaveData(tag);
    EXPECT_TRUE(static_cast<bool>(result));

    // 重新保存应能读回 -1（UNSET_WEATHERING_TICK）
    nbt::tags::compound_tag tag2;
    golem_->addAdditionalSaveData(tag2);
    auto val = nbt_helper::tryGetLong(tag2, nbt_keys::NEXT_WEATHER_AGE);
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, -1);
}

TEST_F(CopperGolemEntityTest, NBT_RoundTrip_PreservesWeatherState)
{
    golem_->setWeatherState(CopperGolemWeatherState::Oxidized);

    nbt::tags::compound_tag saveTag;
    golem_->addAdditionalSaveData(saveTag);

    auto golem2 = std::make_unique<CopperGolemEntity>(EntityInstanceId(2), mc::test::testEcsRegistry());
    auto result = golem2->readAdditionalSaveData(saveTag);
    EXPECT_TRUE(static_cast<bool>(result));

    EXPECT_EQ(golem2->getWeatherState(), CopperGolemWeatherState::Oxidized);
}

TEST_F(CopperGolemEntityTest, NBT_ReadResetsBehaviorStateToIdle)
{
    // behaviorState 不持久化，加载后应重置为 Idle
    golem_->setBehaviorState(CopperGolemState::GettingItem);

    nbt::tags::compound_tag saveTag;
    golem_->addAdditionalSaveData(saveTag);

    auto golem2 = std::make_unique<CopperGolemEntity>(EntityInstanceId(2), mc::test::testEcsRegistry());
    golem2->setBehaviorState(CopperGolemState::DroppingItem);
    auto result = golem2->readAdditionalSaveData(saveTag);
    EXPECT_TRUE(static_cast<bool>(result));

    EXPECT_EQ(golem2->getBehaviorState(), CopperGolemState::Idle);
}

TEST_F(CopperGolemEntityTest, NBT_AllWeatherStateStrings_RoundTrip)
{
    const struct {
        CopperGolemWeatherState state;
        const char* str;
    } cases[] = {
        {CopperGolemWeatherState::Unaffected, "unaffected"},
        {CopperGolemWeatherState::Exposed, "exposed"},
        {CopperGolemWeatherState::Weathered, "weathered"},
        {CopperGolemWeatherState::Oxidized, "oxidized"},
    };

    for (const auto& c : cases) {
        auto golem = std::make_unique<CopperGolemEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
        golem->setWeatherState(c.state);

        nbt::tags::compound_tag tag;
        golem->addAdditionalSaveData(tag);

        auto strVal = nbt_helper::tryGetString(tag, nbt_keys::WEATHER_STATE);
        ASSERT_TRUE(strVal.has_value());
        EXPECT_EQ(*strVal, c.str) << "Failed for weather state " << static_cast<i32>(c.state);

        auto golem2 = std::make_unique<CopperGolemEntity>(EntityInstanceId(2), mc::test::testEcsRegistry());
        auto result = golem2->readAdditionalSaveData(tag);
        EXPECT_TRUE(static_cast<bool>(result));
        EXPECT_EQ(golem2->getWeatherState(), c.state);
    }
}

// ============================================================================
// 氧化工具类测试（CopperGolemOxidationUtils）
// ============================================================================

TEST(CopperGolemOxidationUtilsTest, Next_UnaffectedReturnsExposed)
{
    EXPECT_EQ(CopperGolemOxidationUtils::next(CopperGolemWeatherState::Unaffected), CopperGolemWeatherState::Exposed);
}

TEST(CopperGolemOxidationUtilsTest, Next_ExposedReturnsWeathered)
{
    EXPECT_EQ(CopperGolemOxidationUtils::next(CopperGolemWeatherState::Exposed), CopperGolemWeatherState::Weathered);
}

TEST(CopperGolemOxidationUtilsTest, Next_WeatheredReturnsOxidized)
{
    EXPECT_EQ(CopperGolemOxidationUtils::next(CopperGolemWeatherState::Weathered), CopperGolemWeatherState::Oxidized);
}

TEST(CopperGolemOxidationUtilsTest, Next_OxidizedReturnsNullopt)
{
    EXPECT_FALSE(CopperGolemOxidationUtils::next(CopperGolemWeatherState::Oxidized).has_value());
}

TEST(CopperGolemOxidationUtilsTest, Previous_UnaffectedReturnsNullopt)
{
    EXPECT_FALSE(CopperGolemOxidationUtils::previous(CopperGolemWeatherState::Unaffected).has_value());
}

TEST(CopperGolemOxidationUtilsTest, Previous_OxidizedReturnsWeathered)
{
    EXPECT_EQ(
        CopperGolemOxidationUtils::previous(CopperGolemWeatherState::Oxidized), CopperGolemWeatherState::Weathered);
}

TEST(CopperGolemOxidationUtilsTest, ToString_AllStatesCorrect)
{
    EXPECT_EQ(CopperGolemOxidationUtils::toString(CopperGolemWeatherState::Unaffected), "unaffected");
    EXPECT_EQ(CopperGolemOxidationUtils::toString(CopperGolemWeatherState::Exposed), "exposed");
    EXPECT_EQ(CopperGolemOxidationUtils::toString(CopperGolemWeatherState::Weathered), "weathered");
    EXPECT_EQ(CopperGolemOxidationUtils::toString(CopperGolemWeatherState::Oxidized), "oxidized");
}

TEST(CopperGolemOxidationUtilsTest, FromString_AllStatesCorrect)
{
    EXPECT_EQ(CopperGolemOxidationUtils::fromString("unaffected"), CopperGolemWeatherState::Unaffected);
    EXPECT_EQ(CopperGolemOxidationUtils::fromString("exposed"), CopperGolemWeatherState::Exposed);
    EXPECT_EQ(CopperGolemOxidationUtils::fromString("weathered"), CopperGolemWeatherState::Weathered);
    EXPECT_EQ(CopperGolemOxidationUtils::fromString("oxidized"), CopperGolemWeatherState::Oxidized);
}

TEST(CopperGolemOxidationUtilsTest, FromString_UnknownStringReturnsUnaffected)
{
    EXPECT_EQ(CopperGolemOxidationUtils::fromString("unknown"), CopperGolemWeatherState::Unaffected);
    EXPECT_EQ(CopperGolemOxidationUtils::fromString(""), CopperGolemWeatherState::Unaffected);
}
