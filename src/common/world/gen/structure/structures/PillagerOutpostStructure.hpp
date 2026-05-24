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

#include "../JigsawStructure.hpp"
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace structure {

/**
 * @brief 掠夺者前哨站结构
 *
 * 在平原、沙漠、热带草原、针叶林、雪地等生物群系生成的塔楼结构。
 * 包含掠夺者塔楼和周围的辅助设施（笼子、帐篷、原木堆等）。
 * 掠夺者会在前哨站内生成。
 *
 * 参考: MC 1.16.5 PillagerOutpostStructure.java
 */
class PillagerOutpostStructure : public JigsawStructure {
public:
    PillagerOutpostStructure();
    ~PillagerOutpostStructure() override = default;

    [[nodiscard]] const std::string& name() const override { return s_name; }
    [[nodiscard]] StructureSeparationSettings separationSettings() const override { return s_settings; }
    [[nodiscard]] const std::vector<BiomeId>& validBiomes() const override { return s_validBiomes; }

    [[nodiscard]] bool canGenerate(
        IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) override;

private:
    /**
     * @brief 检查附近是否有村庄
     *
     * 掠夺者前哨站不会在村庄附近生成（至少10个区块距离）
     */
    [[nodiscard]] bool isNearVillage(
        IChunkGenerator& generator, i64 seed, math::Random& rng, i32 chunkX, i32 chunkZ) const;

    static const std::string s_name;
    static constexpr StructureSeparationSettings s_settings{32, 8, 165745296};
    static const std::vector<BiomeId> s_validBiomes;
};

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
