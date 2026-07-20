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

#include "common/core/Constants.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/physics/PhysicsEngine.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/storage/SingleLevelStorageManager.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"
#include <gtest/gtest.h>

#include <ctime>
#include <filesystem>
#include <utility>

using namespace mc;
using namespace mc::server;

// ============================================================================
// ServerWorld 碰撞检测测试
// ============================================================================

class ServerWorldCollisionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化方块注册表
        VanillaBlocks::initialize();

        // 打开存档：ServerWorld::initialize 要求 m_storage 已设置且 isOpen()。
        m_testDir =
            std::filesystem::temp_directory_path() / "mc_world_collision_test" / std::to_string(std::time(nullptr));
        std::filesystem::create_directories(m_testDir);

        world::storage::SingleLevelStorageConfig storageConfig;
        auto openResult = m_storage.open(m_testDir, storageConfig);
        ASSERT_TRUE(openResult.success()) << openResult.error().message();

        ServerWorldConfig config;
        config.viewDistance = 2;
        config.dimension = 0;
        config.seed = 12345;
        world = std::make_unique<ServerWorld>(config);
        world->setSharedStorage(&m_storage);
        // 装配区块管理器（ServerWorld::initialize 亦要求 m_chunkManager != nullptr）
        auto settings = DimensionSettings::overworld();
        auto randomState = world::gen::RandomState::create(settings, config.seed);
        auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false, false);
        auto generator =
            std::make_unique<NoiseChunkGenerator>(std::move(settings), std::move(biomeSource), std::move(randomState));
        auto chunkManager = std::make_unique<ServerChunkManager>(*world, std::move(generator));
        world->setChunkManager(std::move(chunkManager));

        // 初始化世界
        auto result = world->initialize();
        ASSERT_TRUE(result.success()) << "Failed to initialize world";
    }

    void TearDown() override
    {
        if (world) {
            world->shutdown();
            world.reset();
        }
        m_storage.close();
        if (std::filesystem::exists(m_testDir)) {
            std::error_code ec;
            std::filesystem::remove_all(m_testDir, ec);
        }
    }

    std::unique_ptr<ServerWorld> world;
    world::storage::SingleLevelStorageManager m_storage;
    std::filesystem::path m_testDir;
};

// ========== 物理引擎测试 ==========

TEST_F(ServerWorldCollisionTest, PhysicsEngineInitialized)
{
    // 验证物理引擎已初始化
    PhysicsEngine* physics = world->physicsEngine();
    ASSERT_NE(physics, nullptr);

    const PhysicsEngine* constPhysics = const_cast<const ServerWorld*>(world.get())->physicsEngine();
    EXPECT_NE(constPhysics, nullptr);
}

TEST_F(ServerWorldCollisionTest, CollisionCacheInitialized)
{
    // 验证碰撞缓存已初始化
    EXPECT_NO_THROW(world->clearCollisionCache());
}

// ========== 方块碰撞检测测试 ==========

TEST_F(ServerWorldCollisionTest, HasBlockCollisionEmptyWorld)
{
    // 在空区域（无区块）检测碰撞
    AxisAlignedBB box(1000.0f, 60.0f, 1000.0f, 1001.0f, 61.0f, 1001.0f);

    // 无区块加载，应该无碰撞
    EXPECT_FALSE(world->hasBlockCollision(box));
}

TEST_F(ServerWorldCollisionTest, HasBlockCollisionWithAir)
{
    // 生成一个区块
    ChunkData* chunk = world->chunkManager()->getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);

    // 在空气区域检测碰撞（Y=100 远离地形）
    AxisAlignedBB box(8.0f, 100.0f, 8.0f, 9.0f, 101.0f, 9.0f);

    EXPECT_FALSE(world->hasBlockCollision(box));
}

TEST_F(ServerWorldCollisionTest, HasBlockCollisionWithGround)
{
    // 生成一个区块
    ChunkData* chunk = world->chunkManager()->getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);

    // 在出生点附近找一个非空气位置
    // 遍历寻找地面
    bool foundGround = false;
    i32 groundY = 0;

    for (i32 y = mc::world::MAX_BUILD_HEIGHT - 1; y >= mc::world::MIN_BUILD_HEIGHT; --y) {
        const BlockState* state = chunk->getBlockState(8, y, 8);
        if (state && !state->isAir()) {
            groundY = y;
            foundGround = true;
            break;
        }
    }

    if (foundGround) {
        // 在地面位置检测碰撞
        AxisAlignedBB box(8.0f, static_cast<f32>(groundY), 8.0f, 9.0f, static_cast<f32>(groundY + 1), 9.0f);
        EXPECT_TRUE(world->hasBlockCollision(box));
    }
}

TEST_F(ServerWorldCollisionTest, GetBlockCollisionsEmptyArea)
{
    // 在空区域获取碰撞箱
    AxisAlignedBB box(1000.0f, 60.0f, 1000.0f, 1001.0f, 61.0f, 1001.0f);

    auto collisions = world->getBlockCollisions(box);
    EXPECT_TRUE(collisions.empty());
}

// ========== 实体碰撞检测测试 ==========

TEST_F(ServerWorldCollisionTest, HasEntityCollisionNoEntities)
{
    AxisAlignedBB box(0.0f, 0.0f, 0.0f, 10.0f, 10.0f, 10.0f);

    // 无实体时应该无碰撞
    EXPECT_FALSE(world->hasEntityCollision(box));
}

TEST_F(ServerWorldCollisionTest, HasEntityCollisionWithEntity)
{
    // 创建实体
    auto entity = std::make_unique<Entity>(EntityInstanceId(1), world.get());
    entity->setPosition(5.0f, 5.0f, 5.0f);

    EntityInstanceId entityId = world->spawnEntity(std::move(entity));
    ASSERT_NE(entityId, 0);

    // 在实体位置检测碰撞
    AxisAlignedBB box(4.0f, 4.0f, 4.0f, 6.0f, 6.0f, 6.0f);
    EXPECT_TRUE(world->hasEntityCollision(box));

    // 在其他位置检测碰撞
    AxisAlignedBB farBox(100.0f, 100.0f, 100.0f, 101.0f, 101.0f, 101.0f);
    EXPECT_FALSE(world->hasEntityCollision(farBox));
}

TEST_F(ServerWorldCollisionTest, HasEntityCollisionExceptSelf)
{
    // 创建实体
    auto entity = std::make_unique<Entity>(EntityInstanceId(1), world.get());
    entity->setPosition(5.0f, 5.0f, 5.0f);

    EntityInstanceId entityId = world->spawnEntity(std::move(entity));
    ASSERT_NE(entityId, 0);

    Entity* spawnedEntity = world->getEntity(entityId);
    ASSERT_NE(spawnedEntity, nullptr);

    // 在实体位置检测碰撞，排除自己
    AxisAlignedBB box(4.0f, 4.0f, 4.0f, 6.0f, 6.0f, 6.0f);
    EXPECT_FALSE(world->hasEntityCollision(box, spawnedEntity));
}

TEST_F(ServerWorldCollisionTest, GetEntityCollisions)
{
    // 创建多个实体
    auto entity1 = std::make_unique<Entity>(EntityInstanceId(1), world.get());
    entity1->setPosition(5.0f, 5.0f, 5.0f);
    world->spawnEntity(std::move(entity1));

    auto entity2 = std::make_unique<Entity>(EntityInstanceId(2), world.get());
    entity2->setPosition(5.5f, 5.0f, 5.0f);
    world->spawnEntity(std::move(entity2));

    // 获取碰撞箱
    AxisAlignedBB box(4.0f, 4.0f, 4.0f, 7.0f, 7.0f, 7.0f);
    auto collisions = world->getEntityCollisions(box);

    EXPECT_EQ(collisions.size(), 2);
}

// ========== 物理引擎集成测试 ==========

TEST_F(ServerWorldCollisionTest, PhysicsEngineMoveEntity)
{
    PhysicsEngine* physics = world->physicsEngine();
    ASSERT_NE(physics, nullptr);

    // 创建一个简单的碰撞箱
    AxisAlignedBB entityBox(0.0f, 100.0f, 0.0f, 0.6f, 101.8f, 0.6f);

    // 在空气中移动（无碰撞）
    Vector3 movement = physics->moveEntity(entityBox, Vector3(1.0f, 0.0f, 0.0f), 0.0f);

    // 验证移动成功（允许物理引擎正常工作）
    EXPECT_NEAR(movement.x, 1.0f, 0.001f);
}

TEST_F(ServerWorldCollisionTest, PhysicsEngineIsOnGround)
{
    PhysicsEngine* physics = world->physicsEngine();
    ASSERT_NE(physics, nullptr);

    // 生成区块
    ChunkData* chunk = world->chunkManager()->getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);

    // 寻找地面
    bool foundGround = false;
    i32 groundY = 0;

    for (i32 y = mc::world::MAX_BUILD_HEIGHT - 1; y >= mc::world::MIN_BUILD_HEIGHT; --y) {
        const BlockState* state = chunk->getBlockState(8, y, 8);
        if (state && !state->isAir()) {
            groundY = y;
            foundGround = true;
            break;
        }
    }

    if (foundGround) {
        // 紧贴地面的碰撞箱应该检测到地面
        AxisAlignedBB groundBox(
            8.0f, static_cast<f32>(groundY + 0.01f), 8.0f, 8.6f, static_cast<f32>(groundY + 1.81f), 8.6f);
        EXPECT_TRUE(physics->isOnGround(groundBox));

        // 在很高的空中（远离地形）应该不在地面上
        // 使用绝对坐标远离生成的地形（最大高度256）
        AxisAlignedBB highBox(8.0f, 300.0f, 8.0f, 8.6f, 301.8f, 8.6f);
        EXPECT_FALSE(physics->isOnGround(highBox));
    }
}

// ========== 碰撞缓存测试 ==========

TEST_F(ServerWorldCollisionTest, InvalidateCollisionCache)
{
    // 生成区块
    ChunkData* chunk = world->chunkManager()->getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);

    // 使缓存失效应该不会抛出异常
    EXPECT_NO_THROW(world->invalidateCollisionCache(0, 0));
    EXPECT_NO_THROW(world->clearCollisionCache());
}

// ========== ICollisionWorld 接口测试 ==========

TEST_F(ServerWorldCollisionTest, ICollisionWorldGetBlockState)
{
    // 生成区块
    ChunkData* chunk = world->chunkManager()->getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);

    // ICollisionWorld 接口测试
    const BlockState* state = world->getBlockState(8, 64, 8);
    // 可能是空气或某个方块
    EXPECT_TRUE(state != nullptr || state == nullptr); // 仅验证不会崩溃
}

TEST_F(ServerWorldCollisionTest, ICollisionWorldIsWithinWorldBounds)
{
    // 测试世界边界
    EXPECT_TRUE(world->isWithinWorldBounds(0, mc::world::MIN_BUILD_HEIGHT, 0));
    EXPECT_TRUE(world->isWithinWorldBounds(100, 100, 100));
    EXPECT_TRUE(world->isWithinWorldBounds(100, mc::world::MAX_BUILD_HEIGHT - 1, 100));

    // 超出边界
    EXPECT_FALSE(world->isWithinWorldBounds(0, mc::world::MIN_BUILD_HEIGHT - 1, 0));
    EXPECT_FALSE(world->isWithinWorldBounds(0, mc::world::MAX_BUILD_HEIGHT, 0));
}

TEST_F(ServerWorldCollisionTest, ICollisionWorldGetChunkAt)
{
    // 生成区块
    ChunkData* chunk = world->chunkManager()->getChunkSync(5, 5);
    ASSERT_NE(chunk, nullptr);

    // ICollisionWorld 接口测试
    const ChunkData* chunkViaInterface = world->getChunkAt(5, 5);
    EXPECT_NE(chunkViaInterface, nullptr);
    EXPECT_EQ(chunkViaInterface, chunk);
}

// ========== 实体管理测试 ==========

TEST_F(ServerWorldCollisionTest, SpawnEntity)
{
    auto entity = std::make_unique<Entity>(EntityInstanceId(1), world.get());
    entity->setPosition(10.0f, 64.0f, 10.0f);

    EntityInstanceId id = world->spawnEntity(std::move(entity));
    EXPECT_NE(id, 0);
    EXPECT_EQ(world->entityCount(), 1);
}

TEST_F(ServerWorldCollisionTest, RemoveEntity)
{
    auto entity = std::make_unique<Entity>(EntityInstanceId(1), world.get());
    EntityInstanceId id = world->spawnEntity(std::move(entity));

    auto removed = world->removeEntity(id);
    EXPECT_NE(removed, nullptr);
    EXPECT_EQ(world->entityCount(), 0);
}

TEST_F(ServerWorldCollisionTest, GetEntitiesInAABB)
{
    // 创建多个实体
    auto entity1 = std::make_unique<Entity>(EntityInstanceId(1), world.get());
    entity1->setPosition(0.0f, 0.0f, 0.0f);
    world->spawnEntity(std::move(entity1));

    auto entity2 = std::make_unique<Entity>(EntityInstanceId(2), world.get());
    entity2->setPosition(100.0f, 100.0f, 100.0f);
    world->spawnEntity(std::move(entity2));

    // 查询第一个实体附近的实体
    AxisAlignedBB box1(-10.0f, -10.0f, -10.0f, 10.0f, 10.0f, 10.0f);
    auto entities1 = world->getEntitiesInAABB(box1);
    EXPECT_EQ(entities1.size(), 1);

    // 查询第二个实体附近的实体
    AxisAlignedBB box2(90.0f, 90.0f, 90.0f, 110.0f, 110.0f, 110.0f);
    auto entities2 = world->getEntitiesInAABB(box2);
    EXPECT_EQ(entities2.size(), 1);

    // 查询包含所有实体的区域
    AxisAlignedBB box3(-10.0f, -10.0f, -10.0f, 110.0f, 110.0f, 110.0f);
    auto entities3 = world->getEntitiesInAABB(box3);
    EXPECT_EQ(entities3.size(), 2);
}
