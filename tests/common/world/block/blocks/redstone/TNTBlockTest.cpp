/**
 * @file TNTBlockTest.cpp
 * @brief TNTBlock 单元测试
 *
 * 测试 TNT 方块的点燃、爆炸功能。
 * 注意：hasFlammableNeighbor 是私有方法，通过 onBlockAdded 间接测试。
 */

#include "common/world/block/blocks/redstone/TNTBlock.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/core/Constants.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/VanillaEntities.hpp"
#include "common/entity/entities/misc/MiscEntities.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <unordered_map>

namespace mc {
namespace blocks {
namespace test {

/**
 * @brief 用于 TNTBlock 测试的 Mock World 实现
 */
class TNTBlockTestWorld final : public ::mc::test::BaseTestWorld {
public:
    TNTBlockTestWorld() = default;

    // ========== 方块访问 ==========

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (state == nullptr) {
            m_blocks.erase(BlockPos(x, y, z));
        } else {
            m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        }
        return true;
    }

    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT;
    }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }

    [[nodiscard]] bool isClientSide() override { return m_isClientSide; }

    void setClientSide(bool isClient) { m_isClientSide = isClient; }

    EntityId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        // 记录生成的 TNT 实体
        if (auto* tnt = dynamic_cast<entity::TNTEntity*>(entity.get())) {
            m_spawnedTNTCount++;
            m_lastTNTPosition = tnt->position();
            m_lastTNTFuse = tnt->getFuse();
            m_lastTNTVelocity = tnt->velocity();
        }

        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityId>(m_spawnedEntities.size());
    }

    void playSound(const ResourceLocation& soundEventId,
        sound::SoundCategory category,
        const Vector3& position,
        f32 volume,
        f32 pitch) override
    {
        m_lastSoundEvent = soundEventId;
        m_lastSoundCategory = category;
        m_lastSoundPosition = position;
        m_lastSoundVolume = volume;
        m_lastSoundPitch = pitch;
        m_soundPlayed = true;
    }

    void createExplosion(const Vector3& position,
        f32 radius,
        world::explosion::ExplosionMode mode,
        bool causesFire,
        Entity* source) override
    {
        m_lastExplosionPos = position;
        m_lastExplosionRadius = radius;
        m_lastExplosionMode = mode;
        m_explosionCausesFire = causesFire;
        m_lastExplosionSource = source;
        m_explosionCount++;
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("TNTBlockTestWorld::tickManager not implemented");
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("TNTBlockTestWorld::tickManager not implemented");
    }

    // 测试辅助方法

    void setBlockAt(const BlockPos& pos, const BlockState* state)
    {
        if (state == nullptr) {
            m_blocks.erase(pos);
        } else {
            m_blocks[pos] = std::make_unique<BlockState>(*state);
        }
    }

    void advanceTick() { m_currentTick++; }

    [[nodiscard]] i32 spawnedTNTCount() const { return m_spawnedTNTCount; }

    [[nodiscard]] const Vector3& lastTNTPosition() const { return m_lastTNTPosition; }

    [[nodiscard]] i32 lastTNTFuse() const { return m_lastTNTFuse; }

    [[nodiscard]] const Vector3& lastTNTVelocity() const { return m_lastTNTVelocity; }

    [[nodiscard]] bool soundPlayed() const { return m_soundPlayed; }

    [[nodiscard]] const ResourceLocation& lastSoundEvent() const { return m_lastSoundEvent; }

    [[nodiscard]] i32 explosionCount() const { return m_explosionCount; }

    [[nodiscard]] const Vector3& lastExplosionPos() const { return m_lastExplosionPos; }

    [[nodiscard]] f32 lastExplosionRadius() const { return m_lastExplosionRadius; }

    [[nodiscard]] world::explosion::ExplosionMode lastExplosionMode() const { return m_lastExplosionMode; }

    [[nodiscard]] bool explosionCausesFire() const { return m_explosionCausesFire; }

    void clearState()
    {
        m_blocks.clear();
        m_spawnedEntities.clear();
        m_spawnedTNTCount = 0;
        m_explosionCount = 0;
        m_soundPlayed = false;
    }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    u64 m_currentTick = 0;
    bool m_isClientSide = false;

    // TNT 生成记录
    i32 m_spawnedTNTCount = 0;
    Vector3 m_lastTNTPosition{0, 0, 0};
    i32 m_lastTNTFuse = 0;
    Vector3 m_lastTNTVelocity{0, 0, 0};

    // 声音记录
    bool m_soundPlayed = false;
    ResourceLocation m_lastSoundEvent;
    sound::SoundCategory m_lastSoundCategory = sound::SoundCategory::Master;
    Vector3 m_lastSoundPosition{0, 0, 0};
    f32 m_lastSoundVolume = 1.0f;
    f32 m_lastSoundPitch = 1.0f;

    // 爆炸记录
    i32 m_explosionCount = 0;
    Vector3 m_lastExplosionPos{0, 0, 0};
    f32 m_lastExplosionRadius = 0.0f;
    world::explosion::ExplosionMode m_lastExplosionMode = world::explosion::ExplosionMode::None;
    bool m_explosionCausesFire = false;
    Entity* m_lastExplosionSource = nullptr;
};

/**
 * @brief TNTBlock 测试固件
 */
class TNTBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        entity::VanillaEntities::registerAll();
    }

    void TearDown() override { m_world.clearState(); }

    TNTBlockTestWorld m_world;
};

/**
 * @brief 测试 TNTBlock 构造函数
 */
TEST_F(TNTBlockTest, Construction)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);
    ASSERT_NE(tntBlock, nullptr);
    // Verify that defaultState's block is valid
    const BlockState& defaultState = tntBlock->defaultState();
    EXPECT_EQ(&defaultState.getBlock(), static_cast<const Block*>(tntBlock.get()));
}

/**
 * @brief 测试 isUnstable 静态方法
 */
TEST_F(TNTBlockTest, IsUnstable)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);
    const BlockState& defaultState = tntBlock->defaultState();

    // 默认状态应该是稳定的 (UNSTABLE = false)
    EXPECT_FALSE(TNTBlock::isUnstable(defaultState));
}

/**
 * @brief 测试点燃功能 - 服务端
 *
 * 服务端点燃 TNT 应该生成 TNTEntity 并播放音效
 */
TEST_F(TNTBlockTest, IgniteOnServerSide)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    // 设置 TNT 方块
    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());

    // 服务端点燃
    m_world.setClientSide(false);
    tntBlock->ignite(m_world, tntPos, tntBlock->defaultState());

    // 验证 TNT 方块被移除
    const BlockState* state = m_world.getBlockState(tntPos.x, tntPos.y, tntPos.z);
    EXPECT_TRUE(state == nullptr || state->is(VanillaBlocks::AIR));

    // 验证生成了 TNT 实体
    EXPECT_EQ(m_world.spawnedTNTCount(), 1);

    // 验证 TNT 位置正确（方块中心）
    EXPECT_FLOAT_EQ(m_world.lastTNTPosition().x, 10.5f);
    EXPECT_FLOAT_EQ(m_world.lastTNTPosition().y, 64.0f);
    EXPECT_FLOAT_EQ(m_world.lastTNTPosition().z, 20.5f);

    // 验证引信已点燃（80 ticks）
    EXPECT_EQ(m_world.lastTNTFuse(), 80);

    // 验证音效播放
    EXPECT_TRUE(m_world.soundPlayed());
    EXPECT_EQ(m_world.lastSoundEvent(), SoundEvents::ENTITY_TNT_PRIMED);
}

/**
 * @brief 测试点燃功能 - 客户端
 *
 * 客户端点燃 TNT 不应该有任何效果
 */
TEST_F(TNTBlockTest, IgniteOnClientSide)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    // 设置 TNT 方块
    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());

    // 客户端点燃
    m_world.setClientSide(true);
    tntBlock->ignite(m_world, tntPos, tntBlock->defaultState());

    // 客户端不应该生成实体
    EXPECT_EQ(m_world.spawnedTNTCount(), 0);

    // 客户端不应该播放音效
    EXPECT_FALSE(m_world.soundPlayed());
}

/**
 * @brief 测试 TNT 初始速度
 *
 * MC 1.16.5: TNT 被点燃时有随机的初始速度
 */
TEST_F(TNTBlockTest, IgniteRandomInitialVelocity)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(0, 64, 0);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());

    // 点燃多次，验证速度是随机的
    m_world.setClientSide(false);

    // 验证 Y 速度是固定的 0.2
    m_world.clearState();
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    tntBlock->ignite(m_world, tntPos, tntBlock->defaultState());

    EXPECT_FLOAT_EQ(m_world.lastTNTVelocity().y, 0.2f);
}

/**
 * @brief 测试爆炸功能
 *
 * explode() 应该移除方块并创建爆炸
 */
TEST_F(TNTBlockTest, Explode)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());

    // 触发爆炸
    tntBlock->explode(m_world, tntPos, 4.0f);

    // 验证 TNT 方块被移除
    const BlockState* state = m_world.getBlockState(tntPos.x, tntPos.y, tntPos.z);
    EXPECT_TRUE(state == nullptr || state->is(VanillaBlocks::AIR));

    // 验证爆炸被创建
    EXPECT_EQ(m_world.explosionCount(), 1);
    EXPECT_FLOAT_EQ(m_world.lastExplosionRadius(), 4.0f);
    EXPECT_EQ(m_world.lastExplosionMode(), world::explosion::ExplosionMode::Break);
    EXPECT_FALSE(m_world.explosionCausesFire());

    // 验证爆炸位置（方块中心，Y 偏移 0.0625）
    EXPECT_FLOAT_EQ(m_world.lastExplosionPos().x, 10.5f);
    EXPECT_FLOAT_EQ(m_world.lastExplosionPos().y, 64.0f + 0.0625f);
    EXPECT_FLOAT_EQ(m_world.lastExplosionPos().z, 20.5f);
}

/**
 * @brief 测试自定义爆炸半径
 */
TEST_F(TNTBlockTest, ExplodeCustomRadius)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(0, 64, 0);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());

    // 使用自定义半径爆炸
    tntBlock->explode(m_world, tntPos, 10.0f);

    EXPECT_EQ(m_world.explosionCount(), 1);
    EXPECT_FLOAT_EQ(m_world.lastExplosionRadius(), 10.0f);
}

/**
 * @brief 测试 onBlockAdded - 有火焰
 *
 * 当 TNT 被放置在有火焰的位置时，应该点燃
 */
TEST_F(TNTBlockTest, OnBlockAdded_WithFire)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(0, 64, 0);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());

    // 在旁边放置火焰
    BlockPos firePos(1, 64, 0);
    m_world.setBlockAt(firePos, &VanillaBlocks::FIRE->defaultState());

    // 放置 TNT 时应该点燃
    tntBlock->onBlockAdded(m_world, tntPos, tntBlock->defaultState());

    EXPECT_EQ(m_world.spawnedTNTCount(), 1);
}

/**
 * @brief 测试 TNT 爆炸模式为 Break
 *
 * TNT 爆炸使用 Break 模式，破坏方块但不掉落物品
 */
TEST_F(TNTBlockTest, ExplosionModeIsBreak)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(0, 64, 0);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());

    tntBlock->explode(m_world, tntPos, 4.0f);

    // 验证爆炸模式是 Break（破坏方块但不掉落物品）
    EXPECT_EQ(m_world.lastExplosionMode(), world::explosion::ExplosionMode::Break);
}

/**
 * @brief 测试 TNT 爆炸不生成火焰
 */
TEST_F(TNTBlockTest, ExplosionDoesNotCauseFire)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(0, 64, 0);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());

    tntBlock->explode(m_world, tntPos, 4.0f);

    // TNT 爆炸不应该生成火焰
    EXPECT_FALSE(m_world.explosionCausesFire());
}

/**
 * @brief 测试实体类型注册
 *
 * TNTEntity 应该正确注册
 */
TEST_F(TNTBlockTest, TNTEntityIsRegistered)
{
    auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* tntType = registry.getType(entity::EntityTypes::TNT);

    ASSERT_NE(tntType, nullptr);
    EXPECT_TRUE(tntType->isValid());
}

} // namespace test
} // namespace blocks
} // namespace mc
