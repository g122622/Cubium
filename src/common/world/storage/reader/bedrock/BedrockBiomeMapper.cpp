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

#include "BedrockBiomeMapper.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/biome/BiomeIds.hpp"

namespace mc::world::storage::reader::bedrock {

BedrockBiomeMapper::BedrockBiomeMapper() noexcept
{
    _initializeMappings();
}

BiomeId BedrockBiomeMapper::mapBiome(i32 bedrockBiomeId, DimensionId dimension)
{
    MC_UNUSED(dimension);
    auto it = m_bedrockToJava.find(bedrockBiomeId);
    if (it != m_bedrockToJava.end()) {
        return it->second;
    }

    // 如果在映射表中没有找到，且 ID 在合理范围内，直接使用
    if (bedrockBiomeId >= 0 && bedrockBiomeId <= 255) {
        return static_cast<BiomeId>(bedrockBiomeId);
    }

    // 未知生物群系 ID，默认返回海洋
    return Biomes::Ocean;
}

void BedrockBiomeMapper::_initializeMappings()
{
    // 基岩版与 Java 版 ID 一致的生物群系（直接映射）
    // 大部分基础生物群系 ID 相同，以下是差异项

    // 基岩版独有或 ID 不同的生物群系映射
    m_bedrockToJava[0] = Biomes::Ocean;
    m_bedrockToJava[1] = Biomes::Plains;
    m_bedrockToJava[2] = Biomes::Desert;
    m_bedrockToJava[3] = Biomes::Mountains;
    m_bedrockToJava[4] = Biomes::Forest;
    m_bedrockToJava[5] = Biomes::Taiga;
    m_bedrockToJava[6] = Biomes::Swamp;
    m_bedrockToJava[7] = Biomes::River;
    m_bedrockToJava[8] = Biomes::NetherWastes;
    m_bedrockToJava[9] = Biomes::TheEnd;
    m_bedrockToJava[10] = Biomes::FrozenOcean;
    m_bedrockToJava[11] = Biomes::FrozenRiver;
    m_bedrockToJava[12] = Biomes::SnowyPlains;
    m_bedrockToJava[13] = Biomes::SnowyMountains;
    m_bedrockToJava[14] = Biomes::MushroomFields;
    m_bedrockToJava[15] = Biomes::MushroomFieldShore;
    m_bedrockToJava[16] = Biomes::Beach;
    m_bedrockToJava[17] = Biomes::DesertHills;
    m_bedrockToJava[18] = Biomes::WoodedHills;
    m_bedrockToJava[19] = Biomes::TaigaHills;
    m_bedrockToJava[20] = Biomes::MountainEdge;
    m_bedrockToJava[21] = Biomes::Jungle;
    m_bedrockToJava[22] = Biomes::JungleHills;
    m_bedrockToJava[23] = Biomes::JungleEdge;
    m_bedrockToJava[24] = Biomes::DeepOcean;
    m_bedrockToJava[25] = Biomes::StoneShore;
    m_bedrockToJava[26] = Biomes::SnowyBeach;
    m_bedrockToJava[27] = Biomes::BirchForest;
    m_bedrockToJava[28] = Biomes::BirchForestHills;
    m_bedrockToJava[29] = Biomes::DarkForest;
    m_bedrockToJava[30] = Biomes::SnowyTaiga;
    m_bedrockToJava[31] = Biomes::SnowyTaigaHills;
    m_bedrockToJava[32] = Biomes::GiantTreeTaiga;
    m_bedrockToJava[33] = Biomes::GiantTreeTaigaHills;
    m_bedrockToJava[34] = Biomes::WoodedMountains;
    m_bedrockToJava[35] = Biomes::Savanna;
    m_bedrockToJava[36] = Biomes::SavannaPlateau;
    m_bedrockToJava[37] = Biomes::Badlands;
    m_bedrockToJava[38] = Biomes::WoodedBadlandsPlateau;
    m_bedrockToJava[39] = Biomes::BadlandsPlateau;

    // 基岩版独有的 ID 映射
    // 基岩版 40-43 是末地生物群系，ID 与 Java 一致
    m_bedrockToJava[40] = Biomes::SmallEndIslands;
    m_bedrockToJava[41] = Biomes::EndMidlands;
    m_bedrockToJava[42] = Biomes::EndHighlands;
    m_bedrockToJava[43] = Biomes::EndBarrens;

    // 海洋温度变体
    m_bedrockToJava[44] = Biomes::WarmOcean;
    m_bedrockToJava[45] = Biomes::LukewarmOcean;
    m_bedrockToJava[46] = Biomes::ColdOcean;
    m_bedrockToJava[47] = Biomes::DeepWarmOcean;
    m_bedrockToJava[48] = Biomes::DeepLukewarmOcean;
    m_bedrockToJava[49] = Biomes::DeepColdOcean;
    m_bedrockToJava[50] = Biomes::DeepFrozenOcean;

    // 下界生物群系（基岩版 ID 与 Java 版不同）
    m_bedrockToJava[176] = Biomes::SoulSandValley;
    m_bedrockToJava[177] = Biomes::CrimsonForest;
    m_bedrockToJava[178] = Biomes::WarpedForest;
    m_bedrockToJava[179] = Biomes::BasaltDeltas;

    // 基岩版独有的变体生物群系（ID > 128）
    m_bedrockToJava[129] = Biomes::SunflowerPlains;
    m_bedrockToJava[130] = Biomes::DesertLakes;
    m_bedrockToJava[131] = Biomes::GravellyMountains;
    m_bedrockToJava[132] = Biomes::FlowerForest;
    m_bedrockToJava[133] = Biomes::TaigaMountains;
    m_bedrockToJava[134] = Biomes::SwampHills;
    m_bedrockToJava[140] = Biomes::IceSpikes;
    m_bedrockToJava[149] = Biomes::ModifiedJungle;
    m_bedrockToJava[151] = Biomes::ModifiedJungleEdge;
    m_bedrockToJava[155] = Biomes::TallBirchForest;
    m_bedrockToJava[156] = Biomes::TallBirchHills;
    m_bedrockToJava[157] = Biomes::DarkForestHills;
    m_bedrockToJava[158] = Biomes::SnowyTaigaMountains;
    m_bedrockToJava[160] = Biomes::GiantSpruceTaiga;
    m_bedrockToJava[161] = Biomes::GiantSpruceTaigaHills;
    m_bedrockToJava[162] = Biomes::ModifiedGravellyMountains;
    m_bedrockToJava[163] = Biomes::ShatteredSavanna;
    m_bedrockToJava[164] = Biomes::ShatteredSavannaPlateau;
    m_bedrockToJava[165] = Biomes::ErodedBadlands;
    m_bedrockToJava[166] = Biomes::ModifiedWoodedBadlandsPlateau;
    m_bedrockToJava[167] = Biomes::ModifiedBadlandsPlateau;
    m_bedrockToJava[168] = Biomes::BambooJungle;
    m_bedrockToJava[169] = Biomes::BambooJungleHills;
}

} // namespace mc::world::storage::reader::bedrock
