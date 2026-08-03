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

#include "common/world/block/BlockPos.hpp"
#include "world/blockentity/BlockEntity.hpp"
#include <memory>

namespace mc {
namespace blockentity {

/**
 * @brief 日光探测器方块实体
 *
 * 日光探测器使用方块 tick 来管理定期更新，每20游戏tick更新一次信号强度。
 * 这个方块实体主要用于未来的扩展（如自定义名称存储）。
 *
 * 当前实现使用方块 tick 机制而非方块实体 tick。
 */
class DaylightDetectorEntity : public BlockEntity {
public:
    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit DaylightDetectorEntity(const BlockPos& pos);

    /**
     * @brief 创建方块实体的副本
     * @return 副本的unique_ptr
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;
};

} // namespace blockentity
} // namespace mc
