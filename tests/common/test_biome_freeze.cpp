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

#include "common/TestWorldHelper.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeClimate.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::world::biome;

// ============================================================================
// 测试用世界桩 - 用于 shouldFreeze/shouldSnow 测试
// ============================================================================

/**
 * @brief Biome 冻结/降雪测试用的世界桩
 *
 * 继承 BaseTestWorld，提供可控的方块状态、流体状态和光照。
 */
class FreezeTestWorld : public mc::test::BaseTestWorld {
public:
    // 设置指定位置的方块状态
    void setBlockStateAt(i32 x, i32 y, i32 z, const BlockState* state) { m_blocks[{x, y, z}] = state; }

    // 设置指定位置的流体状态
    void setFluidStateAt(i32 x, i32 y, i32 z, const fluid::FluidState* state) { m_fluids[{x, y, z}] = state; }

    // 设置指定位置的方块光照
    void setBlockLightAt(i32 x, i32 y, i32 z, u8 light) { m_blockLights[{x, y, z}] = light; }

    // IWorld 接口覆写
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        auto it = m_blocks.find({x, y, z});
        return it != m_blocks.end() ? it->second : nullptr;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        auto it = m_fluids.find({x, y, z});
        return it != m_fluids.end() ? it->second : mc::test::BaseTestWorld::getFluidState(x, y, z);
    }

    [[nodiscard]] u8 getBlockLight(i32 x, i32 y, i32 z) const override
    {
        auto it = m_blockLights.find({x, y, z});
        return it != m_blockLights.end() ? it->second : 0;
    }

private:
    struct PosHash {
        size_t operator()(const std::tuple<i32, i32, i32>& p) const
        {
            auto [x, y, z] = p;
            return static_cast<size_t>(x) * 73856093u ^ static_cast<size_t>(y) * 19349663u ^
                static_cast<size_t>(z) * 83492791u;
        }
    };
    std::unordered_map<std::tuple<i32, i32, i32>, const BlockState*, PosHash> m_blocks;
    std::unordered_map<std::tuple<i32, i32, i32>, const fluid::FluidState*, PosHash> m_fluids;
    std::unordered_map<std::tuple<i32, i32, i32>, u8, PosHash> m_blockLights;
};

// ============================================================================
// Biome 冻结/降雪测试夹具
// ============================================================================

class BiomeFreezeTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
        BiomeRegistry::instance().initialize();
    }

    /**
     * @brief 获取水源方块的流体状态
     */
    const fluid::FluidState* getWaterFluidState() const
    {
        auto* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
        return waterFluid ? &waterFluid->defaultState() : nullptr;
    }

    static constexpr i32 SEA_LEVEL = 63;
    FreezeTestWorld world;
};

// ============================================================================
// warmEnoughToRain 测试
// ============================================================================

TEST_F(BiomeFreezeTest, WarmEnoughToRain_ColdBiome_ReturnsFalse)
{
    // 冰原生物群系：温度 0.0 < 0.15
    const Biome& biome = BiomeRegistry::instance().get(Biomes::SnowyPlains);
    EXPECT_FALSE(biome.warmEnoughToRain(0, 64, 0, SEA_LEVEL));
}

TEST_F(BiomeFreezeTest, WarmEnoughToRain_WarmBiome_ReturnsTrue)
{
    // 平原生物群系：温度 > 0.15
    const Biome& biome = BiomeRegistry::instance().get(Biomes::Plains);
    EXPECT_TRUE(biome.warmEnoughToRain(0, 64, 0, SEA_LEVEL));
}

TEST_F(BiomeFreezeTest, WarmEnoughToRain_AtExactThreshold)
{
    // 温度恰好等于阈值 0.15 的生物群系，应该返回 true（>= 0.15）
    // 沼泽温度 0.8 > 0.15，沙漠温度 2.0 > 0.15
    // 直接构造一个温度恰好为 0.15 的生物群系
    Biome biome(Biomes::Plains, "test_threshold");
    BiomeClimate climate(true, 0.15f, BiomeClimate::TemperatureModifier::None, 0.5f, 0.5f, 0.0f, 0.0f);
    biome.setClimate(climate);
    EXPECT_TRUE(biome.warmEnoughToRain(0, 64, 0, SEA_LEVEL));
}

TEST_F(BiomeFreezeTest, WarmEnoughToRain_HighAltitudeCold)
{
    // 平原海平面温暖，但极高处寒冷
    // 平原温度 0.8，需要温度降到 < 0.15 才不够温暖
    // 温度降低公式：(noise * 8.0 + y - seaLevel - 17) * 0.05 / 40.0
    // 当 noise = -1, y = 500: 降温 = (-8 + 500 - 80) * 0.00125 = 0.515，仍不够
    // 改用构造低温生物群系来测试高度降温效果
    Biome biome(Biomes::Plains, "test_altitude");
    // 设置温度为 0.2（略高于 0.15 阈值），使得高度降温能把它推到 0.15 以下
    BiomeClimate climate(true, 0.2f, BiomeClimate::TemperatureModifier::None, 0.5f, 0.5f, 0.0f, 0.0f);
    biome.setClimate(climate);
    // 海平面处温暖
    EXPECT_TRUE(biome.warmEnoughToRain(0, 64, 0, SEA_LEVEL));
    // 极高处寒冷
    EXPECT_FALSE(biome.warmEnoughToRain(0, 500, 0, SEA_LEVEL));
}

// ============================================================================
// coldEnoughToSnow 测试
// ============================================================================

TEST_F(BiomeFreezeTest, ColdEnoughToSnow_ColdBiome_ReturnsTrue)
{
    const Biome& biome = BiomeRegistry::instance().get(Biomes::SnowyPlains);
    EXPECT_TRUE(biome.coldEnoughToSnow(0, 64, 0, SEA_LEVEL));
}

TEST_F(BiomeFreezeTest, ColdEnoughToSnow_WarmBiome_ReturnsFalse)
{
    const Biome& biome = BiomeRegistry::instance().get(Biomes::Plains);
    EXPECT_FALSE(biome.coldEnoughToSnow(0, 64, 0, SEA_LEVEL));
}

TEST_F(BiomeFreezeTest, ColdEnoughToSnow_IsOppositeOfWarmEnoughToRain)
{
    const Biome& coldBiome = BiomeRegistry::instance().get(Biomes::SnowyPlains);
    const Biome& warmBiome = BiomeRegistry::instance().get(Biomes::Plains);
    EXPECT_NE(coldBiome.warmEnoughToRain(0, 64, 0, SEA_LEVEL), coldBiome.coldEnoughToSnow(0, 64, 0, SEA_LEVEL));
    EXPECT_NE(warmBiome.warmEnoughToRain(0, 64, 0, SEA_LEVEL), warmBiome.coldEnoughToSnow(0, 64, 0, SEA_LEVEL));
}

// ============================================================================
// shouldFreeze 测试
// ============================================================================

TEST_F(BiomeFreezeTest, ShouldFreeze_ColdBiome_WaterBlock_LowLight_ReturnsTrue)
{
    const Biome& biome = BiomeRegistry::instance().get(Biomes::SnowyPlains);
    const BlockState* waterState = &VanillaBlocks::WATER->defaultState();
    world.setBlockStateAt(0, 64, 0, waterState);
    world.setFluidStateAt(0, 64, 0, getWaterFluidState());
    world.setBlockLightAt(0, 64, 0, 0);
    EXPECT_TRUE(biome.shouldFreeze(world, 0, 64, 0, SEA_LEVEL, false));
}

TEST_F(BiomeFreezeTest, ShouldFreeze_WarmBiome_ReturnsFalse)
{
    const Biome& biome = BiomeRegistry::instance().get(Biomes::Plains);
    const BlockState* waterState = &VanillaBlocks::WATER->defaultState();
    world.setBlockStateAt(0, 64, 0, waterState);
    world.setFluidStateAt(0, 64, 0, getWaterFluidState());
    world.setBlockLightAt(0, 64, 0, 0);
    EXPECT_FALSE(biome.shouldFreeze(world, 0, 64, 0, SEA_LEVEL, false));
}

TEST_F(BiomeFreezeTest, ShouldFreeze_HighLight_ReturnsFalse)
{
    const Biome& biome = BiomeRegistry::instance().get(Biomes::SnowyPlains);
    const BlockState* waterState = &VanillaBlocks::WATER->defaultState();
    world.setBlockStateAt(0, 64, 0, waterState);
    world.setFluidStateAt(0, 64, 0, getWaterFluidState());
    // 方块光照 >= 10，不应该冻结
    world.setBlockLightAt(0, 64, 0, 10);
    EXPECT_FALSE(biome.shouldFreeze(world, 0, 64, 0, SEA_LEVEL, false));
    world.setBlockLightAt(0, 64, 0, 15);
    EXPECT_FALSE(biome.shouldFreeze(world, 0, 64, 0, SEA_LEVEL, false));
}

TEST_F(BiomeFreezeTest, ShouldFreeze_NoWater_ReturnsFalse)
{
    const Biome& biome = BiomeRegistry::instance().get(Biomes::SnowyPlains);
    world.setBlockLightAt(0, 64, 0, 0);
    EXPECT_FALSE(biome.shouldFreeze(world, 0, 64, 0, SEA_LEVEL, false));
}

TEST_F(BiomeFreezeTest, ShouldFreeze_NonLiquidBlock_ReturnsFalse)
{
    const Biome& biome = BiomeRegistry::instance().get(Biomes::SnowyPlains);
    // 流体为水但方块不是液体（例如含水方块场景）
    const BlockState* airState = &VanillaBlocks::AIR->defaultState();
    world.setBlockStateAt(0, 64, 0, airState);
    world.setFluidStateAt(0, 64, 0, getWaterFluidState());
    world.setBlockLightAt(0, 64, 0, 0);
    EXPECT_FALSE(biome.shouldFreeze(world, 0, 64, 0, SEA_LEVEL, false));
}

TEST_F(BiomeFreezeTest, ShouldFreeze_CheckNeighbors_AllWater_ReturnsFalse)
{
    const Biome& biome = BiomeRegistry::instance().get(Biomes::SnowyPlains);
    const BlockState* waterState = &VanillaBlocks::WATER->defaultState();
    const fluid::FluidState* waterFluid = getWaterFluidState();

    // 设置中心位置和四个邻居全是水
    for (auto pos : {std::make_tuple(0, 64, 0),
             std::make_tuple(-1, 64, 0),
             std::make_tuple(1, 64, 0),
             std::make_tuple(0, 64, -1),
             std::make_tuple(0, 64, 1)}) {
        world.setBlockStateAt(std::get<0>(pos), std::get<1>(pos), std::get<2>(pos), waterState);
        world.setFluidStateAt(std::get<0>(pos), std::get<1>(pos), std::get<2>(pos), waterFluid);
    }
    world.setBlockLightAt(0, 64, 0, 0);

    // checkNeighbors=true 且四个邻居全是水，不应该冻结
    EXPECT_FALSE(biome.shouldFreeze(world, 0, 64, 0, SEA_LEVEL, true));
}

TEST_F(BiomeFreezeTest, ShouldFreeze_CheckNeighbors_SomeNonWater_ReturnsTrue)
{
    const Biome& biome = BiomeRegistry::instance().get(Biomes::SnowyPlains);
    const BlockState* waterState = &VanillaBlocks::WATER->defaultState();
    const fluid::FluidState* waterFluid = getWaterFluidState();

    // 设置中心位置为水
    world.setBlockStateAt(0, 64, 0, waterState);
    world.setFluidStateAt(0, 64, 0, waterFluid);
    world.setBlockLightAt(0, 64, 0, 0);

    // 只有两个邻居是水
    world.setBlockStateAt(-1, 64, 0, waterState);
    world.setFluidStateAt(-1, 64, 0, waterFluid);
    world.setBlockStateAt(1, 64, 0, waterState);
    world.setFluidStateAt(1, 64, 0, waterFluid);
    // z 方向邻居不是水（保持默认）

    // checkNeighbors=true 但不是所有邻居都是水，应该冻结
    EXPECT_TRUE(biome.shouldFreeze(world, 0, 64, 0, SEA_LEVEL, true));
}

TEST_F(BiomeFreezeTest, ShouldFreeze_NoCheckNeighbors_AllWater_ReturnsTrue)
{
    const Biome& biome = BiomeRegistry::instance().get(Biomes::SnowyPlains);
    const BlockState* waterState = &VanillaBlocks::WATER->defaultState();
    const fluid::FluidState* waterFluid = getWaterFluidState();

    // 设置中心位置和四个邻居全是水
    for (auto pos : {std::make_tuple(0, 64, 0),
             std::make_tuple(-1, 64, 0),
             std::make_tuple(1, 64, 0),
             std::make_tuple(0, 64, -1),
             std::make_tuple(0, 64, 1)}) {
        world.setBlockStateAt(std::get<0>(pos), std::get<1>(pos), std::get<2>(pos), waterState);
        world.setFluidStateAt(std::get<0>(pos), std::get<1>(pos), std::get<2>(pos), waterFluid);
    }
    world.setBlockLightAt(0, 64, 0, 0);

    // checkNeighbors=false，即使所有邻居都是水也应该冻结
    EXPECT_TRUE(biome.shouldFreeze(world, 0, 64, 0, SEA_LEVEL, false));
}

TEST_F(BiomeFreezeTest, ShouldFreeze_OutOfBuildHeight_ReturnsFalse)
{
    const Biome& biome = BiomeRegistry::instance().get(Biomes::SnowyPlains);
    const BlockState* waterState = &VanillaBlocks::WATER->defaultState();
    world.setBlockStateAt(0, 500, 0, waterState);
    world.setFluidStateAt(0, 500, 0, getWaterFluidState());
    world.setBlockLightAt(0, 500, 0, 0);
    // Y=500 > MAX_BUILD_HEIGHT=320
    EXPECT_FALSE(biome.shouldFreeze(world, 0, 500, 0, SEA_LEVEL, false));
}

TEST_F(BiomeFreezeTest, ShouldFreeze_LightExactly9_ReturnsTrue)
{
    const Biome& biome = BiomeRegistry::instance().get(Biomes::SnowyPlains);
    const BlockState* waterState = &VanillaBlocks::WATER->defaultState();
    world.setBlockStateAt(0, 64, 0, waterState);
    world.setFluidStateAt(0, 64, 0, getWaterFluidState());
    // 光照 < 10，应该冻结
    world.setBlockLightAt(0, 64, 0, 9);
    EXPECT_TRUE(biome.shouldFreeze(world, 0, 64, 0, SEA_LEVEL, false));
}

// ============================================================================
// shouldSnow 测试
// ============================================================================

TEST_F(BiomeFreezeTest, ShouldSnow_ColdBiomeWithSnowPrecipitation_AirBlockLowLight_ReturnsTrue)
{
    const Biome& biome = BiomeRegistry::instance().get(Biomes::SnowyPlains);
    world.setBlockStateAt(0, 65, 0, &VanillaBlocks::AIR->defaultState());
    world.setBlockStateAt(0, 64, 0, &VanillaBlocks::STONE->defaultState());
    world.setBlockLightAt(0, 65, 0, 0);
    EXPECT_TRUE(biome.shouldSnow(world, 0, 65, 0, SEA_LEVEL));
}

TEST_F(BiomeFreezeTest, ShouldSnow_WarmBiome_ReturnsFalse)
{
    const Biome& biome = BiomeRegistry::instance().get(Biomes::Plains);
    world.setBlockStateAt(0, 65, 0, &VanillaBlocks::AIR->defaultState());
    world.setBlockStateAt(0, 64, 0, &VanillaBlocks::STONE->defaultState());
    world.setBlockLightAt(0, 65, 0, 0);
    EXPECT_FALSE(biome.shouldSnow(world, 0, 65, 0, SEA_LEVEL));
}

TEST_F(BiomeFreezeTest, ShouldSnow_NoPrecipitation_ReturnsFalse)
{
    // 沙漠生物群系：降水类型为 None
    const Biome& biome = BiomeRegistry::instance().get(Biomes::Desert);
    world.setBlockStateAt(0, 65, 0, &VanillaBlocks::AIR->defaultState());
    world.setBlockStateAt(0, 64, 0, &VanillaBlocks::STONE->defaultState());
    world.setBlockLightAt(0, 65, 0, 0);
    EXPECT_FALSE(biome.shouldSnow(world, 0, 65, 0, SEA_LEVEL));
}

TEST_F(BiomeFreezeTest, ShouldSnow_HighLight_ReturnsFalse)
{
    const Biome& biome = BiomeRegistry::instance().get(Biomes::SnowyPlains);
    world.setBlockStateAt(0, 65, 0, &VanillaBlocks::AIR->defaultState());
    world.setBlockStateAt(0, 64, 0, &VanillaBlocks::STONE->defaultState());
    world.setBlockLightAt(0, 65, 0, 10);
    EXPECT_FALSE(biome.shouldSnow(world, 0, 65, 0, SEA_LEVEL));
}

TEST_F(BiomeFreezeTest, ShouldSnow_NonAirBlock_ReturnsFalse)
{
    const Biome& biome = BiomeRegistry::instance().get(Biomes::SnowyPlains);
    world.setBlockStateAt(0, 65, 0, &VanillaBlocks::STONE->defaultState());
    world.setBlockStateAt(0, 64, 0, &VanillaBlocks::STONE->defaultState());
    world.setBlockLightAt(0, 65, 0, 0);
    EXPECT_FALSE(biome.shouldSnow(world, 0, 65, 0, SEA_LEVEL));
}

TEST_F(BiomeFreezeTest, ShouldSnow_ExistingSnowBlock_ReturnsTrue)
{
    const Biome& biome = BiomeRegistry::instance().get(Biomes::SnowyPlains);
    world.setBlockStateAt(0, 65, 0, &VanillaBlocks::SNOW->defaultState());
    world.setBlockStateAt(0, 64, 0, &VanillaBlocks::STONE->defaultState());
    world.setBlockLightAt(0, 65, 0, 0);
    EXPECT_TRUE(biome.shouldSnow(world, 0, 65, 0, SEA_LEVEL));
}

TEST_F(BiomeFreezeTest, ShouldSnow_NoSolidBelow_ReturnsFalse)
{
    const Biome& biome = BiomeRegistry::instance().get(Biomes::SnowyPlains);
    world.setBlockStateAt(0, 65, 0, &VanillaBlocks::AIR->defaultState());
    world.setBlockStateAt(0, 64, 0, &VanillaBlocks::AIR->defaultState());
    world.setBlockLightAt(0, 65, 0, 0);
    EXPECT_FALSE(biome.shouldSnow(world, 0, 65, 0, SEA_LEVEL));
}

TEST_F(BiomeFreezeTest, ShouldSnow_OutOfBuildHeight_ReturnsFalse)
{
    const Biome& biome = BiomeRegistry::instance().get(Biomes::SnowyPlains);
    world.setBlockStateAt(0, 500, 0, &VanillaBlocks::AIR->defaultState());
    world.setBlockLightAt(0, 500, 0, 0);
    EXPECT_FALSE(biome.shouldSnow(world, 0, 500, 0, SEA_LEVEL));
}

TEST_F(BiomeFreezeTest, ShouldSnow_LightExactly9_ReturnsTrue)
{
    const Biome& biome = BiomeRegistry::instance().get(Biomes::SnowyPlains);
    world.setBlockStateAt(0, 65, 0, &VanillaBlocks::AIR->defaultState());
    world.setBlockStateAt(0, 64, 0, &VanillaBlocks::STONE->defaultState());
    world.setBlockLightAt(0, 65, 0, 9);
    EXPECT_TRUE(biome.shouldSnow(world, 0, 65, 0, SEA_LEVEL));
}
