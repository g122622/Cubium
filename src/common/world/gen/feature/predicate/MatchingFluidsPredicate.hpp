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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT ANY WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "BlockPredicate.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include <vector>

namespace mc::world::gen::feature::predicate {

/**
 * @brief 检查方块流体状态是否匹配指定流体的谓词
 *
 * 对齐 MC 1.21.11 net.minecraft.world.level.levelgen.blockpredicates.MatchingFluidsPredicate：
 * 继承自 StateTestingPredicate，在 pos+offset 处取方块状态，判断其流体状态是否属于
 * 给定的流体集合（HolderSet<Fluid>，可含 #tag 引用）。空方块的流体状态为 minecraft:empty。
 */
class MatchingFluidsPredicate : public BlockPredicate {
public:
    /**
     * @brief 构造谓词
     * @param fluids 已解析的流体指针列表（可为空）
     * @param tags 已解析的流体标签指针列表（可为空）
     * @param offset 测试位置偏移（默认 0,0,0）
     */
    MatchingFluidsPredicate(
        std::vector<const fluid::Fluid*> fluids, std::vector<const fluid::FluidTag*> tags, BlockPos offset)
        : m_fluids(std::move(fluids))
        , m_tags(std::move(tags))
        , m_offset(offset)
    {}

    [[nodiscard]] bool test(const IWorld& world, const BlockPos& pos) const override;
    [[nodiscard]] std::unique_ptr<BlockPredicate> clone() const override
    {
        return std::make_unique<MatchingFluidsPredicate>(m_fluids, m_tags, m_offset);
    }

private:
    std::vector<const fluid::Fluid*> m_fluids;
    std::vector<const fluid::FluidTag*> m_tags;
    BlockPos m_offset;
};

} // namespace mc::world::gen::feature::predicate
