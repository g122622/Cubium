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

#pragma once

#include "ConfiguredFeature.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include <array>

namespace mc {

// 前向声明
class WorldGenRegion;
class BlockState;

namespace world::gen::feature {

/**
 * @brief 地牢（MonsterRoom）特征
 *
 * 算法分四个阶段：
 * 1. 合法性扫描：地板（l2==-1）与天花板（l2==4）必须为固体，否则放弃；
 *    统计边界空格数 j2，仅当 j2∈[1,5] 才继续。
 * 2. 建造房间：边界格在"下方非固体且 y>=minY"时设为 CAVE_AIR；
 *    边界固体格（非 CHEST）按 i4==-1 且 nextInt(4)!=0 概率设 MOSSY_COBBLESTONE，
 *    否则 COBBLESTONE；内部非 CHEST 非 SPAWNER 设 AIR。所有设置走 safeSetBlock
 *    （不替换 FEATURES_CANNOT_REPLACE 标签方块）。
 * 3. 宝箱：2 轮，每轮 3 次尝试；命中"空格 + 恰 1 个水平固体邻居"时放置朝向宝箱，
 *    设置 SIMPLE_DUNGEON 战利品表，break。
 * 4. 刷怪笼：origin 处放 SPAWNER，setEntityId 从 {skeleton, zombie, zombie, spider} 随机选取。
 *
 * 该类是底层算法类，不继承 ConfiguredFeatureBase。
 * 配置化包装见 ConfiguredMonsterRoomFeature。
 */
class MonsterRoomFeature {
public:
    /**
     * @brief 在指定坐标尝试生成地牢
     *
     * @param region 世界生成区域
     * @param random 随机数生成器
     * @param x 起点世界 X 坐标
     * @param y 起点世界 Y 坐标
     * @param z 起点世界 Z 坐标
     * @return 是否成功生成（j2∈[1,5] 且完成建造返回 true，否则 false）
     */
    bool place(WorldGenRegion& region, math::Random& random, i32 x, i32 y, i32 z);

private:
    /**
     * @brief 安全放置方块（不替换 FEATURES_CANNOT_REPLACE 标签方块）
     *
     * @return 实际是否放置（受保护方块返回 false）
     */
    static bool safeSetBlock(WorldGenRegion& region, const BlockPos& pos, const BlockState* state);

    /**
     * @brief 从 MOBS 数组随机选取一个实体 ID
     *
     * MOBS = [skeleton, zombie, zombie, spider]。
     */
    static ResourceLocation randomEntityId(math::Random& random);
};

/**
 * @brief 配置化地牢特征
 *
 * 将 MonsterRoomFeature 适配到 ConfiguredFeatureBase 流水线。
 * 配置为空（NoneFeatureConfiguration），概率由 placed_feature 的 Count 提供。
 * 装饰阶段为 UndergroundStructures。
 */
class ConfiguredMonsterRoomFeature : public ConfiguredFeatureBase {
public:
    ConfiguredMonsterRoomFeature();

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return "monster_room"; }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::UndergroundStructures; }

private:
    mutable MonsterRoomFeature m_feature;
};

} // namespace world::gen::feature
} // namespace mc
