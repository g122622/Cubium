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

#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/template/Template.hpp"
#include "common/world/gen/feature/template/TemplateManager.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include <memory>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace structure {

/**
 * @brief 沉船配置
 */
struct ShipwreckConfig {
    bool isBeached = false; ///< 是否为搁浅沉船（在沙滩上）
};

/**
 * @brief 沉船结构片段
 *
 * 使用模板系统生成沉船。
 */
class ShipwreckPiece : public StructurePiece {
public:
    /**
     * @brief 构造函数
     * @param templateName 模板名称（资源位置）
     * @param position 放置位置
     * @param rotation 旋转角度
     * @param isBeached 是否为搁浅沉船
     */
    ShipwreckPiece(const std::string& templateName, const BlockPos& position, Rotation rotation, bool isBeached);

    ~ShipwreckPiece() override = default;

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
    [[nodiscard]] bool isBeached() const { return m_isBeached; }

    // 结构偏移
    static const BlockPos STRUCTURE_OFFSET;

private:
    void _loadTemplate();

    std::string m_templateName;
    Rotation m_rotation;
    bool m_isBeached;
    feature::template_::TemplateManager* m_templateManager = nullptr;
    const feature::template_::Template* m_template = nullptr;
    BlockPos m_size;
};

/**
 * @brief 沉船结构
 *
 * 使用模板系统生成沉船，支持水下和搁浅两种类型。
 */
class ShipwreckStructure : public Structure {
public:
    explicit ShipwreckStructure(ResourceLocation id);

    [[nodiscard]] const std::string& name() const override { return m_name; }

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
    void setConfig(const ShipwreckConfig& config) { m_config = config; }

    [[nodiscard]] const ShipwreckConfig& config() const { return m_config; }

    // 模板名称常量（公开供测试访问）
    // 搁浅沉船变体（只有部分变体）
    static const std::vector<std::string> s_beachedTemplates;
    // 所有沉船变体（包括水下）
    static const std::vector<std::string> s_allTemplates;

private:
    /**
     * @brief 获取随机沉船模板名称
     * @param rng 随机数生成器
     * @param isBeached 是否为搁浅沉船
     * @return 模板名称
     */
    [[nodiscard]] std::string _getRandomTemplateName(math::Random& rng, bool isBeached) const;

    static const std::string m_name;
    ShipwreckConfig m_config;
    feature::template_::TemplateManager* m_templateManager = nullptr;
};

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
