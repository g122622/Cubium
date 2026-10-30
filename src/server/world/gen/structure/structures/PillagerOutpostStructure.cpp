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

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/structure/JigsawStructure.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include "server/world/gen/valueprovider/HeightProvider.hpp"
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
    : JigsawStructure(std::move(id),
          // 起始高度由结构 JSON 提供（start_height = absolute 0 + project_start_to_heightmap =
          // WORLD_SURFACE_WG）。旧实现把这两个字段硬编码成"nearTerrain + 起始高度 0 或 64"，
          // 只有 1/16 的情形恰好等于原版的"投影到地表"——前哨站因此常被放进地下或悬空。
          JigsawConfig(ResourceLocation("minecraft", "pillager_outpost/base_plates"),
              7,
              valueprovider::ConstantHeight::create(valueprovider::VerticalAnchor::absolute(0)),
              {},
              MaxDistance(80),
              DimensionPadding(),
              LiquidSettings::IgnoreWaterlogging),
          0,     // startY：由 startHeight 决定
          false, // nearTerrain：投影由 project_start_to_heightmap 完成，不再走"贴近地形"分支
          true)
{
    // 结构 JSON：project_start_to_heightmap = WORLD_SURFACE_WG
    m_config.projectStartToHeightmap = true;
}

const biome::BiomeTag* PillagerOutpostStructure::defaultBiomeTag() const
{
    return &biome::BiomeTags::HAS_STRUCTURE_PILLAGER_OUTPOST();
}

bool PillagerOutpostStructure::canGenerate(
    IWorld& world, IChunkGenerator& generator, math::IRandom& rng, i32 chunkX, i32 chunkZ)
{
    MC_UNUSED(world);

    // 生物群系与地形高度都不在这里判定：原版只在候选生成点（起始块中心、投影后的地面线）
    // 处校验群系，且前哨站没有额外的可生成性条件。区块中心 + 固定 y 的采样会在群系边界与
    // 洞穴上方给出与原版相反的结论，把合法区块整片误杀。
    MC_UNUSED(generator);

    // 20% 的生成概率：原版在 findGenerationPoint 里用 random.nextInt(5) != 0 淘汰。
    // 注意此时起始旋转与起始块都还没抽，因此这次随机数必须在这里消耗，
    // 不能挪到基类装配之后再判。
    i32 i = chunkX >> 4;
    i32 j = chunkZ >> 4;
    rng.setSeed(static_cast<i64>(i ^ (j << 4)) ^ static_cast<i64>(generator.seed()));
    (void)rng.nextInt();
    if (rng.nextInt(5) != 0) {
        return false;
    }

    // 附近有村庄则让位（原版 PillagerOutpostStructure.isNearVillage）。
    return !_isNearVillage(generator, static_cast<i64>(generator.seed()), rng, chunkX, chunkZ);
}

bool PillagerOutpostStructure::_isNearVillage(
    IChunkGenerator& generator, i64 seed, math::IRandom& rng, i32 chunkX, i32 chunkZ) const
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
