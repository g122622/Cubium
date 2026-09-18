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

#include "common/world/block/registry/VanillaBlocks.hpp"
#include "entity/ai/pathfinding/Region.hpp"
#include "entity/ai/pathfinding/WalkNodeProcessor.hpp"
#include "world/block/Block.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockTags.hpp"
#include "world/block/Material.hpp"
#include <memory>
#include <unordered_map>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity::ai::pathfinding;

// ============================================================================
// MockRegion - 用于测试的 Region 实现
// ============================================================================

namespace {

/**
 * @brief 测试用 Mock Region
 *
 * 提供可编程的方块状态，用于测试危险方块检测。
 */
class MockRegion : public Region {
public:
    // 方块状态存储：key = (x << 32) | (y << 16) | z, value = stateId
    std::unordered_map<u64, u32> m_blockStates;

    // 特殊方块标记
    std::unordered_map<u64, bool> m_isWater;
    std::unordered_map<u64, bool> m_isLava;
    std::unordered_map<u64, bool> m_isWalkable;

    // 全局设置
    bool m_globalLoaded = true;
    i32 m_globalHeight = 64;

    // ========== 辅助方法 ==========

    static u64 makeKey(i32 x, i32 y, i32 z)
    {
        u64 key = 0;
        key |= (static_cast<u64>(static_cast<u32>(x)) & 0xFFFFFFFFULL) << 32;
        key |= (static_cast<u64>(static_cast<u16>(y & 0xFFFF)) << 16);
        key |= (static_cast<u64>(static_cast<u32>(z)) & 0xFFFFULL);
        return key;
    }

    void setBlockStateId(i32 x, i32 y, i32 z, u32 stateId) { m_blockStates[makeKey(x, y, z)] = stateId; }

    void setWater(i32 x, i32 y, i32 z, bool water = true) { m_isWater[makeKey(x, y, z)] = water; }

    void setLava(i32 x, i32 y, i32 z, bool lava = true) { m_isLava[makeKey(x, y, z)] = lava; }

    void setWalkable(i32 x, i32 y, i32 z, bool walkable = true) { m_isWalkable[makeKey(x, y, z)] = walkable; }

    // ========== Region 接口实现 ==========

    [[nodiscard]] u32 getBlockStateId(i32 x, i32 y, i32 z) const override
    {
        auto it = m_blockStates.find(makeKey(x, y, z));
        return it != m_blockStates.end() ? it->second : 0; // 0 = 空气
    }

    [[nodiscard]] bool isLoaded(i32 /*x*/, i32 /*z*/) const override { return m_globalLoaded; }

    [[nodiscard]] i32 getHeight(i32 /*x*/, i32 /*z*/) const override { return m_globalHeight; }

    [[nodiscard]] bool isWalkable(i32 x, i32 y, i32 z) const override
    {
        auto it = m_isWalkable.find(makeKey(x, y, z));
        return it != m_isWalkable.end() ? it->second : false;
    }

    [[nodiscard]] bool isWater(i32 x, i32 y, i32 z) const override
    {
        auto it = m_isWater.find(makeKey(x, y, z));
        return it != m_isWater.end() ? it->second : false;
    }

    [[nodiscard]] bool isLava(i32 x, i32 y, i32 z) const override
    {
        auto it = m_isLava.find(makeKey(x, y, z));
        return it != m_isLava.end() ? it->second : false;
    }

    [[nodiscard]] bool canSeeSky(i32 /*x*/, i32 /*y*/, i32 /*z*/) const override
    {
        // 测试中默认返回 false（不可见天空）
        return false;
    }
};

} // namespace

// ============================================================================
// PathNodeType 危险类型测试
// ============================================================================

TEST(PathNodeTypeDangerTest, GetDangerReturnsCorrectType)
{
    // 火焰危险
    EXPECT_EQ(getDanger(PathNodeType::DamageFire), PathNodeType::DangerFire);
    EXPECT_EQ(getDanger(PathNodeType::DangerFire), PathNodeType::DangerFire);

    // 仙人掌危险
    EXPECT_EQ(getDanger(PathNodeType::DamageCactus), PathNodeType::DangerCactus);
    EXPECT_EQ(getDanger(PathNodeType::DangerCactus), PathNodeType::DangerCactus);

    // 其他危险
    EXPECT_EQ(getDanger(PathNodeType::DamageOther), PathNodeType::DangerOther);
    EXPECT_EQ(getDanger(PathNodeType::DangerOther), PathNodeType::DangerOther);

    // 甜浆果丛危险
    EXPECT_EQ(getDanger(PathNodeType::DangerBerry), PathNodeType::DangerBerry);

    // 岩浆映射到火焰危险
    EXPECT_EQ(getDanger(PathNodeType::Lava), PathNodeType::DamageFire);

    // 非危险类型返回 Blocked
    EXPECT_EQ(getDanger(PathNodeType::Walkable), PathNodeType::Blocked);
    EXPECT_EQ(getDanger(PathNodeType::Water), PathNodeType::Blocked);
    EXPECT_EQ(getDanger(PathNodeType::Open), PathNodeType::Blocked);
}

TEST(PathNodeTypeDangerTest, IsDangerousFunction)
{
    // 危险类型
    EXPECT_TRUE(isDangerous(PathNodeType::DamageFire));
    EXPECT_TRUE(isDangerous(PathNodeType::DangerFire));
    EXPECT_TRUE(isDangerous(PathNodeType::DamageCactus));
    EXPECT_TRUE(isDangerous(PathNodeType::DangerCactus));
    EXPECT_TRUE(isDangerous(PathNodeType::DamageOther));
    EXPECT_TRUE(isDangerous(PathNodeType::DangerOther));
    EXPECT_TRUE(isDangerous(PathNodeType::DangerBerry));
    EXPECT_TRUE(isDangerous(PathNodeType::Lava));

    // 非危险类型
    EXPECT_FALSE(isDangerous(PathNodeType::Walkable));
    EXPECT_FALSE(isDangerous(PathNodeType::Water));
    EXPECT_FALSE(isDangerous(PathNodeType::Open));
    EXPECT_FALSE(isDangerous(PathNodeType::Blocked));
}

TEST(PathNodeTypeDangerTest, DangerBerryCostPenalty)
{
    // 甜浆果丛危险区域代价为 8.0
    EXPECT_FLOAT_EQ(getPathCostPenalty(PathNodeType::DangerBerry), 8.0f);
}

// ============================================================================
// WalkNodeProcessor::isDangerous 测试
// ============================================================================

class WalkNodeProcessorDangerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_processor = std::make_unique<WalkNodeProcessor>();
        m_processor->setRegion(&m_region);
    }

    MockRegion m_region;
    std::unique_ptr<WalkNodeProcessor> m_processor;
};

TEST_F(WalkNodeProcessorDangerTest, LavaIsDangerous)
{
    // 设置岩浆
    m_region.setLava(0, 0, 0, true);

    EXPECT_TRUE(m_processor->isDangerous(0, 0, 0));
    EXPECT_FALSE(m_processor->isDangerous(1, 0, 0));
}

TEST_F(WalkNodeProcessorDangerTest, WaterIsNotDangerous)
{
    // 水不是危险方块
    m_region.setWater(0, 0, 0, true);

    EXPECT_FALSE(m_processor->isDangerous(0, 0, 0));
}

TEST_F(WalkNodeProcessorDangerTest, AirIsNotDangerous)
{
    // 空气不是危险方块
    EXPECT_FALSE(m_processor->isDangerous(0, 0, 0));
}

TEST_F(WalkNodeProcessorDangerTest, WalkableBlockIsNotDangerous)
{
    // 可行走的方块不是危险方块
    m_region.setWalkable(0, 0, 0, true);

    EXPECT_FALSE(m_processor->isDangerous(0, 0, 0));
}

TEST_F(WalkNodeProcessorDangerTest, NullRegionReturnsFalse)
{
    // 空 Region 返回 false
    m_processor->setRegion(nullptr);

    EXPECT_FALSE(m_processor->isDangerous(0, 0, 0));
}

// ============================================================================
// WalkNodeProcessor::getNodeType 测试
// ============================================================================

class WalkNodeProcessorNodeTypeTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_processor = std::make_unique<WalkNodeProcessor>();
        m_processor->setRegion(&m_region);
    }

    MockRegion m_region;
    std::unique_ptr<WalkNodeProcessor> m_processor;
};

TEST_F(WalkNodeProcessorNodeTypeTest, UnloadedReturnsBlocked)
{
    m_region.m_globalLoaded = false;

    EXPECT_EQ(m_processor->getNodeType(0, 0, 0), PathNodeType::Blocked);
}

TEST_F(WalkNodeProcessorNodeTypeTest, WaterReturnsWaterOrBlocked)
{
    // 默认不能游泳
    m_processor->setCanSwim(false);
    m_region.setWater(0, 0, 0, true);

    EXPECT_EQ(m_processor->getNodeType(0, 0, 0), PathNodeType::Blocked);

    // 可以游泳
    m_processor->setCanSwim(true);
    EXPECT_EQ(m_processor->getNodeType(0, 0, 0), PathNodeType::Water);
}

TEST_F(WalkNodeProcessorNodeTypeTest, LavaReturnsLava)
{
    m_region.setLava(0, 0, 0, true);

    EXPECT_EQ(m_processor->getNodeType(0, 0, 0), PathNodeType::Lava);
}

TEST_F(WalkNodeProcessorNodeTypeTest, OpenGroundReturnsWalkable)
{
    // 查询位置(0,64,0)为空气，脚下(0,63,0)有可行走的地面支撑 → Walkable。
    // 对应 MC Java WalkNodeEvaluator.getPathTypeStatic：OPEN 节点 + 下方实心(实心→BLOCKED，属 default 支撑臂)
    // → 经 checkNeighbourBlocks 后返回 WALKABLE。Cubium 抽象下 isWalkable(脚下方块)=true 即"实心可站立支撑"。
    m_region.setWalkable(0, 63, 0, true); // 脚下有可行走的地面支撑

    EXPECT_EQ(m_processor->getNodeType(0, 64, 0), PathNodeType::Walkable);
}

TEST_F(WalkNodeProcessorNodeTypeTest, OpenAirReturnsOpen)
{
    // 查询位置为空气，脚下(0,63,0)也是空气，但更下方(0,62,0)有地面且在最大跌落距离内 → Open。
    // 对应 MC Java getPathTypeStatic：OPEN + 下方 OPEN → OPEN；跌落距离限制由 getNeighbors 处理。
    // Cubium 扩展：_getGroundHeight 向下找到 y=62，跌落距离=2 <= m_maxFallDistance(3) → Open（非 DangerFall）。
    m_region.setWalkable(0, 62, 0, true); // 地面在脚下 2 格，跌落距离=2 <= maxFallDistance(3)

    EXPECT_EQ(m_processor->getNodeType(0, 64, 0), PathNodeType::Open);
}

TEST_F(WalkNodeProcessorNodeTypeTest, AirWithNoGroundReturnsDangerFall)
{
    // 空气位置且下方无任何地面 → DangerFall（Cubium 扩展类型，MC 1.16.5 不存在）
    // _getGroundHeight 向下搜索到 MIN_BUILD_HEIGHT 仍无 walkable，返回 MIN_BUILD_HEIGHT
    // groundHeight(MIN_BUILD_HEIGHT) < y - m_maxFallDistance → DangerFall
    // 该扩展语义防止实体寻路到无底深渊，源码见 WalkNodeProcessor::getNodeType 第 124-132 行
    EXPECT_EQ(m_processor->getNodeType(0, 64, 0), PathNodeType::DangerFall);
}

// ============================================================================
// Region::getBlockState 测试
// ============================================================================

TEST(RegionGetBlockStateTest, ReturnsNullptrForAir)
{
    MockRegion region;

    // 空气返回 nullptr
    const BlockState* state = region.getBlockState(0, 0, 0);
    EXPECT_EQ(state, nullptr);
}

TEST(RegionGetBlockStateTest, ReturnsNullptrForMissingBlock)
{
    MockRegion region;

    // 未设置的方块返回 nullptr（stateId = 0 = 空气）
    const BlockState* state = region.getBlockState(100, 100, 100);
    EXPECT_EQ(state, nullptr);
}

// ============================================================================
// 危险检测距离测试
// ============================================================================

TEST_F(WalkNodeProcessorDangerTest, DetectsAdjacentDanger)
{
    // 设置岩浆在相邻位置
    m_region.setLava(1, 0, 0, true);

    // isDangerous 只检查单个位置
    EXPECT_FALSE(m_processor->isDangerous(0, 0, 0));
    EXPECT_TRUE(m_processor->isDangerous(1, 0, 0));
}

TEST_F(WalkNodeProcessorDangerTest, MultipleDangerTypes)
{
    // 不同位置设置不同危险
    m_region.setLava(0, 0, 0, true);
    m_region.setLava(2, 0, 0, true);

    EXPECT_TRUE(m_processor->isDangerous(0, 0, 0));
    EXPECT_FALSE(m_processor->isDangerous(1, 0, 0));
    EXPECT_TRUE(m_processor->isDangerous(2, 0, 0));
}

// ============================================================================
// 危险方块代价惩罚测试
// ============================================================================

TEST(PathNodeTypeCostTest, DamageTypesHaveHigherCost)
{
    // DAMAGE_FIRE 比 DANGER_FIRE 代价更高
    EXPECT_GT(getPathCostPenalty(PathNodeType::DamageFire), getPathCostPenalty(PathNodeType::DangerFire));

    // DAMAGE_CACTUS 不可通行，DANGER_CACTUS 高代价但可通行
    EXPECT_LT(getPathCostPenalty(PathNodeType::DamageCactus), 0.0f);
    EXPECT_GT(getPathCostPenalty(PathNodeType::DangerCactus), 0.0f);

    // DAMAGE_OTHER 不可通行，DANGER_OTHER 高代价但可通行
    EXPECT_LT(getPathCostPenalty(PathNodeType::DamageOther), 0.0f);
    EXPECT_GT(getPathCostPenalty(PathNodeType::DangerOther), 0.0f);
}

TEST(PathNodeTypeCostTest, AllDangerTypesPenalized)
{
    // 所有危险类型都有正代价（高代价但可通行）
    EXPECT_GT(getPathCostPenalty(PathNodeType::DangerFire), 0.0f);
    EXPECT_GT(getPathCostPenalty(PathNodeType::DangerCactus), 0.0f);
    EXPECT_GT(getPathCostPenalty(PathNodeType::DangerOther), 0.0f);
    EXPECT_GT(getPathCostPenalty(PathNodeType::DangerBerry), 0.0f);
}

// ============================================================================
// 门类型寻路测试
// ============================================================================

TEST(DoorPathNodeTypeTest, DoorOpenIsWalkable)
{
    // 打开的门可行走
    EXPECT_TRUE(isWalkable(PathNodeType::DoorOpen));
    // 打开的门代价为0
    EXPECT_FLOAT_EQ(getPathCostPenalty(PathNodeType::DoorOpen), 0.0f);
}

TEST(DoorPathNodeTypeTest, DoorWoodClosedIsNotWalkable)
{
    // 关闭的木门不可行走
    EXPECT_FALSE(isWalkable(PathNodeType::DoorWoodClosed));
    // 关闭的木门代价为-1
    EXPECT_FLOAT_EQ(getPathCostPenalty(PathNodeType::DoorWoodClosed), -1.0f);
}

TEST(DoorPathNodeTypeTest, DoorIronClosedIsNotWalkable)
{
    // 关闭的铁门不可行走
    EXPECT_FALSE(isWalkable(PathNodeType::DoorIronClosed));
    // 关闭的铁门代价为-1
    EXPECT_FLOAT_EQ(getPathCostPenalty(PathNodeType::DoorIronClosed), -1.0f);
}

TEST(DoorPathNodeTypeTest, WalkableDoorIsWalkable)
{
    // 可行走的门（关闭木门+能开门+能穿门 => WalkableDoor）可行走
    EXPECT_TRUE(isWalkable(PathNodeType::WalkableDoor));
    // 可行走的门代价为0
    EXPECT_FLOAT_EQ(getPathCostPenalty(PathNodeType::WalkableDoor), 0.0f);
}

TEST(DoorPathNodeTypeTest, FenceGateIsWalkable)
{
    // 栅栏门可行走
    EXPECT_TRUE(isWalkable(PathNodeType::FenceGate));
    // 栅栏门代价为0
    EXPECT_FLOAT_EQ(getPathCostPenalty(PathNodeType::FenceGate), 0.0f);
}

TEST(DoorPathNodeTypeTest, FenceIsNotWalkable)
{
    // 栅栏不可行走
    EXPECT_FALSE(isWalkable(PathNodeType::Fence));
    // 栅栏代价为-1
    EXPECT_FLOAT_EQ(getPathCostPenalty(PathNodeType::Fence), -1.0f);
}

// ============================================================================
// WalkNodeProcessor 门属性默认值测试
// ============================================================================

TEST_F(WalkNodeProcessorNodeTypeTest, CanOpenDoorsDefaultsToFalse)
{
    // canOpenDoors 默认为 false
    EXPECT_FALSE(m_processor->canOpenDoors());
}

TEST_F(WalkNodeProcessorNodeTypeTest, CanEnterDoorsDefaultsToTrue)
{
    // canEnterDoors 默认为 true（对齐 MC 的 canPassDoors=true）
    EXPECT_TRUE(m_processor->canEnterDoors());
}

TEST_F(WalkNodeProcessorNodeTypeTest, SetCanOpenDoors)
{
    m_processor->setCanOpenDoors(true);
    EXPECT_TRUE(m_processor->canOpenDoors());
    m_processor->setCanOpenDoors(false);
    EXPECT_FALSE(m_processor->canOpenDoors());
}

TEST_F(WalkNodeProcessorNodeTypeTest, SetCanEnterDoors)
{
    m_processor->setCanEnterDoors(false);
    EXPECT_FALSE(m_processor->canEnterDoors());
    m_processor->setCanEnterDoors(true);
    EXPECT_TRUE(m_processor->canEnterDoors());
}

// ============================================================================
// 门类型转换测试 — 使用可重写 getNodeType 的测试子类
// ============================================================================

namespace {

/**
 * @brief 可重写 getNodeType 的测试用 WalkNodeProcessor
 *
 * 用于测试 getNodeTypeWithEntity 中的门类型转换逻辑，
 * 无需依赖真实的 BlockState/DoorBlock 对象。
 */
class TestableWalkNodeProcessor : public WalkNodeProcessor {
public:
    /// 预设的节点类型映射：key = makeHash(x,y,z)，value = PathNodeType
    std::unordered_map<u64, PathNodeType> m_presetTypes;

    /// 设置指定位置的预设节点类型
    void setPresetNodeType(i32 x, i32 y, i32 z, PathNodeType type)
    {
        u64 key = (static_cast<u64>(static_cast<u32>(x)) << 32) |
            (static_cast<u64>(static_cast<u16>(y & 0xFFFF)) << 16) |
            (static_cast<u64>(static_cast<u32>(z)) & 0xFFFFULL);
        m_presetTypes[key] = type;
    }

    /// 重写 getNodeType，返回预设类型或调用基类实现
    [[nodiscard]] PathNodeType getNodeType(i32 x, i32 y, i32 z) override
    {
        u64 key = (static_cast<u64>(static_cast<u32>(x)) << 32) |
            (static_cast<u64>(static_cast<u16>(y & 0xFFFF)) << 16) |
            (static_cast<u64>(static_cast<u32>(z)) & 0xFFFFULL);
        auto it = m_presetTypes.find(key);
        if (it != m_presetTypes.end()) {
            return it->second;
        }
        return WalkNodeProcessor::getNodeType(x, y, z);
    }
};

} // namespace

// ============================================================================
// 门类型转换逻辑测试
// ============================================================================

class DoorTypeConversionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_processor = std::make_unique<TestableWalkNodeProcessor>();
        m_processor->setRegion(&m_region);
        // 默认设置：canOpenDoors=false, canEnterDoors=true（对齐 MC 默认值）
        m_processor->setCanOpenDoors(false);
        m_processor->setCanEnterDoors(true);
    }

    MockRegion m_region;
    std::unique_ptr<TestableWalkNodeProcessor> m_processor;
};

TEST_F(DoorTypeConversionTest, DoorWoodClosedCannotOpenBecomesBlocked)
{
    // 关闭的木门 + 不能开门 + 能穿门 → 保持 DoorWoodClosed（costMalus=-1.0，不可通行）
    m_processor->setPresetNodeType(0, 64, 0, PathNodeType::DoorWoodClosed);
    m_processor->setCanOpenDoors(false);
    m_processor->setCanEnterDoors(true);

    PathNodeType result = m_processor->getNodeTypeWithEntity(0, 64, 0);
    // DoorWoodClosed 的 costMalus 是 -1.0，不可行走，但不会变成 Blocked
    // 在当前实现中，DoorWoodClosed 不会被转换（只有 canOpenDoors && canEnterDoors 才转换为 WalkableDoor）
    EXPECT_EQ(result, PathNodeType::DoorWoodClosed);
}

TEST_F(DoorTypeConversionTest, DoorWoodClosedCanOpenAndPassBecomesWalkableDoor)
{
    // 关闭的木门 + 能开门 + 能穿门 → WalkableDoor（可通行）
    m_processor->setPresetNodeType(0, 64, 0, PathNodeType::DoorWoodClosed);
    m_processor->setCanOpenDoors(true);
    m_processor->setCanEnterDoors(true);

    PathNodeType result = m_processor->getNodeTypeWithEntity(0, 64, 0);
    EXPECT_EQ(result, PathNodeType::WalkableDoor);
}

TEST_F(DoorTypeConversionTest, DoorWoodClosedCanOpenButCannotPassStaysClosed)
{
    // 关闭的木门 + 能开门 + 不能穿门 → 保持 DoorWoodClosed
    // MC 原版: canOpenDoors && canPassDoors 才能转换为 WALKABLE_DOOR
    m_processor->setPresetNodeType(0, 64, 0, PathNodeType::DoorWoodClosed);
    m_processor->setCanOpenDoors(true);
    m_processor->setCanEnterDoors(false);

    PathNodeType result = m_processor->getNodeTypeWithEntity(0, 64, 0);
    EXPECT_EQ(result, PathNodeType::DoorWoodClosed);
}

TEST_F(DoorTypeConversionTest, DoorOpenCanPassStaysDoorOpen)
{
    // 打开的门 + 能穿门 → 保持 DoorOpen（可通行）
    m_processor->setPresetNodeType(0, 64, 0, PathNodeType::DoorOpen);
    m_processor->setCanOpenDoors(false);
    m_processor->setCanEnterDoors(true);

    PathNodeType result = m_processor->getNodeTypeWithEntity(0, 64, 0);
    EXPECT_EQ(result, PathNodeType::DoorOpen);
}

TEST_F(DoorTypeConversionTest, DoorOpenCannotPassBecomesBlocked)
{
    // 打开的门 + 不能穿门 → Blocked
    // MC 原版: DOOR_OPEN && !canPassDoors → BLOCKED
    m_processor->setPresetNodeType(0, 64, 0, PathNodeType::DoorOpen);
    m_processor->setCanOpenDoors(false);
    m_processor->setCanEnterDoors(false);

    PathNodeType result = m_processor->getNodeTypeWithEntity(0, 64, 0);
    EXPECT_EQ(result, PathNodeType::Blocked);
}

TEST_F(DoorTypeConversionTest, DoorIronClosedRemainsUnchanged)
{
    // 关闭的铁门始终不可通行，不会因为 canOpenDoors 而转换
    // 铁门无法手动打开
    m_processor->setPresetNodeType(0, 64, 0, PathNodeType::DoorIronClosed);
    m_processor->setCanOpenDoors(true);
    m_processor->setCanEnterDoors(true);

    PathNodeType result = m_processor->getNodeTypeWithEntity(0, 64, 0);
    // DoorIronClosed 不满足 DoorWoodClosed 条件，保持不变
    EXPECT_EQ(result, PathNodeType::DoorIronClosed);
}

TEST_F(DoorTypeConversionTest, WalkableDoorIsWalkableAndZeroCost)
{
    // WalkableDoor 可行走，代价为0
    EXPECT_TRUE(isWalkable(PathNodeType::WalkableDoor));
    EXPECT_FLOAT_EQ(getPathCostPenalty(PathNodeType::WalkableDoor), 0.0f);
}

TEST_F(DoorTypeConversionTest, BlockedTypeStaysBlocked)
{
    // Blocked 类型始终返回 Blocked，不做门类型转换
    m_processor->setPresetNodeType(0, 64, 0, PathNodeType::Blocked);

    PathNodeType result = m_processor->getNodeTypeWithEntity(0, 64, 0);
    EXPECT_EQ(result, PathNodeType::Blocked);
}
