/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the the rights
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

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"

#include <functional>
#include <memory>
#include <optional>

namespace mc::world::gen::structure::placement {

/**
 * @brief 随机分布类型
 *
 * 控制结构在网格内的偏移分布方式。
 * - Linear: 均匀随机分布
 * - Triangular: 两次随机取平均，产生更集中的分布
 */
enum class RandomSpreadType : u8 {
    Linear,    ///< 均匀随机分布
    Triangular ///< 三角分布（两次随机取平均，更集中）
};

/**
 * @brief 频率缩减方法
 *
 * 不同的结构使用不同的频率缩减算法来决定候选区块是否真正生成结构。
 * 这些方法对应 MC 中不同历史时期的结构放置逻辑。
 */
enum class FrequencyReductionMethod : u8 {
    Default,     ///< 使用 setLargeFeatureWithSalt 后 nextFloat() < frequency
    LegacyType1, ///< 掠夺者前哨站风格：基于方块坐标的种子
    LegacyType2, ///< 埋藏宝藏风格：使用固定盐值 10387320
    LegacyType3  ///< 废弃矿井风格：setLargeFeatureSeed + nextDouble
};

/**
 * @brief 排斥区配置
 *
 * 防止两种结构在同一个区块附近同时生成。
 * 例如掠夺者前哨站需要远离村庄。
 */
struct ExclusionZone {
    ResourceLocation otherSetId; ///< 被排斥的另一个 StructureSet 的 ID
    i32 chunkCount;              ///< 搜索半径（1-16），表示在多少区块范围内检查
};

/**
 * @brief 结构放置基类
 *
 * 决定结构在世界中的空间分布方式。
 * 两个子类实现：
 * - RandomSpreadStructurePlacement: 基于网格的均匀/三角分布（大多数结构）
 * - ConcentricRingsStructurePlacement: 同心环分布（要塞）
 */
class StructurePlacement {
public:
    virtual ~StructurePlacement() = default;

    /**
     * @brief 检查指定区块是否是结构生成区块
     *
     * 三步检查流程：
     * 1. 计算候选区块位置
     * 2. 频率缩减检查
     * 3. 排斥区检查
     *
     * @param worldSeed 世界种子
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @return 是否在此区块生成结构
     */
    virtual bool isStructureChunk(i64 worldSeed, i32 chunkX, i32 chunkZ) const = 0;

    /**
     * @brief 获取结构的定位位置（用于 /locate 命令）
     *
     * 默认实现返回区块原点加上定位偏移。
     *
     * @param chunkPos 结构所在区块坐标
     * @return 定位位置（方块坐标）
     */
    virtual BlockPos getLocatePos(const world::chunk::ChunkPos& chunkPos) const;

    /**
     * @brief 克隆此放置策略
     * @return 新的深拷贝实例
     */
    virtual std::unique_ptr<StructurePlacement> clone() const = 0;

    /**
     * @brief 设置排斥区检查回调
     *
     * 用于检查另一个 StructureSet 是否在指定区块附近有结构。
     * 回调参数：(structureSetId, chunkX, chunkZ, worldSeed)
     * 返回 true 表示该 StructureSet 在附近有结构。
     *
     * @param callback 排斥区检查回调函数
     */
    void setExclusionZoneChecker(std::function<bool(const ResourceLocation&, i32, i32, i64)> callback);

protected:
    /**
     * @brief 频率缩减检查
     *
     * 根据 m_frequencyReduction 方法判断候选区块是否通过频率检查。
     *
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @param worldSeed 世界种子
     * @return 是否通过频率检查
     */
    bool applyAdditionalChunkRestrictions(i32 chunkX, i32 chunkZ, i64 worldSeed) const;

    /**
     * @brief 排斥区检查
     *
     * 检查另一个 StructureSet 是否在此区块附近有结构，
     * 如果有则当前区块不能生成结构。
     *
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @param worldSeed 世界种子
     * @return 是否通过排斥区检查（true = 没有排斥，可以生成）
     */
    bool applyInteractionsWithOtherStructures(i32 chunkX, i32 chunkZ, i64 worldSeed) const;

    /// 定位偏移（/locate 命令使用的偏移量）
    math::Vector3i m_locateOffset;
    /// 频率缩减方法
    FrequencyReductionMethod m_frequencyReduction;
    /// 生成频率（0.0 ~ 1.0），1.0 表示所有候选区块都生成
    f32 m_frequency;
    /// 种子盐值，确保不同结构使用不同的随机序列
    i64 m_salt;
    /// 排斥区配置（可选）
    std::optional<ExclusionZone> m_exclusionZone;

private:
    /// 排斥区检查回调（由外部注入）
    std::function<bool(const ResourceLocation&, i32, i32, i64)> m_exclusionZoneChecker;
};

} // namespace mc::world::gen::structure::placement
