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

#include "../../chunk/IChunkGenerator.hpp"
#include "../Structure.hpp"
#include <memory>

namespace mc {
namespace world {
namespace gen {
namespace structure {

/**
 * @brief 海洋纪念碑结构
 *
 * 海洋纪念碑是生成在深海的大型结构，由海晶石构成。
 * 参考 MC 1.16.5: OceanMonumentStructure
 *
 * 特点：
 * - 生成在深海生物群系
 * - 由海晶石、海晶灯、暗海晶石构成
 * - 包含守卫者和远古守卫者
 * - 有复杂的房间布局和宝藏
 */
class OceanMonumentStructure : public Structure {
public:
    OceanMonumentStructure();

    [[nodiscard]] const std::string& name() const override { return m_name; }
    [[nodiscard]] StructureSeparationSettings separationSettings() const override { return m_settings; }
    [[nodiscard]] const std::vector<BiomeId>& validBiomes() const override { return m_validBiomes; }

    /**
     * @brief 海洋纪念碑使用非均匀间距分布
     *
     * MC 1.16.5: OceanMonumentStructure.func_230365_b_() 返回 false
     * 使用两次随机平均值作为偏移，产生更集中的分布
     */
    [[nodiscard]] bool useUniformSpacing() const override { return false; }

    /**
     * @brief 检查是否可以生成
     */
    [[nodiscard]] bool canGenerate(
        IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) override;

    /**
     * @brief 生成海洋纪念碑
     */
    [[nodiscard]] std::unique_ptr<StructureStart> generate(
        IWorldWriter& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const override;

private:
    void initializeBiomes();
    void generateMonument(IWorldWriter& world, math::Random& rng, const BlockPos& startPos) const;
    void generateWing(IWorldWriter& world,
        const BlockState* prismarine,
        const BlockState* darkPrismarine,
        const BlockState* seaLantern,
        i32 baseX,
        i32 baseY,
        i32 baseZ,
        i32 width,
        i32 height,
        i32 depth,
        bool isLeft) const;
    void generateRoom(IWorldWriter& world,
        const BlockState* prismarine,
        const BlockState* seaLantern,
        i32 baseX,
        i32 baseY,
        i32 baseZ,
        i32 width,
        i32 height,
        i32 depth) const;

    /**
     * @brief 检查生物群系是否属于海洋或河流类别
     * MC 1.16.5: Biome.Category.OCEAN 或 Biome.Category.RIVER
     */
    [[nodiscard]] bool isOceanOrRiverBiome(BiomeId biomeId) const;

    static constexpr StructureSeparationSettings m_settings{32, 5, 10387313};
    static const std::string m_name;
    std::vector<BiomeId> m_validBiomes;
};

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
