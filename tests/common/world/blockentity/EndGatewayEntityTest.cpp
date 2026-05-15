/*
* Copyright (c) 2026 Guo Yi
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to subject to the following conditions:
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

#include "world/blockentity/interactive/EndGatewayEntity.hpp"
#include "world/block/BlockPos.hpp"
#include "world/blockentity/BlockEntityType.hpp"
#include "world/blockentity/core/BlockEntityRegistry.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::blockentity;

// ========== EndGatewayEntity 注册测试 ==========

class EndGatewayEntityRegistryTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 确保内置类型已注册
        BlockEntityRegistry::instance().registerBuiltinTypes();
    }
};

TEST_F(EndGatewayEntityRegistryTest, EndGatewayType_IsRegistered)
{
    // 验证 EndGateway 类型已在注册表中注册
    EXPECT_TRUE(BlockEntityRegistry::instance().hasType(BlockEntityType::EndGateway));
}

TEST_F(EndGatewayEntityRegistryTest, CreateEndGatewayEntity_ReturnsValidEntity)
{
    BlockPos pos(100, 75, -200);
    auto entity = BlockEntityRegistry::instance().create(BlockEntityType::EndGateway, pos);

    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->getType(), BlockEntityType::EndGateway);
    EXPECT_EQ(entity->getPos(), pos);
}

TEST_F(EndGatewayEntityRegistryTest, CreateEndGatewayEntity_CreatesEndGatewayEntity)
{
    BlockPos pos(0, 0, 0);
    auto entity = BlockEntityRegistry::instance().create(BlockEntityType::EndGateway, pos);

    // 验证创建的是 EndGatewayEntity 类型
    auto* gatewayEntity = dynamic_cast<EndGatewayEntity*>(entity.get());
    EXPECT_NE(gatewayEntity, nullptr);
}

TEST_F(EndGatewayEntityRegistryTest, CreateFromJson_EndGatewayEntity)
{
    nlohmann::json data;
    data["id"] = "minecraft:end_gateway";
    data["x"] = 1024;
    data["y"] = 75;
    data["z"] = -512;

    auto entity = BlockEntityRegistry::instance().createFromJson(data);

    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->getType(), BlockEntityType::EndGateway);
    EXPECT_EQ(entity->getPos().x, 1024);
    EXPECT_EQ(entity->getPos().y, 75);
    EXPECT_EQ(entity->getPos().z, -512);
}

// ========== EndGatewayEntity 基础功能测试 ==========

class EndGatewayEntityTest : public ::testing::Test {
protected:
    void SetUp() override { gatewayEntity = std::make_unique<EndGatewayEntity>(BlockPos(100, 50, 0)); }

    std::unique_ptr<EndGatewayEntity> gatewayEntity;
};

TEST_F(EndGatewayEntityTest, Constructor_InitializesDefaultState)
{
    EXPECT_EQ(gatewayEntity->getType(), BlockEntityType::EndGateway);
    EXPECT_EQ(gatewayEntity->getPos(), BlockPos(100, 50, 0));
    EXPECT_EQ(gatewayEntity->getAge(), 0);
    EXPECT_EQ(gatewayEntity->getTeleportCooldown(), 0);
    EXPECT_FALSE(gatewayEntity->getExitPortal().has_value());
    EXPECT_FALSE(gatewayEntity->isExactTeleport());
    EXPECT_FALSE(gatewayEntity->isCoolingDown());
    EXPECT_TRUE(gatewayEntity->isSpawning()); // 年龄为0，正在生成
    EXPECT_TRUE(gatewayEntity->needsTick()); // 折跃门需要 tick
}

TEST_F(EndGatewayEntityTest, Constants_CorrectValues)
{
    EXPECT_EQ(EndGatewayEntity::TELEPORT_COOLDOWN, 100);
    EXPECT_EQ(EndGatewayEntity::AUTO_COOLDOWN_INTERVAL, 2400L);
    EXPECT_EQ(EndGatewayEntity::SPAWN_DURATION, 200L);
    EXPECT_EQ(EndGatewayEntity::TRIGGER_COOLDOWN, 40);
}

// ========== 年龄和生成状态测试 ==========

TEST_F(EndGatewayEntityTest, IsSpawning_AgeBelow200_ReturnsTrue)
{
    // 初始年龄为 0，正在生成
    EXPECT_TRUE(gatewayEntity->isSpawning());
}

TEST_F(EndGatewayEntityTest, IsSpawning_AgeAt200_ReturnsFalse)
{
    // 模拟 tick 直到年龄达到 200
    // 注意：需要 Mock IWorld 才能真正测试 tick
    // 这里只测试状态判断逻辑

    // 创建一个年龄已超过生成期的实体
    nlohmann::json data;
    data["id"] = "minecraft:end_gateway";
    data["x"] = 0;
    data["y"] = 0;
    data["z"] = 0;
    data["Age"] = 200;

    auto entity = std::make_unique<EndGatewayEntity>(BlockPos(0, 0, 0));
    entity->load(data);

    EXPECT_FALSE(entity->isSpawning());
}

TEST_F(EndGatewayEntityTest, GetSpawnPercent_CalculatesCorrectly)
{
    // 年龄 0，进度 0%
    EXPECT_FLOAT_EQ(gatewayEntity->getSpawnPercent(0.0f), 0.0f);

    // 创建年龄为 100 的实体
    nlohmann::json data;
    data["id"] = "minecraft:end_gateway";
    data["x"] = 0;
    data["y"] = 0;
    data["z"] = 0;
    data["Age"] = 100;

    auto entity = std::make_unique<EndGatewayEntity>(BlockPos(0, 0, 0));
    entity->load(data);

    // 年龄 100，进度 50%
    EXPECT_FLOAT_EQ(entity->getSpawnPercent(0.0f), 0.5f);
}

// ========== 冷却测试 ==========

TEST_F(EndGatewayEntityTest, IsCoolingDown_NoCooldown_ReturnsFalse)
{
    EXPECT_FALSE(gatewayEntity->isCoolingDown());
}

TEST_F(EndGatewayEntityTest, IsCoolingDown_WithCooldown_ReturnsTrue)
{
    nlohmann::json data;
    data["id"] = "minecraft:end_gateway";
    data["x"] = 0;
    data["y"] = 0;
    data["z"] = 0;
    data["TeleportCooldown"] = 50;

    auto entity = std::make_unique<EndGatewayEntity>(BlockPos(0, 0, 0));
    entity->load(data);

    EXPECT_TRUE(entity->isCoolingDown());
}

TEST_F(EndGatewayEntityTest, GetCooldownPercent_CalculatesCorrectly)
{
    // 无冷却时，进度为 100%（冷却完成）
    EXPECT_FLOAT_EQ(gatewayEntity->getCooldownPercent(0.0f), 1.0f);

    // 创建有冷却的实体
    nlohmann::json data;
    data["id"] = "minecraft:end_gateway";
    data["x"] = 0;
    data["y"] = 0;
    data["z"] = 0;
    data["TeleportCooldown"] = 20; // 一半冷却

    auto entity = std::make_unique<EndGatewayEntity>(BlockPos(0, 0, 0));
    entity->load(data);

    // 冷却 20/40 = 50% 剩余，进度 50%
    EXPECT_FLOAT_EQ(entity->getCooldownPercent(0.0f), 0.5f);
}

// ========== 出口传送门设置测试 ==========

TEST_F(EndGatewayEntityTest, SetExitPortal_SetsPosition)
{
    BlockPos exitPos(200, 80, 100);
    gatewayEntity->setExitPortal(exitPos);

    EXPECT_TRUE(gatewayEntity->getExitPortal().has_value());
    EXPECT_EQ(gatewayEntity->getExitPortal().value(), exitPos);
    EXPECT_FALSE(gatewayEntity->isExactTeleport());
}

TEST_F(EndGatewayEntityTest, SetExitPortal_WithExactTeleport)
{
    BlockPos exitPos(200, 80, 100);
    gatewayEntity->setExitPortal(exitPos, true);

    EXPECT_TRUE(gatewayEntity->getExitPortal().has_value());
    EXPECT_EQ(gatewayEntity->getExitPortal().value(), exitPos);
    EXPECT_TRUE(gatewayEntity->isExactTeleport());
}

TEST_F(EndGatewayEntityTest, SetExitPortal_MarksChanged)
{
    // 设置出口应该标记实体已修改
    BlockPos exitPos(200, 80, 100);
    gatewayEntity->setExitPortal(exitPos);

    EXPECT_TRUE(gatewayEntity->isChanged());
}

// ========== 序列化测试 ==========

TEST_F(EndGatewayEntityTest, Save_PreservesBasicInfo)
{
    gatewayEntity->setExitPortal(BlockPos(500, 60, 300), true);

    nlohmann::json data;
    gatewayEntity->save(data);

    EXPECT_EQ(data["id"], "minecraft:end_gateway");
    EXPECT_EQ(data["x"].get<i32>(), 100);
    EXPECT_EQ(data["y"].get<i32>(), 50);
    EXPECT_EQ(data["z"].get<i32>(), 0);
    EXPECT_EQ(data["Age"].get<i64>(), 0);
}

TEST_F(EndGatewayEntityTest, Save_PreservesExitPortal)
{
    gatewayEntity->setExitPortal(BlockPos(500, 60, 300), true);

    nlohmann::json data;
    gatewayEntity->save(data);

    ASSERT_TRUE(data.contains("ExitPortal"));
    EXPECT_EQ(data["ExitPortal"]["X"].get<i32>(), 500);
    EXPECT_EQ(data["ExitPortal"]["Y"].get<i32>(), 60);
    EXPECT_EQ(data["ExitPortal"]["Z"].get<i32>(), 300);
    EXPECT_TRUE(data["ExactTeleport"].get<bool>());
}

TEST_F(EndGatewayEntityTest, Load_PreservesAllData)
{
    // 创建原始数据
    gatewayEntity->setExitPortal(BlockPos(1024, 75, -512), true);

    nlohmann::json data;
    data["Age"] = 500;
    data["TeleportCooldown"] = 30;
    nlohmann::json exitPortal;
    exitPortal["X"] = 1024;
    exitPortal["Y"] = 75;
    exitPortal["Z"] = -512;
    data["ExitPortal"] = std::move(exitPortal);
    data["ExactTeleport"] = true;

    // 加载到新实体
    auto loaded = std::make_unique<EndGatewayEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));

    EXPECT_EQ(loaded->getAge(), 500);
    EXPECT_EQ(loaded->getTeleportCooldown(), 30);
    EXPECT_TRUE(loaded->getExitPortal().has_value());
    EXPECT_EQ(loaded->getExitPortal().value(), BlockPos(1024, 75, -512));
    EXPECT_TRUE(loaded->isExactTeleport());
    EXPECT_TRUE(loaded->isCoolingDown()); // 冷却 > 0
    EXPECT_FALSE(loaded->isSpawning());   // 年龄 > 200
}

TEST_F(EndGatewayEntityTest, Load_WithoutOptionalFields)
{
    nlohmann::json data;
    data["Age"] = 100;

    auto loaded = std::make_unique<EndGatewayEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));

    EXPECT_EQ(loaded->getAge(), 100);
    EXPECT_EQ(loaded->getTeleportCooldown(), 0);
    EXPECT_FALSE(loaded->getExitPortal().has_value());
    EXPECT_FALSE(loaded->isExactTeleport());
}

// ========== 克隆测试 ==========

TEST_F(EndGatewayEntityTest, Clone_CreatesExactCopy)
{
    gatewayEntity->setExitPortal(BlockPos(500, 60, 300), true);

    // 手动设置年龄（通过序列化）
    nlohmann::json data;
    gatewayEntity->save(data);
    data["Age"] = 1000;
    data["TeleportCooldown"] = 50;
    gatewayEntity->load(data);

    auto copy = gatewayEntity->clone();

    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->getType(), BlockEntityType::EndGateway);
    EXPECT_EQ(copy->getPos(), BlockPos(100, 50, 0));

    auto* gatewayCopy = dynamic_cast<EndGatewayEntity*>(copy.get());
    ASSERT_NE(gatewayCopy, nullptr);
    EXPECT_EQ(gatewayCopy->getAge(), 1000);
    EXPECT_EQ(gatewayCopy->getTeleportCooldown(), 50);
    EXPECT_TRUE(gatewayCopy->getExitPortal().has_value());
    EXPECT_EQ(gatewayCopy->getExitPortal().value(), BlockPos(500, 60, 300));
    EXPECT_TRUE(gatewayCopy->isExactTeleport());
}

// ========== 客户端事件测试 ==========

TEST_F(EndGatewayEntityTest, ReceiveClientEvent_Event1_SetsCooldown)
{
    EXPECT_FALSE(gatewayEntity->isCoolingDown());

    bool result = gatewayEntity->receiveClientEvent(1, 0);

    EXPECT_TRUE(result);
    EXPECT_TRUE(gatewayEntity->isCoolingDown());
    EXPECT_EQ(gatewayEntity->getTeleportCooldown(), EndGatewayEntity::TRIGGER_COOLDOWN);
}

TEST_F(EndGatewayEntityTest, ReceiveClientEvent_UnknownEvent_ReturnsFalse)
{
    bool result = gatewayEntity->receiveClientEvent(99, 0);
    EXPECT_FALSE(result);
}
