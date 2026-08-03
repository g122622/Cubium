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
#include "../../../../util/math/MathUtils.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../biome/BiomeTags.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "common/world/gen/structure/JigsawStructure.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include <string>
#include <utility>

namespace mc {
namespace world {
namespace gen {
namespace structure {

using namespace mc::Biomes;
using namespace mc::world;

const std::string PillagerOutpostStructure::s_name = "Pillager_Outpost";

const SpawnOverrides PillagerOutpostStructure::s_spawnOverrides = {
    SpawnOverrideType::Full, {SpawnOverrideEntry{"monster", 1, 1}}};

PillagerOutpostStructure::PillagerOutpostStructure(ResourceLocation id)
    : JigsawStructure(
          std::move(id), JigsawConfig(ResourceLocation("minecraft", "pillager_outpost/base_plates"), 7), 0, true, true)
{}

const biome::BiomeTag* PillagerOutpostStructure::defaultBiomeTag() const
{
    return &biome::BiomeTags::HAS_STRUCTURE_PILLAGER_OUTPOST();
}

bool PillagerOutpostStructure::canGenerate(
    IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ)
{
    MC_UNUSED(world);

    // 检查生物群系
    BiomeId biome = generator.getBiome(chunkX * CHUNK_WIDTH + 8, 64, chunkZ * CHUNK_WIDTH + 8);
    if (isValidBiome(biome)) {
        // 20% 的生成概率
        i32 i = chunkX >> 4;
        i32 j = chunkZ >> 4;
        rng.setSeed(static_cast<i64>(i ^ j << 4) ^ static_cast<i64>(generator.seed()));
        (void)rng.nextInt();

        if (rng.nextInt(5) != 0) {
            return false;
        }

        // 检查附近是否有村庄
        return !_isNearVillage(generator, static_cast<i64>(generator.seed()), rng, chunkX, chunkZ);
    }
    return false;
}

bool PillagerOutpostStructure::_isNearVillage(
    IChunkGenerator& generator, i64 seed, math::Random& rng, i32 chunkX, i32 chunkZ) const
{
    MC_UNUSED(rng);
    MC_UNUSED(generator);

    // 检查 21x21 区块范围内是否有村庄起始位置
    // 如果在范围内有村庄，则不生成前哨站

    // 村庄的间距设置
    constexpr i32 villageSpacing = 32;
    constexpr i32 villageSeparation = 8;
    constexpr i32 villageSalt = 10387312;

    // 检查范围内的区块网格（10 区块半径）
    constexpr i32 searchRadius = 10;

    for (i32 dx = -searchRadius; dx <= searchRadius; ++dx) {
        for (i32 dz = -searchRadius; dz <= searchRadius; ++dz) {
            i32 testChunkX = chunkX + dx;
            i32 testChunkZ = chunkZ + dz;

            // 计算此区块是否是村庄起始位置
            i32 gridX = math::floorDiv(testChunkX, villageSpacing);
            i32 gridZ = math::floorDiv(testChunkZ, villageSpacing);

            // 使用相同种子计算偏移
            u64 combinedSeed = static_cast<u64>(gridX) * 341873128712ULL + static_cast<u64>(gridZ) * 132897987541ULL +
                static_cast<u64>(seed) + static_cast<u64>(villageSalt);
            math::Random villageRng(static_cast<i64>(combinedSeed));

            i32 offsetRange = villageSpacing - villageSeparation;
            i32 offsetX = villageRng.nextInt(offsetRange);
            i32 offsetZ = villageRng.nextInt(offsetRange);

            i32 villageStartX = gridX * villageSpacing + offsetX;
            i32 villageStartZ = gridZ * villageSpacing + offsetZ;

            // 如果此区块是村庄起始位置，则村庄在前哨站附近
            if (villageStartX == testChunkX && villageStartZ == testChunkZ) {
                return true;
            }
        }
    }

    return false;
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
