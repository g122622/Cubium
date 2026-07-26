/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/ sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

// ============================================================================
// 表面条件生命周期回归测试
//
// 曾经的 bug：VerticalGradientCondition / NoiseThresholdCondition 用 std::call_once
// 在【共享的表面规则树节点】上缓存 PositionalRandomFactory* / NormalNoise* 原始指针。
// 表面规则树由 NoiseSettingsRegistry 单例持有，跨所有 RandomState 共享；而工厂/噪声对象
// 的所有权在 RandomState（m_randomFactoryCache / m_noiseCache）。当首个 RandomState 销毁后，
// 缓存指针即悬垂；下一个 RandomState 复用同一规则树时 compute() 解引用已释放内存 → UAF
// （ACCESS_VIOLATION，崩在 PositionalRandomFactory::at / NormalNoise::getValue）。
//
// 该 bug 仅在“同维度创建第二个 RandomState 并复用规则树”时触发：单区块、单 RandomState 的
// 用例永远命中不到。本测试显式制造该场景——连续生成两个 RandomState（不同种子），各自
// 跑完整 biomes->noise->surface 管线，断言两个都不崩、且基岩层确定性可复现，从而把这条
// 生命周期不变量钉死在测试里。
//
// 修复方式（对齐 MC 1.21 SurfaceRules）：不在共享 Condition 上缓存指针，每次 compute()
// 经 ctx.randomState()->getOrCreate*() 现解析；布尔结果仍由 SurfaceRuleContext 按 XZ/Y 戳缓存。
// ============================================================================

#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/chunk/gen/ChunkStatus.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include <memory>
#include <vector>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::world::chunk;

namespace {

class SurfaceConditionLifecycleTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        fluid::FluidRegistry::instance().initialize();
    }

    // 单区块生成（radius=0），返回 ownership 完整的中心区块。每次调用创建独立的 RandomState，
    // 调用方释放返回值时该 RandomState 连同其工厂/噪声缓存一起销毁——正是触发原 UAF 的形态。
    struct GeneratedChunk {
        std::vector<std::unique_ptr<ChunkPrimer>> ownedChunks;
        std::unique_ptr<WorldGenRegion> region;
        std::unique_ptr<NoiseChunkGenerator> generator;
        ChunkPrimer* centerChunk = nullptr;
    };

    static GeneratedChunk generateSingleChunk(u64 seed, ChunkCoord cx = 0, ChunkCoord cz = 0)
    {
        GeneratedChunk result;
        constexpr i32 radius = 0;
        constexpr i32 diameter = radius * 2 + 1;

        auto settings = DimensionSettings::overworld();
        auto randomState = world::gen::RandomState::create(settings, seed);
        auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false, false);
        result.generator =
            std::make_unique<NoiseChunkGenerator>(std::move(settings), std::move(biomeSource), std::move(randomState));

        std::vector<IChunk*> chunkPtrs;
        for (i32 dz = -radius; dz <= radius; ++dz) {
            for (i32 dx = -radius; dx <= radius; ++dx) {
                auto primer = std::make_unique<ChunkPrimer>(cx + dx, cz + dz);
                chunkPtrs.push_back(primer.get());
                result.ownedChunks.push_back(std::move(primer));
            }
        }
        result.centerChunk = dynamic_cast<ChunkPrimer*>(chunkPtrs[static_cast<size_t>((radius * diameter) + radius)]);
        result.region = std::make_unique<WorldGenRegion>(cx, cz, radius, std::move(chunkPtrs), 0);

        for (i32 dz = -radius; dz <= radius; ++dz) {
            for (i32 dx = -radius; dx <= radius; ++dx) {
                size_t idx = static_cast<size_t>((dz + radius) * diameter + (dx + radius));
                ChunkPrimer* chunk = result.ownedChunks[idx].get();
                // 完整管线：biomes -> noise -> surface。surface 阶段会命中 VerticalGradientCondition
                // （基岩层 verticalGradient）和 NoiseThresholdCondition，正是原 UAF 的两个 compute()。
                result.generator->generateBiomes(*result.region, *chunk);
                result.generator->generateNoise(*result.region, *chunk);
                result.generator->buildSurface(*result.region, *chunk);
            }
        }

        return result;
    }

    // 判断方块是否为特定 VanillaBlock（与 SurfaceRuleParityTest 同款辅助）
    static bool isBlock(const BlockState* state, const Block* block) { return state != nullptr && state->is(block); }

    // 统计区块最底层（MIN_BUILD_HEIGHT）的基岩数量。基岩由 VerticalGradientCondition 驱动的
    // verticalGradient 规则生成，确定性取决于 PositionalRandomFactory.at(x,y,z) —— 同种子同坐标
    // 必须同结果。若 UAF 残留，第二个 RandomState 会读已释放内存，结果非确定甚至崩溃。
    static i32 countBedrockAtBottom(const ChunkPrimer& chunk)
    {
        i32 bedrock = 0;
        for (i32 x = 0; x < world::CHUNK_WIDTH; ++x) {
            for (i32 z = 0; z < world::CHUNK_WIDTH; ++z) {
                if (isBlock(chunk.getBlockState(x, world::MIN_BUILD_HEIGHT, z), VanillaBlocks::BEDROCK)) {
                    ++bedrock;
                }
            }
        }
        return bedrock;
    }
};

// 核心回归：连续两个 RandomState（同种子、同坐标）各自独立生成，两个都必须成功且结果一致。
// 修复前：第一个生成结束后 RandomState 销毁，第二个 buildSurface 解引用悬垂 m_cachedFactory 崩溃。
TEST_F(SurfaceConditionLifecycleTest, TwoSequentialRandomStates_DoNotDangleCachedPointers)
{
    // 第一个 RandomState：完整生成，统计基岩。
    auto first = generateSingleChunk(42, 0, 0);
    ASSERT_NE(first.centerChunk, nullptr);
    const i32 bedrockFirst = countBedrockAtBottom(*first.centerChunk);
    // 底层基岩层应至少有一块基岩（verticalGradient 在 trueAtAndBelow 处恒真）。
    EXPECT_GT(bedrockFirst, 0);

    // 第一个 RandomState 在此销毁（first 析构）——原 bug 的悬垂源头。
    // 显式作用域让销毁发生在第二个生成之前，最大化复现确定性。
    {
        auto doomed = generateSingleChunk(7, 1, 1);
        ASSERT_NE(doomed.centerChunk, nullptr);
        EXPECT_GT(countBedrockAtBottom(*doomed.centerChunk), 0);
    } // doomed 在此销毁，其 RandomState 的工厂/噪声缓存被释放。

    // 第二个 RandomState：复用同一份共享表面规则树。修复前在此崩溃（UAF）；
    // 修复后应正常生成，且基岩数量与第一个（同种子同坐标）一致——证明 VerticalGradientCondition
    // 经现解析路径仍确定性，没有被第一个 RandomState 的残留缓存污染。
    auto second = generateSingleChunk(42, 0, 0);
    ASSERT_NE(second.centerChunk, nullptr);
    const i32 bedrockSecond = countBedrockAtBottom(*second.centerChunk);
    EXPECT_GT(bedrockSecond, 0);
    EXPECT_EQ(bedrockFirst, bedrockSecond)
        << "同种子同坐标的两次生成基岩数量不一致，说明 VerticalGradientCondition 解析路径非确定"
           "（可能残留了首个 RandomState 的悬垂缓存）。";
}

// 跨多个种子连续生成，确保任何种子序列下都不触发悬垂（覆盖 NoiseThresholdCondition 的 XZ 缓存路径）。
TEST_F(SurfaceConditionLifecycleTest, ManySequentialRandomStates_AcrossSeeds_Stable)
{
    constexpr u64 seeds[] = {42ULL, 12345ULL, 987654321ULL, 0xCAFEBABEULL, 0xDEADBEEFULL, 1ULL, 2ULL};
    i32 lastBedrock = -1;
    for (u64 seed : seeds) {
        auto result = generateSingleChunk(seed, 0, 0); // 每个种子一个独立 RandomState，循环结束即销毁
        ASSERT_NE(result.centerChunk, nullptr);
        // buildSurface 已在 generateSingleChunk 内完成；能走到这里就说明没有 UAF 崩溃。
        const i32 bedrock = countBedrockAtBottom(*result.centerChunk);
        EXPECT_GE(bedrock, 0);
        (void)lastBedrock;
        lastBedrock = bedrock;
    }
    // 至少跑完所有种子未崩溃即通过；lastBedrock 仅作消费避免告警。
    EXPECT_GE(lastBedrock, 0);
}

} // namespace
