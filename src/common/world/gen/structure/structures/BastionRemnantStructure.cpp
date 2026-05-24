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

#include "BastionRemnantStructure.hpp"
#include "../../../../resource/ResourceLocation.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../biome/Biome.hpp"
#include "../../../block/BlockPos.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include <spdlog/spdlog.h>

namespace mc {
namespace world {
namespace gen {
namespace structure {

using namespace mc::Biomes;

const std::string BastionRemnantStructure::s_name = "Bastion_Remnant";
// MC 1.16.5: 堡垒遗迹在除玄武岩三角洲外的所有下界生物群系中生成
const std::vector<BiomeId> BastionRemnantStructure::s_validBiomes = {
    NetherWastes, CrimsonForest, WarpedForest, SoulSandValley};

BastionRemnantStructure::BastionRemnantStructure()
    : JigsawStructure(JigsawConfig(ResourceLocation("minecraft", "bastion/starts"), 7), 0, true, true)
{}

bool BastionRemnantStructure::canGenerate(
    IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ)
{
    MC_UNUSED(world);
    MC_UNUSED(rng);

    // 检查生物群系 - 堡垒遗迹在所有下界生物群系中生成（玄武岩三角洲除外）
    BiomeId biome = generator.getBiome(chunkX * 16 + 8, 64, chunkZ * 16 + 8);
    for (BiomeId valid : s_validBiomes) {
        if (biome == valid) {
            return true;
        }
    }
    return false;
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
