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
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/feature/template/Template.hpp"
#include "server/world/gen/feature/template/TemplateManager.hpp"
#include "server/world/gen/structure/Structure.hpp"
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
    OceanRuinType biomeType; ///< 废墟风格（按生成点群系判定后填入）
    f32 largeProbability;    ///< 大型废墟概率（数据包 `large_probability`）
    f32 clusterProbability;  ///< 集群概率（数据包 `cluster_probability`）
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
        math::IRandom& rng,
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
    void setTemplateManager(feature::template_::TemplateManager* manager)
    {
        m_templateManager = manager;
    }

    [[nodiscard]] const std::string& templateName() const
    {
        return m_templateName;
    }

    [[nodiscard]] f32 integrity() const
    {
        return m_integrity;
    }

    [[nodiscard]] OceanRuinType ruinType() const
    {
        return m_type;
    }

    [[nodiscard]] bool isLarge() const
    {
        return m_isLarge;
    }

private:
    void _loadTemplate();

    std::string m_templateName;
    BlockPos m_templatePosition;
    // 不能用「子类同名成员」保存旋转：它会遮蔽 StructurePiece::m_rotation，
    // 而 getRotation()/getMirror() 读的是基类成员，差分里表现为「抽到了旋转却始终输出 NONE」。
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

    [[nodiscard]] const std::string& name() const override
    {
        return m_name;
    }

    /**
     * @brief 获取结构关联的生物群系标签
     *
     * 返回 minecraft:has_structure/ocean_ruin_cold 标签。
     * 暖/冷变体由结构定义 JSON 的 `biome_temp` 决定，不按落点群系现判。
     */
    [[nodiscard]] const biome::BiomeTag* defaultBiomeTag() const override;

    [[nodiscard]] std::unique_ptr<StructureStart> generate(
        IChunkGenerator& generator, math::IRandom& rng, i32 chunkX, i32 chunkZ) const override;

    /**
     * @brief 设置模板管理器
     */
    void setTemplateManager(feature::template_::TemplateManager* manager)
    {
        m_templateManager = manager;
    }

    /**
     * @brief 设置配置（两个概率来自数据包的 `large_probability` / `cluster_probability`）
     */
    void setConfig(const OceanRuinConfig& config)
    {
        m_config = config;
    }

    [[nodiscard]] const OceanRuinConfig& config() const
    {
        return m_config;
    }

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
    [[nodiscard]] bool _isWarmBiome(BiomeId biomeId) const noexcept;

    static const std::string m_name;
    OceanRuinConfig m_config;
    feature::template_::TemplateManager* m_templateManager = nullptr;
};

} // namespace mc::world::gen::structure
