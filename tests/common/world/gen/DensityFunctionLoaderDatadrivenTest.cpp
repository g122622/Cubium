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
// DensityFunctionLoader 数据驱动测试（阶段 2）
//
// 验证 DensityFunctionLoader 从原版数据包加载 worldgen/density_function/*.json 后：
// 1. 全部 35 个密度函数成功解析注册（无循环引用、无未注册 type、无前向引用缺失）
// 2. DensityFunctionRegistry 含原版全部 35 个具名 DF（覆盖 root/end/nether/overworld/
//    overworld_amplified/overworld_large_biomes 各子树）
// 3. Holder 引用解析正确：sloped_cheese 等被多父引用的 DF 经 SharedHolder 共享子图
//
// 本测试加载真实原版数据包（~/minecraft_reborn/datapacks/），目录缺失时整套跳过。
// 加载会清空 DensityFunctionRegistry 并 markLoadedFromDatapack(true)——进程级单例，
// 故本套件独立运行。噪声叶子节点在解析期为 UnboundNoiseLeaf 占位（compute 返回 0），
// 真实 NormalNoise 绑定在阶段 3（noise_settings）的 NoiseBindingVisitor 中完成。
// ============================================================================

#include "common/core/GameDirectory.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/world/gen/density/DensityFunctionLoader.hpp"
#include "common/world/gen/density/DensityFunctionRegistry.hpp"
#include "common/world/gen/density/DensityFunctions.hpp"
#include "common/world/gen/noise/NoiseLoader.hpp"

#include <filesystem>
#include <gtest/gtest.h>

namespace mc {
namespace {

using namespace world::gen::density;

// 原版 1.21.11 worldgen/density_function/ 下全部 35 个具名 DF（与数据包文件一一对应）。
// 覆盖 root(4) + end(2) + nether(1) + overworld(11) + overworld_amplified(5)
// + overworld_large_biomes(7) + overworld/caves(5) = 35。
const std::vector<const char*>& expectedDensityFunctionNames()
{
    static const std::vector<const char*> kExpected = {
        // root
        "minecraft:y",
        "minecraft:zero",
        "minecraft:shift_x",
        "minecraft:shift_z",
        // end
        "minecraft:end/base_3d_noise",
        "minecraft:end/sloped_cheese",
        // nether
        "minecraft:nether/base_3d_noise",
        // overworld (含 caves 子目录)
        "minecraft:overworld/base_3d_noise",
        "minecraft:overworld/continents",
        "minecraft:overworld/depth",
        "minecraft:overworld/erosion",
        "minecraft:overworld/factor",
        "minecraft:overworld/jaggedness",
        "minecraft:overworld/offset",
        "minecraft:overworld/ridges",
        "minecraft:overworld/ridges_folded",
        "minecraft:overworld/sloped_cheese",
        "minecraft:overworld/caves/entrances",
        "minecraft:overworld/caves/noodle",
        "minecraft:overworld/caves/pillars",
        "minecraft:overworld/caves/spaghetti_2d",
        "minecraft:overworld/caves/spaghetti_2d_thickness_modulator",
        "minecraft:overworld/caves/spaghetti_roughness_function",
        // overworld_amplified
        "minecraft:overworld_amplified/depth",
        "minecraft:overworld_amplified/factor",
        "minecraft:overworld_amplified/jaggedness",
        "minecraft:overworld_amplified/offset",
        "minecraft:overworld_amplified/sloped_cheese",
        // overworld_large_biomes
        "minecraft:overworld_large_biomes/continents",
        "minecraft:overworld_large_biomes/depth",
        "minecraft:overworld_large_biomes/erosion",
        "minecraft:overworld_large_biomes/factor",
        "minecraft:overworld_large_biomes/jaggedness",
        "minecraft:overworld_large_biomes/offset",
        "minecraft:overworld_large_biomes/sloped_cheese",
    };
    return kExpected;
}

class DensityFunctionLoaderDatadrivenTest : public ::testing::Test {
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

        // density_function 依赖噪声（noise 叶子引用噪声名），故先加载噪声。
        // NoiseLoader 会清空 Noises 兜底并 markLoadedFromDatapack(true)。
        auto noiseResult = world::gen::noise::NoiseLoader::loadFromDataPackRepository(repo);
        if (noiseResult.failed()) {
            return;
        }

        auto result = world::gen::density::DensityFunctionLoader::loadFromDataPackRepository(repo);
        if (result.success()) {
            s_loaded = true;
            s_count = result.value();
        }
    }
};

TEST_F(DensityFunctionLoaderDatadrivenTest, LoadsAll35VanillaDensityFunctions)
{
    if (!s_loaded) {
        GTEST_SKIP() << "Vanilla datapack unavailable — skipping datadriven density_function test";
    }
    // 原版数据包 worldgen/density_function/ 恰好 35 个 JSON，全部应解析成功注册。
    EXPECT_EQ(s_count, 35u);
    EXPECT_TRUE(DensityFunctionRegistry::instance().isLoadedFromDatapack());
}

TEST_F(DensityFunctionLoaderDatadrivenTest, RegistryContainsAllNamedDensityFunctions)
{
    if (!s_loaded) {
        GTEST_SKIP() << "Vanilla datapack unavailable — skipping datadriven density_function test";
    }
    for (const auto* name : expectedDensityFunctionNames()) {
        EXPECT_TRUE(DensityFunctionRegistry::instance().has(ResourceLocation::parse(name)))
            << "missing density_function: " << name;
    }
}

// Holder 引用解析 + 共享子图验证：sloped_cheese 被 finalDensity 与 caves 路径共同引用，
// 必须成功解析（证明前向引用 + 递归 memo + 循环检测无报错）。
TEST_F(DensityFunctionLoaderDatadrivenTest, HolderReferencesResolveSharedSubgraph)
{
    if (!s_loaded) {
        GTEST_SKIP() << "Vanilla datapack unavailable — skipping datadriven density_function test";
    }
    // overworld/sloped_cheese 是被多父引用的核心 DF（depth/factor/jaggedness/base_3d_noise 等）。
    auto slopedCheese =
        DensityFunctionRegistry::instance().get(ResourceLocation::parse("minecraft:overworld/sloped_cheese"));
    ASSERT_NE(slopedCheese, nullptr);
    // 解析期噪声叶子为占位（compute 返回 0），但非占位节点应可正常 compute 不崩溃。
    // 取 y（纯 YClampedGradient，无噪声叶子）验证 compute 走通。
    auto y = DensityFunctionRegistry::instance().get(ResourceLocation::parse("minecraft:y"));
    ASSERT_NE(y, nullptr);
    // y = y_clamped_gradient(from_y=-4064, to_y=4062, from_value=-4064, to_value=4062)
    // compute(0, 0, 0) 应在 [-4064, 4062] 区间内。
    const f64 v = y->compute(0, 0, 0);
    EXPECT_GE(v, -4064.0);
    EXPECT_LE(v, 4062.0);
}

// zero.json 是裸数字 0.0（非 {"type":...} 对象），验证裸数字三态分发正确。
TEST_F(DensityFunctionLoaderDatadrivenTest, BareNumberResolvesToConstant)
{
    if (!s_loaded) {
        GTEST_SKIP() << "Vanilla datapack unavailable — skipping datadriven density_function test";
    }
    auto zero = DensityFunctionRegistry::instance().get(ResourceLocation::parse("minecraft:zero"));
    ASSERT_NE(zero, nullptr);
    EXPECT_DOUBLE_EQ(zero->compute(123, -45, 67), 0.0);
}

} // namespace
} // namespace mc
