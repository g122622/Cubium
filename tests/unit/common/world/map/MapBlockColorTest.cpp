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

/**
 * @file MapBlockColorTest.cpp
 * @brief 地图取色使用的方块状态选择（含水方块 → 水色）
 *
 * 覆盖 `world::map::mapColorStateFor` 的三条分支：
 * 1. 不含流体 → 用状态本身；
 * 2. 含流体但顶面不是完整支撑面（含水下半台阶等）→ 换成流体的 legacy 方块（水）；
 * 3. 含流体且顶面完整（`type=double` 的含水台阶这类完整立方体）→ 仍用状态本身。
 */

#include "common/world/map/MapBlockColor.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/command/StringReader.hpp"
#include "common/command/arguments/BlockStateArgument.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/block/state/BlockState.hpp"
#include "common/world/map/MaterialColor.hpp"
#include <gtest/gtest.h>

using namespace mc;

namespace {

/// 按 SNBT 风格的状态描述解析出方块状态（复用命令参数解析器，避免在测试里手搓状态）
const BlockState* _state(std::string_view text)
{
    const std::string source(text);
    command::StringReader reader{source};
    command::BlockStateInput input = command::BlockStateArgumentType().parse(reader);
    if (!input.isValid()) {
        return nullptr;
    }
    return input.state();
}

class MapBlockColorTest : public testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
    }

    mc::test::MapBackedTestWorld m_world{Difficulty::Easy};
};

} // namespace

TEST_F(MapBlockColorTest, NonFluidStateKeepsItself)
{
    const BlockState* stone = _state("stone");
    ASSERT_NE(stone, nullptr);

    const BlockPos pos(0, 64, 0);
    EXPECT_EQ(world::map::mapColorStateFor(m_world, *stone, pos), stone);
}

// 含水但顶面不完整（下半台阶）→ 地图应显示成水
TEST_F(MapBlockColorTest, WaterloggedPartialBlockBecomesWater)
{
    const BlockState* slab = _state("oak_slab[type=bottom,waterlogged=true]");
    ASSERT_NE(slab, nullptr);

    const BlockPos pos(0, 64, 0);
    const BlockState* colored = world::map::mapColorStateFor(m_world, *slab, pos);
    ASSERT_NE(colored, nullptr);
    EXPECT_NE(colored, slab) << "a waterlogged partial block must not keep its own map color";
    EXPECT_EQ(colored->getMapColor(), world::map::MaterialColorId::WATER)
        << "the substituted state must carry the water map color";
}

// 含水且顶面完整（double 台阶是完整立方体）→ 仍显示自身颜色，不能换成水
TEST_F(MapBlockColorTest, WaterloggedFullCubeKeepsItsOwnColor)
{
    const BlockState* slab = _state("oak_slab[type=double,waterlogged=true]");
    ASSERT_NE(slab, nullptr);

    const BlockPos pos(0, 64, 0);
    EXPECT_EQ(world::map::mapColorStateFor(m_world, *slab, pos), slab)
        << "a waterlogged full cube must keep its own map color";
}
