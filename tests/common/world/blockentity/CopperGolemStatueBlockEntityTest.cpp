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
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/passive/golem/CopperGolemEntity.hpp"
#include "common/entity/entities/passive/golem/CopperGolemTypes.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/CopperGolemStatueBlockEntity.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::blockentity;

namespace {

// ============================================================================
// 测试用世界 - 支持 spawnEntity / playSound / setBlockState / getBlockEntity
// ============================================================================

class CopperGolemStatueEntityTestWorld final : public test::BaseTestWorld {
public:
    CopperGolemStatueEntityTestWorld() { m_tickManagerPtr = std::make_unique<world::tick::TickManager>(*this); }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (state == nullptr) {
            m_blocks.erase(BlockPos(x, y, z));
        } else {
            m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        }
        return true;
    }

    bool setBlockState(const BlockPos& pos, const BlockState* state)
    {
        return setBlockState(pos.x, pos.y, pos.z, state);
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    [[nodiscard]] const BlockState* getBlockState(const BlockPos& pos) const
    {
        return getBlockState(pos.x, pos.y, pos.z);
    }

    [[nodiscard]] BlockEntity* getBlockEntity(const BlockPos& pos) override
    {
        const auto it = m_blockEntities.find(pos);
        return it != m_blockEntities.end() ? it->second.get() : nullptr;
    }

    [[nodiscard]] const BlockEntity* getBlockEntity(const BlockPos& pos) const override
    {
        const auto it = m_blockEntities.find(pos);
        return it != m_blockEntities.end() ? it->second.get() : nullptr;
    }

    void setBlockEntity(const BlockPos& pos, BlockEntity* entity) override
    {
        if (entity != nullptr) {
            entity->setWorld(this);
            m_blockEntities[pos] = std::unique_ptr<BlockEntity>(entity);
        } else {
            m_blockEntities.erase(pos);
        }
    }

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

    [[nodiscard]] EntityId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityId>(m_spawnedEntities.size());
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override { return *m_tickManagerPtr; }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override { return *m_tickManagerPtr; }

    [[nodiscard]] u64 seed() const override { return m_seed; }
    [[nodiscard]] bool isRaining() const override { return false; }
    [[nodiscard]] bool canRainAt(const BlockPos&) const override { return false; }

    // ========== 测试辅助 ==========

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
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::unordered_map<BlockPos, std::unique_ptr<BlockEntity>> m_blockEntities;
    std::vector<SoundRecord> m_sounds;
    std::vector<GameEventRecord> m_gameEvents;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    u64 m_seed = 0;
};

} // namespace

// ============================================================================
// 基础属性测试
// ============================================================================

class CopperGolemStatueBlockEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        // 注册实体类型，使 removeStatue 能通过 EntityRegistry 查找铜傀儡工厂
        entity::VanillaEntities::registerAll();
        be_ = std::make_unique<CopperGolemStatueBlockEntity>(BlockPos(10, 64, 20));
    }

    std::unique_ptr<CopperGolemStatueBlockEntity> be_;
};

TEST_F(CopperGolemStatueBlockEntityTest, Create_HasCorrectType)
{
    EXPECT_EQ(be_->getType(), BlockEntityType::CopperGolemStatue);
}

TEST_F(CopperGolemStatueBlockEntityTest, Create_HasCorrectPosition)
{
    EXPECT_EQ(be_->getPos(), BlockPos(10, 64, 20));
}

TEST_F(CopperGolemStatueBlockEntityTest, Create_EmptyCustomNameByDefault)
{
    EXPECT_TRUE(be_->getCustomName().empty());
}

TEST_F(CopperGolemStatueBlockEntityTest, SetCustomName_StoresAndReturnsName)
{
    be_->setCustomName("MyGolem");
    EXPECT_EQ(be_->getCustomName(), "MyGolem");
}

TEST_F(CopperGolemStatueBlockEntityTest, SetCustomName_OverwritesExistingName)
{
    be_->setCustomName("First");
    be_->setCustomName("Second");
    EXPECT_EQ(be_->getCustomName(), "Second");
}

TEST_F(CopperGolemStatueBlockEntityTest, SetCustomName_EmptyStringClearsName)
{
    be_->setCustomName("Hello");
    be_->setCustomName("");
    EXPECT_TRUE(be_->getCustomName().empty());
}

// ============================================================================
// 克隆测试
// ============================================================================

TEST_F(CopperGolemStatueBlockEntityTest, Clone_PreservesCustomName)
{
    be_->setCustomName("CloneMe");

    auto clone = be_->clone();
    ASSERT_NE(clone, nullptr);

    auto* statueClone = dynamic_cast<CopperGolemStatueBlockEntity*>(clone.get());
    ASSERT_NE(statueClone, nullptr);
    EXPECT_EQ(statueClone->getCustomName(), "CloneMe");
    EXPECT_EQ(statueClone->getPos(), be_->getPos());
    EXPECT_EQ(statueClone->getType(), be_->getType());
}

TEST_F(CopperGolemStatueBlockEntityTest, Clone_EmptyCustomNameClonesCorrectly)
{
    auto clone = be_->clone();
    ASSERT_NE(clone, nullptr);

    auto* statueClone = dynamic_cast<CopperGolemStatueBlockEntity*>(clone.get());
    ASSERT_NE(statueClone, nullptr);
    EXPECT_TRUE(statueClone->getCustomName().empty());
}

// ============================================================================
// JSON 序列化测试
// ============================================================================

TEST_F(CopperGolemStatueBlockEntityTest, SaveLoadJson_PreservesCustomName)
{
    be_->setCustomName("JsonGolem");

    nlohmann::json data;
    be_->save(data);

    CopperGolemStatueBlockEntity loaded(BlockPos(10, 64, 20));
    ASSERT_TRUE(loaded.load(data));

    EXPECT_EQ(loaded.getCustomName(), "JsonGolem");
}

TEST_F(CopperGolemStatueBlockEntityTest, SaveJson_OmitsEmptyCustomName)
{
    // 空名称不应写入 JSON（与 MC 默认行为一致）
    nlohmann::json data;
    be_->save(data);

    EXPECT_FALSE(data.contains("custom_name"));
}

TEST_F(CopperGolemStatueBlockEntityTest, SaveJson_WritesCustomNameWhenSet)
{
    be_->setCustomName("Persist");

    nlohmann::json data;
    be_->save(data);

    ASSERT_TRUE(data.contains("custom_name"));
    EXPECT_EQ(data["custom_name"], "Persist");
}

TEST_F(CopperGolemStatueBlockEntityTest, LoadJson_MissingCustomNameLeavesEmpty)
{
    nlohmann::json data = nlohmann::json::object();

    CopperGolemStatueBlockEntity loaded(BlockPos(10, 64, 20));
    ASSERT_TRUE(loaded.load(data));

    EXPECT_TRUE(loaded.getCustomName().empty());
}

// ============================================================================
// NBT 序列化测试
// ============================================================================

TEST_F(CopperGolemStatueBlockEntityTest, SaveToNBT_WritesCustomName)
{
    be_->setCustomName("NbtGolem");

    nbt::tags::compound_tag tag;
    be_->saveToNBT(tag);

    const auto it = tag.value.find("CustomName");
    ASSERT_NE(it, tag.value.end());
    const auto* strTag = dynamic_cast<const nbt::tags::string_tag*>(it->second.get());
    ASSERT_NE(strTag, nullptr);
    EXPECT_EQ(strTag->value, "NbtGolem");
}

TEST_F(CopperGolemStatueBlockEntityTest, SaveToNBT_OmitsEmptyCustomName)
{
    nbt::tags::compound_tag tag;
    be_->saveToNBT(tag);

    EXPECT_EQ(tag.value.find("CustomName"), tag.value.end());
}

TEST_F(CopperGolemStatueBlockEntityTest, LoadFromNBT_ReadsCustomName)
{
    nbt::tags::compound_tag tag;
    tag.put("CustomName", std::string("LoadedGolem"));

    CopperGolemStatueBlockEntity loaded(BlockPos(10, 64, 20));
    ASSERT_TRUE(loaded.loadFromNBT(tag));

    EXPECT_EQ(loaded.getCustomName(), "LoadedGolem");
}

TEST_F(CopperGolemStatueBlockEntityTest, LoadFromNBT_MissingCustomNameLeavesEmpty)
{
    nbt::tags::compound_tag tag;

    CopperGolemStatueBlockEntity loaded(BlockPos(10, 64, 20));
    ASSERT_TRUE(loaded.loadFromNBT(tag));

    EXPECT_TRUE(loaded.getCustomName().empty());
}

TEST_F(CopperGolemStatueBlockEntityTest, NBT_RoundTrip_PreservesCustomName)
{
    be_->setCustomName("RoundTrip");

    nbt::tags::compound_tag saveTag;
    be_->saveToNBT(saveTag);

    CopperGolemStatueBlockEntity loaded(BlockPos(10, 64, 20));
    ASSERT_TRUE(loaded.loadFromNBT(saveTag));

    EXPECT_EQ(loaded.getCustomName(), "RoundTrip");
}

// ============================================================================
// removeStatue 测试
// ============================================================================
//
// 对应 MC 1.21.11: CopperGolemStatueBlockEntity.removeStatue(BlockState)

class RemoveStatueTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        entity::VanillaEntities::registerAll();
    }
};

TEST_F(RemoveStatueTest, RemoveStatue_CreatesCopperGolemEntity)
{
    CopperGolemStatueEntityTestWorld world;
    const BlockPos pos(10, 64, 20);

    auto be = std::make_unique<CopperGolemStatueBlockEntity>(pos);
    CopperGolemStatueBlockEntity* bePtr = be.get();
    world.setBlockEntity(pos, be.release());
    bePtr->setWorld(&world);

    const BlockState state = VanillaBlocks::COPPER_GOLEM_STATUE->defaultState().with(
        BlockStateProperties::HORIZONTAL_FACING(), Direction::South);

    auto entity = bePtr->removeStatue(state);
    ASSERT_NE(entity, nullptr);

    auto* golem = dynamic_cast<CopperGolemEntity*>(entity.get());
    EXPECT_NE(golem, nullptr);
}

TEST_F(RemoveStatueTest, RemoveStatue_TransfersCustomName)
{
    CopperGolemStatueEntityTestWorld world;
    const BlockPos pos(11, 64, 20);

    auto be = std::make_unique<CopperGolemStatueBlockEntity>(pos);
    be->setCustomName("NamedGolem");
    CopperGolemStatueBlockEntity* bePtr = be.get();
    world.setBlockEntity(pos, be.release());
    bePtr->setWorld(&world);

    const BlockState state = VanillaBlocks::COPPER_GOLEM_STATUE->defaultState().with(
        BlockStateProperties::HORIZONTAL_FACING(), Direction::South);

    auto entity = bePtr->removeStatue(state);
    ASSERT_NE(entity, nullptr);

    auto* golem = dynamic_cast<CopperGolemEntity*>(entity.get());
    ASSERT_NE(golem, nullptr);
    EXPECT_EQ(golem->customNameText(), "NamedGolem");
}

TEST_F(RemoveStatueTest, RemoveStatue_NoCustomNameLeavesEmpty)
{
    CopperGolemStatueEntityTestWorld world;
    const BlockPos pos(12, 64, 20);

    auto be = std::make_unique<CopperGolemStatueBlockEntity>(pos);
    CopperGolemStatueBlockEntity* bePtr = be.get();
    world.setBlockEntity(pos, be.release());
    bePtr->setWorld(&world);

    const BlockState state = VanillaBlocks::COPPER_GOLEM_STATUE->defaultState().with(
        BlockStateProperties::HORIZONTAL_FACING(), Direction::South);

    auto entity = bePtr->removeStatue(state);
    ASSERT_NE(entity, nullptr);

    auto* golem = dynamic_cast<CopperGolemEntity*>(entity.get());
    ASSERT_NE(golem, nullptr);
    EXPECT_FALSE(golem->hasCustomName());
}

TEST_F(RemoveStatueTest, RemoveStatue_PositionAtBlockCenter)
{
    // MC: snapTo(blockpos.getCenter().x, blockpos.getY(), blockpos.getCenter().z, ...)
    // 即 x = pos.x + 0.5, y = pos.y, z = pos.z + 0.5
    CopperGolemStatueEntityTestWorld world;
    const BlockPos pos(13, 65, 21);

    auto be = std::make_unique<CopperGolemStatueBlockEntity>(pos);
    CopperGolemStatueBlockEntity* bePtr = be.get();
    world.setBlockEntity(pos, be.release());
    bePtr->setWorld(&world);

    const BlockState state = VanillaBlocks::COPPER_GOLEM_STATUE->defaultState().with(
        BlockStateProperties::HORIZONTAL_FACING(), Direction::South);

    auto entity = bePtr->removeStatue(state);
    ASSERT_NE(entity, nullptr);

    // x/z 应为方块中心，y 应为方块位置（不加 0.5）
    EXPECT_FLOAT_EQ(entity->x(), static_cast<f32>(pos.x) + 0.5f);
    EXPECT_FLOAT_EQ(entity->y(), static_cast<f32>(pos.y));
    EXPECT_FLOAT_EQ(entity->z(), static_cast<f32>(pos.z) + 0.5f);
}

TEST_F(RemoveStatueTest, RemoveStatue_FacingSouth_YawZero)
{
    CopperGolemStatueEntityTestWorld world;
    const BlockPos pos(14, 64, 22);

    auto be = std::make_unique<CopperGolemStatueBlockEntity>(pos);
    CopperGolemStatueBlockEntity* bePtr = be.get();
    world.setBlockEntity(pos, be.release());
    bePtr->setWorld(&world);

    const BlockState state = VanillaBlocks::COPPER_GOLEM_STATUE->defaultState().with(
        BlockStateProperties::HORIZONTAL_FACING(), Direction::South);

    auto entity = bePtr->removeStatue(state);
    ASSERT_NE(entity, nullptr);

    EXPECT_FLOAT_EQ(entity->yaw(), 0.0f);
}

TEST_F(RemoveStatueTest, RemoveStatue_FacingWest_Yaw90)
{
    CopperGolemStatueEntityTestWorld world;
    const BlockPos pos(15, 64, 23);

    auto be = std::make_unique<CopperGolemStatueBlockEntity>(pos);
    CopperGolemStatueBlockEntity* bePtr = be.get();
    world.setBlockEntity(pos, be.release());
    bePtr->setWorld(&world);

    const BlockState state = VanillaBlocks::COPPER_GOLEM_STATUE->defaultState().with(
        BlockStateProperties::HORIZONTAL_FACING(), Direction::West);

    auto entity = bePtr->removeStatue(state);
    ASSERT_NE(entity, nullptr);

    EXPECT_FLOAT_EQ(entity->yaw(), 90.0f);
}

TEST_F(RemoveStatueTest, RemoveStatue_FacingNorth_Yaw180)
{
    CopperGolemStatueEntityTestWorld world;
    const BlockPos pos(16, 64, 24);

    auto be = std::make_unique<CopperGolemStatueBlockEntity>(pos);
    CopperGolemStatueBlockEntity* bePtr = be.get();
    world.setBlockEntity(pos, be.release());
    bePtr->setWorld(&world);

    const BlockState state = VanillaBlocks::COPPER_GOLEM_STATUE->defaultState().with(
        BlockStateProperties::HORIZONTAL_FACING(), Direction::North);

    auto entity = bePtr->removeStatue(state);
    ASSERT_NE(entity, nullptr);

    EXPECT_FLOAT_EQ(entity->yaw(), 180.0f);
}

TEST_F(RemoveStatueTest, RemoveStatue_FacingEast_Yaw270)
{
    CopperGolemStatueEntityTestWorld world;
    const BlockPos pos(17, 64, 25);

    auto be = std::make_unique<CopperGolemStatueBlockEntity>(pos);
    CopperGolemStatueBlockEntity* bePtr = be.get();
    world.setBlockEntity(pos, be.release());
    bePtr->setWorld(&world);

    const BlockState state = VanillaBlocks::COPPER_GOLEM_STATUE->defaultState().with(
        BlockStateProperties::HORIZONTAL_FACING(), Direction::East);

    auto entity = bePtr->removeStatue(state);
    ASSERT_NE(entity, nullptr);

    EXPECT_FLOAT_EQ(entity->yaw(), 270.0f);
}

TEST_F(RemoveStatueTest, RemoveStatue_GolemStartsAtUnaffectedWeatherState)
{
    // 对应 MC: spawnFromStatue(WeatherState.UNAFFECTED)
    CopperGolemStatueEntityTestWorld world;
    const BlockPos pos(18, 64, 26);

    auto be = std::make_unique<CopperGolemStatueBlockEntity>(pos);
    CopperGolemStatueBlockEntity* bePtr = be.get();
    world.setBlockEntity(pos, be.release());
    bePtr->setWorld(&world);

    const BlockState state = VanillaBlocks::COPPER_GOLEM_STATUE->defaultState().with(
        BlockStateProperties::HORIZONTAL_FACING(), Direction::South);

    auto entity = bePtr->removeStatue(state);
    ASSERT_NE(entity, nullptr);

    auto* golem = dynamic_cast<CopperGolemEntity*>(entity.get());
    ASSERT_NE(golem, nullptr);
    EXPECT_EQ(golem->getWeatherState(), entity::CopperGolemWeatherState::Unaffected);
}

TEST_F(RemoveStatueTest, RemoveStatue_PlaysSpawnSound)
{
    // 对应 MC: coppergolem.playSpawnSound()
    CopperGolemStatueEntityTestWorld world;
    const BlockPos pos(19, 64, 27);

    auto be = std::make_unique<CopperGolemStatueBlockEntity>(pos);
    CopperGolemStatueBlockEntity* bePtr = be.get();
    world.setBlockEntity(pos, be.release());
    bePtr->setWorld(&world);

    const BlockState state = VanillaBlocks::COPPER_GOLEM_STATUE->defaultState().with(
        BlockStateProperties::HORIZONTAL_FACING(), Direction::South);

    world.clearSounds();
    auto entity = bePtr->removeStatue(state);
    ASSERT_NE(entity, nullptr);

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

TEST_F(RemoveStatueTest, RemoveStatue_WithoutWorldReturnsNull)
{
    // 边界场景：方块实体未关联世界时应返回 nullptr
    CopperGolemStatueBlockEntity be(BlockPos(20, 64, 28));
    // 注意：未调用 setWorld()

    const BlockState state = VanillaBlocks::COPPER_GOLEM_STATUE->defaultState().with(
        BlockStateProperties::HORIZONTAL_FACING(), Direction::South);

    auto entity = be.removeStatue(state);
    EXPECT_EQ(entity, nullptr);
}

TEST_F(RemoveStatueTest, RemoveStatue_PitchIsZero)
{
    // 验证 pitch = 0.0f（MC: snapTo(..., 0.0F)）
    CopperGolemStatueEntityTestWorld world;
    const BlockPos pos(21, 64, 29);

    auto be = std::make_unique<CopperGolemStatueBlockEntity>(pos);
    CopperGolemStatueBlockEntity* bePtr = be.get();
    world.setBlockEntity(pos, be.release());
    bePtr->setWorld(&world);

    const BlockState state = VanillaBlocks::COPPER_GOLEM_STATUE->defaultState().with(
        BlockStateProperties::HORIZONTAL_FACING(), Direction::South);

    auto entity = bePtr->removeStatue(state);
    ASSERT_NE(entity, nullptr);

    EXPECT_FLOAT_EQ(entity->pitch(), 0.0f);
}
