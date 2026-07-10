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

/**
 * @file ClientWorldBlockEntityTest.cpp
 * @brief ClientWorld 方块实体客户端存储测试
 *
 * 测试 ClientWorld 的 BlockEntity 客户端同步与存储接口：
 * - onBlockEntityData：接收网络包并创建/更新 BlockEntity
 * - getBlockEntity：按位置查询
 * - removeBlockEntity：按位置移除
 * - clearBlockEntities：全量清空
 * - clearChunks 与 clearBlockEntities 的联动
 * - 错误处理（NBT 反序列化失败、未注册类型等）
 */

#include "client/world/ClientWorld.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/network/packet/BlockEntityDataPacket.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/core/BlockEntityRegistry.hpp"

#include <gtest/gtest.h>

using namespace mc;
using namespace mc::client;
using mc::BlockEntityType;
using mc::blockentity::BlockEntityRegistry;
using mc::nbt::CompoundTag;
using mc::network::BlockEntityDataPacket;

/**
 * @brief ClientWorld 方块实体存储单元测试夹具
 *
 * 每个 TEST_F 都使用独立的 ClientWorld 实例，避免 BlockEntity 注册表
 * 全局状态在不同用例间互相干扰。
 */
class ClientWorldBlockEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 确保注册表已注册内置类型（幂等操作）
        BlockEntityRegistry::instance().registerBuiltinTypes();

        auto result = m_world.initialize(12345);
        ASSERT_TRUE(result.success()) << "Failed to initialize ClientWorld";
    }

    void TearDown() override { m_world.destroy(); }

    // 构造一个最小有效的告示牌 NBT 字节流
    std::vector<u8> makeSignNbtBytes(i32 x, i32 y, i32 z)
    {
        CompoundTag tag;
        tag.put("id", std::string("minecraft:sign"));
        tag.put("x", x);
        tag.put("y", y);
        tag.put("z", z);
        tag.put("color", static_cast<i32>(0));
        tag.put("glowing", static_cast<i8>(0));
        tag.put("is_waxed", static_cast<i8>(0));
        return BlockEntityDataPacket::serializeNbtToBytes(tag);
    }

    ClientWorld m_world;
};

// ========== getBlockEntity 基础查询 ==========

TEST_F(ClientWorldBlockEntityTest, GetBlockEntityReturnsNullptrWhenNotFound)
{
    EXPECT_EQ(m_world.getBlockEntity(BlockPos(0, 0, 0)), nullptr);
}

TEST_F(ClientWorldBlockEntityTest, ConstGetBlockEntityReturnsNullptrWhenNotFound)
{
    const auto& constWorld = m_world;
    EXPECT_EQ(constWorld.getBlockEntity(BlockPos(10, 64, 10)), nullptr);
}

// ========== onBlockEntityData 创建路径 ==========

TEST_F(ClientWorldBlockEntityTest, OnBlockEntityDataCreatesEntity)
{
    BlockPos pos(10, 64, 10);
    auto nbtBytes = makeSignNbtBytes(10, 64, 10);

    m_world.onBlockEntityData(pos, BlockEntityType::Sign, nbtBytes);

    BlockEntity* entity = m_world.getBlockEntity(pos);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->getType(), BlockEntityType::Sign);
    EXPECT_EQ(entity->getPos(), pos);
}

TEST_F(ClientWorldBlockEntityTest, OnBlockEntityDataUpdatesExistingEntity)
{
    BlockPos pos(10, 64, 10);
    auto nbtBytes1 = makeSignNbtBytes(10, 64, 10);
    auto nbtBytes2 = makeSignNbtBytes(10, 64, 10);

    m_world.onBlockEntityData(pos, BlockEntityType::Sign, nbtBytes1);
    BlockEntity* firstEntity = m_world.getBlockEntity(pos);
    ASSERT_NE(firstEntity, nullptr);

    // 再次发送更新：应复用已存在的 BlockEntity 实例
    m_world.onBlockEntityData(pos, BlockEntityType::Sign, nbtBytes2);
    BlockEntity* secondEntity = m_world.getBlockEntity(pos);
    ASSERT_NE(secondEntity, nullptr);
    // 同一位置应返回相同的指针（复用实例）
    EXPECT_EQ(secondEntity, firstEntity);
}

TEST_F(ClientWorldBlockEntityTest, OnBlockEntityDataHandlesMultipleEntities)
{
    BlockPos pos1(10, 64, 10);
    BlockPos pos2(20, 70, 20);
    BlockPos pos3(30, 80, 30);

    m_world.onBlockEntityData(pos1, BlockEntityType::Sign, makeSignNbtBytes(10, 64, 10));
    m_world.onBlockEntityData(pos2, BlockEntityType::Sign, makeSignNbtBytes(20, 70, 20));
    m_world.onBlockEntityData(pos3, BlockEntityType::Sign, makeSignNbtBytes(30, 80, 30));

    EXPECT_NE(m_world.getBlockEntity(pos1), nullptr);
    EXPECT_NE(m_world.getBlockEntity(pos2), nullptr);
    EXPECT_NE(m_world.getBlockEntity(pos3), nullptr);

    // 各位置的实体应是独立的实例
    EXPECT_NE(m_world.getBlockEntity(pos1), m_world.getBlockEntity(pos2));
    EXPECT_NE(m_world.getBlockEntity(pos2), m_world.getBlockEntity(pos3));
}

TEST_F(ClientWorldBlockEntityTest, OnBlockEntityDataHandlesNegativeCoordinates)
{
    BlockPos pos(-100, -32, -200);
    auto nbtBytes = makeSignNbtBytes(-100, -32, -200);

    m_world.onBlockEntityData(pos, BlockEntityType::Sign, nbtBytes);

    BlockEntity* entity = m_world.getBlockEntity(pos);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->getPos(), pos);
}

// ========== onBlockEntityData 错误处理 ==========

TEST_F(ClientWorldBlockEntityTest, OnBlockEntityDataWithEmptyNbtDoesNotCreateEntity)
{
    BlockPos pos(10, 64, 10);
    std::vector<u8> emptyNbt;

    m_world.onBlockEntityData(pos, BlockEntityType::Sign, emptyNbt);

    // 空 NBT 字节流会反序列化失败，不应创建实体
    EXPECT_EQ(m_world.getBlockEntity(pos), nullptr);
}

TEST_F(ClientWorldBlockEntityTest, OnBlockEntityDataWithInvalidNbtDoesNotCreateEntity)
{
    BlockPos pos(10, 64, 10);
    std::vector<u8> garbage = {0xDE, 0xAD, 0xBE, 0xEF};

    m_world.onBlockEntityData(pos, BlockEntityType::Sign, garbage);

    EXPECT_EQ(m_world.getBlockEntity(pos), nullptr);
}

TEST_F(ClientWorldBlockEntityTest, OnBlockEntityDataWithUnregisteredTypeDoesNotCreateEntity)
{
    // 使用 Count 类型（未注册工厂），注册表应返回 nullptr
    BlockPos pos(10, 64, 10);
    auto nbtBytes = makeSignNbtBytes(10, 64, 10);

    m_world.onBlockEntityData(pos, BlockEntityType::Count, nbtBytes);

    EXPECT_EQ(m_world.getBlockEntity(pos), nullptr);
}

// ========== removeBlockEntity ==========

TEST_F(ClientWorldBlockEntityTest, RemoveBlockEntityRemovesExistingEntity)
{
    BlockPos pos(10, 64, 10);
    m_world.onBlockEntityData(pos, BlockEntityType::Sign, makeSignNbtBytes(10, 64, 10));
    ASSERT_NE(m_world.getBlockEntity(pos), nullptr);

    m_world.removeBlockEntity(pos);

    EXPECT_EQ(m_world.getBlockEntity(pos), nullptr);
}

TEST_F(ClientWorldBlockEntityTest, RemoveBlockEntityHandlesNonExistentEntity)
{
    // 移除不存在的实体不应崩溃
    EXPECT_NO_THROW(m_world.removeBlockEntity(BlockPos(999, 999, 999)));
}

TEST_F(ClientWorldBlockEntityTest, RemoveBlockEntityDoesNotAffectOthers)
{
    BlockPos pos1(10, 64, 10);
    BlockPos pos2(20, 70, 20);

    m_world.onBlockEntityData(pos1, BlockEntityType::Sign, makeSignNbtBytes(10, 64, 10));
    m_world.onBlockEntityData(pos2, BlockEntityType::Sign, makeSignNbtBytes(20, 70, 20));

    m_world.removeBlockEntity(pos1);

    EXPECT_EQ(m_world.getBlockEntity(pos1), nullptr);
    EXPECT_NE(m_world.getBlockEntity(pos2), nullptr);
}

// ========== clearBlockEntities ==========

TEST_F(ClientWorldBlockEntityTest, ClearBlockEntitiesOnEmptyWorld)
{
    // 空世界调用 clearBlockEntities 不应崩溃
    EXPECT_NO_THROW(m_world.clearBlockEntities());
}

TEST_F(ClientWorldBlockEntityTest, ClearBlockEntitiesRemovesAllEntities)
{
    BlockPos pos1(10, 64, 10);
    BlockPos pos2(20, 70, 20);
    BlockPos pos3(30, 80, 30);

    m_world.onBlockEntityData(pos1, BlockEntityType::Sign, makeSignNbtBytes(10, 64, 10));
    m_world.onBlockEntityData(pos2, BlockEntityType::Sign, makeSignNbtBytes(20, 70, 20));
    m_world.onBlockEntityData(pos3, BlockEntityType::Sign, makeSignNbtBytes(30, 80, 30));

    m_world.clearBlockEntities();

    EXPECT_EQ(m_world.getBlockEntity(pos1), nullptr);
    EXPECT_EQ(m_world.getBlockEntity(pos2), nullptr);
    EXPECT_EQ(m_world.getBlockEntity(pos3), nullptr);
}

// ========== clearChunks 与 clearBlockEntities 联动 ==========

TEST_F(ClientWorldBlockEntityTest, ClearChunksAlsoClearsBlockEntities)
{
    BlockPos pos(10, 64, 10);
    m_world.onBlockEntityData(pos, BlockEntityType::Sign, makeSignNbtBytes(10, 64, 10));
    ASSERT_NE(m_world.getBlockEntity(pos), nullptr);

    m_world.clearChunks();

    // clearChunks 应一并清空 BlockEntity 存储
    EXPECT_EQ(m_world.getBlockEntity(pos), nullptr);
}

// ========== 完整流程：NBT 数据正确性 ==========

TEST_F(ClientWorldBlockEntityTest, OnBlockEntityDataPreservesPositionFromPacket)
{
    // 包的 pos 字段与 NBT 中的 x/y/z 不一致时，BlockEntity::m_pos 应以包的 pos 为准
    // （因为 ClientWorld::onBlockEntityData 用 packet.pos 创建实体）
    BlockPos packetPos(100, 200, 300);
    auto nbtBytes = makeSignNbtBytes(100, 200, 300);

    m_world.onBlockEntityData(packetPos, BlockEntityType::Sign, nbtBytes);

    BlockEntity* entity = m_world.getBlockEntity(packetPos);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->getPos(), packetPos);
}
