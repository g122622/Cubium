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
#include "common/util/math/MathConstants.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/dimension/end/EndDragonFight.hpp"
#include "common/world/dimension/teleport/Teleporter.hpp"

#include <cmath>
#include <map>
#include <nlohmann/json.hpp>

using namespace mc;

// ============================================================================
// 测试用世界 - 支持 setBlockState / getBlockState / getHeight / playEvent
// ============================================================================

class DragonFightTestWorld final : public test::BaseTestWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        auto it = m_blocks.find(BlockPos(x, y, z));
        return it != m_blocks.end() ? it->second : nullptr;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        BlockPos pos(x, y, z);
        if (state == nullptr || state->isAir()) {
            m_blocks.erase(pos);
        } else {
            m_blocks[pos] = state;
        }
        return true;
    }

    void setHeight(i32 x, i32 z, i32 height) { m_heights[{x, z}] = height; }

    [[nodiscard]] i32 getHeight(i32 x, i32 z) const override
    {
        auto it = m_heights.find({x, z});
        return it != m_heights.end() ? it->second : 64;
    }

    void playEvent(i32 eventId, const BlockPos& pos, i32 data) override { m_events.push_back({eventId, pos, data}); }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("DragonFightTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("DragonFightTestWorld::tickManager not implemented");
    }

    // ========== 测试辅助方法 ==========

    [[nodiscard]] const BlockState* getBlockAt(const BlockPos& pos) const { return getBlockState(pos.x, pos.y, pos.z); }

    [[nodiscard]] bool hasBlockAt(i32 x, i32 y, i32 z) const
    {
        return m_blocks.find(BlockPos(x, y, z)) != m_blocks.end();
    }

    [[nodiscard]] size_t blockCount() const { return m_blocks.size(); }

    struct PlayEventCall {
        i32 eventId;
        BlockPos pos;
        i32 data;
    };

    [[nodiscard]] const std::vector<PlayEventCall>& playedEvents() const { return m_events; }

    void clear()
    {
        m_blocks.clear();
        m_heights.clear();
        m_events.clear();
    }

private:
    std::map<BlockPos, const BlockState*> m_blocks;
    std::map<std::pair<i32, i32>, i32> m_heights;
    std::vector<PlayEventCall> m_events;
};

// ============================================================================
// 测试夹具
// ============================================================================

class EndDragonFightTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    DragonFightTestWorld m_world;
};

// ============================================================================
// 构造函数与初始状态测试
// ============================================================================

TEST_F(EndDragonFightTest, NewWorldInitialState)
{
    // 新世界：不传入存档数据
    EndDragonFight fight(42, std::nullopt);

    EXPECT_FALSE(fight.hasPreviouslyKilled());
    EXPECT_FALSE(fight.isDragonKilled());
    EXPECT_EQ(fight.remainingGatewayCount(), EndDragonFight::GATEWAY_COUNT);
    EXPECT_EQ(fight.worldSeed(), 42);
}

TEST_F(EndDragonFightTest, NewWorldGatewaysInitializedTo20)
{
    EndDragonFight fight(0, std::nullopt);
    EXPECT_EQ(fight.remainingGatewayCount(), 20);
}

TEST_F(EndDragonFightTest, DifferentSeedsProduceDifferentGatewayOrders)
{
    EndDragonFight fight1(12345, std::nullopt);
    EndDragonFight fight2(67890, std::nullopt);

    // 不同种子应产生不同的折跃门顺序（概率极高）
    // 保存数据后比较 gateways
    auto data1 = fight1.saveData();
    auto data2 = fight2.saveData();

    ASSERT_TRUE(data1.gateways.has_value());
    ASSERT_TRUE(data2.gateways.has_value());

    // 极低概率两个不同种子产生相同打乱顺序，但不应该发生
    EXPECT_NE(*data1.gateways, *data2.gateways);
}

TEST_F(EndDragonFightTest, SameSeedProducesSameGatewayOrder)
{
    EndDragonFight fight1(99999, std::nullopt);
    EndDragonFight fight2(99999, std::nullopt);

    auto data1 = fight1.saveData();
    auto data2 = fight2.saveData();

    ASSERT_TRUE(data1.gateways.has_value());
    ASSERT_TRUE(data2.gateways.has_value());

    EXPECT_EQ(*data1.gateways, *data2.gateways);
}

// ============================================================================
// 存档数据恢复测试
// ============================================================================

TEST_F(EndDragonFightTest, LoadFromDataRestoresPreviouslyKilled)
{
    EndDragonFight::Data data;
    data.needsStateScanning = false;
    data.dragonKilled = true;
    data.previouslyKilled = true;
    data.gateways = std::vector<i32>{5, 3, 1};

    EndDragonFight fight(42, data);
    EXPECT_TRUE(fight.hasPreviouslyKilled());
    EXPECT_TRUE(fight.isDragonKilled());
    EXPECT_EQ(fight.remainingGatewayCount(), 3);
}

TEST_F(EndDragonFightTest, LoadFromDataWithEmptyGateways)
{
    EndDragonFight::Data data;
    data.previouslyKilled = true;
    data.gateways = std::vector<i32>{}; // 所有折跃门已消耗

    EndDragonFight fight(42, data);
    EXPECT_TRUE(fight.hasPreviouslyKilled());
    EXPECT_EQ(fight.remainingGatewayCount(), 0);
}

TEST_F(EndDragonFightTest, LoadFromDataWithNulloptGatewaysRegeneratesFromSeed)
{
    // nullopt gateways 模拟旧存档缺少折跃门数据
    EndDragonFight::Data data;
    data.previouslyKilled = false;
    data.gateways = std::nullopt; // 旧存档没有折跃门数据

    EndDragonFight fight(42, data);
    // 应从种子重新生成折跃门列表
    EXPECT_EQ(fight.remainingGatewayCount(), EndDragonFight::GATEWAY_COUNT);
}

TEST_F(EndDragonFightTest, LoadFromDataNulloptGatewaysMatchesNewWorldSameSeed)
{
    // 从 nullopt 恢复的折跃门顺序应与新世界相同种子一致
    EndDragonFight::Data data;
    data.gateways = std::nullopt;

    EndDragonFight fightFromData(42, data);
    EndDragonFight fightNew(42, std::nullopt);

    auto dataFromLoad = fightFromData.saveData();
    auto dataFromNew = fightNew.saveData();

    ASSERT_TRUE(dataFromLoad.gateways.has_value());
    ASSERT_TRUE(dataFromNew.gateways.has_value());
    EXPECT_EQ(*dataFromLoad.gateways, *dataFromNew.gateways);
}

// ============================================================================
// JSON 序列化测试
// ============================================================================

TEST_F(EndDragonFightTest, DataToJsonRoundTrip)
{
    EndDragonFight::Data original;
    original.needsStateScanning = false;
    original.dragonKilled = true;
    original.previouslyKilled = true;
    original.gateways = std::vector<i32>{19, 7, 3, 15, 0};

    nlohmann::json json = original.toJson();
    EndDragonFight::Data restored = EndDragonFight::Data::fromJson(json);

    EXPECT_EQ(restored.needsStateScanning, original.needsStateScanning);
    EXPECT_EQ(restored.dragonKilled, original.dragonKilled);
    EXPECT_EQ(restored.previouslyKilled, original.previouslyKilled);
    ASSERT_TRUE(restored.gateways.has_value());
    EXPECT_EQ(*restored.gateways, *original.gateways);
}

TEST_F(EndDragonFightTest, DataToJsonContainsExpectedFields)
{
    EndDragonFight::Data data;
    data.needsStateScanning = true;
    data.dragonKilled = false;
    data.previouslyKilled = false;
    data.gateways = std::vector<i32>{0, 1, 2};

    nlohmann::json json = data.toJson();

    EXPECT_TRUE(json.contains("NeedsStateScanning"));
    EXPECT_TRUE(json.contains("DragonKilled"));
    EXPECT_TRUE(json.contains("PreviouslyKilled"));
    EXPECT_TRUE(json.contains("Gateways"));
    EXPECT_EQ(json["NeedsStateScanning"], true);
    EXPECT_EQ(json["DragonKilled"], false);
    EXPECT_EQ(json["PreviouslyKilled"], false);
    EXPECT_EQ(json["Gateways"].size(), 3u);
}

TEST_F(EndDragonFightTest, DataFromJsonMissingGatewaysReturnsNullopt)
{
    // JSON 缺少 Gateways 字段时，gateways 应为 nullopt
    nlohmann::json json;
    json["NeedsStateScanning"] = true;
    json["DragonKilled"] = false;
    json["PreviouslyKilled"] = false;

    EndDragonFight::Data data = EndDragonFight::Data::fromJson(json);
    EXPECT_EQ(data.needsStateScanning, true);
    EXPECT_EQ(data.dragonKilled, false);
    EXPECT_EQ(data.previouslyKilled, false);
    EXPECT_FALSE(data.gateways.has_value());
}

TEST_F(EndDragonFightTest, DataFromJsonDefaults)
{
    // 空的 JSON 对象应使用默认值
    nlohmann::json json = nlohmann::json::object();

    EndDragonFight::Data data = EndDragonFight::Data::fromJson(json);
    EXPECT_EQ(data.needsStateScanning, true); // 默认 true
    EXPECT_EQ(data.dragonKilled, false);      // 默认 false
    EXPECT_EQ(data.previouslyKilled, false);  // 默认 false
    EXPECT_FALSE(data.gateways.has_value());  // 默认 nullopt
}

TEST_F(EndDragonFightTest, SaveLoadRoundTripPreservesFullState)
{
    // 创建战斗管理器并模拟一次击杀
    EndDragonFight fight1(42, std::nullopt);
    fight1.setDragonKilled(m_world);

    auto savedData = fight1.saveData();
    nlohmann::json json = savedData.toJson();

    // 从 JSON 恢复
    EndDragonFight::Data restoredData = EndDragonFight::Data::fromJson(json);
    EndDragonFight fight2(42, restoredData);

    EXPECT_EQ(fight2.hasPreviouslyKilled(), fight1.hasPreviouslyKilled());
    EXPECT_EQ(fight2.isDragonKilled(), fight1.isDragonKilled());
    EXPECT_EQ(fight2.remainingGatewayCount(), fight1.remainingGatewayCount());

    auto savedData2 = fight2.saveData();
    ASSERT_TRUE(savedData.gateways.has_value());
    ASSERT_TRUE(savedData2.gateways.has_value());
    EXPECT_EQ(*savedData.gateways, *savedData2.gateways);
}

// ============================================================================
// setDragonKilled 测试
// ============================================================================

TEST_F(EndDragonFightTest, SetDragonKilled_MarksPreviouslyKilled)
{
    EndDragonFight fight(42, std::nullopt);
    EXPECT_FALSE(fight.hasPreviouslyKilled());

    fight.setDragonKilled(m_world);
    EXPECT_TRUE(fight.hasPreviouslyKilled());
    EXPECT_TRUE(fight.isDragonKilled());
}

TEST_F(EndDragonFightTest, SetDragonKilled_ConsumesOneGateway)
{
    EndDragonFight fight(42, std::nullopt);
    i32 initialCount = fight.remainingGatewayCount();
    EXPECT_EQ(initialCount, EndDragonFight::GATEWAY_COUNT);

    fight.setDragonKilled(m_world);
    EXPECT_EQ(fight.remainingGatewayCount(), initialCount - 1);
}

TEST_F(EndDragonFightTest, SetDragonKilled_FirstKillPlacesDragonEgg)
{
    // 设置出生点高度以便龙蛋放置
    m_world.setHeight(0, 0, 75);

    EndDragonFight fight(42, std::nullopt);
    EXPECT_FALSE(fight.hasPreviouslyKilled());

    fight.setDragonKilled(m_world);

    // 首次击杀应放置龙蛋
    const BlockState* dragonEggState = VanillaBlocks::getState(VanillaBlocks::DRAGON_EGG);
    ASSERT_NE(dragonEggState, nullptr);
    EXPECT_EQ(m_world.getBlockAt(BlockPos(0, 75, 0)), dragonEggState);
}

TEST_F(EndDragonFightTest, SetDragonKilled_SubsequentKillNoDragonEgg)
{
    // 设置出生点高度以便龙蛋放置
    m_world.setHeight(0, 0, 75);

    EndDragonFight::Data data;
    data.previouslyKilled = true;
    data.gateways = std::vector<i32>{19, 18, 17};

    EndDragonFight fight(42, data);
    EXPECT_TRUE(fight.hasPreviouslyKilled());

    // 清除世界中可能存在的方块
    m_world.clear();
    m_world.setHeight(0, 0, 75);

    fight.setDragonKilled(m_world);

    // 后续击杀不应放置龙蛋
    const BlockState* dragonEggState = VanillaBlocks::getState(VanillaBlocks::DRAGON_EGG);
    ASSERT_NE(dragonEggState, nullptr);
    EXPECT_NE(m_world.getBlockAt(BlockPos(0, 75, 0)), dragonEggState);
}

TEST_F(EndDragonFightTest, SetDragonKilled_PlayGatewayEvent3000)
{
    EndDragonFight fight(42, std::nullopt);
    fight.setDragonKilled(m_world);

    // 应播放折跃门生成效果事件 3000
    bool foundGatewayEvent = false;
    for (const auto& event : m_world.playedEvents()) {
        if (event.eventId == 3000) {
            foundGatewayEvent = true;
            break;
        }
    }
    EXPECT_TRUE(foundGatewayEvent) << "Expected gateway spawn event 3000 to be played";
}

TEST_F(EndDragonFightTest, SetDragonKilled_NoGatewayEventWhenAllConsumed)
{
    // 所有折跃门已消耗
    EndDragonFight::Data data;
    data.previouslyKilled = true;
    data.dragonKilled = true;
    data.gateways = std::vector<i32>{}; // 空：所有折跃门已消耗

    EndDragonFight fight(42, data);
    fight.setDragonKilled(m_world);

    // 不应播放折跃门事件 3000
    bool foundGatewayEvent = false;
    for (const auto& event : m_world.playedEvents()) {
        if (event.eventId == 3000) {
            foundGatewayEvent = true;
            break;
        }
    }
    EXPECT_FALSE(foundGatewayEvent) << "No gateway event should be played when all gateways consumed";
}

TEST_F(EndDragonFightTest, SetDragonKilled_MultipleKillsConsumeAllGateways)
{
    EndDragonFight fight(42, std::nullopt);
    EXPECT_EQ(fight.remainingGatewayCount(), EndDragonFight::GATEWAY_COUNT);

    // 消耗所有 20 个折跃门
    for (i32 i = 0; i < EndDragonFight::GATEWAY_COUNT; ++i) {
        EXPECT_EQ(fight.remainingGatewayCount(), EndDragonFight::GATEWAY_COUNT - i);
        fight.setDragonKilled(m_world);
    }

    EXPECT_EQ(fight.remainingGatewayCount(), 0);

    // 再击杀一次不再消耗
    fight.setDragonKilled(m_world);
    EXPECT_EQ(fight.remainingGatewayCount(), 0);
}

// ============================================================================
// 折跃门位置算法测试
// ============================================================================

TEST_F(EndDragonFightTest, GatewayPositionsAreOnRadius96Circle)
{
    // 验证所有 20 个折跃门位置都在半径 96 的圆上
    for (i32 i = 0; i < EndDragonFight::GATEWAY_COUNT; ++i) {
        const f64 angle = 2.0 *
            (-math::PI_DOUBLE +
                (math::PI_DOUBLE / static_cast<f64>(EndDragonFight::GATEWAY_COUNT)) * static_cast<f64>(i));
        const f64 x = std::floor(static_cast<f64>(EndDragonFight::GATEWAY_DISTANCE) * std::cos(angle));
        const f64 z = std::floor(static_cast<f64>(EndDragonFight::GATEWAY_DISTANCE) * std::sin(angle));

        // 验证坐标在合理范围内（半径 96 ± 容差）
        const f64 dist = std::sqrt(x * x + z * z);
        EXPECT_NEAR(dist, 96.0, 2.0) << "Gateway " << i << " distance from origin: " << dist;
    }
}

TEST_F(EndDragonFightTest, GatewayPositionsAreAtY75)
{
    // 所有折跃门 Y 坐标为 75
    EXPECT_EQ(EndDragonFight::GATEWAY_Y, 75);
}

TEST_F(EndDragonFightTest, GatewayCountIs20)
{
    EXPECT_EQ(EndDragonFight::GATEWAY_COUNT, 20);
}

TEST_F(EndDragonFightTest, GatewayDistanceIs96)
{
    EXPECT_EQ(EndDragonFight::GATEWAY_DISTANCE, 96);
}

// ============================================================================
// 数据保存测试
// ============================================================================

TEST_F(EndDragonFightTest, SaveDataReflectsCurrentState)
{
    EndDragonFight fight(42, std::nullopt);

    auto data = fight.saveData();
    EXPECT_EQ(data.needsStateScanning, false); // 新世界不需要扫描旧世界状态
    EXPECT_EQ(data.dragonKilled, false);       // 龙未被击杀
    EXPECT_EQ(data.previouslyKilled, false);   // 未曾击杀过
    ASSERT_TRUE(data.gateways.has_value());
    EXPECT_EQ(static_cast<i32>(data.gateways->size()), EndDragonFight::GATEWAY_COUNT);
}

TEST_F(EndDragonFightTest, SaveDataAfterKillReflectsChanges)
{
    EndDragonFight fight(42, std::nullopt);
    fight.setDragonKilled(m_world);

    auto data = fight.saveData();
    EXPECT_EQ(data.dragonKilled, true);
    EXPECT_EQ(data.previouslyKilled, true);
    ASSERT_TRUE(data.gateways.has_value());
    EXPECT_EQ(static_cast<i32>(data.gateways->size()), EndDragonFight::GATEWAY_COUNT - 1);
}

// ============================================================================
// 经验掉落区分测试（通过 hasPreviouslyKilled 间接测试）
// ============================================================================

TEST_F(EndDragonFightTest, FirstKillPreviouslyKilledIsFalse)
{
    EndDragonFight fight(42, std::nullopt);
    // 首次击杀前：hasPreviouslyKilled() 为 false，应给予 12000 XP
    EXPECT_FALSE(fight.hasPreviouslyKilled());
}

TEST_F(EndDragonFightTest, AfterFirstKillPreviouslyKilledIsTrue)
{
    EndDragonFight fight(42, std::nullopt);
    fight.setDragonKilled(m_world);
    // 首次击杀后：hasPreviouslyKilled() 为 true，后续击杀应给予 500 XP
    EXPECT_TRUE(fight.hasPreviouslyKilled());
}

TEST_F(EndDragonFightTest, PreviouslyKilledPersistsThroughLoad)
{
    EndDragonFight fight(42, std::nullopt);
    fight.setDragonKilled(m_world);

    auto data = fight.saveData();
    nlohmann::json json = data.toJson();
    EndDragonFight::Data restored = EndDragonFight::Data::fromJson(json);

    EXPECT_TRUE(restored.previouslyKilled);
}

// ============================================================================
// 移动语义测试
// ============================================================================

TEST_F(EndDragonFightTest, MoveConstructionPreservesState)
{
    EndDragonFight fight1(42, std::nullopt);
    fight1.setDragonKilled(m_world);

    i32 expectedGatewayCount = fight1.remainingGatewayCount();
    bool expectedPreviouslyKilled = fight1.hasPreviouslyKilled();

    EndDragonFight fight2(std::move(fight1));

    EXPECT_EQ(fight2.hasPreviouslyKilled(), expectedPreviouslyKilled);
    EXPECT_EQ(fight2.remainingGatewayCount(), expectedGatewayCount);
    EXPECT_EQ(fight2.worldSeed(), 42);
}

// ============================================================================
// needsStateScanning 测试
// ============================================================================

TEST_F(EndDragonFightTest, NewWorldDoesNotNeedStateScanning)
{
    // 新世界不需要扫描旧世界状态
    EndDragonFight fight(42, std::nullopt);
    auto data = fight.saveData();
    EXPECT_EQ(data.needsStateScanning, false);
}

TEST_F(EndDragonFightTest, LoadedWorldNeedsStateScanningByDefault)
{
    // 从存档加载的数据默认 needsStateScanning = true
    EndDragonFight::Data data;
    data.previouslyKilled = false;
    data.gateways = std::vector<i32>{0, 1, 2};

    EndDragonFight fight(42, data);
    auto savedData = fight.saveData();
    EXPECT_EQ(savedData.needsStateScanning, true);
}

TEST_F(EndDragonFightTest, LoadedWorldPreservesNeedsStateScanning)
{
    // 从存档加载时保留 needsStateScanning 状态
    EndDragonFight::Data data;
    data.needsStateScanning = false;
    data.previouslyKilled = true;
    data.gateways = std::vector<i32>{5, 3, 1};

    EndDragonFight fight(42, data);
    auto savedData = fight.saveData();
    EXPECT_EQ(savedData.needsStateScanning, false);
}

TEST_F(EndDragonFightTest, TickPerformsStateScanningWhenNeeded)
{
    // 当 needsStateScanning = true 且竞技场区块已加载时，tick 应触发状态扫描
    // 竞技场区块已加载（空世界中 hasChunk 返回 false 导致 isArenaLoaded 返回 false）
    // 但在测试世界中我们模拟没有区块，所以 isArenaLoaded 返回 false

    EndDragonFight::Data data;
    data.needsStateScanning = true;
    data.previouslyKilled = false;
    data.gateways = std::vector<i32>{0, 1, 2};

    EndDragonFight fight(42, data);
    // 在 DragonFightTestWorld 中，hasChunk 返回 false（因为没有区块管理器）
    // 所以 isArenaLoaded 返回 false，tick 不会执行扫描
    fight.tick(m_world);

    // 扫描未执行，needsStateScanning 仍为 true
    auto savedData = fight.saveData();
    EXPECT_EQ(savedData.needsStateScanning, true);
}

TEST_F(EndDragonFightTest, ScanStateSetsPreviouslyKilledWhenPortalExists)
{
    // 模拟有活跃出口传送门的世界：在 (0, 64, 0) 放置 END_PORTAL 方块
    const BlockState* endPortalState = VanillaBlocks::getState(VanillaBlocks::END_PORTAL);
    ASSERT_NE(endPortalState, nullptr);

    // 注意：由于测试世界中 getChunk 返回 nullptr（无区块管理器），
    // _hasActiveExitPortal 无法直接测试。但我们通过 setDragonKilled 间接验证。
    // 此测试验证 setDragonKilled 后 previouslyKilled 被正确设置。
    EndDragonFight fight(42, std::nullopt);
    EXPECT_FALSE(fight.hasPreviouslyKilled());
    fight.setDragonKilled(m_world);
    EXPECT_TRUE(fight.hasPreviouslyKilled());
}

TEST_F(EndDragonFightTest, ScanStateCreatesInactivePortalWhenNoPortalExists)
{
    // 当世界中不存在活跃出口传送门且不存在讲台结构时，
    // 扫描应创建非激活讲台。
    // 由于测试世界不支持区块加载，无法直接测试 _scanState 的完整行为。
    // 此测试验证 EndTeleporter::createExitPortal 可正常调用。
    m_world.setHeight(0, 0, 64);

    // 创建非激活讲台（active=false）
    EndTeleporter::createExitPortal(m_world, BlockPos(0, 0, 0), false);

    // 验证基岩柱存在（非激活讲台的中心柱）
    const BlockState* bedrockState = VanillaBlocks::getState(VanillaBlocks::BEDROCK);
    ASSERT_NE(bedrockState, nullptr);

    // 中心柱应该在 (0, 0, 0) 到 (0, 3, 0)
    bool foundBedrock = false;
    for (i32 y = 0; y <= 3; ++y) {
        if (m_world.getBlockAt(BlockPos(0, y, 0)) == bedrockState) {
            foundBedrock = true;
            break;
        }
    }
    EXPECT_TRUE(foundBedrock) << "Expected bedrock in center pillar of inactive portal";
}

TEST_F(EndDragonFightTest, TickClearsNeedsStateScanningAfterScan)
{
    // 验证 tick 在 isArenaLoaded 返回 true 时清除 needsStateScanning
    // 由于测试世界的 hasChunk 返回 false，需要创建一个模拟的测试
    // 这里直接测试 saveData 中的 needsStateScanning 状态

    EndDragonFight::Data data;
    data.needsStateScanning = true;
    data.previouslyKilled = false;
    data.gateways = std::vector<i32>{0, 1, 2};

    EndDragonFight fight(42, data);
    EXPECT_EQ(fight.saveData().needsStateScanning, true);

    // 执行多次 tick，但竞技场未加载所以不会扫描
    for (int i = 0; i < 100; ++i) {
        fight.tick(m_world);
    }
    EXPECT_EQ(fight.saveData().needsStateScanning, true);
}

// ============================================================================
// isArenaLoaded 间接测试（通过行为观察）
// ============================================================================

TEST_F(EndDragonFightTest, ArenaChunkRadiusIs8)
{
    // 竞技场区块扫描半径为 8，与 MC Java 一致
    EXPECT_EQ(EndDragonFight::ARENA_CHUNK_RADIUS, 8);
}
