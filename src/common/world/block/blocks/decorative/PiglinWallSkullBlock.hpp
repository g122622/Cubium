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
#pragma once

#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/block/blocks/decorative/WallSkullBlock.hpp"
#include "common/world/block/core/Block.hpp"

#include <array>

namespace mc {
namespace blocks {

/**
 * @brief 贴墙猪灵头方块
 *
 * 猪灵头的贴墙碰撞箱略大（10 像素宽），单独重写形状。
 *
 */
class PiglinWallSkullBlock : public WallSkullBlock {
public:
    explicit PiglinWallSkullBlock(const BlockProperties& properties);

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

private:
    CollisionShape m_piglinShapes[4]; // North, South, West, East
};

} // namespace blocks
} // namespace mc
