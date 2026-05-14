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

#include "../Structure.hpp"
#include <memory>

namespace mc::world::gen::structure {

/**
 * @brief 埋藏的宝藏结构片段
 */
class BuriedTreasurePiece : public StructurePiece {
public:
    BuriedTreasurePiece(i32 x, i32 y, i32 z);

    /**
     * @brief 在区块中生成片段
     */
    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds) override;

private:
    /**
     * @brief 检查位置是否在区块边界内
     */
    [[nodiscard]] bool isInBounds(i32 x, i32 y, i32 z, const StructureBoundingBox& chunkBounds) const;
};

/**
 * @brief 埋藏的宝藏结构
 *
 * 最简单的结构类型，只包含一个箱子。
 * 参考 MC 1.16.5: BuriedTreasureStructure
 */
class BuriedTreasureStructure : public Structure {
public:
    BuriedTreasureStructure()
        : Structure(StructureType::BuriedTreasure)
    {}

    [[nodiscard]] const std::string& name() const override { return m_name; }
    [[nodiscard]] StructureSeparationSettings separationSettings() const override { return m_settings; }
    [[nodiscard]] const std::vector<BiomeId>& validBiomes() const override { return m_validBiomes; }

    /**
     * @brief 检查是否可以生成
     * 埋藏的宝藏只在沙滩类生物群系生成，且概率较低
     */
    [[nodiscard]] bool canGenerate(
        IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) override;

    /**
     * @brief 生成埋藏的宝藏
     * 在区块中心生成一个宝藏箱子
     */
    [[nodiscard]] std::unique_ptr<StructureStart> generate(
        IWorldWriter& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const override;

private:
    // MC 1.16.5: spacing=1, separation=0, salt=0
    // BuriedTreasure使用DimensionStructuresSettings中的默认配置
    // 注意：canGenerate中使用单独的salt(10387320)来计算概率种子
    // 参考: DimensionStructuresSettings.java 第21行
    static constexpr StructureSeparationSettings m_settings{1, 0, 0};
    static const std::string m_name;
    static const std::vector<BiomeId> m_validBiomes;
};

} // namespace mc::world::gen::structure
