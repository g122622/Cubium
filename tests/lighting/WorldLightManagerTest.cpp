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

#include <gtest/gtest.h>

#include "common/core/Constants.hpp"
#include "common/world/chunk/base/SectionPos.hpp"
#include "common/world/lighting/IChunkLightProvider.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"

namespace {

/**
 * @brief 测试用的光照提供者
 *
 * 实现 StarLightLightingProvider 接口，提供最小化测试环境。
 */
class TestLightingProvider final : public mc::StarLightLightingProvider {
public:
    [[nodiscard]] mc::IChunk* getChunkForLight(mc::ChunkCoord x, mc::ChunkCoord z) override { return nullptr; }

    [[nodiscard]] const mc::IChunk* getChunkForLight(mc::ChunkCoord x, mc::ChunkCoord z) const override
    {
        return nullptr;
    }

    [[nodiscard]] const mc::BlockState* getBlockStateForLight(const mc::BlockPos&) const override { return nullptr; }

    [[nodiscard]] mc::IWorld* getWorld() override { return nullptr; }

    [[nodiscard]] const mc::IWorld* getWorld() const override { return nullptr; }

    void markLightChanged(mc::LightType, const mc::SectionPos&) override {}

    [[nodiscard]] bool hasSkyLight() const override { return true; }

    [[nodiscard]] mc::i32 getMinBuildHeight() const override { return 0; }

    [[nodiscard]] mc::i32 getMaxBuildHeight() const override { return mc::world::MAX_BUILD_HEIGHT; }

    [[nodiscard]] mc::i32 getSectionCount() const override { return mc::world::CHUNK_SECTIONS; }
};

} // namespace

// ============================================================================
// WorldLightManager 调试信息测试
// ============================================================================

TEST(WorldLightManagerDebugTest, GetDebugInfoReturnsSectionState)
{
    TestLightingProvider provider;
    mc::WorldLightManager manager(&provider, true, true);

    mc::SectionPos pos(0, 0, 0);

    // 没有数据时应该返回 EMPTY 状态（"2"）
    std::string blockInfo = manager.getDebugInfo(mc::LightType::BLOCK, pos);
    EXPECT_TRUE(blockInfo.find("BlockLight:") == 0);
    EXPECT_TRUE(blockInfo.find("2") != std::string::npos) << "Expected EMPTY state '2', got: " << blockInfo;

    std::string skyInfo = manager.getDebugInfo(mc::LightType::SKY, pos);
    EXPECT_TRUE(skyInfo.find("SkyLight:") == 0);
    EXPECT_TRUE(skyInfo.find("2") != std::string::npos) << "Expected EMPTY state '2', got: " << skyInfo;
}

TEST(WorldLightManagerDebugTest, NoSkyLightEngineReturnsNA)
{
    TestLightingProvider provider;
    // 下界维度：没有天空光
    mc::WorldLightManager manager(&provider, true, false);

    mc::SectionPos pos(0, 0, 0);
    std::string blockInfo = manager.getDebugInfo(mc::LightType::BLOCK, pos);
    EXPECT_TRUE(blockInfo.find("BlockLight:") == 0);

    std::string skyInfo = manager.getDebugInfo(mc::LightType::SKY, pos);
    EXPECT_EQ(skyInfo, "SkyLight: N/A");
}
