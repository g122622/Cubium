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

#include "BiomeValues.hpp"
#include <unordered_map>

namespace mc {
namespace layer {

// ============================================================================
// 生物群系类别映射（用于 areBiomesSimilar）
// 参考 MC 1.16.5 LayerUtil.Type 和 field_242937_a
// ============================================================================

namespace {

/**
 * @brief 生物群系类别枚举
 * 参考 MC LayerUtil.Type
 */
enum class BiomeCategory : i32 {
    None = 0,
    Taiga,
    ExtremeHills,
    Jungle,
    Mesa,
    BadlandsPlateau,
    Plains,
    Savanna,
    Icy,
    Beach,
    Forest,
    Ocean,
    Desert,
    River,
    Swamp,
    Mushroom
};

// 生物群系 ID 到类别的映射
// 参考 MC LayerUtil.field_242937_a
const std::unordered_map<i32, BiomeCategory>& getBiomeCategoryMap()
{
    static const std::unordered_map<i32, BiomeCategory> map = {
        // Beach (MC: 16, 26)
        {BiomeValues::Beach, BiomeCategory::Beach},      // 16
        {BiomeValues::SnowyBeach, BiomeCategory::Beach}, // 26

        // Desert (MC: 2, 17, 130)
        // DesertHills (17) 在 MC 中属于沙漠类
        {BiomeValues::Desert, BiomeCategory::Desert},      // 2
        {BiomeValues::DesertHills, BiomeCategory::Desert}, // 17
        {BiomeValues::DesertLakes, BiomeCategory::Desert}, // 130

        // ExtremeHills/Mountains (MC: 131, 162, 20, 3, 34)
        {BiomeValues::GravellyMountains, BiomeCategory::ExtremeHills},         // 131
        {BiomeValues::ModifiedGravellyMountains, BiomeCategory::ExtremeHills}, // 162
        {BiomeValues::MountainEdge, BiomeCategory::ExtremeHills},              // 20
        {BiomeValues::Mountains, BiomeCategory::ExtremeHills},                 // 3
        {BiomeValues::WoodedMountains, BiomeCategory::ExtremeHills},           // 34

        // Forest (MC: 27, 28, 29, 157, 132, 4, 155, 156, 18)
        {BiomeValues::BirchForest, BiomeCategory::Forest},      // 27
        {BiomeValues::BirchForestHills, BiomeCategory::Forest}, // 28
        {BiomeValues::DarkForest, BiomeCategory::Forest},       // 29
        {BiomeValues::DarkForestHills, BiomeCategory::Forest},  // 157
        {BiomeValues::FlowerForest, BiomeCategory::Forest},     // 132
        {BiomeValues::Forest, BiomeCategory::Forest},           // 4
        {BiomeValues::TallBirchForest, BiomeCategory::Forest},  // 155
        {BiomeValues::TallBirchHills, BiomeCategory::Forest},   // 156
        {BiomeValues::WoodedHills, BiomeCategory::Forest},      // 18

        // Icy (MC: 140, 13, 12)
        {BiomeValues::IceSpikes, BiomeCategory::Icy},      // 140
        {BiomeValues::SnowyMountains, BiomeCategory::Icy}, // 13
        {BiomeValues::SnowyPlains, BiomeCategory::Icy},    // 12

        // Jungle (MC: 168, 169, 21, 22, 23, 149, 151)
        {BiomeValues::BambooJungle, BiomeCategory::Jungle},       // 168
        {BiomeValues::BambooJungleHills, BiomeCategory::Jungle},  // 169
        {BiomeValues::Jungle, BiomeCategory::Jungle},             // 21
        {BiomeValues::JungleHills, BiomeCategory::Jungle},        // 22
        {BiomeValues::JungleEdge, BiomeCategory::Jungle},         // 23
        {BiomeValues::ModifiedJungle, BiomeCategory::Jungle},     // 149
        {BiomeValues::ModifiedJungleEdge, BiomeCategory::Jungle}, // 151

        // Mesa (Badlands variants, MC: 37, 165, 167, 166)
        {BiomeValues::Badlands, BiomeCategory::Mesa},                      // 37
        {BiomeValues::ErodedBadlands, BiomeCategory::Mesa},                // 165
        {BiomeValues::ModifiedBadlandsPlateau, BiomeCategory::Mesa},       // 167
        {BiomeValues::ModifiedWoodedBadlandsPlateau, BiomeCategory::Mesa}, // 166

        // BadlandsPlateau (MC: 39, 38)
        {BiomeValues::BadlandsPlateau, BiomeCategory::BadlandsPlateau},       // 39
        {BiomeValues::WoodedBadlandsPlateau, BiomeCategory::BadlandsPlateau}, // 38

        // Mushroom (MC: 14, 15)
        {BiomeValues::MushroomFields, BiomeCategory::Mushroom},     // 14
        {BiomeValues::MushroomFieldShore, BiomeCategory::Mushroom}, // 15

        // Ocean (MC: 46, 49, 50, 48, 24, 47, 10, 45, 0, 44)
        {BiomeValues::ColdOcean, BiomeCategory::Ocean},         // 46
        {BiomeValues::DeepColdOcean, BiomeCategory::Ocean},     // 49
        {BiomeValues::DeepFrozenOcean, BiomeCategory::Ocean},   // 50
        {BiomeValues::DeepLukewarmOcean, BiomeCategory::Ocean}, // 48
        {BiomeValues::DeepOcean, BiomeCategory::Ocean},         // 24
        {BiomeValues::DeepWarmOcean, BiomeCategory::Ocean},     // 47
        {BiomeValues::FrozenOcean, BiomeCategory::Ocean},       // 10
        {BiomeValues::LukewarmOcean, BiomeCategory::Ocean},     // 45
        {BiomeValues::Ocean, BiomeCategory::Ocean},             // 0
        {BiomeValues::WarmOcean, BiomeCategory::Ocean},         // 44

        // Plains (MC: 1, 129)
        {BiomeValues::Plains, BiomeCategory::Plains},          // 1
        {BiomeValues::SunflowerPlains, BiomeCategory::Plains}, // 129

        // River (MC: 11, 7)
        {BiomeValues::FrozenRiver, BiomeCategory::River}, // 11
        {BiomeValues::River, BiomeCategory::River},       // 7

        // Savanna (MC: 35, 36, 163, 164)
        {BiomeValues::Savanna, BiomeCategory::Savanna},                 // 35
        {BiomeValues::SavannaPlateau, BiomeCategory::Savanna},          // 36
        {BiomeValues::ShatteredSavanna, BiomeCategory::Savanna},        // 163
        {BiomeValues::ShatteredSavannaPlateau, BiomeCategory::Savanna}, // 164

        // Swamp (MC: 6, 134)
        {BiomeValues::Swamp, BiomeCategory::Swamp},      // 6
        {BiomeValues::SwampHills, BiomeCategory::Swamp}, // 134

        // Taiga (MC: 160, 161, 32, 33, 30, 31, 28, 29, 158, 5, 19, 133)
        {BiomeValues::GiantSpruceTaiga, BiomeCategory::Taiga},      // 160
        {BiomeValues::GiantSpruceTaigaHills, BiomeCategory::Taiga}, // 161
        {BiomeValues::GiantTreeTaiga, BiomeCategory::Taiga},        // 32
        {BiomeValues::GiantTreeTaigaHills, BiomeCategory::Taiga},   // 33
        {BiomeValues::SnowyTaiga, BiomeCategory::Taiga},            // 30
        {BiomeValues::SnowyTaigaHills, BiomeCategory::Taiga},       // 31
        // BirchForest(27), BirchForestHills(28), DarkForest(29), DarkForestHills(157) 已映射到 Forest
        {BiomeValues::SnowyTaigaMountains, BiomeCategory::Taiga}, // 158
        {BiomeValues::Taiga, BiomeCategory::Taiga},               // 5
        {BiomeValues::TaigaHills, BiomeCategory::Taiga},          // 19 (注：BiomeValues 中映射到 19)
        {BiomeValues::TaigaMountains, BiomeCategory::Taiga},      // 133
    };
    return map;
}

BiomeCategory getBiomeCategory(i32 biome)
{
    const auto& map = getBiomeCategoryMap();
    auto it = map.find(biome);
    if (it != map.end()) {
        return it->second;
    }
    return BiomeCategory::None;
}

} // anonymous namespace

// ============================================================================
// areBiomesSimilar 实现
// ============================================================================

bool BiomeValues::areBiomesSimilar(i32 a, i32 b)
{
    if (a == b) {
        return true;
    }
    return getBiomeCategory(a) == getBiomeCategory(b);
}

} // namespace layer
} // namespace mc
