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

#include <memory>
#include <vector>

#include "common/world/chunk/data/light/SWMRNibbleArray.hpp"
#include "server/world/lighting/engine/BaseLightEngine.hpp"

using namespace mc;

namespace {

/**
 * @brief 把 protected 的静态工具暴露给测试
 *
 * `getFilledEmptyLight` 是引擎内部的重建工具，不适合为了测试放宽生产 API，
 * 因此按既有的 `BaseLightEngineQueueTest` 模式经子类暴露。
 */
class TestLightEngine : public StarLightEngine {
public:
    TestLightEngine()
        : StarLightEngine(false)
    {
    }

    using StarLightEngine::getFilledEmptyLight;
};

} // namespace

/**
 * @brief 点亮流程输入 nibble 工厂的单元测试
 *
 * 这批 nibble 是 `StarLightEngine::light` 的一次性输入：lightChunk 期间它们被当作
 * "全 NULL 段"参与传播，结束后由 setSkyNibbles/setBlockNibbles 按移动语义搬进区块。
 * 两个性质必须守住：**段数正确** 与 **全部为 NULL 状态**（NULL ≠ 全零的 UNINIT/INIT，
 * 天空光在 NULL 段上走的是单独的 checkNullSection 路径）。
 */
class FilledEmptyLightTest : public ::testing::Test {
protected:
    static void expectAllNull(const std::vector<std::unique_ptr<SWMRNibbleArray>>& nibbles, i32 expectedCount)
    {
        ASSERT_EQ(nibbles.size(), static_cast<size_t>(expectedCount));
        for (size_t i = 0; i < nibbles.size(); ++i) {
            ASSERT_NE(nibbles[i], nullptr) << "nibble " << i << " must be allocated";
            EXPECT_TRUE(nibbles[i]->isNullUpdating()) << "nibble " << i << " must be in NULL state";
            EXPECT_TRUE(nibbles[i]->isNullVisible()) << "nibble " << i << " must be NULL on visible side too";
        }
    }
};

TEST_F(FilledEmptyLightTest, CreatesRequestedNumberOfNullNibbles)
{
    const auto nibbles = TestLightEngine::getFilledEmptyLight(24);
    expectAllNull(nibbles, 24);
}

TEST_F(FilledEmptyLightTest, EachNibbleIsDistinctObject)
{
    const auto nibbles = TestLightEngine::getFilledEmptyLight(8);
    ASSERT_EQ(nibbles.size(), 8u);
    // 缓存里存的是裸指针，各段必须是独立对象，不能共享同一地址。
    for (size_t i = 0; i < nibbles.size(); ++i) {
        for (size_t j = i + 1; j < nibbles.size(); ++j) {
            EXPECT_NE(nibbles[i].get(), nibbles[j].get());
        }
    }
}

TEST_F(FilledEmptyLightTest, ZeroSectionsYieldsEmptyVector)
{
    EXPECT_TRUE(TestLightEngine::getFilledEmptyLight(0).empty());
}

TEST_F(FilledEmptyLightTest, NullStateDiffersFromUninitializedState)
{
    const auto nibbles = TestLightEngine::getFilledEmptyLight(1);
    ASSERT_EQ(nibbles.size(), 1u);
    ASSERT_TRUE(nibbles[0]->isNullUpdating());

    // 转成 UNINIT/INIT 后就不再是 NULL：这条边界是天空光 checkNullSection 的判据，
    // 若工厂误建 UNINIT 段，空段会被当成"亮度 0 的实心段"，天空光无法向下传播。
    nibbles[0]->setNonNull();
    EXPECT_FALSE(nibbles[0]->isNullUpdating());
}