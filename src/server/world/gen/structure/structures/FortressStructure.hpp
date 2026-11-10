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
#include "common/util/math/random/Random.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "common/world/generation/base/DecorationStage.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include <memory>
#include <string>

namespace mc::world::gen::structure {

/**
 * @brief 下界要塞结构
 *
 * 原版要塞是**程序化构件**结构（`NetherFortressPieces`），不是模板池拼图：
 * 起始片段固定在区块内 (minX+2, 64, minZ+2)，以随机水平朝向展开 19×10×19 的桥；
 * 之后按权重在两个片段表（桥段 / 城堡段）里抽签向下展开，最后整座要塞被
 * `moveInsideHeights(48, 70)` 平移到高度区间内。
 *
 * 片段几何与方块放置全部在 `NetherFortressPieces` 中，本类只负责起点判定与装配循环。
 */
class FortressStructure : public Structure {
public:
    explicit FortressStructure(ResourceLocation id);

    [[nodiscard]] const std::string& name() const override
    {
        return m_name;
    }

    [[nodiscard]] DecorationStage defaultDecorationStage() const override
    {
        return DecorationStage::UndergroundDecoration;
    }

    [[nodiscard]] const SpawnOverrides* spawnOverrides() const override
    {
        return &s_spawnOverrides;
    }

    /**
     * @brief 获取结构关联的生物群系标签
     *
     * 返回 `minecraft:has_structure/nether_fortress`，用于判断要塞可生成的生物群系。
     */
    [[nodiscard]] const biome::BiomeTag* defaultBiomeTag() const override;

    /**
     * @brief 生成下界要塞起点（仅装配片段，禁止写方块）
     */
    [[nodiscard]] std::unique_ptr<StructureStart> generate(
        IChunkGenerator& generator, math::IRandom& rng, i32 chunkX, i32 chunkZ) const override;

private:
    static const std::string m_name;
    static const SpawnOverrides s_spawnOverrides;
};

} // namespace mc::world::gen::structure
