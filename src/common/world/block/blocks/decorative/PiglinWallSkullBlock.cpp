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

#include "common/world/block/blocks/decorative/PiglinWallSkullBlock.hpp"

#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include <cstddef>

namespace mc {
namespace blocks {

namespace {

/// 构造墙壁头颅的形状表（按 North/South/West/East 顺序，占据靠墙一侧 8/16 深度）
void _buildWallShapes(CollisionShape out[4], f32 widthPixels)
{
    // 原版墙壁头颅形状（实测 forAllBoxes）：
    //
    //   北向: box(0.25, 0.25, 0.5 -> 0.75, 0.75, 1.0)
    //
    // 即宽度居中、**Y 也居中**（4..12 像素）、深度占据靠墙一侧的一半。墙上的头颅朝向
    // 与站立时一致，无需旋转 Y。
    //
    // 此前 Y 范围写成 8..16 像素（贴顶），头颅比原版高出一半的位置。
    const f32 half = widthPixels / 2.0f / 16.0f;
    const f32 cx = 0.5f;
    constexpr f32 yMin = 4.0f / 16.0f;
    constexpr f32 yMax = 12.0f / 16.0f;
    // 朝向与占据侧的对应（依据原版实测）：
    //   北向 -> Z 靠 1 侧；南向 -> Z 靠 0 侧；西向 -> X 靠 1 侧；东向 -> X 靠 0 侧
    // 头颅的「面朝」方向与它贴附的墙面相反，因此占据的是远离 FACING 的那一半。
    out[0] = CollisionShape::box(cx - half, yMin, 0.5f, cx + half, yMax, 1.0f); // North
    out[1] = CollisionShape::box(cx - half, yMin, 0.0f, cx + half, yMax, 0.5f); // South
    out[2] = CollisionShape::box(0.5f, yMin, cx - half, 1.0f, yMax, cx + half); // West
    out[3] = CollisionShape::box(0.0f, yMin, cx - half, 0.5f, yMax, cx + half); // East
}

/// 方向到形状表下标（水平方向）
i32 _facingIndex(Direction facing)
{
    switch (facing) {
        case Direction::North:
            return 0;
        case Direction::South:
            return 1;
        case Direction::West:
            return 2;
        case Direction::East:
            return 3;
        default:
            return 0;
    }
}

} // namespace

// ============================================================================
// PiglinWallSkullBlock
// ============================================================================

PiglinWallSkullBlock::PiglinWallSkullBlock(const BlockProperties& properties)
    : WallSkullBlock(SkullType::Piglin, properties)
{
    _buildWallShapes(m_piglinShapes, 10.0f);
}

const CollisionShape& PiglinWallSkullBlock::getShape(const BlockState& state) const
{
    return m_piglinShapes[_facingIndex(state.get(BlockStateProperties::HORIZONTAL_FACING()))];
}

} // namespace blocks
} // namespace mc
