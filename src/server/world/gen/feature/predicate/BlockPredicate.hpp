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
 */

#pragma once

#include "common/world/IWorld.hpp"
#include <memory>

namespace mc::world::gen::feature::predicate {

/**
 * @brief 方块谓词基类
 *
 * 用于特征生成中判断方块位置是否满足特定条件。
 */
class BlockPredicate {
public:
    virtual ~BlockPredicate() = default;

    /**
     * @brief 测试指定位置的方块是否满足条件
     * @param world 世界读取接口
     * @param pos 方块位置
     * @return 是否满足条件
     */
    [[nodiscard]] virtual bool test(const IWorld& world, const BlockPos& pos) const = 0;

    /**
     * @brief 克隆谓词
     */
    [[nodiscard]] virtual std::unique_ptr<BlockPredicate> clone() const = 0;
};

} // namespace mc::world::gen::feature::predicate

// 向后兼容：包含此头文件即可获取所有方块谓词类型
#include "Predicates.hpp"
