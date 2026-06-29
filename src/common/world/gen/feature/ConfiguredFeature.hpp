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
#include "common/util/math/random/Random.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

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
 */
class ConfiguredFeatureBase {
public:
    virtual ~ConfiguredFeatureBase() = default;

    /**
     * @brief 在指定位置放置特征
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
        const BlockPos& pos) = 0;

    /**
     * @brief 获取特征名称
     */
    [[nodiscard]] virtual const char* name() const = 0;

    /**
     * @brief 获取装饰阶段
     */
    [[nodiscard]] virtual DecorationStage stage() const = 0;

    /**
     * @brief 获取特征ID
     *
     * 由 FeatureRegistry 在注册时自动赋值。
     * 用于 BiomeFilterPlacement 反向查询生物群系是否包含此特征。
     */
    [[nodiscard]] u32 featureId() const noexcept { return m_featureId; }

    /**
     * @brief 设置特征ID（仅由 FeatureRegistry 调用）
     */
    void setFeatureId(u32 id) noexcept { m_featureId = id; }

private:
    u32 m_featureId = 0;
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

/**
 * @brief 特征注册表
 *
 * 管理所有配置化特征，按装饰阶段组织。
 */
class FeatureRegistry {
public:
    /**
     * @brief 获取单例实例
     */
    static FeatureRegistry& instance();

    /**
     * @brief 初始化所有特征
     */
    void initialize();

    /**
     * @brief 注册配置化特征
     * @param feature 特征
     * @param stage 装饰阶段
     */
    void registerFeature(std::unique_ptr<ConfiguredFeatureBase> feature, DecorationStage stage);

    /**
     * @brief 获取指定阶段的所有特征
     * @param stage 装饰阶段
     * @return 特征列表
     */
    [[nodiscard]] const std::vector<ConfiguredFeatureBase*>& getFeatures(DecorationStage stage) const;

    /**
     * @brief 根据特征ID获取特征
     * @param featureId 特征ID
     * @return 特征指针，如果ID无效则返回nullptr
     */
    [[nodiscard]] ConfiguredFeatureBase* getFeatureById(u32 featureId) const;

    /**
     * @brief 根据特征名称获取特征
     *
     * 用于 FeatureJigsawPiece::place() 通过 ResourceLocation 名称（如 "minecraft:oak_tree"）
     * 查找配置化特征。名称匹配去除 "minecraft:" 前缀后与 ConfiguredFeatureBase::name() 比较
     * （name() 返回不带命名空间前缀的简单名称，如 "oak_tree"）。
     *
     * @param name 特征名称（可带或不带 "minecraft:" 前缀）
     * @return 特征指针，如果名称未注册则返回 nullptr
     */
    [[nodiscard]] ConfiguredFeatureBase* getFeatureByName(const std::string& name) const;

    /**
     * @brief 获取所有特征
     * @return 所有特征（按阶段组织）
     */
    [[nodiscard]] const std::vector<std::vector<ConfiguredFeatureBase*>>& getAllFeatures() const
    {
        return m_featuresByStage;
    }

    /**
     * @brief 清除所有特征
     */
    void clear();

private:
    FeatureRegistry();
    ~FeatureRegistry();

    // 存储所有特征的所有权
    std::vector<std::unique_ptr<ConfiguredFeatureBase>> m_ownedFeatures;

    // 按阶段索引的特征指针（不拥有所有权）
    std::vector<std::vector<ConfiguredFeatureBase*>> m_featuresByStage;

    // 按名称索引的特征指针（不拥有所有权），用于 getFeatureByName 查找
    std::unordered_map<std::string, ConfiguredFeatureBase*> m_featuresByName;
};

} // namespace mc
