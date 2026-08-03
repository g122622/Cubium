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

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/template/Template.hpp"
#include "common/world/gen/feature/template/TemplateManager.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc::world::gen::structure {

/**
 * @brief 海底废墟类型
 */
enum class OceanRuinType : u8 {
    Warm, ///< 暖海废墟（砂岩材质）
    Cold  ///< 冷海废墟（石砖材质）
};

/**
 * @brief 海底废墟配置
 */
struct OceanRuinConfig {
    OceanRuinType biomeType = OceanRuinType::Cold;
    f32 largeProbability = 0.3f;   ///< 大型废墟概率
    f32 clusterProbability = 0.9f; ///< 集群概率
};

/**
 * @brief 海底废墟结构片段
 *
 * 使用模板系统生成海底废墟。
 */
class OceanRuinPiece : public StructurePiece {
public:
    /**
     * @brief 构造函数
     * @param templateName 模板名称（资源位置）
     * @param position 放置位置
     * @param rotation 旋转角度
     * @param integrity 完整度 (0.0-1.0)
     * @param type 海底废墟类型
     * @param isLarge 是否为大型废墟
     */
    OceanRuinPiece(const std::string& templateName,
        const BlockPos& position,
        Rotation rotation,
        f32 integrity,
        OceanRuinType type,
        bool isLarge);

    ~OceanRuinPiece() override = default;

    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    /**
     * @brief 设置模板管理器
     *
     * 必须在 generate 之前调用
     */
    void setTemplateManager(feature::template_::TemplateManager* manager) { m_templateManager = manager; }

    [[nodiscard]] const std::string& templateName() const { return m_templateName; }
    [[nodiscard]] f32 integrity() const { return m_integrity; }
    [[nodiscard]] OceanRuinType ruinType() const { return m_type; }
    [[nodiscard]] bool isLarge() const { return m_isLarge; }

private:
    void _loadTemplate();

    std::string m_templateName;
    Rotation m_rotation;
    f32 m_integrity;
    OceanRuinType m_type;
    bool m_isLarge;
    feature::template_::TemplateManager* m_templateManager = nullptr;
    const feature::template_::Template* m_template = nullptr;
    BlockPos m_size;
};

/**
 * @brief 海底废墟结构
 *
 * 使用模板系统生成海底废墟，支持暖海/冷海两种材质风格。
 */
class OceanRuinStructure : public Structure {
public:
    explicit OceanRuinStructure(ResourceLocation id);

    [[nodiscard]] const std::string& name() const override { return m_name; }

    /**
     * @brief 获取结构关联的生物群系标签
     *
     * 返回 minecraft:has_structure/ocean_ruin_cold 标签。
     * 海底废墟同时处理冷海和暖海两种变体，
     * 实际变体区分在 canGenerate() 中根据生物群系判断。
     */
    [[nodiscard]] const biome::BiomeTag* defaultBiomeTag() const override;

    [[nodiscard]] bool canGenerate(
        IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) override;

    [[nodiscard]] std::unique_ptr<StructureStart> generate(
        IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const override;

    /**
     * @brief 设置模板管理器
     */
    void setTemplateManager(feature::template_::TemplateManager* manager) { m_templateManager = manager; }

    /**
     * @brief 设置配置
     */
    void setConfig(const OceanRuinConfig& config) { m_config = config; }

    [[nodiscard]] const OceanRuinConfig& config() const { return m_config; }

    // 模板名称常量（公开供测试访问）
    static const std::vector<std::string> s_warmTemplates;
    static const std::vector<std::string> s_warmBigTemplates;
    static const std::vector<std::string> s_brickTemplates;
    static const std::vector<std::string> s_brickBigTemplates;
    static const std::vector<std::string> s_crackedTemplates;
    static const std::vector<std::string> s_crackedBigTemplates;
    static const std::vector<std::string> s_mossyTemplates;
    static const std::vector<std::string> s_mossyBigTemplates;

private:
    /**
     * @param templateManager 模板管理器
     * @param pos 放置位置
     * @param rotation 旋转
     * @param pieces 输出片段列表
     * @param rng 随机数生成器
     * @param config 配置
     * @param isLarge 是否为大型废墟
     * @param integrity 完整度
     */
    void generatePiece(feature::template_::TemplateManager& templateManager,
        const BlockPos& pos,
        Rotation rotation,
        std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::Random& rng,
        const OceanRuinConfig& config,
        bool isLarge,
        f32 integrity) const;

    /**
     * @brief 生成集群片段
     * @param templateManager 模板管理器
     * @param rng 随机数生成器
     * @param rotation 旋转
     * @param pos 放置位置
     * @param config 配置
     * @param pieces 输出片段列表
     */
    void generateClusterPieces(feature::template_::TemplateManager& templateManager,
        math::Random& rng,
        Rotation rotation,
        const BlockPos& pos,
        const OceanRuinConfig& config,
        std::vector<std::unique_ptr<StructurePiece>>& pieces) const;

    /**
     * @brief 获取候选位置列表
     * @param rng 随机数生成器
     * @param x X 坐标
     * @param z Z 坐标
     * @return 候选位置列表
     */
    [[nodiscard]] std::vector<BlockPos> getCandidatePositions(math::Random& rng, i32 x, i32 z) const;

    [[nodiscard]] bool _isWarmBiome(BiomeId biomeId) const noexcept;

    static const std::string m_name;
    OceanRuinConfig m_config;
    feature::template_::TemplateManager* m_templateManager = nullptr;
};

} // namespace mc::world::gen::structure
