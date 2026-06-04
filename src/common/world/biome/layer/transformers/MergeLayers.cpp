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

#include "MergeLayers.hpp"
#include <memory>
#include <unordered_map>

namespace mc {
namespace layer {

// ============================================================================
// AddMushroomIslandLayer 实现
// ============================================================================

i32 AddMushroomIslandLayer::apply(IAreaContext& ctx, i32 x, i32 sw, i32 se, i32 ne, i32 nw, i32 center)
{
    // 如果中心和四个对角都是浅海，有 1% 概率生成蘑菇岛
    if (BiomeValues::isShallowOcean(center) && BiomeValues::isShallowOcean(nw) && BiomeValues::isShallowOcean(ne) &&
        BiomeValues::isShallowOcean(sw) && BiomeValues::isShallowOcean(se)) {
        if (ctx.nextInt(100) == 0) {
            return BiomeValues::MushroomFields;
        }
    }

    return center;
}

// ============================================================================
// AddBambooForestLayer 实现
// ============================================================================

i32 AddBambooForestLayer::apply(IAreaContext& ctx, i32 value)
{
    // 丛林有 1/10 概率变成竹林
    if (value == BiomeValues::Jungle && ctx.nextInt(10) == 0) {
        return BiomeValues::BambooJungle;
    }
    return value;
}

// ============================================================================
// StartRiverLayer 实现
// ============================================================================

i32 StartRiverLayer::apply(IAreaContext& ctx, i32 value)
{
    // 浅海保持不变，否则返回河流噪声值 (2-300000)
    if (BiomeValues::isShallowOcean(value)) {
        return value;
    }
    return ctx.nextInt(299999) + 2;
}

// ============================================================================
// RiverLayer 实现
// ============================================================================

i32 RiverLayer::apply(IAreaContext& ctx, i32 north, i32 east, i32 south, i32 west, i32 center)
{
    i32 c = _riverFilter(center);
    i32 n = _riverFilter(north);
    i32 e = _riverFilter(east);
    i32 s = _riverFilter(south);
    i32 w = _riverFilter(west);

    // 如果所有方向过滤后的值相同，返回 -1（无河流）
    // 否则返回河流生物群系
    return (c == e && c == n && c == w && c == s) ? -1 : BiomeValues::River;
}

i32 RiverLayer::_riverFilter(i32 value)
{
    if (value >= 2) {
        return 2 + (value & 1);
    }
    return value;
}

// ============================================================================
// HillsLayer 实现
// ============================================================================

namespace {

// 山丘变体映射表：将基础生物群系映射到稀有变体
const std::unordered_map<i32, i32> s_hillsRareBiomes = {
    {BiomeValues::Plains, BiomeValues::SunflowerPlains},
    {BiomeValues::Desert, BiomeValues::DesertLakes},
    {BiomeValues::Mountains, BiomeValues::GravellyMountains},
    {BiomeValues::Forest, BiomeValues::FlowerForest},
    {BiomeValues::Taiga, BiomeValues::TaigaMountains},
    {BiomeValues::Swamp, BiomeValues::SwampHills},
    {BiomeValues::SnowyPlains, BiomeValues::IceSpikes},
    {BiomeValues::Jungle, BiomeValues::ModifiedJungle},
    {BiomeValues::JungleEdge, BiomeValues::ModifiedJungleEdge},
    {BiomeValues::BirchForest, BiomeValues::TallBirchForest},
    {BiomeValues::BirchForestHills, BiomeValues::TallBirchHills},
    {BiomeValues::DarkForest, BiomeValues::DarkForestHills},
    {BiomeValues::SnowyTaiga, BiomeValues::SnowyTaigaMountains},
    {BiomeValues::GiantTreeTaiga, BiomeValues::GiantSpruceTaiga},
    {BiomeValues::GiantTreeTaigaHills, BiomeValues::GiantSpruceTaigaHills},
    {BiomeValues::WoodedMountains, BiomeValues::ModifiedGravellyMountains},
    {BiomeValues::Savanna, BiomeValues::ShatteredSavanna},
    {BiomeValues::SavannaPlateau, BiomeValues::ShatteredSavannaPlateau},
    {BiomeValues::Badlands, BiomeValues::ErodedBadlands},
    {BiomeValues::WoodedBadlandsPlateau, BiomeValues::ModifiedWoodedBadlandsPlateau},
    {BiomeValues::BadlandsPlateau, BiomeValues::ModifiedBadlandsPlateau},
};

} // namespace

i32 HillsLayer::apply(IAreaContext& ctx, const IArea& biomeArea, const IArea& riverArea, i32 x, i32 z)
{
    // 采样中心点和周围点
    // IDimOffset1Transformer: getOffsetX(x) = x + 1, getOffsetZ(z) = z + 1
    // 所以这里需要使用 x+1, z+1 作为中心点
    i32 biomeValue = biomeArea.getValue(x + 1, z + 1);
    i32 riverValue = riverArea.getValue(x + 1, z + 1);

    // 提取河流噪声的低位
    i32 riverNoise = (riverValue - 2) % 29;

    // 检查是否应该生成稀有变体 (k == 1)
    if (!BiomeValues::isShallowOcean(biomeValue) && riverValue >= 2 && riverNoise == 1) {
        auto it = s_hillsRareBiomes.find(biomeValue);
        if (it != s_hillsRareBiomes.end()) {
            return it->second;
        }
    }

    // 随机生成山丘变体 (约 1/3 概率或 k == 0)
    if (ctx.nextInt(3) == 0 || riverNoise == 0) {
        i32 result = biomeValue;

        // 山丘映射逻辑
        switch (biomeValue) {
            case BiomeValues::Desert:
                result = BiomeValues::DesertHills;
                break;
            case BiomeValues::Forest:
                result = BiomeValues::WoodedHills;
                break;
            case BiomeValues::BirchForest:
                result = BiomeValues::BirchForestHills;
                break;
            case BiomeValues::DarkForest:
                result = BiomeValues::Plains;
                break;
            case BiomeValues::Taiga:
                result = BiomeValues::TaigaHills;
                break;
            case BiomeValues::GiantTreeTaiga:
                result = BiomeValues::GiantTreeTaigaHills;
                break;
            case BiomeValues::SnowyTaiga:
                result = BiomeValues::SnowyTaigaHills;
                break;
            case BiomeValues::Plains:
                result = ctx.nextInt(3) == 0 ? BiomeValues::WoodedHills : BiomeValues::Forest;
                break;
            case BiomeValues::SnowyPlains:
                result = BiomeValues::SnowyMountains;
                break;
            case BiomeValues::Jungle:
                result = BiomeValues::JungleHills;
                break;
            case BiomeValues::BambooJungle:
                result = BiomeValues::BambooJungleHills;
                break;
            case BiomeValues::Ocean:
                result = BiomeValues::DeepOcean;
                break;
            case BiomeValues::WarmOcean:
                result = BiomeValues::DeepWarmOcean;
                break;
            case BiomeValues::LukewarmOcean:
                result = BiomeValues::DeepLukewarmOcean;
                break;
            case BiomeValues::ColdOcean:
                result = BiomeValues::DeepColdOcean;
                break;
            case BiomeValues::FrozenOcean:
                result = BiomeValues::DeepFrozenOcean;
                break;
            case BiomeValues::Mountains:
                result = BiomeValues::WoodedMountains;
                break;
            case BiomeValues::Savanna:
                result = BiomeValues::SavannaPlateau;
                break;
            default:
                // 检查是否为恶地类型
                if (BiomeValues::isBadlands(biomeValue) && biomeValue != BiomeValues::ErodedBadlands) {
                    if (biomeValue == BiomeValues::WoodedBadlandsPlateau) {
                        result = BiomeValues::Badlands;
                    }
                }
                // 深海有可能变成陆地
                if ((biomeValue == BiomeValues::DeepOcean || biomeValue == BiomeValues::DeepLukewarmOcean ||
                        biomeValue == BiomeValues::DeepColdOcean || biomeValue == BiomeValues::DeepFrozenOcean) &&
                    ctx.nextInt(3) == 0) {
                    result = ctx.nextInt(2) == 0 ? BiomeValues::Plains : BiomeValues::Forest;
                }
                break;
        }

        // 如果 k == 0 且结果发生了变化，再次应用稀有变体映射
        if (riverNoise == 0 && result != biomeValue) {
            auto it = s_hillsRareBiomes.find(result);
            if (it != s_hillsRareBiomes.end()) {
                result = it->second;
            }
        }

        // 检查周围邻居是否相似
        if (result != biomeValue) {
            i32 neighborCount = 0;

            // 检查四个方向的邻居
            i32 north = biomeArea.getValue(x + 1, z);
            i32 east = biomeArea.getValue(x + 2, z + 1);
            i32 south = biomeArea.getValue(x, z + 1);
            i32 west = biomeArea.getValue(x + 1, z + 2);

            if (BiomeValues::areBiomesSimilar(north, biomeValue)) neighborCount++;
            if (BiomeValues::areBiomesSimilar(east, biomeValue)) neighborCount++;
            if (BiomeValues::areBiomesSimilar(south, biomeValue)) neighborCount++;
            if (BiomeValues::areBiomesSimilar(west, biomeValue)) neighborCount++;

            // 只有当至少3个邻居相似时才生成山丘变体
            if (neighborCount >= 3) {
                return result;
            }
        }
    }

    return biomeValue;
}

std::unique_ptr<IAreaFactory> HillsLayer::apply(
    IExtendedAreaContext& context, std::unique_ptr<IAreaFactory> input1, std::unique_ptr<IAreaFactory> input2)
{
    auto sharedContext = std::dynamic_pointer_cast<LayerContext>(context.shared_from_this());
    return std::make_unique<MergeFactory>(this, sharedContext, std::move(input1), std::move(input2));
}

// ============================================================================
// MixRiverLayer 实现
// ============================================================================

i32 MixRiverLayer::apply(IAreaContext& ctx, const IArea& biomeArea, const IArea& riverArea, i32 x, i32 z)
{
    (void)ctx; // 不使用

    i32 biome = biomeArea.getValue(getOffsetX(x), getOffsetZ(z));
    i32 river = riverArea.getValue(getOffsetX(x), getOffsetZ(z));

    // 海洋保持不变
    if (BiomeValues::isOcean(biome)) {
        return biome;
    }

    // 河流
    if (river == BiomeValues::River) {
        if (biome == BiomeValues::SnowyPlains) {
            return BiomeValues::FrozenRiver;
        }
        // 蘑菇岛变成岸边
        if (biome != BiomeValues::MushroomFields && biome != BiomeValues::MushroomFieldShore) {
            return BiomeValues::River;
        }
        return BiomeValues::MushroomFieldShore;
    }

    return biome;
}

std::unique_ptr<IAreaFactory> MixRiverLayer::apply(
    IExtendedAreaContext& context, std::unique_ptr<IAreaFactory> input1, std::unique_ptr<IAreaFactory> input2)
{
    auto sharedContext = std::dynamic_pointer_cast<LayerContext>(context.shared_from_this());
    return std::make_unique<MergeFactory>(this, sharedContext, std::move(input1), std::move(input2));
}

// ============================================================================
// MixOceansLayer 实现
// ============================================================================

i32 MixOceansLayer::apply(IAreaContext& ctx, const IArea& biomeArea, const IArea& oceanArea, i32 x, i32 z)
{
    (void)ctx; // 不使用

    i32 biome = biomeArea.getValue(getOffsetX(x), getOffsetZ(z));
    i32 ocean = oceanArea.getValue(getOffsetX(x), getOffsetZ(z));

    // 非海洋保持不变
    if (!BiomeValues::isOcean(biome)) {
        return biome;
    }

    // 检查周围是否有陆地
    for (i32 dx = -8; dx <= 8; dx += 4) {
        for (i32 dz = -8; dz <= 8; dz += 4) {
            i32 neighbor = biomeArea.getValue(getOffsetX(x + dx), getOffsetZ(z + dz));
            if (!BiomeValues::isOcean(neighbor)) {
                // 有陆地相邻，调整极端海洋温度
                if (ocean == BiomeValues::WarmOcean) {
                    return BiomeValues::LukewarmOcean;
                }
                if (ocean == BiomeValues::FrozenOcean) {
                    return BiomeValues::ColdOcean;
                }
            }
        }
    }

    // 深海根据海洋温度调整
    if (biome == BiomeValues::DeepOcean) {
        switch (ocean) {
            case BiomeValues::LukewarmOcean:
                return BiomeValues::DeepLukewarmOcean;
            case BiomeValues::Ocean:
                return BiomeValues::DeepOcean;
            case BiomeValues::ColdOcean:
                return BiomeValues::DeepColdOcean;
            case BiomeValues::FrozenOcean:
                return BiomeValues::DeepFrozenOcean;
            default:
                return ocean;
        }
    }

    return ocean;
}

std::unique_ptr<IAreaFactory> MixOceansLayer::apply(
    IExtendedAreaContext& context, std::unique_ptr<IAreaFactory> input1, std::unique_ptr<IAreaFactory> input2)
{
    auto sharedContext = std::dynamic_pointer_cast<LayerContext>(context.shared_from_this());
    return std::make_unique<MergeFactory>(this, sharedContext, std::move(input1), std::move(input2));
}

} // namespace layer
} // namespace mc
