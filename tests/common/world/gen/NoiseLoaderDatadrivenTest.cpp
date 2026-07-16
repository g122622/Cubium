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

// ============================================================================
// NoiseLoader 数据驱动测试
//
// 验证 NoiseLoader 从原版数据包加载 worldgen/noise/*.json 后，Noises 注册表
// 中的值与 JSON 一致（即数据驱动注入路径正确：路径推导、JSON 解析、注册）。
//
// 本测试加载真实原版数据包（~/minecraft_reborn/datapacks/），目录缺失时
// 整套跳过。加载会清空 Noises 硬编码兜底并 markLoadedFromDatapack(true)，
// 后续 NoisesTest 的 fallback 路径在测试间因进程级单例会受影响——故本套件
// 独立、放最后运行，且不依赖 fallback 行为。
// ============================================================================

#include "common/core/GameDirectory.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/world/gen/noise/NoiseLoader.hpp"
#include "common/world/gen/noise/Noises.hpp"

#include <filesystem>
#include <gtest/gtest.h>

namespace mc {
namespace {

using namespace world::gen::noise;

// 期望值（取自原版 worldgen/noise/*.json）。
struct ExpectedNoise {
    const char* name;
    i32 firstOctave;
    std::vector<f64> amplitudes;
};

// 采样覆盖各噪声类别（气候/洞穴/矿脉/面条/地表/下界/其他），逐项核对 firstOctave
// 与 amplitudes。值与 Noises::initialize() 硬编码兜底一致（数据驱动与兜底等价）。
const std::vector<ExpectedNoise>& expectedNoises()
{
    static const std::vector<ExpectedNoise> kExpected = {
        // 气候
        {Noises::TEMPERATURE, -10, {1.5, 0.0, 1.0, 0.0, 0.0, 0.0}},
        {Noises::VEGETATION, -8, {1.0, 1.0, 0.0, 0.0, 0.0, 0.0}},
        {Noises::CONTINENTALNESS, -9, {1.0, 1.0, 2.0, 2.0, 2.0, 1.0, 1.0, 1.0, 1.0}},
        {Noises::EROSION, -9, {1.0, 1.0, 0.0, 1.0, 1.0}},
        {Noises::TEMPERATURE_LARGE, -12, {1.5, 0.0, 1.0, 0.0, 0.0, 0.0}},
        {Noises::CONTINENTALNESS_LARGE, -11, {1.0, 1.0, 2.0, 2.0, 2.0, 1.0, 1.0, 1.0, 1.0}},
        {Noises::EROSION_LARGE, -11, {1.0, 1.0, 0.0, 1.0, 1.0}},
        {Noises::RIDGE, -7, {1.0, 2.0, 1.0, 0.0, 0.0, 0.0}},
        // SHIFT 常量指向 minecraft:offset（原版文件名 offset.json）
        {Noises::SHIFT, -3, {1.0, 1.0, 1.0, 0.0}},
        // 洞穴
        {Noises::AQUIFER_BARRIER, -3, {1.0}},
        {Noises::AQUIFER_FLUID_LEVEL_FLOODEDNESS, -7, {1.0}},
        {Noises::AQUIFER_LAVA, -1, {1.0}},
        {Noises::AQUIFER_FLUID_LEVEL_SPREAD, -5, {1.0}},
        {Noises::PILLAR, -7, {1.0, 1.0}},
        {Noises::PILLAR_RARENESS, -8, {1.0}},
        {Noises::PILLAR_THICKNESS, -8, {1.0}},
        {Noises::SPAGHETTI_2D, -7, {1.0}},
        {Noises::SPAGHETTI_2D_ELEVATION, -8, {1.0}},
        {Noises::SPAGHETTI_2D_MODULATOR, -11, {1.0}},
        {Noises::SPAGHETTI_2D_THICKNESS, -11, {1.0}},
        {Noises::SPAGHETTI_3D_1, -7, {1.0}},
        {Noises::SPAGHETTI_3D_2, -7, {1.0}},
        {Noises::SPAGHETTI_3D_RARITY, -11, {1.0}},
        {Noises::SPAGHETTI_3D_THICKNESS, -8, {1.0}},
        {Noises::SPAGHETTI_ROUGHNESS, -5, {1.0}},
        {Noises::SPAGHETTI_ROUGHNESS_MODULATOR, -8, {1.0}},
        {Noises::CAVE_ENTRANCE, -7, {0.4, 0.5, 1.0}},
        {Noises::CAVE_LAYER, -8, {1.0}},
        {Noises::CAVE_CHEESE, -8, {0.5, 1.0, 2.0, 1.0, 2.0, 1.0, 0.0, 2.0, 0.0}},
        // 矿脉
        {Noises::ORE_VEININESS, -8, {1.0}},
        {Noises::ORE_VEIN_A, -7, {1.0}},
        {Noises::ORE_VEIN_B, -7, {1.0}},
        {Noises::ORE_GAP, -5, {1.0}},
        // 面条洞穴
        {Noises::NOODLE, -8, {1.0}},
        {Noises::NOODLE_THICKNESS, -8, {1.0}},
        {Noises::NOODLE_RIDGE_A, -7, {1.0}},
        {Noises::NOODLE_RIDGE_B, -7, {1.0}},
        // 地表
        {Noises::SURFACE, -6, {1.0, 1.0, 1.0}},
        {Noises::SURFACE_SECONDARY, -6, {1.0, 1.0, 0.0, 1.0}},
        {Noises::CLAY_BANDS_OFFSET, -8, {1.0}},
        {Noises::BADLANDS_PILLAR, -2, {1.0, 1.0, 1.0, 1.0}},
        {Noises::BADLANDS_PILLAR_ROOF, -8, {1.0}},
        {Noises::BADLANDS_SURFACE, -6, {1.0, 1.0, 1.0}},
        {Noises::ICEBERG_PILLAR, -6, {1.0, 1.0, 1.0, 1.0}},
        {Noises::ICEBERG_PILLAR_ROOF, -3, {1.0}},
        {Noises::ICEBERG_SURFACE, -6, {1.0, 1.0, 1.0}},
        // SWAMP 常量指向 minecraft:surface_swamp（原版文件名 surface_swamp.json）
        {Noises::SWAMP, -2, {1.0}},
        {Noises::CALCITE, -9, {1.0, 1.0, 1.0, 1.0}},
        {Noises::GRAVEL, -8, {1.0, 1.0, 1.0, 1.0}},
        {Noises::POWDER_SNOW, -6, {1.0, 1.0, 1.0, 1.0}},
        {Noises::PACKED_ICE, -7, {1.0, 1.0, 1.0, 1.0}},
        {Noises::ICE, -4, {1.0, 1.0, 1.0, 1.0}},
        // 下界
        {Noises::SOUL_SAND_LAYER, -8, {1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.013333333333333334}},
        {Noises::GRAVEL_LAYER, -8, {1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.013333333333333334}},
        {Noises::PATCH, -5, {1.0, 0.0, 0.0, 0.0, 0.0, 0.013333333333333334}},
        {Noises::NETHERRACK, -3, {1.0, 0.0, 0.0, 0.35}},
        {Noises::NETHER_WART, -3, {1.0, 0.0, 0.0, 0.9}},
        {Noises::NETHER_STATE_SELECTOR, -4, {1.0}},
        // 其他
        {Noises::JAGGED, -16, {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0}},
    };
    return kExpected;
}

class NoiseLoaderDatadrivenTest : public ::testing::Test {
protected:
    static inline bool s_loaded = false;
    static inline size_t s_count = 0;

    static void SetUpTestSuite()
    {
        const auto dataPackDir = GameDirectory::defaultDirectory().dataPacksDir();
        if (!std::filesystem::exists(dataPackDir)) {
            return;
        }

        resource::DataPackRepository repo;
        auto scanResult = repo.scanDirectory(dataPackDir);
        if (!scanResult.success() || scanResult.value() == 0) {
            return;
        }

        auto result = world::gen::noise::NoiseLoader::loadFromDataPackRepository(repo);
        if (result.success()) {
            s_loaded = true;
            s_count = result.value();
        }
    }

    static void expectParams(const ExpectedNoise& e)
    {
        ASSERT_TRUE(Noises::has(e.name)) << "missing noise: " << e.name;
        const auto& params = Noises::get(e.name);
        EXPECT_EQ(params.firstOctave, e.firstOctave) << e.name;
        ASSERT_EQ(params.amplitudes.size(), e.amplitudes.size()) << e.name;
        for (size_t i = 0; i < e.amplitudes.size(); ++i) {
            EXPECT_NEAR(params.amplitudes[i], e.amplitudes[i], 1e-12) << e.name << " [" << i << "]";
        }
    }
};

TEST_F(NoiseLoaderDatadrivenTest, LoadsAll60VanillaNoises)
{
    if (!s_loaded) {
        GTEST_SKIP() << "Vanilla datapack unavailable — skipping datadriven noise test";
    }
    // 原版数据包 worldgen/noise/ 恰好 60 个 JSON。
    EXPECT_EQ(s_count, 60u);
    EXPECT_TRUE(Noises::isLoadedFromDatapack());
}

TEST_F(NoiseLoaderDatadrivenTest, DatadrivenValuesMatchVanillaJson)
{
    if (!s_loaded) {
        GTEST_SKIP() << "Vanilla datapack unavailable — skipping datadriven noise test";
    }
    for (const auto& e : expectedNoises()) {
        expectParams(e);
    }
}

// 独立验证：offset.json / surface_swamp.json 这两个 RL 与硬编码常量名不同源的噪声，
// 确保路径推导（去 .json、namespace 拼接）正确——它们由 SHIFT/SWAMP 常量按真实 RL 引用。
TEST_F(NoiseLoaderDatadrivenTest, PathResolutionHandlesNonConstantNames)
{
    if (!s_loaded) {
        GTEST_SKIP() << "Vanilla datapack unavailable — skipping datadriven noise test";
    }
    // minecraft:offset ← offset.json（非 "shift" 命名）
    ASSERT_TRUE(Noises::has("minecraft:offset"));
    const auto& offset = Noises::get("minecraft:offset");
    EXPECT_EQ(offset.firstOctave, -3);
    ASSERT_EQ(offset.amplitudes.size(), 4u);

    // minecraft:surface_swamp ← surface_swamp.json
    ASSERT_TRUE(Noises::has("minecraft:surface_swamp"));
    const auto& swamp = Noises::get("minecraft:surface_swamp");
    EXPECT_EQ(swamp.firstOctave, -2);
    ASSERT_EQ(swamp.amplitudes.size(), 1u);
}

} // namespace
} // namespace mc
