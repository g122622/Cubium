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

#include "../../../core/Constants.hpp"
#include "../../../resource/ResourceLocation.hpp"
#include "../jigsaw/JigsawPattern.hpp"
#include "Structure.hpp"
#include "StructureBoundingBox.hpp"
#include <memory>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace structure {

/**
 * @brief Jigsaw 结构配置
 *
 * 配置 Jigsaw 结构的生成参数，包括起始模板池和结构尺寸。
 */
struct JigsawConfig {
    ResourceLocation startPool; ///< 起始模板池
    i32 size = 7;               ///< 结构尺寸（递归深度）

    JigsawConfig() = default;

    /**
     * @brief 构造 Jigsaw 配置
     * @param pool 起始模板池
     * @param s 结构尺寸
     */
    JigsawConfig(const ResourceLocation& pool, i32 s)
        : startPool(pool)
        , size(s)
    {}
};

/**
 * @brief Jigsaw 结构
 *
 * 使用 Jigsaw 模板池生成的结构，如村庄、掠夺者前哨站等。
 * 支持地形适配和动态组装。
 */
class JigsawStructure : public Structure {
public:
    /**
     * @brief 构造 Jigsaw 结构
     * @param config Jigsaw 配置
     * @param startY 起始 Y 坐标，0 表示自动检测
     * @param nearTerrain 是否需要贴近地形生成
     * @param adjustForTerrain 是否根据地形调整高度
     */
    explicit JigsawStructure(
        const JigsawConfig& config, i32 startY = 0, bool nearTerrain = false, bool adjustForTerrain = false);

    [[nodiscard]] const std::string& name() const override { return m_name; }
    [[nodiscard]] StructureSeparationSettings separationSettings() const override { return m_settings; }
    [[nodiscard]] const std::vector<BiomeId>& validBiomes() const override { return m_validBiomes; }

    /**
     * @brief 检查是否可以在指定位置生成结构
     */
    bool canGenerate(IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) override;

    /**
     * @brief 生成 Jigsaw 结构
     */
    std::unique_ptr<StructureStart> generate(
        IWorldWriter& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const override;

private:
    JigsawConfig m_config;   ///< Jigsaw 配置
    i32 m_startY;            ///< 起始 Y 坐标
    bool m_nearTerrain;      ///< 是否贴近地形
    bool m_adjustForTerrain; ///< 是否根据地形调整

    static constexpr StructureSeparationSettings m_settings{8, 4, 12345};
    static const std::string m_name;
    static const std::vector<BiomeId> m_validBiomes;
};

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
