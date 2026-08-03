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
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeGenerationSettings.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include <string>
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
 */
class PillagerOutpostStructure : public JigsawStructure {
public:
    explicit PillagerOutpostStructure(ResourceLocation id);
    ~PillagerOutpostStructure() override = default;

    [[nodiscard]] const std::string& name() const override { return s_name; }

    /**
     * @brief 获取掠夺者前哨站关联的生物群系标签
     */
    [[nodiscard]] const biome::BiomeTag* defaultBiomeTag() const override;

    /**
     * @brief 掠夺者前哨站的生成覆盖
     *
     * 掠夺者在完整结构边界框内生成（1 只掠夺者）。
     */
    [[nodiscard]] const SpawnOverrides* spawnOverrides() const override { return &s_spawnOverrides; }

    [[nodiscard]] bool canGenerate(
        IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) override;

private:
    /**
     * @brief 检查附近是否有村庄
     *
     * 掠夺者前哨站不会在村庄附近生成（至少10个区块距离）
     */
    [[nodiscard]] bool _isNearVillage(
        IChunkGenerator& generator, i64 seed, math::Random& rng, i32 chunkX, i32 chunkZ) const;

    static const std::string s_name;
    static const SpawnOverrides s_spawnOverrides;
};

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
