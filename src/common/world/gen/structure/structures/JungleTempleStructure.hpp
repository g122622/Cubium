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
 * @brief 丛林神庙结构
 *
 * 丛林神庙是生成在丛林生物群系的结构，包含陷阱和谜题。
 *
 * 特点：
 * - 生成在丛林生物群系
 * - 包含谜题机关（拉杆谜题）
 * - 箭矢陷阱
 * - 隐藏宝箱
 * - 苔石和錾制石砖装饰
 */
class JungleTempleStructure : public Structure {
public:
    JungleTempleStructure() noexcept;

    [[nodiscard]] const std::string& name() const noexcept override { return m_name; }
    [[nodiscard]] StructureSeparationSettings separationSettings() const noexcept override { return m_settings; }
    [[nodiscard]] const std::vector<BiomeId>& validBiomes() const noexcept override { return m_validBiomes; }

    /**
     * @brief 检查是否可以生成
     */
    [[nodiscard]] bool canGenerate(
        IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) override;

    /**
     * @brief 生成丛林神庙
     */
    [[nodiscard]] std::unique_ptr<StructureStart> generate(
        IWorldWriter& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const override;

private:
    void _initializeBiomes() noexcept;
    void _generateTemple(IWorldWriter& world, math::Random& rng, const BlockPos& startPos) const;

    // spacing=32, separation=8, salt=14357619
    static constexpr StructureSeparationSettings m_settings{32, 8, 14357619};
    static const std::string m_name;
    std::vector<BiomeId> m_validBiomes;
};

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
