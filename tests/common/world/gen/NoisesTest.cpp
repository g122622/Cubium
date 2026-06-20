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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT KIND, EXPRESS OR IMPLIED,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

// ============================================================================
// Noises 注册表测试
//
// 验证 Noises 注册表与 MC 1.21.11 NoiseData 的对齐：
// - 所有气候噪声参数正确
// - 洞穴噪声参数正确
// - 矿脉/面条洞穴/地表/下界噪声参数正确
// - 线程安全（std::call_once）
// - has() 和 get() 正确工作
// ============================================================================

#include "common/world/gen/noise/Noises.hpp"
#include <thread>
#include <vector>
#include <gtest/gtest.h>

namespace mc {
namespace {

using namespace world::gen::noise;

// ============================================================================
// 基础访问测试
// ============================================================================

TEST(NoisesTest, HasReturnsTrueForExistingNoise)
{
    EXPECT_TRUE(Noises::has(Noises::TEMPERATURE));
    EXPECT_TRUE(Noises::has(Noises::SURFACE));
    EXPECT_TRUE(Noises::has(Noises::JAGGED));
}

TEST(NoisesTest, HasReturnsFalseForNonexistentNoise)
{
    EXPECT_FALSE(Noises::has("minecraft:nonexistent"));
    EXPECT_FALSE(Noises::has(""));
}

TEST(NoisesTest, GetThrowsForNonexistentNoise)
{
    EXPECT_THROW(Noises::get("minecraft:nonexistent"), std::out_of_range);
}

TEST(NoisesTest, GetReturnsValidReference)
{
    const auto& params = Noises::get(Noises::TEMPERATURE);
    EXPECT_FALSE(params.amplitudes.empty());
}

// ============================================================================
// 气候噪声参数验证（MC 1.21.11 NoiseData.java — registerBiomeNoises(offset=0)）
// ============================================================================

TEST(NoisesTest, TemperatureParameters)
{
    const auto& params = Noises::get(Noises::TEMPERATURE);
    EXPECT_EQ(params.firstOctave, -10);
    ASSERT_EQ(params.amplitudes.size(), 6u);
    EXPECT_NEAR(params.amplitudes[0], 1.5, 1e-15);
    EXPECT_NEAR(params.amplitudes[1], 0.0, 1e-15);
    EXPECT_NEAR(params.amplitudes[2], 1.0, 1e-15);
    EXPECT_NEAR(params.amplitudes[3], 0.0, 1e-15);
    EXPECT_NEAR(params.amplitudes[4], 0.0, 1e-15);
    EXPECT_NEAR(params.amplitudes[5], 0.0, 1e-15);
}

TEST(NoisesTest, VegetationParameters)
{
    const auto& params = Noises::get(Noises::VEGETATION);
    EXPECT_EQ(params.firstOctave, -8);
    ASSERT_EQ(params.amplitudes.size(), 6u);
    EXPECT_NEAR(params.amplitudes[0], 1.0, 1e-15);
    EXPECT_NEAR(params.amplitudes[1], 1.0, 1e-15);
    // 其余为 0
}

TEST(NoisesTest, ContinentalnessParameters)
{
    const auto& params = Noises::get(Noises::CONTINENTALNESS);
    EXPECT_EQ(params.firstOctave, -9);
    ASSERT_EQ(params.amplitudes.size(), 9u);
    EXPECT_NEAR(params.amplitudes[0], 1.0, 1e-15);
    EXPECT_NEAR(params.amplitudes[1], 1.0, 1e-15);
    EXPECT_NEAR(params.amplitudes[2], 2.0, 1e-15);
    EXPECT_NEAR(params.amplitudes[3], 2.0, 1e-15);
    EXPECT_NEAR(params.amplitudes[4], 2.0, 1e-15);
    EXPECT_NEAR(params.amplitudes[5], 1.0, 1e-15);
    EXPECT_NEAR(params.amplitudes[6], 1.0, 1e-15);
    EXPECT_NEAR(params.amplitudes[7], 1.0, 1e-15);
    EXPECT_NEAR(params.amplitudes[8], 1.0, 1e-15);
}

TEST(NoisesTest, ErosionParameters)
{
    const auto& params = Noises::get(Noises::EROSION);
    EXPECT_EQ(params.firstOctave, -9);
    ASSERT_EQ(params.amplitudes.size(), 5u);
    EXPECT_NEAR(params.amplitudes[0], 1.0, 1e-15);
    EXPECT_NEAR(params.amplitudes[1], 1.0, 1e-15);
    EXPECT_NEAR(params.amplitudes[2], 0.0, 1e-15);
    EXPECT_NEAR(params.amplitudes[3], 1.0, 1e-15);
    EXPECT_NEAR(params.amplitudes[4], 1.0, 1e-15);
}

TEST(NoisesTest, RidgeParameters)
{
    const auto& params = Noises::get(Noises::RIDGE);
    EXPECT_EQ(params.firstOctave, -7);
    ASSERT_EQ(params.amplitudes.size(), 6u);
    EXPECT_NEAR(params.amplitudes[0], 1.0, 1e-15);
    EXPECT_NEAR(params.amplitudes[1], 2.0, 1e-15);
    EXPECT_NEAR(params.amplitudes[2], 1.0, 1e-15);
}

TEST(NoisesTest, ShiftParameters)
{
    const auto& params = Noises::get(Noises::SHIFT);
    EXPECT_EQ(params.firstOctave, -3);
    ASSERT_EQ(params.amplitudes.size(), 4u);
    EXPECT_NEAR(params.amplitudes[0], 1.0, 1e-15);
    EXPECT_NEAR(params.amplitudes[1], 1.0, 1e-15);
    EXPECT_NEAR(params.amplitudes[2], 1.0, 1e-15);
    EXPECT_NEAR(params.amplitudes[3], 0.0, 1e-15);
}

// ============================================================================
// 大型气候噪声参数（offset=-2）
// ============================================================================

TEST(NoisesTest, TemperatureLargeParameters)
{
    const auto& params = Noises::get(Noises::TEMPERATURE_LARGE);
    EXPECT_EQ(params.firstOctave, -12);
    ASSERT_EQ(params.amplitudes.size(), 6u);
    EXPECT_NEAR(params.amplitudes[0], 1.5, 1e-15);
}

TEST(NoisesTest, ContinentalnessLargeParameters)
{
    const auto& params = Noises::get(Noises::CONTINENTALNESS_LARGE);
    EXPECT_EQ(params.firstOctave, -11);
    ASSERT_EQ(params.amplitudes.size(), 9u);
}

TEST(NoisesTest, ErosionLargeParameters)
{
    const auto& params = Noises::get(Noises::EROSION_LARGE);
    EXPECT_EQ(params.firstOctave, -11);
    ASSERT_EQ(params.amplitudes.size(), 5u);
}

// ============================================================================
// 洞穴噪声参数验证
// ============================================================================

TEST(NoisesTest, CaveEntranceParameters)
{
    const auto& params = Noises::get(Noises::CAVE_ENTRANCE);
    EXPECT_EQ(params.firstOctave, -7);
    ASSERT_EQ(params.amplitudes.size(), 3u);
    EXPECT_NEAR(params.amplitudes[0], 0.4, 1e-15);
    EXPECT_NEAR(params.amplitudes[1], 0.5, 1e-15);
    EXPECT_NEAR(params.amplitudes[2], 1.0, 1e-15);
}

TEST(NoisesTest, CaveCheeseParameters)
{
    const auto& params = Noises::get(Noises::CAVE_CHEESE);
    EXPECT_EQ(params.firstOctave, -8);
    ASSERT_EQ(params.amplitudes.size(), 9u);
    EXPECT_NEAR(params.amplitudes[0], 0.5, 1e-15);
    EXPECT_NEAR(params.amplitudes[1], 1.0, 1e-15);
    EXPECT_NEAR(params.amplitudes[2], 2.0, 1e-15);
}

// ============================================================================
// 矿脉噪声参数验证
// ============================================================================

TEST(NoisesTest, OreVeininessParameters)
{
    const auto& params = Noises::get(Noises::ORE_VEININESS);
    EXPECT_EQ(params.firstOctave, -8);
    ASSERT_EQ(params.amplitudes.size(), 1u);
    EXPECT_NEAR(params.amplitudes[0], 1.0, 1e-15);
}

// ============================================================================
// 面条洞穴噪声参数验证
// ============================================================================

TEST(NoisesTest, NoodleParameters)
{
    const auto& params = Noises::get(Noises::NOODLE);
    EXPECT_EQ(params.firstOctave, -8);
    ASSERT_EQ(params.amplitudes.size(), 1u);
}

// ============================================================================
// 地表噪声参数验证
// ============================================================================

TEST(NoisesTest, SurfaceParameters)
{
    const auto& params = Noises::get(Noises::SURFACE);
    EXPECT_EQ(params.firstOctave, -6);
    ASSERT_EQ(params.amplitudes.size(), 3u);
    EXPECT_NEAR(params.amplitudes[0], 1.0, 1e-15);
    EXPECT_NEAR(params.amplitudes[1], 1.0, 1e-15);
    EXPECT_NEAR(params.amplitudes[2], 1.0, 1e-15);
}

TEST(NoisesTest, SurfaceSecondaryParameters)
{
    const auto& params = Noises::get(Noises::SURFACE_SECONDARY);
    EXPECT_EQ(params.firstOctave, -6);
    ASSERT_EQ(params.amplitudes.size(), 4u);
    EXPECT_NEAR(params.amplitudes[0], 1.0, 1e-15);
    EXPECT_NEAR(params.amplitudes[1], 1.0, 1e-15);
    EXPECT_NEAR(params.amplitudes[2], 0.0, 1e-15);
    EXPECT_NEAR(params.amplitudes[3], 1.0, 1e-15);
}

// ============================================================================
// 下界噪声参数验证
// ============================================================================

TEST(NoisesTest, SoulSandLayerParameters)
{
    const auto& params = Noises::get(Noises::SOUL_SAND_LAYER);
    EXPECT_EQ(params.firstOctave, -8);
    ASSERT_EQ(params.amplitudes.size(), 9u);
    EXPECT_NEAR(params.amplitudes[0], 1.0, 1e-15);
    // 最后一个振幅 = 0.013333333333333334
    EXPECT_NEAR(params.amplitudes[8], 0.013333333333333334, 1e-12);
}

TEST(NoisesTest, NetherrackParameters)
{
    const auto& params = Noises::get(Noises::NETHERRACK);
    EXPECT_EQ(params.firstOctave, -3);
    ASSERT_EQ(params.amplitudes.size(), 4u);
    EXPECT_NEAR(params.amplitudes[0], 1.0, 1e-15);
    EXPECT_NEAR(params.amplitudes[3], 0.35, 1e-15);
}

// ============================================================================
// JAGGED 噪声（最多倍频层）
// ============================================================================

TEST(NoisesTest, JaggedParameters)
{
    const auto& params = Noises::get(Noises::JAGGED);
    EXPECT_EQ(params.firstOctave, -16);
    ASSERT_EQ(params.amplitudes.size(), 17u);
    // 所有振幅应为 1.0
    for (size_t i = 0; i < params.amplitudes.size(); ++i) {
        EXPECT_NEAR(params.amplitudes[i], 1.0, 1e-15) << "JAGGED amplitude[" << i << "] should be 1.0";
    }
}

// ============================================================================
// 线程安全测试（std::call_once）
// ============================================================================

TEST(NoisesTest, ThreadSafeInitialization)
{
    // 多线程并发调用 get() 应不会崩溃或产生数据竞争
    constexpr int kThreadCount = 8;
    std::vector<std::thread> threads;
    std::vector<i32> firstOctaves(kThreadCount, 999);
    std::vector<size_t> ampSizes(kThreadCount, 0);

    for (int i = 0; i < kThreadCount; ++i) {
        threads.emplace_back([&firstOctaves, &ampSizes, i]() {
            const auto& params = Noises::get(Noises::TEMPERATURE);
            firstOctaves[i] = params.firstOctave;
            ampSizes[i] = params.amplitudes.size();
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    for (int i = 0; i < kThreadCount; ++i) {
        EXPECT_EQ(firstOctaves[i], -10) << "Thread " << i << " got wrong firstOctave";
        EXPECT_EQ(ampSizes[i], 6u) << "Thread " << i << " got wrong amplitudes size";
    }
}

// ============================================================================
// 常量名称一致性测试
// ============================================================================

TEST(NoisesTest, ConstantNamesMatchRegistry)
{
    // 所有常量名称应以 "minecraft:" 开头
    EXPECT_TRUE(std::string(Noises::TEMPERATURE).starts_with("minecraft:"));
    EXPECT_TRUE(std::string(Noises::SURFACE).starts_with("minecraft:"));
    EXPECT_TRUE(std::string(Noises::JAGGED).starts_with("minecraft:"));

    // 确认 has() 对所有常量返回 true
    EXPECT_TRUE(Noises::has(Noises::TEMPERATURE));
    EXPECT_TRUE(Noises::has(Noises::VEGETATION));
    EXPECT_TRUE(Noises::has(Noises::CONTINENTALNESS));
    EXPECT_TRUE(Noises::has(Noises::EROSION));
    EXPECT_TRUE(Noises::has(Noises::TEMPERATURE_LARGE));
    EXPECT_TRUE(Noises::has(Noises::VEGETATION_LARGE));
    EXPECT_TRUE(Noises::has(Noises::CONTINENTALNESS_LARGE));
    EXPECT_TRUE(Noises::has(Noises::EROSION_LARGE));
    EXPECT_TRUE(Noises::has(Noises::RIDGE));
    EXPECT_TRUE(Noises::has(Noises::SHIFT));
    EXPECT_TRUE(Noises::has(Noises::AQUIFER_BARRIER));
    EXPECT_TRUE(Noises::has(Noises::SURFACE));
    EXPECT_TRUE(Noises::has(Noises::JAGGED));
}

} // namespace
} // namespace mc
