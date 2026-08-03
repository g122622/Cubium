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

#include "BlockPredicate.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include <memory>
#include <utility>
#include <vector>

namespace mc::world::gen::feature::predicate {

/**
 * @brief 所有子条件都满足时返回true的谓词
 *
 * 逻辑与（AND）组合：所有子谓词都满足时返回true，空列表返回true。
 */
class AllOfPredicate : public BlockPredicate {
public:
    /**
     * @brief 构造谓词
     * @param predicates 子谓词列表
     */
    explicit AllOfPredicate(std::vector<std::unique_ptr<BlockPredicate>> predicates)
        : m_predicates(std::move(predicates))
    {}

    [[nodiscard]] bool test(const IWorld& world, const BlockPos& pos) const override;
    [[nodiscard]] std::unique_ptr<BlockPredicate> clone() const override;

private:
    std::vector<std::unique_ptr<BlockPredicate>> m_predicates;
};

/**
 * @brief 任一子条件满足时返回true的谓词
 *
 * 逻辑或（OR）组合：任一子谓词满足时返回true，空列表返回false。
 */
class AnyOfPredicate : public BlockPredicate {
public:
    /**
     * @brief 构造谓词
     * @param predicates 子谓词列表
     */
    explicit AnyOfPredicate(std::vector<std::unique_ptr<BlockPredicate>> predicates)
        : m_predicates(std::move(predicates))
    {}

    [[nodiscard]] bool test(const IWorld& world, const BlockPos& pos) const override;
    [[nodiscard]] std::unique_ptr<BlockPredicate> clone() const override;

private:
    std::vector<std::unique_ptr<BlockPredicate>> m_predicates;
};

/**
 * @brief 条件取反谓词
 *
 * 对子谓词的结果取反。
 */
class NotPredicate : public BlockPredicate {
public:
    /**
     * @brief 构造谓词
     * @param predicate 被取反的子谓词
     */
    explicit NotPredicate(std::unique_ptr<BlockPredicate> predicate)
        : m_predicate(std::move(predicate))
    {}

    [[nodiscard]] bool test(const IWorld& world, const BlockPos& pos) const override;
    [[nodiscard]] std::unique_ptr<BlockPredicate> clone() const override;

private:
    std::unique_ptr<BlockPredicate> m_predicate;
};

} // namespace mc::world::gen::feature::predicate
