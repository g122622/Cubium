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

#include "TreeDecorator.hpp"
#include "common/util/Direction.hpp"
#include "common/world/gen/feature/parser/BlockStateProviderParser.hpp"
#include <memory>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace tree {
namespace decorator {

/**
 * @brief 附着原木装饰器（MC AttachedToLogsDecorator）
 *
 * 对每根原木：从 directions 随机选一个方向，若 nextFloat()<=probability 且
 * 该邻居为空气，则用 block_provider 采样方块放置。fallen_tree 的 log_decorators
 * 用它在倒木上方放红/棕蘑菇。
 *
 * 配置字段：probability[0.0,1.0] / block_provider(BlockStateProvider) /
 * directions(非空 Direction 列表)。
 */
class AttachToLogsDecorator final : public TreeDecorator {
public:
    AttachToLogsDecorator(f32 probability,
        std::unique_ptr<parser::BlockStateProviderHandle> blockProvider,
        std::vector<Direction> directions);

    void place(const TreeDecoratorContext& context) const override;

private:
    f32 m_probability;
    std::unique_ptr<parser::BlockStateProviderHandle> m_blockProvider;
    std::vector<Direction> m_directions;
};

} // namespace decorator
} // namespace tree
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
