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

namespace mc {
namespace world {
namespace gen {
namespace structure {

/**
 * @brief 雪屋结构片段
 *
 * 对应原版 `IglooPieces.IglooPiece`：每个模板（地上 / 中间层 / 地下室）各是一个独立构件，
 * 它们的包围盒在构造期由模板尺寸绕各自的旋转枢轴展开，后处理的顺序也与装配顺序一致。
 */
class IglooPiece : public StructurePiece {
public:
    /**
     * @brief 构造一个雪屋构件
     * @param templateName 模板资源路径（`igloo/top`、`igloo/middle`、`igloo/bottom`）
     * @param position 模板原点附近的地面锚点，组装时已按模板偏移调整过
     * @param rotation 旋转
     */
    IglooPiece(const std::string& templateName, const BlockPos& position, Rotation rotation);

    ~IglooPiece() override = default;

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

    [[nodiscard]] const BlockPos& templatePosition() const
    {
        return m_templatePosition;
    }

private:
    void _updateBoundingBox();

    /**
     * @brief 处理模板的 DATA 标记（原版 IglooPiece.handleDataMarker）
     *
     * 雪屋只有 "chest" 一种标记：清掉标记处的结构方块，并给正下方的
     * 箱子挂上 minecraft:chests/igloo_chest 战利品表。
     *
     * @param metadata 标记字符串（原版结构方块的 metadata）
     * @param pos 标记的世界坐标
     * @param world 世界
     * @param rng 随机源（战利品表种子）
     */
    void _handleDataMarker(const std::string& metadata, const BlockPos& pos, IWorld& world, math::IRandom& rng);

    std::string m_templateName;
    BlockPos m_templatePosition;
    BlockPos m_size{1, 1, 1};
    feature::template_::TemplateManager* m_templateManager = nullptr;
    const feature::template_::Template* m_template = nullptr;
};

/**
 * @brief 雪屋结构
 *
 * 生成点是区块中心的最高地表，构件围绕 Y=90 建造；地下室按 50% 概率附带。
 */
class IglooStructure : public Structure {
public:
    explicit IglooStructure(ResourceLocation id);

    [[nodiscard]] const std::string& name() const override
    {
        return s_name;
    }

    [[nodiscard]] const biome::BiomeTag* defaultBiomeTag() const override;

    [[nodiscard]] std::unique_ptr<StructureStart> generate(
        IChunkGenerator& generator, math::IRandom& rng, i32 chunkX, i32 chunkZ) const override;

    void setTemplateManager(feature::template_::TemplateManager* manager)
    {
        m_templateManager = manager;
    }

    /// 模板路径常量
    static const std::string s_name;
    static const std::string s_topTemplateName;
    static const std::string s_middleTemplateName;
    static const std::string s_bottomTemplateName;

private:
    feature::template_::TemplateManager* m_templateManager = nullptr;
};

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
