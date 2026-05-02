#include "MergeLayers.hpp"
#include <memory>
#include <unordered_map>

namespace mc {
namespace layer {

// ============================================================================
// AddMushroomIslandLayer 实现
// ============================================================================

i32 AddMushroomIslandLayer::apply(IAreaContext& ctx, i32 x, i32 sw, i32 se, i32 ne, i32 nw, i32 center) {
    // 参考 MC AddMushroomIslandLayer.apply:
    // return LayerUtil.isShallowOcean(center) && LayerUtil.isShallowOcean(nw) &&
    //        LayerUtil.isShallowOcean(ne) && LayerUtil.isShallowOcean(sw) &&
    //        LayerUtil.isShallowOcean(se) && context.random(100) == 0 ? 14 : center;

    // 如果中心和四个对角都是浅海，有 1% 概率生成蘑菇岛
    if (BiomeValues::isShallowOcean(center) &&
        BiomeValues::isShallowOcean(nw) &&
        BiomeValues::isShallowOcean(ne) &&
        BiomeValues::isShallowOcean(sw) &&
        BiomeValues::isShallowOcean(se)) {
        if (ctx.nextInt(100) == 0) {
            return BiomeValues::MushroomFields;
        }
    }

    return center;
}

// ============================================================================
// AddBambooForestLayer 实现
// ============================================================================

i32 AddBambooForestLayer::apply(IAreaContext& ctx, i32 value) {
    // 参考 MC AddBambooForestLayer.apply:
    // return context.random(10) == 0 && value == 21 ? 168 : value;

    // 丛林 (21) 有 1/10 概率变成竹林 (168)
    if (value == BiomeValues::Jungle && ctx.nextInt(10) == 0) {
        return BiomeValues::BambooJungle;
    }
    return value;
}

// ============================================================================
// StartRiverLayer 实现
// ============================================================================

i32 StartRiverLayer::apply(IAreaContext& ctx, i32 value) {
    // 参考 MC StartRiverLayer.apply:
    // return LayerUtil.isShallowOcean(value) ? value : context.random(299999) + 2;

    // 浅海保持不变，否则返回河流噪声值 (2-300000)
    if (BiomeValues::isShallowOcean(value)) {
        return value;
    }
    return ctx.nextInt(299999) + 2;
}

// ============================================================================
// RiverLayer 实现
// ============================================================================

i32 RiverLayer::apply(IAreaContext& ctx, i32 north, i32 east, i32 south, i32 west, i32 center) {
    // 参考 MC RiverLayer.apply:
    // int i = riverFilter(center);
    // return i == riverFilter(east) && i == riverFilter(north) &&
    //        i == riverFilter(west) && i == riverFilter(south) ? -1 : 7;

    i32 c = riverFilter(center);
    i32 n = riverFilter(north);
    i32 e = riverFilter(east);
    i32 s = riverFilter(south);
    i32 w = riverFilter(west);

    // 如果所有方向过滤后的值相同，返回 -1（无河流）
    // 否则返回 7（河流）
    return (c == e && c == n && c == w && c == s) ? -1 : BiomeValues::River;
}

i32 RiverLayer::riverFilter(i32 value) {
    // 参考 MC RiverLayer.riverFilter:
    // return p_151630_0_ >= 2 ? 2 + (p_151630_0_ & 1) : p_151630_0_;
    if (value >= 2) {
        return 2 + (value & 1);
    }
    return value;
}

// ============================================================================
// HillsLayer 实现
// ============================================================================

// MC 1.16.5 山丘变体映射表 (field_242940_c)
// 将基础生物群系映射到稀有变体
static const std::unordered_map<i32, i32> s_hillsRareBiomes = {
    {BiomeValues::Plains, BiomeValues::SunflowerPlains},            // 1 -> 129
    {BiomeValues::Desert, BiomeValues::DesertLakes},                // 2 -> 130
    {BiomeValues::Mountains, BiomeValues::GravellyMountains},       // 3 -> 131
    {BiomeValues::Forest, BiomeValues::FlowerForest},               // 4 -> 132
    {BiomeValues::Taiga, BiomeValues::TaigaMountains},              // 5 -> 133
    {BiomeValues::Swamp, BiomeValues::SwampHills},                  // 6 -> 134
    {BiomeValues::SnowyPlains, BiomeValues::IceSpikes},             // 12 -> 140
    {BiomeValues::Jungle, BiomeValues::ModifiedJungle},             // 21 -> 149
    {BiomeValues::JungleEdge, BiomeValues::ModifiedJungleEdge},     // 23 -> 151
    {BiomeValues::BirchForest, BiomeValues::TallBirchForest},       // 27 -> 155
    {BiomeValues::BirchForestHills, BiomeValues::TallBirchHills},   // 28 -> 156
    {BiomeValues::DarkForest, BiomeValues::DarkForestHills},        // 29 -> 157
    {BiomeValues::SnowyTaiga, BiomeValues::SnowyTaigaMountains},    // 30 -> 158
    {BiomeValues::GiantTreeTaiga, BiomeValues::GiantSpruceTaiga},   // 32 -> 160
    {BiomeValues::GiantTreeTaigaHills, BiomeValues::GiantSpruceTaigaHills}, // 33 -> 161
    {BiomeValues::WoodedMountains, BiomeValues::ModifiedGravellyMountains}, // 34 -> 162
    {BiomeValues::Savanna, BiomeValues::ShatteredSavanna},          // 35 -> 163
    {BiomeValues::SavannaPlateau, BiomeValues::ShatteredSavannaPlateau}, // 36 -> 164
    {BiomeValues::Badlands, BiomeValues::ErodedBadlands},           // 37 -> 165
    {BiomeValues::WoodedBadlandsPlateau, BiomeValues::ModifiedWoodedBadlandsPlateau}, // 38 -> 166
    {BiomeValues::BadlandsPlateau, BiomeValues::ModifiedBadlandsPlateau}, // 39 -> 167
};

i32 HillsLayer::apply(IAreaContext& ctx, const IArea& biomeArea, const IArea& riverArea, i32 x, i32 z) {
    // 参考 MC 1.16.5 HillsLayer.apply (这是最复杂的层之一)

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

        // MC 1.16.5 的山丘映射逻辑
        switch (biomeValue) {
            case BiomeValues::Desert:  // 2
                result = BiomeValues::DesertHills;  // 17
                break;
            case BiomeValues::Forest:  // 4
                result = BiomeValues::WoodedHills;  // 18
                break;
            case BiomeValues::BirchForest:  // 27
                result = BiomeValues::BirchForestHills;  // 28
                break;
            case BiomeValues::DarkForest:  // 29
                result = BiomeValues::Plains;  // 1 (MC 奇怪的设计)
                break;
            case BiomeValues::Taiga:  // 5
                result = BiomeValues::TaigaHills;  // 19
                break;
            case BiomeValues::GiantTreeTaiga:  // 32
                result = BiomeValues::GiantTreeTaigaHills;  // 33
                break;
            case BiomeValues::SnowyTaiga:  // 30
                result = BiomeValues::SnowyTaigaHills;  // 31
                break;
            case BiomeValues::Plains:  // 1
                result = ctx.nextInt(3) == 0 ? BiomeValues::WoodedHills : BiomeValues::Forest;  // 18 或 4
                break;
            case BiomeValues::SnowyPlains:  // 12
                result = BiomeValues::SnowyMountains;  // 13
                break;
            case BiomeValues::Jungle:  // 21
                result = BiomeValues::JungleHills;  // 22
                break;
            case BiomeValues::BambooJungle:  // 168
                result = BiomeValues::BambooJungleHills;  // 169
                break;
            case BiomeValues::Ocean:  // 0
                result = BiomeValues::DeepOcean;  // 24
                break;
            case BiomeValues::WarmOcean:  // 44
                result = BiomeValues::DeepWarmOcean;  // 47
                break;
            case BiomeValues::LukewarmOcean:  // 45
                result = BiomeValues::DeepLukewarmOcean;  // 48
                break;
            case BiomeValues::ColdOcean:  // 46
                result = BiomeValues::DeepColdOcean;  // 49
                break;
            case BiomeValues::FrozenOcean:  // 10
                result = BiomeValues::DeepFrozenOcean;  // 50
                break;
            case BiomeValues::Mountains:  // 3
                result = BiomeValues::WoodedMountains;  // 34
                break;
            case BiomeValues::Savanna:  // 35
                result = BiomeValues::SavannaPlateau;  // 36
                break;
            default:
                // 检查是否为恶地类型
                if (BiomeValues::isBadlands(biomeValue) && biomeValue != BiomeValues::ErodedBadlands) {
                    // areBiomesSimilar(i, 38) -> 37
                    if (biomeValue == BiomeValues::WoodedBadlandsPlateau) {
                        result = BiomeValues::Badlands;
                    }
                }
                // 深海有可能变成陆地
                if ((biomeValue == BiomeValues::DeepOcean ||
                     biomeValue == BiomeValues::DeepLukewarmOcean ||
                     biomeValue == BiomeValues::DeepColdOcean ||
                     biomeValue == BiomeValues::DeepFrozenOcean) &&
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
    IExtendedAreaContext& context,
    std::unique_ptr<IAreaFactory> input1,
    std::unique_ptr<IAreaFactory> input2)
{
    auto sharedContext = std::dynamic_pointer_cast<LayerContext>(context.shared_from_this());
    return std::make_unique<MergeFactory>(this, sharedContext, std::move(input1), std::move(input2));
}

// ============================================================================
// MixRiverLayer 实现
// ============================================================================

i32 MixRiverLayer::apply(IAreaContext& ctx, const IArea& biomeArea, const IArea& riverArea, i32 x, i32 z) {
    // 参考 MC MixRiverLayer.apply:
    // int i = biomeArea.getValue(this.getOffsetX(x), this.getOffsetZ(z));
    // int j = riverArea.getValue(this.getOffsetX(x), this.getOffsetZ(z));
    // if (LayerUtil.isOcean(i)) {
    //     return i;
    // } else if (j == 7) {
    //     if (i == 12) {  // snowy_plains
    //         return 11;  // frozen_river
    //     } else {
    //         return i != 14 && i != 15 ? j & 255 : 15;  // mushroom_fields or mushroom_field_shore -> shore
    //     }
    // } else {
    //     return i;
    // }

    (void)ctx;  // 不使用

    i32 biome = biomeArea.getValue(getOffsetX(x), getOffsetZ(z));
    i32 river = riverArea.getValue(getOffsetX(x), getOffsetZ(z));

    // 海洋保持不变
    if (BiomeValues::isOcean(biome)) {
        return biome;
    }

    // 河流 (j == 7)
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
    IExtendedAreaContext& context,
    std::unique_ptr<IAreaFactory> input1,
    std::unique_ptr<IAreaFactory> input2)
{
    auto sharedContext = std::dynamic_pointer_cast<LayerContext>(context.shared_from_this());
    return std::make_unique<MergeFactory>(this, sharedContext, std::move(input1), std::move(input2));
}

// ============================================================================
// MixOceansLayer 实现
// ============================================================================

i32 MixOceansLayer::apply(IAreaContext& ctx, const IArea& biomeArea, const IArea& oceanArea, i32 x, i32 z) {
    // 参考 MC 1.16.5 MixOceansLayer.apply:
    // int i = biomeArea.getValue(this.getOffsetX(x), this.getOffsetZ(z));  // 生物群系值
    // int j = oceanArea.getValue(this.getOffsetX(x), this.getOffsetZ(z));  // 海洋温度值
    // if (!LayerUtil.isOcean(i)) {
    //     return i;  // 非海洋保持不变
    // } else {
    //     // 检查周围是否有陆地
    //     for(int i1 = -8; i1 <= 8; i1 += 4) {
    //         for(int j1 = -8; j1 <= 8; j1 += 4) {
    //             int k1 = biomeArea.getValue(this.getOffsetX(x + i1), this.getOffsetZ(z + j1));
    //             if (!LayerUtil.isOcean(k1)) {
    //                 // 有陆地相邻，调整海洋温度
    //                 if (j == 44) return 45;  // warm_ocean -> lukewarm_ocean
    //                 if (j == 10) return 46;  // frozen_ocean -> cold_ocean
    //             }
    //         }
    //     }
    //     // 深海处理
    //     if (i == 24) {  // deep_ocean
    //         if (j == 45) return 48;  // lukewarm -> deep_lukewarm
    //         if (j == 0) return 24;   // ocean -> deep_ocean
    //         if (j == 46) return 49;  // cold -> deep_cold
    //         if (j == 10) return 50;  // frozen -> deep_frozen
    //     }
    //     return j;  // 返回海洋温度
    // }

    (void)ctx;  // 不使用

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
                    return BiomeValues::LukewarmOcean;  // 44 -> 45
                }
                if (ocean == BiomeValues::FrozenOcean) {
                    return BiomeValues::ColdOcean;  // 10 -> 46
                }
            }
        }
    }

    // 深海根据海洋温度调整
    if (biome == BiomeValues::DeepOcean) {  // 24
        switch (ocean) {
            case BiomeValues::LukewarmOcean:  // 45
                return BiomeValues::DeepLukewarmOcean;  // 48
            case BiomeValues::Ocean:  // 0
                return BiomeValues::DeepOcean;  // 24
            case BiomeValues::ColdOcean:  // 46
                return BiomeValues::DeepColdOcean;  // 49
            case BiomeValues::FrozenOcean:  // 10
                return BiomeValues::DeepFrozenOcean;  // 50
            default:
                return ocean;
        }
    }

    return ocean;
}

std::unique_ptr<IAreaFactory> MixOceansLayer::apply(
    IExtendedAreaContext& context,
    std::unique_ptr<IAreaFactory> input1,
    std::unique_ptr<IAreaFactory> input2)
{
    auto sharedContext = std::dynamic_pointer_cast<LayerContext>(context.shared_from_this());
    return std::make_unique<MergeFactory>(this, sharedContext, std::move(input1), std::move(input2));
}

} // namespace layer
} // namespace mc
