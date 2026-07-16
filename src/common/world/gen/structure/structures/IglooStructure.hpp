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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR THE DEALINGS IN THE
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
 * @brief 雪屋结构片段
 *
 * 使用模板系统生成雪屋，支持地上部分和地下室。
 */
class IglooPiece : public StructurePiece {
public:
    /**
     * @brief 构造函数
     * @param position 放置位置（地上部分的底部）
     * @param rotation 旋转角度
     * @param hasBasement 是否有地下室
     * @param middleCount 中间层数量（0-2）
     */
    IglooPiece(const BlockPos& position, Rotation rotation, bool hasBasement, i32 middleCount);

    ~IglooPiece() override = default;

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

    [[nodiscard]] bool hasBasement() const { return m_hasBasement; }
    [[nodiscard]] i32 middleCount() const { return m_middleCount; }

private:
    void _loadTemplates();
    void _generateTop(IWorldWriter& world, math::Random& rng, const StructureBoundingBox& bounds);
    void _generateMiddle(IWorldWriter& world, math::Random& rng, i32 index, const StructureBoundingBox& bounds);
    void _generateBottom(IWorldWriter& world, math::Random& rng, const StructureBoundingBox& bounds);
    void _updateBoundingBox();

    Rotation m_rotation;
    bool m_hasBasement;
    i32 m_middleCount; ///< 中间层数量（0-2）

    feature::template_::TemplateManager* m_templateManager = nullptr;
    const feature::template_::Template* m_topTemplate = nullptr;
    const feature::template_::Template* m_middleTemplate = nullptr;
    const feature::template_::Template* m_bottomTemplate = nullptr;

    BlockPos m_topSize{1, 1, 1};
    BlockPos m_middleSize{1, 1, 1};
    BlockPos m_bottomSize{1, 1, 1};
};

/**
 * @brief 雪屋结构
 *
 * 在雪地生物群系中生成的小型雪屋结构。
 * 50% 概率生成地下室。
 * 使用模板系统从 igloo/top.nbt, igloo/middle.nbt, igloo/bottom.nbt 加载。
 */
class IglooStructure : public Structure {
public:
    IglooStructure();

    [[nodiscard]] const std::string& name() const override { return s_name; }

    /**
     * @brief 获取结构关联的生物群系标签
     *
     * 返回 minecraft:has_structure/igloo 标签，用于 O(1) 生物群系查找。
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

    // 模板名称常量
    static const std::string s_topTemplateName;
    static const std::string s_middleTemplateName;
    static const std::string s_bottomTemplateName;

private:
    static const std::string s_name;
    feature::template_::TemplateManager* m_templateManager = nullptr;
};

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
