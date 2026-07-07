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

#include "DecorationStage.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include <memory>

namespace mc {

// 前向声明
class WorldGenRegion;
namespace world::chunk {
class ChunkPrimer;
}
using world::chunk::ChunkPrimer;
class IChunkGenerator;
class ConfiguredPlacement;
struct OreFeatureConfig;

/**
 * @brief 配置化特征基类
 *
 * 组合特征与其放置配置。
 *
 * 标识体系：每个配置化特征用 ResourceLocation 唯一标识（对应 configured_feature JSON
 * 文件名，如 "minecraft:monster_room"）。取代旧的 u32 featureId 机制。
 */
class ConfiguredFeatureBase {
public:
    virtual ~ConfiguredFeatureBase() = default;

    /**
     * @brief 在指定位置放置特征
     *
     * 标记为 const：特征对象本身在放置过程中不可变，仅通过 region/chunk 写入世界。
     * 这使 const ConfiguredFeatureBase* 可调用 place()，与 PlacedFeature::place() const
     * 及 ConfiguredFeatureRegistry::get() 返回 const 指针的语义一致。
     *
     * @param region 世界生成区域
     * @param chunk 区块数据
     * @param generator 区块生成器
     * @param random 随机数生成器
     * @param pos 起始位置
     * @return 是否成功放置
     */
    virtual bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const = 0;

    /**
     * @brief 获取特征名称（feature type 字符串，如 "monster_room"）
     */
    [[nodiscard]] virtual const char* name() const = 0;

    /**
     * @brief 获取装饰阶段
     */
    [[nodiscard]] virtual DecorationStage stage() const = 0;

    /**
     * @brief 获取特征的 ResourceLocation 标识
     *
     * 由 ConfiguredFeatureLoader 在注册时根据 JSON 文件名赋值。
     * 用于 BiomeFilterPlacement 反向查询生物群系是否包含此特征。
     */
    [[nodiscard]] const ResourceLocation& id() const noexcept { return m_id; }

    /**
     * @brief 设置特征标识（仅由 ConfiguredFeatureLoader 调用）
     */
    void setId(ResourceLocation id) noexcept { m_id = std::move(id); }

private:
    ResourceLocation m_id;
};

/**
 * @brief 配置化矿石特征
 *
 * 组合矿石特征、配置和放置规则。
 */
class ConfiguredOreFeature;

/**
 * @brief 配置化树木特征
 */
class ConfiguredTreeFeature;

} // namespace mc
