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

#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/gen/structure/StructureCheck.hpp"

using namespace mc;
using namespace mc::world::gen::structure;
using namespace mc::math;

// ============================================================================
// 辅助工具
// ============================================================================

/// 将区块坐标打包为 64 位 ID，复用项目中的 chunkPosToId
static u64 packChunkPos(i32 chunkX, i32 chunkZ)
{
    return chunkPosToId(chunkX, chunkZ);
}

// ============================================================================
// 空缓存查询测试
// ============================================================================

TEST(StructureCheckTest, CheckStart_EmptyCache_ReturnsChunkLoadNeeded)
{
    // 空缓存中查询任何区块和结构都应返回 ChunkLoadNeeded
    StructureCheck check;
    ResourceLocation villageId("minecraft:village_plains");

    EXPECT_EQ(StructureCheckResult::ChunkLoadNeeded, check.checkStart(packChunkPos(0, 0), villageId));
    EXPECT_EQ(StructureCheckResult::ChunkLoadNeeded, check.checkStart(packChunkPos(5, -3), villageId));
    EXPECT_EQ(StructureCheckResult::ChunkLoadNeeded, check.checkStart(packChunkPos(-100, 200), villageId));
}

TEST(StructureCheckTest, LoadedChunkCount_EmptyCache_ReturnsZero)
{
    StructureCheck check;
    EXPECT_EQ(0u, check.loadedChunkCount());
}

// ============================================================================
// onStructureLoad + checkStart 测试
// ============================================================================

TEST(StructureCheckTest, OnStructureLoad_ThenCheckStart_StartPresent)
{
    // 加载区块结构数据后，查询该结构应返回 StartPresent
    StructureCheck check;
    ResourceLocation villageId("minecraft:village_plains");
    u64 chunkPos = packChunkPos(10, 20);

    // 模拟 STRUCTURE_STARTS 阶段完成：结构起点存在，引用计数为 0
    std::unordered_map<ResourceLocation, i32> refCounts;
    refCounts[villageId] = 0;
    check.onStructureLoad(chunkPos, refCounts);

    EXPECT_EQ(StructureCheckResult::StartPresent, check.checkStart(chunkPos, villageId));
    EXPECT_EQ(1u, check.loadedChunkCount());
}

TEST(StructureCheckTest, OnStructureLoad_ThenCheckStart_DifferentStructure_StartNotPresent)
{
    // 加载了 village 数据后，查询 fortress 应返回 StartNotPresent
    StructureCheck check;
    ResourceLocation villageId("minecraft:village_plains");
    ResourceLocation fortressId("minecraft:fortress");
    u64 chunkPos = packChunkPos(10, 20);

    std::unordered_map<ResourceLocation, i32> refCounts;
    refCounts[villageId] = 0;
    check.onStructureLoad(chunkPos, refCounts);

    EXPECT_EQ(StructureCheckResult::StartNotPresent, check.checkStart(chunkPos, fortressId));
}

TEST(StructureCheckTest, OnStructureLoad_EmptyRefCounts_StartNotPresent)
{
    // 加载空区块（无任何结构）后，查询任何结构应返回 StartNotPresent
    StructureCheck check;
    ResourceLocation villageId("minecraft:village_plains");
    u64 chunkPos = packChunkPos(5, 5);

    std::unordered_map<ResourceLocation, i32> emptyRefCounts;
    check.onStructureLoad(chunkPos, emptyRefCounts);

    // 空条目中不包含任何结构 ID，查询应返回 StartNotPresent
    EXPECT_EQ(StructureCheckResult::StartNotPresent, check.checkStart(chunkPos, villageId));
}

TEST(StructureCheckTest, OnStructureLoad_MultipleStructures)
{
    // 一个区块可能同时包含多种结构（如废弃矿井和要塞）
    StructureCheck check;
    ResourceLocation mineshaftId("minecraft:mineshaft");
    ResourceLocation strongholdId("minecraft:stronghold");
    ResourceLocation villageId("minecraft:village_plains");
    u64 chunkPos = packChunkPos(1, 1);

    std::unordered_map<ResourceLocation, i32> refCounts;
    refCounts[mineshaftId] = 0;
    refCounts[strongholdId] = 2;
    check.onStructureLoad(chunkPos, refCounts);

    EXPECT_EQ(StructureCheckResult::StartPresent, check.checkStart(chunkPos, mineshaftId));
    EXPECT_EQ(StructureCheckResult::StartPresent, check.checkStart(chunkPos, strongholdId));
    // village 未在此区块中
    EXPECT_EQ(StructureCheckResult::StartNotPresent, check.checkStart(chunkPos, villageId));
}

TEST(StructureCheckTest, OnStructureLoad_NonZeroRefCount)
{
    // 验证非零引用计数也被正确缓存和查询
    StructureCheck check;
    ResourceLocation villageId("minecraft:village_plains");
    u64 chunkPos = packChunkPos(3, 7);

    // 引用计数 > 0（其他区块引用了此结构）
    std::unordered_map<ResourceLocation, i32> refCounts;
    refCounts[villageId] = 5;
    check.onStructureLoad(chunkPos, refCounts);

    EXPECT_EQ(StructureCheckResult::StartPresent, check.checkStart(chunkPos, villageId));
}

TEST(StructureCheckTest, CheckStart_DifferentChunk_ReturnsChunkLoadNeeded)
{
    // 查询已加载区块以外的区块应返回 ChunkLoadNeeded
    StructureCheck check;
    ResourceLocation villageId("minecraft:village_plains");

    std::unordered_map<ResourceLocation, i32> refCounts;
    refCounts[villageId] = 0;
    check.onStructureLoad(packChunkPos(10, 20), refCounts);

    // 不同区块
    EXPECT_EQ(StructureCheckResult::ChunkLoadNeeded, check.checkStart(packChunkPos(10, 21), villageId));
    EXPECT_EQ(StructureCheckResult::ChunkLoadNeeded, check.checkStart(packChunkPos(11, 20), villageId));
    EXPECT_EQ(StructureCheckResult::ChunkLoadNeeded, check.checkStart(packChunkPos(0, 0), villageId));
}

// ============================================================================
// 重复 onStructureLoad 调用测试
// ============================================================================

TEST(StructureCheckTest, OnStructureLoad_OverwritePrevious)
{
    // 对同一区块重复调用 onStructureLoad 应覆盖之前的数据
    StructureCheck check;
    ResourceLocation villageId("minecraft:village_plains");
    ResourceLocation fortressId("minecraft:fortress");
    u64 chunkPos = packChunkPos(4, 4);

    // 第一次加载：只有 village
    std::unordered_map<ResourceLocation, i32> refCounts1;
    refCounts1[villageId] = 0;
    check.onStructureLoad(chunkPos, refCounts1);
    EXPECT_EQ(StructureCheckResult::StartPresent, check.checkStart(chunkPos, villageId));
    EXPECT_EQ(StructureCheckResult::StartNotPresent, check.checkStart(chunkPos, fortressId));

    // 第二次加载：只有 fortress（覆盖了之前的数据）
    std::unordered_map<ResourceLocation, i32> refCounts2;
    refCounts2[fortressId] = 3;
    check.onStructureLoad(chunkPos, refCounts2);
    EXPECT_EQ(StructureCheckResult::StartNotPresent, check.checkStart(chunkPos, villageId));
    EXPECT_EQ(StructureCheckResult::StartPresent, check.checkStart(chunkPos, fortressId));

    // loadedChunkCount 应保持为 1（同一区块覆盖，不是新增）
    EXPECT_EQ(1u, check.loadedChunkCount());
}

// ============================================================================
// incrementReference 测试
// ============================================================================

TEST(StructureCheckTest, IncrementReference_ExistingChunk)
{
    // 对已加载区块中的结构递增引用计数
    StructureCheck check;
    ResourceLocation villageId("minecraft:village_plains");
    u64 chunkPos = packChunkPos(2, 3);

    // 先加载区块
    std::unordered_map<ResourceLocation, i32> refCounts;
    refCounts[villageId] = 0;
    check.onStructureLoad(chunkPos, refCounts);

    // 递增引用计数
    check.incrementReference(chunkPos, villageId);
    // 引用计数从 0 变为 1，仍应返回 StartPresent
    EXPECT_EQ(StructureCheckResult::StartPresent, check.checkStart(chunkPos, villageId));

    // 多次递增
    check.incrementReference(chunkPos, villageId);
    check.incrementReference(chunkPos, villageId);
    // 引用计数变为 3，仍应返回 StartPresent
    EXPECT_EQ(StructureCheckResult::StartPresent, check.checkStart(chunkPos, villageId));
}

TEST(StructureCheckTest, IncrementReference_NonExistentChunk)
{
    // 对未加载区块递增引用计数，应自动创建新条目
    StructureCheck check;
    ResourceLocation villageId("minecraft:village_plains");
    u64 chunkPos = packChunkPos(7, 8);

    // 不调用 onStructureLoad，直接递增引用
    check.incrementReference(chunkPos, villageId);

    // 现在查询该区块应返回 StartPresent
    EXPECT_EQ(StructureCheckResult::StartPresent, check.checkStart(chunkPos, villageId));
    EXPECT_EQ(1u, check.loadedChunkCount());
}

TEST(StructureCheckTest, IncrementReference_NewStructureInExistingChunk)
{
    // 区块已加载结构 A，递增结构 B 的引用，应自动添加结构 B 条目
    StructureCheck check;
    ResourceLocation villageId("minecraft:village_plains");
    ResourceLocation fortressId("minecraft:fortress");
    u64 chunkPos = packChunkPos(1, 1);

    // 加载区块，只有 village
    std::unordered_map<ResourceLocation, i32> refCounts;
    refCounts[villageId] = 0;
    check.onStructureLoad(chunkPos, refCounts);

    // 递增 fortress 的引用（fortress 不在原有数据中）
    check.incrementReference(chunkPos, fortressId);

    // 两种结构都应返回 StartPresent
    EXPECT_EQ(StructureCheckResult::StartPresent, check.checkStart(chunkPos, villageId));
    EXPECT_EQ(StructureCheckResult::StartPresent, check.checkStart(chunkPos, fortressId));
}

TEST(StructureCheckTest, IncrementReference_MultipleStructures)
{
    // 在同一区块中对多个结构递增引用
    StructureCheck check;
    ResourceLocation villageId("minecraft:village_plains");
    ResourceLocation fortressId("minecraft:fortress");
    u64 chunkPos = packChunkPos(0, 0);

    // 区块只有 village
    std::unordered_map<ResourceLocation, i32> refCounts;
    refCounts[villageId] = 1;
    check.onStructureLoad(chunkPos, refCounts);

    // 递增 fortress 引用
    check.incrementReference(chunkPos, fortressId);

    EXPECT_EQ(StructureCheckResult::StartPresent, check.checkStart(chunkPos, villageId));
    EXPECT_EQ(StructureCheckResult::StartPresent, check.checkStart(chunkPos, fortressId));
}

// ============================================================================
// clearCache 测试
// ============================================================================

TEST(StructureCheckTest, ClearCache_EmptiesAllEntries)
{
    // 清空缓存后，所有查询应返回 ChunkLoadNeeded
    StructureCheck check;
    ResourceLocation villageId("minecraft:village_plains");
    ResourceLocation fortressId("minecraft:fortress");

    // 加载多个区块
    std::unordered_map<ResourceLocation, i32> refCounts1;
    refCounts1[villageId] = 0;
    check.onStructureLoad(packChunkPos(1, 1), refCounts1);

    std::unordered_map<ResourceLocation, i32> refCounts2;
    refCounts2[fortressId] = 2;
    check.onStructureLoad(packChunkPos(2, 3), refCounts2);

    EXPECT_EQ(2u, check.loadedChunkCount());

    // 清空缓存
    check.clearCache();

    EXPECT_EQ(0u, check.loadedChunkCount());
    EXPECT_EQ(StructureCheckResult::ChunkLoadNeeded, check.checkStart(packChunkPos(1, 1), villageId));
    EXPECT_EQ(StructureCheckResult::ChunkLoadNeeded, check.checkStart(packChunkPos(2, 3), fortressId));
}

TEST(StructureCheckTest, ClearCache_ThenReload)
{
    // 清空缓存后重新加载数据应正常工作
    StructureCheck check;
    ResourceLocation villageId("minecraft:village_plains");
    u64 chunkPos = packChunkPos(5, 5);

    // 加载、清空、重新加载
    std::unordered_map<ResourceLocation, i32> refCounts;
    refCounts[villageId] = 0;
    check.onStructureLoad(chunkPos, refCounts);
    check.clearCache();
    EXPECT_EQ(0u, check.loadedChunkCount());

    // 重新加载
    refCounts[villageId] = 3;
    check.onStructureLoad(chunkPos, refCounts);
    EXPECT_EQ(1u, check.loadedChunkCount());
    EXPECT_EQ(StructureCheckResult::StartPresent, check.checkStart(chunkPos, villageId));
}

// ============================================================================
// 负坐标区块测试
// ============================================================================

TEST(StructureCheckTest, NegativeChunkCoordinates)
{
    // 验证负坐标区块的打包和查询正确性
    StructureCheck check;
    ResourceLocation villageId("minecraft:village_plains");

    i32 coords[][2] = {{-1, -1}, {-100, 200}, {0, -50}, {-50, 0}};
    for (auto& coord : coords) {
        u64 chunkPos = packChunkPos(coord[0], coord[1]);

        std::unordered_map<ResourceLocation, i32> refCounts;
        refCounts[villageId] = 0;
        check.onStructureLoad(chunkPos, refCounts);

        EXPECT_EQ(StructureCheckResult::StartPresent, check.checkStart(chunkPos, villageId))
            << "Failed at chunk (" << coord[0] << ", " << coord[1] << ")";
    }
}

// ============================================================================
// 多区块并发测试
// ============================================================================

TEST(StructureCheckTest, MultipleChunksIndependentlyTracked)
{
    // 多个区块的结构数据应互不干扰
    StructureCheck check;
    ResourceLocation villageId("minecraft:village_plains");
    ResourceLocation fortressId("minecraft:fortress");
    ResourceLocation monumentId("minecraft:ocean_monument");

    // 区块 (0, 0) 有 village
    std::unordered_map<ResourceLocation, i32> refCounts00;
    refCounts00[villageId] = 0;
    check.onStructureLoad(packChunkPos(0, 0), refCounts00);

    // 区块 (1, 0) 有 fortress
    std::unordered_map<ResourceLocation, i32> refCounts10;
    refCounts10[fortressId] = 1;
    check.onStructureLoad(packChunkPos(1, 0), refCounts10);

    // 区块 (0, 1) 有 monument
    std::unordered_map<ResourceLocation, i32> refCounts01;
    refCounts01[monumentId] = 0;
    check.onStructureLoad(packChunkPos(0, 1), refCounts01);

    EXPECT_EQ(3u, check.loadedChunkCount());

    // 每个区块只能查到自己的结构
    EXPECT_EQ(StructureCheckResult::StartPresent, check.checkStart(packChunkPos(0, 0), villageId));
    EXPECT_EQ(StructureCheckResult::StartNotPresent, check.checkStart(packChunkPos(0, 0), fortressId));
    EXPECT_EQ(StructureCheckResult::StartNotPresent, check.checkStart(packChunkPos(0, 0), monumentId));

    EXPECT_EQ(StructureCheckResult::StartNotPresent, check.checkStart(packChunkPos(1, 0), villageId));
    EXPECT_EQ(StructureCheckResult::StartPresent, check.checkStart(packChunkPos(1, 0), fortressId));
    EXPECT_EQ(StructureCheckResult::StartNotPresent, check.checkStart(packChunkPos(1, 0), monumentId));

    EXPECT_EQ(StructureCheckResult::StartNotPresent, check.checkStart(packChunkPos(0, 1), villageId));
    EXPECT_EQ(StructureCheckResult::StartNotPresent, check.checkStart(packChunkPos(0, 1), fortressId));
    EXPECT_EQ(StructureCheckResult::StartPresent, check.checkStart(packChunkPos(0, 1), monumentId));

    // 未加载的区块
    EXPECT_EQ(StructureCheckResult::ChunkLoadNeeded, check.checkStart(packChunkPos(2, 2), villageId));
}

// ============================================================================
// incrementReference 在清空缓存后
// ============================================================================

TEST(StructureCheckTest, IncrementReference_AfterClearCache)
{
    // 清空缓存后，incrementReference 仍应正常工作
    StructureCheck check;
    ResourceLocation villageId("minecraft:village_plains");
    u64 chunkPos = packChunkPos(3, 3);

    std::unordered_map<ResourceLocation, i32> refCounts;
    refCounts[villageId] = 0;
    check.onStructureLoad(chunkPos, refCounts);
    check.clearCache();

    // 缓存清空后直接递增引用
    check.incrementReference(chunkPos, villageId);
    EXPECT_EQ(StructureCheckResult::StartPresent, check.checkStart(chunkPos, villageId));
    EXPECT_EQ(1u, check.loadedChunkCount());
}

// ============================================================================
// 同一区块同一结构多次 incrementReference
// ============================================================================

TEST(StructureCheckTest, IncrementReference_MultipleTimesOnSameStructure)
{
    // 同一区块同一结构多次递增引用计数，验证计数从 0 开始逐步递增
    StructureCheck check;
    ResourceLocation villageId("minecraft:village_plains");
    u64 chunkPos = packChunkPos(6, 9);

    // 先通过 onStructureLoad 创建初始条目（引用计数为 0）
    std::unordered_map<ResourceLocation, i32> refCounts;
    refCounts[villageId] = 0;
    check.onStructureLoad(chunkPos, refCounts);

    // 递增 10 次
    for (int i = 0; i < 10; ++i) {
        check.incrementReference(chunkPos, villageId);
    }

    // 应仍返回 StartPresent（引用计数 >= 0）
    EXPECT_EQ(StructureCheckResult::StartPresent, check.checkStart(chunkPos, villageId));
}
