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

namespace mc::world::gen::structure {

/**
 * @brief 下界化石结构片段
 *
 * 使用模板系统生成下界化石。
 */
class NetherFossilPiece : public StructurePiece {
public:
    /**
     * @brief 构造函数
     * @param templateName 模板名称（资源位置）
     * @param position 放置位置
     * @param rotation 旋转角度
     */
    NetherFossilPiece(const std::string& templateName, const BlockPos& position, Rotation rotation);

    ~NetherFossilPiece() override = default;

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

private:
    void _loadTemplate();

    std::string m_templateName;
    Rotation m_rotation;
    feature::template_::TemplateManager* m_templateManager = nullptr;
    const feature::template_::Template* m_template = nullptr;
    BlockPos m_size;
};

/**
 * @brief 下界化石结构
 *
 * 在下界灵魂沙峡谷生物群系中生成的大型骨块结构。
 * 使用模板系统从 nether_fossils/fossil_1~14.nbt 加载。
 */
class NetherFossilStructure : public Structure {
public:
    NetherFossilStructure();

    [[nodiscard]] const std::string& name() const override { return s_name; }
    [[nodiscard]] DecorationStage defaultDecorationStage() const override
    {
        return DecorationStage::UndergroundDecoration;
    }

    /**
     * @brief 获取下界化石关联的生物群系标签
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

    // 模板名称常量（共14个化石模板）
    static const std::vector<std::string> s_fossilTemplates;

private:
    static const std::string s_name;
    feature::template_::TemplateManager* m_templateManager = nullptr;
};

} // namespace mc::world::gen::structure
