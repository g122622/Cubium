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
// ============================================================================

namespace {

/**
 * @brief 生物群系类别枚举
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
const std::unordered_map<i32, BiomeCategory>& getBiomeCategoryMap()
{
    static const std::unordered_map<i32, BiomeCategory> map = {
        // Beach
        {BiomeValues::Beach, BiomeCategory::Beach},
        {BiomeValues::SnowyBeach, BiomeCategory::Beach},

        // Desert
        {BiomeValues::Desert, BiomeCategory::Desert},
        {BiomeValues::DesertHills, BiomeCategory::Desert},
        {BiomeValues::DesertLakes, BiomeCategory::Desert},

        // ExtremeHills/Mountains
        {BiomeValues::GravellyMountains, BiomeCategory::ExtremeHills},
        {BiomeValues::ModifiedGravellyMountains, BiomeCategory::ExtremeHills},
        {BiomeValues::MountainEdge, BiomeCategory::ExtremeHills},
        {BiomeValues::Mountains, BiomeCategory::ExtremeHills},
        {BiomeValues::WoodedMountains, BiomeCategory::ExtremeHills},

        // Forest
        {BiomeValues::BirchForest, BiomeCategory::Forest},
        {BiomeValues::BirchForestHills, BiomeCategory::Forest},
        {BiomeValues::DarkForest, BiomeCategory::Forest},
        {BiomeValues::DarkForestHills, BiomeCategory::Forest},
        {BiomeValues::FlowerForest, BiomeCategory::Forest},
        {BiomeValues::Forest, BiomeCategory::Forest},
        {BiomeValues::TallBirchForest, BiomeCategory::Forest},
        {BiomeValues::TallBirchHills, BiomeCategory::Forest},
        {BiomeValues::WoodedHills, BiomeCategory::Forest},

        // Icy
        {BiomeValues::IceSpikes, BiomeCategory::Icy},
        {BiomeValues::SnowyMountains, BiomeCategory::Icy},
        {BiomeValues::SnowyPlains, BiomeCategory::Icy},

        // Jungle
        {BiomeValues::BambooJungle, BiomeCategory::Jungle},
        {BiomeValues::BambooJungleHills, BiomeCategory::Jungle},
        {BiomeValues::Jungle, BiomeCategory::Jungle},
        {BiomeValues::JungleHills, BiomeCategory::Jungle},
        {BiomeValues::JungleEdge, BiomeCategory::Jungle},
        {BiomeValues::ModifiedJungle, BiomeCategory::Jungle},
        {BiomeValues::ModifiedJungleEdge, BiomeCategory::Jungle},

        // Mesa (Badlands variants)
        {BiomeValues::Badlands, BiomeCategory::Mesa},
        {BiomeValues::ErodedBadlands, BiomeCategory::Mesa},
        {BiomeValues::ModifiedBadlandsPlateau, BiomeCategory::Mesa},
        {BiomeValues::ModifiedWoodedBadlandsPlateau, BiomeCategory::Mesa},

        // BadlandsPlateau
        {BiomeValues::BadlandsPlateau, BiomeCategory::BadlandsPlateau},
        {BiomeValues::WoodedBadlandsPlateau, BiomeCategory::BadlandsPlateau},

        // Mushroom
        {BiomeValues::MushroomFields, BiomeCategory::Mushroom},
        {BiomeValues::MushroomFieldShore, BiomeCategory::Mushroom},

        // Ocean
        {BiomeValues::ColdOcean, BiomeCategory::Ocean},
        {BiomeValues::DeepColdOcean, BiomeCategory::Ocean},
        {BiomeValues::DeepFrozenOcean, BiomeCategory::Ocean},
        {BiomeValues::DeepLukewarmOcean, BiomeCategory::Ocean},
        {BiomeValues::DeepOcean, BiomeCategory::Ocean},
        {BiomeValues::DeepWarmOcean, BiomeCategory::Ocean},
        {BiomeValues::FrozenOcean, BiomeCategory::Ocean},
        {BiomeValues::LukewarmOcean, BiomeCategory::Ocean},
        {BiomeValues::Ocean, BiomeCategory::Ocean},
        {BiomeValues::WarmOcean, BiomeCategory::Ocean},

        // Plains
        {BiomeValues::Plains, BiomeCategory::Plains},
        {BiomeValues::SunflowerPlains, BiomeCategory::Plains},

        // River
        {BiomeValues::FrozenRiver, BiomeCategory::River},
        {BiomeValues::River, BiomeCategory::River},

        // Savanna
        {BiomeValues::Savanna, BiomeCategory::Savanna},
        {BiomeValues::SavannaPlateau, BiomeCategory::Savanna},
        {BiomeValues::ShatteredSavanna, BiomeCategory::Savanna},
        {BiomeValues::ShatteredSavannaPlateau, BiomeCategory::Savanna},

        // Swamp
        {BiomeValues::Swamp, BiomeCategory::Swamp},
        {BiomeValues::SwampHills, BiomeCategory::Swamp},

        // Taiga
        {BiomeValues::GiantSpruceTaiga, BiomeCategory::Taiga},
        {BiomeValues::GiantSpruceTaigaHills, BiomeCategory::Taiga},
        {BiomeValues::GiantTreeTaiga, BiomeCategory::Taiga},
        {BiomeValues::GiantTreeTaigaHills, BiomeCategory::Taiga},
        {BiomeValues::SnowyTaiga, BiomeCategory::Taiga},
        {BiomeValues::SnowyTaigaHills, BiomeCategory::Taiga},
        {BiomeValues::SnowyTaigaMountains, BiomeCategory::Taiga},
        {BiomeValues::Taiga, BiomeCategory::Taiga},
        {BiomeValues::TaigaHills, BiomeCategory::Taiga},
        {BiomeValues::TaigaMountains, BiomeCategory::Taiga},
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

bool BiomeValues::areBiomesSimilar(i32 a, i32 b) noexcept
{
    if (a == b) {
        return true;
    }
    return getBiomeCategory(a) == getBiomeCategory(b);
}

} // namespace layer
} // namespace mc
