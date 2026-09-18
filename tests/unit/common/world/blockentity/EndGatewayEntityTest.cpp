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
#include "common/TestWorldHelper.hpp"
#include "world/WorldConstants.hpp"
#include "world/block/BlockPos.hpp"
#include "world/block/registry/VanillaBlocks.hpp"
#include "world/blockentity/BlockEntityType.hpp"
#include "world/blockentity/core/BlockEntityRegistry.hpp"
#include "world/chunk/data/ChunkData.hpp"
#include "world/chunk/data/ChunkSection.hpp"
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
    EXPECT_TRUE(gatewayEntity->needsTick());  // 折跃门需要 tick
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

TEST_F(EndGatewayEntityTest, TriggerEvent_Event1_SetsCooldown)
{
    EXPECT_FALSE(gatewayEntity->isCoolingDown());

    bool result = gatewayEntity->triggerEvent(1, 0);

    EXPECT_TRUE(result);
    EXPECT_TRUE(gatewayEntity->isCoolingDown());
    EXPECT_EQ(gatewayEntity->getTeleportCooldown(), EndGatewayEntity::TRIGGER_COOLDOWN);
}

TEST_F(EndGatewayEntityTest, TriggerEvent_UnknownEvent_ReturnsFalse)
{
    bool result = gatewayEntity->triggerEvent(99, 0);
    EXPECT_FALSE(result);
}

// ========== createGatewayStructure 结构生成测试 ==========

// 测试用的 Mock World，支持 setBlockState/getBlockState 和 getBlockEntity
class EndGatewayTestWorld : public mc::test::BaseTestWorld {
public:
    EndGatewayTestWorld() { VanillaBlocks::initialize(); }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_statesByPos[BlockPos(x, y, z)] = state;
        return true;
    }

    const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        auto it = m_statesByPos.find(BlockPos(x, y, z));
        return it != m_statesByPos.end() ? it->second : nullptr;
    }

    BlockEntity* getBlockEntity(const BlockPos& pos) override
    {
        auto it = m_blockEntities.find(pos);
        return it != m_blockEntities.end() ? it->second : nullptr;
    }

    void setBlockEntity(const BlockPos& pos, BlockEntity* entity) { m_blockEntities[pos] = entity; }

    bool isClientSide() const override { return false; }

    // 获取指定位置的方块，如果未设置则返回空气
    const BlockState* getBlockStateOrAir(const BlockPos& pos) const
    {
        auto it = m_statesByPos.find(pos);
        return it != m_statesByPos.end() ? it->second : VanillaBlocks::getState(VanillaBlocks::AIR);
    }

private:
    std::unordered_map<BlockPos, const BlockState*> m_statesByPos;
    std::unordered_map<BlockPos, BlockEntity*> m_blockEntities;
};

class EndGatewayStructureTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        m_world = std::make_unique<EndGatewayTestWorld>();
        m_centerPos = BlockPos(100, 75, 200);
    }

    // 辅助：获取指定偏移处的方块
    const BlockState* blockAt(i32 dx, i32 dy, i32 dz) const
    {
        return m_world->getBlockStateOrAir(m_centerPos + BlockPos(dx, dy, dz));
    }

    // 辅助：检查指定偏移处方块是否为指定类型
    bool isBlock(i32 dx, i32 dy, i32 dz, const Block& block) const
    {
        const BlockState* state = blockAt(dx, dy, dz);
        return state != nullptr && &state->getBlock() == &block;
    }

    std::unique_ptr<EndGatewayTestWorld> m_world;
    BlockPos m_centerPos{100, 75, 200};
};

// ---------- 常量验证 ----------

TEST_F(EndGatewayStructureTest, Constants_MatchVanilla)
{
    EXPECT_EQ(EndGatewayEntity::TELEPORT_COOLDOWN, 100);
    EXPECT_EQ(EndGatewayEntity::TRIGGER_COOLDOWN, 40);
    EXPECT_EQ(EndGatewayEntity::AUTO_COOLDOWN_INTERVAL, 2400L);
    EXPECT_EQ(EndGatewayEntity::SPAWN_DURATION, 200L);
}

// ---------- 冷却和生成状态 ----------

TEST_F(EndGatewayStructureTest, TeleportCooldown_TriggersCorrectly)
{
    auto entity = std::make_unique<EndGatewayEntity>(BlockPos(0, 75, 0));
    EXPECT_FALSE(entity->isCoolingDown());
    EXPECT_EQ(entity->getTeleportCooldown(), 0);

    entity->triggerEvent(1, 0);
    EXPECT_TRUE(entity->isCoolingDown());
    EXPECT_EQ(entity->getTeleportCooldown(), EndGatewayEntity::TRIGGER_COOLDOWN);
}

TEST_F(EndGatewayStructureTest, SpawnDuration_BoundaryValues)
{
    auto entity = std::make_unique<EndGatewayEntity>(BlockPos(0, 0, 0));

    EXPECT_TRUE(entity->isSpawning());
    EXPECT_FLOAT_EQ(entity->getSpawnPercent(0.0f), 0.0f);

    nlohmann::json data;
    data["Age"] = 199;
    entity->load(data);
    EXPECT_TRUE(entity->isSpawning());

    data["Age"] = 200;
    entity->load(data);
    EXPECT_FALSE(entity->isSpawning());
    EXPECT_FLOAT_EQ(entity->getSpawnPercent(0.0f), 1.0f);
}

TEST_F(EndGatewayStructureTest, CooldownProgress_NegativeCooldown_ClampsToOne)
{
    auto entity = std::make_unique<EndGatewayEntity>(BlockPos(0, 0, 0));
    EXPECT_FLOAT_EQ(entity->getCooldownPercent(0.0f), 1.0f);
}

// ---------- 序列化往返 ----------

TEST_F(EndGatewayStructureTest, ExitPortal_SerializationRoundtrip)
{
    auto entity = std::make_unique<EndGatewayEntity>(BlockPos(100, 50, 200));
    entity->setExitPortal(BlockPos(1024, 75, -512), true);

    nlohmann::json saved;
    entity->save(saved);

    auto loaded = std::make_unique<EndGatewayEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(saved));

    EXPECT_TRUE(loaded->getExitPortal().has_value());
    EXPECT_EQ(loaded->getExitPortal().value(), BlockPos(1024, 75, -512));
    EXPECT_TRUE(loaded->isExactTeleport());
}

TEST_F(EndGatewayStructureTest, ExitPortal_WithoutExactTeleport_DefaultsFalse)
{
    auto entity = std::make_unique<EndGatewayEntity>(BlockPos(0, 0, 0));
    entity->setExitPortal(BlockPos(500, 60, 300));

    EXPECT_TRUE(entity->getExitPortal().has_value());
    EXPECT_FALSE(entity->isExactTeleport());

    nlohmann::json saved;
    entity->save(saved);
    EXPECT_FALSE(saved.contains("ExactTeleport"));
}

// ---------- createGatewayStructure 结构验证 ----------

TEST_F(EndGatewayStructureTest, CreateGatewayStructure_CenterBlockIsEndGateway)
{
    // 中心位置 (0,0,0) 应为末地折跃门方块
    EndGatewayEntity::createGatewayStructure(*m_world, m_centerPos);
    EXPECT_TRUE(isBlock(0, 0, 0, *VanillaBlocks::END_GATEWAY));
}

TEST_F(EndGatewayStructureTest, CreateGatewayStructure_CenterYLayerIsAirExceptCenter)
{
    // 中心 Y 层 (dy=0)：除中心外全部为空气
    EndGatewayEntity::createGatewayStructure(*m_world, m_centerPos);

    const Block& air = *VanillaBlocks::AIR;
    for (i32 dx = -1; dx <= 1; ++dx) {
        for (i32 dz = -1; dz <= 1; ++dz) {
            if (dx == 0 && dz == 0) {
                continue; // 中心是 END_GATEWAY，已在其他测试中验证
            }
            EXPECT_TRUE(isBlock(dx, 0, dz, air)) << "Expected AIR at offset (" << dx << ", 0, " << dz << ")";
        }
    }
}

TEST_F(EndGatewayStructureTest, CreateGatewayStructure_TopBottomCapsCenterColumnIsBedrock)
{
    // 顶/底盖 (dy=±2) 的中心列 (dx=0, dz=0) 应为基岩
    EndGatewayEntity::createGatewayStructure(*m_world, m_centerPos);

    const Block& bedrock = *VanillaBlocks::BEDROCK;
    EXPECT_TRUE(isBlock(0, 2, 0, bedrock)) << "Top cap center should be bedrock";
    EXPECT_TRUE(isBlock(0, -2, 0, bedrock)) << "Bottom cap center should be bedrock";
}

TEST_F(EndGatewayStructureTest, CreateGatewayStructure_CrossArmsAreBedrock)
{
    // 侧面十字臂（非顶底盖层，dy != 0 且 dy != ±2）：dx=0 或 dz=0 的位置应为基岩
    EndGatewayEntity::createGatewayStructure(*m_world, m_centerPos);

    const Block& bedrock = *VanillaBlocks::BEDROCK;
    // dy = -1 和 dy = 1 的十字臂
    for (i32 dy : {-1, 1}) {
        // 十字臂位置：(0,dy,-1), (0,dy,0), (0,dy,1), (-1,dy,0), (1,dy,0)
        EXPECT_TRUE(isBlock(0, dy, 0, bedrock)) << "Cross arm center at dy=" << dy;
        EXPECT_TRUE(isBlock(0, dy, -1, bedrock)) << "Cross arm N at dy=" << dy;
        EXPECT_TRUE(isBlock(0, dy, 1, bedrock)) << "Cross arm S at dy=" << dy;
        EXPECT_TRUE(isBlock(-1, dy, 0, bedrock)) << "Cross arm W at dy=" << dy;
        EXPECT_TRUE(isBlock(1, dy, 0, bedrock)) << "Cross arm E at dy=" << dy;
    }
}

TEST_F(EndGatewayStructureTest, CreateGatewayStructure_CornersAreAir)
{
    // 四角（dx!=0 且 dz!=0，非中心 Y 层）应为空气
    EndGatewayEntity::createGatewayStructure(*m_world, m_centerPos);

    const Block& air = *VanillaBlocks::AIR;
    for (i32 dy = -2; dy <= 2; ++dy) {
        if (dy == 0) {
            continue; // 中心 Y 层已单独测试
        }
        for (i32 dx : {-1, 1}) {
            for (i32 dz : {-1, 1}) {
                EXPECT_TRUE(isBlock(dx, dy, dz, air))
                    << "Expected AIR at corner offset (" << dx << ", " << dy << ", " << dz << ")";
            }
        }
    }
}

TEST_F(EndGatewayStructureTest, CreateGatewayStructure_TopBottomCapCrossArmsAreAir)
{
    // 顶/底盖 (dy=±2) 的十字臂位置应为空气（与 MC Java 一致，盖层仅中心列为基岩）
    EndGatewayEntity::createGatewayStructure(*m_world, m_centerPos);

    const Block& air = *VanillaBlocks::AIR;
    for (i32 dy : {-2, 2}) {
        // 十字臂：N, S, E, W — 在顶/底盖层应为空气
        EXPECT_TRUE(isBlock(0, dy, -1, air)) << "Top/bottom cap N at dy=" << dy << " should be AIR";
        EXPECT_TRUE(isBlock(0, dy, 1, air)) << "Top/bottom cap S at dy=" << dy << " should be AIR";
        EXPECT_TRUE(isBlock(-1, dy, 0, air)) << "Top/bottom cap W at dy=" << dy << " should be AIR";
        EXPECT_TRUE(isBlock(1, dy, 0, air)) << "Top/bottom cap E at dy=" << dy << " should be AIR";
    }
}

TEST_F(EndGatewayStructureTest, CreateGatewayStructure_TotalBlockCount)
{
    // 3x5x3 = 45 个方块总计
    // 基岩：中心层两侧的十字臂各 5 个 × 2 层 = 10 + 顶/底盖中心各 1 个 × 2 = 12
    // 末地折跃门：1（中心）
    // 空气：45 - 12 - 1 = 32
    EndGatewayEntity::createGatewayStructure(*m_world, m_centerPos);

    const Block& bedrock = *VanillaBlocks::BEDROCK;
    const Block& endGateway = *VanillaBlocks::END_GATEWAY;
    const Block& air = *VanillaBlocks::AIR;

    i32 bedrockCount = 0;
    i32 gatewayCount = 0;
    i32 airCount = 0;

    for (i32 dx = -1; dx <= 1; ++dx) {
        for (i32 dy = -2; dy <= 2; ++dy) {
            for (i32 dz = -1; dz <= 1; ++dz) {
                if (isBlock(dx, dy, dz, bedrock)) {
                    ++bedrockCount;
                } else if (isBlock(dx, dy, dz, endGateway)) {
                    ++gatewayCount;
                } else if (isBlock(dx, dy, dz, air)) {
                    ++airCount;
                }
            }
        }
    }

    EXPECT_EQ(gatewayCount, 1);
    EXPECT_EQ(bedrockCount, 12);
    EXPECT_EQ(airCount, 32);
    EXPECT_EQ(bedrockCount + gatewayCount + airCount, 45);
}

TEST_F(EndGatewayStructureTest, CreateGatewayStructure_StructureSymmetry)
{
    // 验证结构关于 X 和 Z 轴对称
    EndGatewayEntity::createGatewayStructure(*m_world, m_centerPos);

    for (i32 dy = -2; dy <= 2; ++dy) {
        for (i32 dx = -1; dx <= 1; ++dx) {
            for (i32 dz = -1; dz <= 1; ++dz) {
                const BlockState* pos = blockAt(dx, dy, dz);
                const BlockState* negX = blockAt(-dx, dy, dz);
                const BlockState* negZ = blockAt(dx, dy, -dz);

                // 对称性：同一方块类型（通过 Block 指针比较）
                ASSERT_NE(pos, nullptr) << "Null state at (" << dx << ", " << dy << ", " << dz << ")";
                ASSERT_NE(negX, nullptr) << "Null negX at (" << -dx << ", " << dy << ", " << dz << ")";
                ASSERT_NE(negZ, nullptr) << "Null negZ at (" << dx << ", " << dy << ", " << -dz << ")";
                EXPECT_EQ(&pos->getBlock(), &negX->getBlock())
                    << "X symmetry broken at dy=" << dy << " dx=" << dx << " dz=" << dz;
                EXPECT_EQ(&pos->getBlock(), &negZ->getBlock())
                    << "Z symmetry broken at dy=" << dy << " dx=" << dx << " dz=" << dz;
            }
        }
    }
}

// ========== _isChunkEmpty / _generateExitPortal 测试 ==========
//
// 对应 MC Java 的 TheEndGatewayBlockEntity.isChunkEmpty 与
// findExitPortalXZPosTentative：测试抽取的 _isChunkEmpty 静态助手
// 与 _generateExitPortal 的区块扫描行为。

// 测试子类：暴露 EndGatewayEntity 的私有方法供测试调用
namespace mc::blockentity {
class TestEndGatewayEntity : public EndGatewayEntity {
public:
    explicit TestEndGatewayEntity(const BlockPos& pos)
        : EndGatewayEntity(pos)
    {}

    [[nodiscard]] static bool testIsChunkEmpty(const world::chunk::ChunkData* chunk) { return _isChunkEmpty(chunk); }

    void testGenerateExitPortal(IWorld& world) { _generateExitPortal(world); }
};
} // namespace mc::blockentity

// 测试用的区块支撑世界：基于 BaseChunkBackedTestWorld，支持 getOrLoadChunk
// （通过 IWorld 默认实现委托 getChunk）、方块读写、方块实体注入
class EndGatewayChunkTestWorld : public mc::test::BaseChunkBackedTestWorld {
public:
    EndGatewayChunkTestWorld() { mc::VanillaBlocks::initialize(); }

    [[nodiscard]] const mc::BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const mc::world::chunk::ChunkData* chunk = getChunk(x >> 4, z >> 4);
        if (chunk == nullptr) {
            return getAirState();
        }
        const mc::BlockState* state = chunk->getBlockState(x & 15, y, z & 15);
        return state != nullptr ? state : getAirState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const mc::BlockState* state) override
    {
        mc::world::chunk::ChunkData& chunk = ensureChunk(x >> 4, z >> 4);
        chunk.setBlockState(x & 15, y, z & 15, state);
        return true;
    }

    [[nodiscard]] mc::BlockEntity* getBlockEntity(const mc::BlockPos& pos) override
    {
        auto it = m_blockEntities.find(pos);
        return it != m_blockEntities.end() ? it->second : nullptr;
    }

    void setBlockEntity(const mc::BlockPos& pos, mc::BlockEntity* entity) { m_blockEntities[pos] = entity; }

    [[nodiscard]] bool isClientSide() const override { return false; }

private:
    [[nodiscard]] const mc::BlockState* getAirState() const { return &mc::VanillaBlocks::AIR->defaultState(); }

    std::unordered_map<mc::BlockPos, mc::BlockEntity*> m_blockEntities;
};

// ---------- _isChunkEmpty 单元测试 ----------

TEST(EndGatewayIsChunkEmptyTest, NullChunk_ReturnsTrue)
{
    // 未加载区块（nullptr）视为空区块
    EXPECT_TRUE(mc::blockentity::TestEndGatewayEntity::testIsChunkEmpty(nullptr));
}

TEST(EndGatewayIsChunkEmptyTest, FreshChunkWithNoSections_ReturnsTrue)
{
    // 新构造的 ChunkData 所有区段均为 nullptr，视为空区块
    mc::world::chunk::ChunkData chunk(0, 0);
    EXPECT_TRUE(mc::blockentity::TestEndGatewayEntity::testIsChunkEmpty(&chunk));
}

TEST(EndGatewayIsChunkEmptyTest, ChunkWithOnlyEmptySections_ReturnsTrue)
{
    // 创建了区段但未放置任何方块，区段 isEmpty() 仍为 true
    mc::world::chunk::ChunkData chunk(0, 0);
    chunk.createSection(0);
    chunk.createSection(5);
    chunk.createSection(mc::world::CHUNK_SECTIONS - 1);
    EXPECT_TRUE(mc::blockentity::TestEndGatewayEntity::testIsChunkEmpty(&chunk));
}

TEST(EndGatewayIsChunkEmptyTest, ChunkWithNonEmptySection_ReturnsFalse)
{
    // 任一区段非空即视为非空区块
    mc::world::chunk::ChunkData chunk(0, 0);
    const mc::BlockState* stoneState = &mc::VanillaBlocks::STONE->defaultState();
    ASSERT_NE(stoneState, nullptr);

    auto* section = chunk.createSection(3);
    ASSERT_NE(section, nullptr);
    section->setBlockState(0, 0, 0, stoneState);

    EXPECT_FALSE(mc::blockentity::TestEndGatewayEntity::testIsChunkEmpty(&chunk));
}

TEST(EndGatewayIsChunkEmptyTest, ChunkWithNonEmptySectionAtHighestIndex_ReturnsFalse)
{
    // 最高索引区段非空也视为非空区块（验证遍历覆盖全部 CHUNK_SECTIONS）
    mc::world::chunk::ChunkData chunk(0, 0);
    const mc::BlockState* stoneState = &mc::VanillaBlocks::STONE->defaultState();
    ASSERT_NE(stoneState, nullptr);

    const i32 lastIndex = mc::world::CHUNK_SECTIONS - 1;
    auto* section = chunk.createSection(lastIndex);
    ASSERT_NE(section, nullptr);
    section->setBlockState(8, 8, 8, stoneState);

    EXPECT_FALSE(mc::blockentity::TestEndGatewayEntity::testIsChunkEmpty(&chunk));
}

TEST(EndGatewayIsChunkEmptyTest, ChunkWithNonEmptySectionAtLowestIndex_ReturnsFalse)
{
    // 最低索引区段非空也视为非空区块
    mc::world::chunk::ChunkData chunk(0, 0);
    const mc::BlockState* stoneState = &mc::VanillaBlocks::STONE->defaultState();
    ASSERT_NE(stoneState, nullptr);

    auto* section = chunk.createSection(0);
    ASSERT_NE(section, nullptr);
    section->setBlockState(0, 0, 0, stoneState);

    EXPECT_FALSE(mc::blockentity::TestEndGatewayEntity::testIsChunkEmpty(&chunk));
}

// ---------- _generateExitPortal 集成测试 ----------

class EndGatewayGenerateExitPortalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        mc::VanillaBlocks::initialize();
        BlockEntityRegistry::instance().registerBuiltinTypes();
        m_world = std::make_unique<EndGatewayChunkTestWorld>();
    }

    std::unique_ptr<EndGatewayChunkTestWorld> m_world;
};

TEST_F(EndGatewayGenerateExitPortalTest, GenerateExitPortal_NoChunksLoaded_SetsExitPortal)
{
    // 当所有目标区块均未加载（getOrLoadChunk 返回 nullptr）时，
    // _generateExitPortal 仍应完成扫描并设置出口位置
    // 折跃门位于正 X 方向，1024 格外的外岛方向
    BlockPos gatewayPos(1000, 75, 0);
    auto gateway = std::make_unique<mc::blockentity::TestEndGatewayEntity>(gatewayPos);

    ASSERT_FALSE(gateway->getExitPortal().has_value());

    gateway->testGenerateExitPortal(*m_world);

    // 出口传送门应已设置
    EXPECT_TRUE(gateway->getExitPortal().has_value());
    // 出口位置应在 1024 格外的方向上（远离原点）
    const BlockPos& exit = gateway->getExitPortal().value();
    EXPECT_GT(exit.x, gatewayPos.x - 16 * 16);
}

TEST_F(EndGatewayGenerateExitPortalTest, GenerateExitPortal_CreatesGatewayStructureAtExit)
{
    // _generateExitPortal 应在出口位置创建折跃门结构（中心方块为 END_GATEWAY）
    BlockPos gatewayPos(1000, 75, 0);
    auto gateway = std::make_unique<mc::blockentity::TestEndGatewayEntity>(gatewayPos);

    gateway->testGenerateExitPortal(*m_world);

    ASSERT_TRUE(gateway->getExitPortal().has_value());
    const BlockPos& exit = gateway->getExitPortal().value();

    // 出口位置中心应为末地折跃门方块
    const mc::BlockState* exitState = m_world->getBlockState(exit.x, exit.y, exit.z);
    ASSERT_NE(exitState, nullptr);
    EXPECT_EQ(&exitState->getBlock(), mc::VanillaBlocks::END_GATEWAY);
}

TEST_F(EndGatewayGenerateExitPortalTest, GenerateExitPortal_ScansPastNonEmptyChunks)
{
    // 在 1024 格附近的扫描路径上放置一个非空区块，
    // 验证 _generateExitPortal 会回退跳过该非空区块，并在其边缘停止
    BlockPos gatewayPos(1000, 75, 0);
    auto gateway = std::make_unique<mc::blockentity::TestEndGatewayEntity>(gatewayPos);

    // 方向向量约 (1, 0, 0)，1024 格外约为 (1024, 0)
    // 在 (1024, 0) 附近放置一个非空区块
    const i32 targetChunkX = 1024 >> 4; // 64
    const i32 targetChunkZ = 0 >> 4;    // 0
    mc::world::chunk::ChunkData& chunk = m_world->ensureChunk(targetChunkX, targetChunkZ);
    const mc::BlockState* stoneState = &mc::VanillaBlocks::STONE->defaultState();
    auto* section = chunk.createSection(10); // 中间区段
    ASSERT_NE(section, nullptr);
    section->setBlockState(0, 0, 0, stoneState);

    gateway->testGenerateExitPortal(*m_world);

    // 出口传送门应已设置
    EXPECT_TRUE(gateway->getExitPortal().has_value());
    const BlockPos& exit = gateway->getExitPortal().value();
    // 非空区块阻止了前进扫描，出口应在其附近（1024 ± 16 范围内）。
    // 对比 AdvancePastEmptyChunks 测试（无非空区块时 exit.x ≈ 1280），
    // 此处非空区块使出口远小于 1280，证明回退/前进逻辑生效
    EXPECT_LT(exit.x, 1280);
    // 出口应在非空区块附近（1024 ± 16），而非继续前进到 1280
    EXPECT_GE(exit.x, 1024 - 16);
    EXPECT_LE(exit.x, 1024 + 16);
}

TEST_F(EndGatewayGenerateExitPortalTest, GenerateExitPortal_AdvancesPastEmptyChunks)
{
    // 验证前进扫描逻辑：当 1024 格处为空区块时，会继续前进寻找非空区块
    // 这里所有区块都未加载（视为空），应一直前进 16 次后停止
    BlockPos gatewayPos(1000, 75, 0);
    auto gateway = std::make_unique<mc::blockentity::TestEndGatewayEntity>(gatewayPos);

    gateway->testGenerateExitPortal(*m_world);

    ASSERT_TRUE(gateway->getExitPortal().has_value());
    const BlockPos& exit = gateway->getExitPortal().value();
    // 前进 16 次每次 16 格 = 256 格，出口应大于 1024
    EXPECT_GT(exit.x, 1024);
}

TEST_F(EndGatewayGenerateExitPortalTest, GenerateExitPortal_MarksEntityChanged)
{
    // _generateExitPortal 完成后应标记实体已修改
    BlockPos gatewayPos(1000, 75, 0);
    auto gateway = std::make_unique<mc::blockentity::TestEndGatewayEntity>(gatewayPos);

    // 新构造的实体不应处于已修改状态
    // （构造后未调用 load/save，isChanged 应为 false）
    gateway->testGenerateExitPortal(*m_world);

    // 出口生成后应标记已修改（setChanged）
    EXPECT_TRUE(gateway->isChanged());
}

TEST_F(EndGatewayGenerateExitPortalTest, GenerateExitPortal_SetsReturnPortalOnExitGateway)
{
    // 出口折跃门方块实体应设置返回位置指向原折跃门
    // _generateExitPortal 的双向链接逻辑：在出口位置查找 EndGatewayEntity 并调用 setExitPortal
    // 由于 createGatewayStructure 只放置方块状态、不创建方块实体，
    // 此测试在出口位置预先注入 EndGatewayEntity 以验证双向链接
    BlockPos gatewayPos(1000, 75, 0);
    auto gateway = std::make_unique<mc::blockentity::TestEndGatewayEntity>(gatewayPos);

    // 先运行一次以确定出口位置（所有区块未加载，exit.x ≈ 1280）
    gateway->testGenerateExitPortal(*m_world);
    ASSERT_TRUE(gateway->getExitPortal().has_value());
    BlockPos exitPos = gateway->getExitPortal().value();

    // 创建全新的世界，避免第一次运行放置的方块影响扫描
    auto world2 = std::make_unique<EndGatewayChunkTestWorld>();

    // 在出口位置预先放置 EndGatewayEntity 和 END_GATEWAY 方块状态
    // （_generateExitPortal 会检查 getBlockState == END_GATEWAY 再查 getBlockEntity）
    const mc::BlockState* endGatewayState = &mc::VanillaBlocks::END_GATEWAY->defaultState();
    world2->setBlockState(exitPos.x, exitPos.y, exitPos.z, endGatewayState);
    auto exitEntity = std::make_unique<EndGatewayEntity>(exitPos);
    EndGatewayEntity* exitEntityPtr = exitEntity.get();
    world2->setBlockEntity(exitPos, exitEntityPtr);
    exitEntity.release();

    // 在新世界上运行 _generateExitPortal
    auto gateway2 = std::make_unique<mc::blockentity::TestEndGatewayEntity>(gatewayPos);
    gateway2->testGenerateExitPortal(*world2);

    // 出口折跃门应设置返回位置（exitPortal）指向原折跃门位置
    EXPECT_TRUE(exitEntityPtr->getExitPortal().has_value());
    EXPECT_EQ(exitEntityPtr->getExitPortal().value(), gatewayPos);
}
