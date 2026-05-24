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

#include "PillagerOutpostStructure.hpp"
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

const std::string PillagerOutpostStructure::s_name = "Pillager_Outpost";
const std::vector<BiomeId> PillagerOutpostStructure::s_validBiomes = {Plains,
    Desert,
    Savanna,
    Taiga,
    SnowyPlains,
    SnowyTaiga,
    SavannaPlateau,
    WoodedHills,
    BirchForest,
    DarkForest,
    TaigaHills,
    GiantTreeTaiga,
    GiantTreeTaigaHills};

PillagerOutpostStructure::PillagerOutpostStructure()
    : JigsawStructure(JigsawConfig(ResourceLocation("minecraft", "pillager_outpost/base_plates"), 7), 0, true, true)
{}

bool PillagerOutpostStructure::canGenerate(
    IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ)
{
    MC_UNUSED(world);

    // 检查生物群系
    BiomeId biome = generator.getBiome(chunkX * 16 + 8, 64, chunkZ * 16 + 8);
    for (BiomeId valid : s_validBiomes) {
        if (biome == valid) {
            // MC 1.16.5: 20% 的生成概率
            i32 i = chunkX >> 4;
            i32 j = chunkZ >> 4;
            rng.setSeed(static_cast<i64>(i ^ j << 4) ^ static_cast<i64>(generator.seed()));
            (void)rng.nextInt();

            if (rng.nextInt(5) != 0) {
                return false;
            }

            // 检查附近是否有村庄
            return !isNearVillage(generator, static_cast<i64>(generator.seed()), rng, chunkX, chunkZ);
        }
    }
    return false;
}

bool PillagerOutpostStructure::isNearVillage(
    IChunkGenerator& generator, i64 seed, math::Random& rng, i32 chunkX, i32 chunkZ) const
{
    MC_UNUSED(generator);
    MC_UNUSED(seed);
    MC_UNUSED(rng);
    MC_UNUSED(chunkX);
    MC_UNUSED(chunkZ);

    // MC 1.16.5: 检查 21x21 区块范围内是否有村庄起始位置
    // 如果在范围内有村庄，则不生成前哨站
    // 简化实现: 暂不实现村庄检测，允许前哨站在任何位置生成
    // 完整实现需要查询 VillageStructure 的起始位置
    return false;
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
