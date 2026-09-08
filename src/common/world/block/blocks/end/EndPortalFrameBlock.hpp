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
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include <memory>

namespace mc {

class BlockItemUseContext;

namespace blockpattern {
class BlockPattern;
} // namespace blockpattern

namespace blocks {

/**
 * @brief 末地传送门框架方块
 *
 * 用于构建末地传送门的框架方块。
 * 可以放入末影之眼。
 *
 * 状态属性：
 * - EYE: 是否有眼
 * - FACING: 朝向（水平）
 *
 * 参考: net.minecraft.block.EndPortalFrameBlock
 */
class EndPortalFrameBlock : public Block {
public:
    explicit EndPortalFrameBlock(const BlockProperties& properties);
    ~EndPortalFrameBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] bool hasEye(const BlockState& state) const noexcept;
    [[nodiscard]] Direction getFacing(const BlockState& state) const noexcept;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 旋转 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const noexcept override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const noexcept override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const noexcept override;

    // ========== 传送门框架模式 ==========

    /**
     * @brief 获取或创建末地传送门框架检测模式
     *
     * 对应 MC Java: EndPortalFrameBlock.getOrCreatePortalShape()
     *
     * 构建 5×5 方块模式，匹配 12 个末地传送门框架方块组成的完整传送门结构：
     *   - '?' 匹配任意方块（传送门内部和外围）
     *   - '^' 匹配含眼且朝向南方的框架方块
     *   - '>' 匹配含眼且朝向西方的框架方块
     *   - 'v' 匹配含眼且朝向北方框架方块
     *
     * 模式布局（每个字符串为一行，aisle 顺序为从上到下）：
     * @code
     *   "?vvv?"
     *   ">???<"
     *   ">???<"
     *   ">???<"
     *   "?^^^?"
     * @endcode
     *
     * 匹配后通过 match->frontTopLeft() 获取左上角框架位置。
     *
     * @return 方块模式（unique_ptr 所有权），首次调用时构造
     */
    [[nodiscard]] static std::unique_ptr<blockpattern::BlockPattern> getOrCreatePortalShape();

private:
    CollisionShape m_frameShape;
    CollisionShape m_frameWithEyeShape;
};

} // namespace blocks
} // namespace mc
