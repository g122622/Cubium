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

#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include <memory>
#include <string>
#include <vector>

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

/**
 * @brief 总是返回true的谓词
 */
class TrueBlockPredicate : public BlockPredicate {
public:
    [[nodiscard]] bool test(const IWorld& /*world*/, const BlockPos& /*pos*/) const override { return true; }
    [[nodiscard]] std::unique_ptr<BlockPredicate> clone() const override
    {
        return std::make_unique<TrueBlockPredicate>();
    }
};

/**
 * @brief 总是返回false的谓词
 */
class FalseBlockPredicate : public BlockPredicate {
public:
    [[nodiscard]] bool test(const IWorld& /*world*/, const BlockPos& /*pos*/) const override { return false; }
    [[nodiscard]] std::unique_ptr<BlockPredicate> clone() const override
    {
        return std::make_unique<FalseBlockPredicate>();
    }
};

/**
 * @brief 检查位置是否为空气的谓词
 *
 * 当方块状态为空气时返回true。
 */
class OnlyInAirPredicate : public BlockPredicate {
public:
    [[nodiscard]] bool test(const IWorld& world, const BlockPos& pos) const override;
    [[nodiscard]] std::unique_ptr<BlockPredicate> clone() const override
    {
        return std::make_unique<OnlyInAirPredicate>();
    }
};

/**
 * @brief 检查方块是否为实心的谓词
 */
class SolidBlockPredicate : public BlockPredicate {
public:
    [[nodiscard]] bool test(const IWorld& world, const BlockPos& pos) const override;
    [[nodiscard]] std::unique_ptr<BlockPredicate> clone() const override
    {
        return std::make_unique<SolidBlockPredicate>();
    }
};

/**
 * @brief 检查相邻方块是否有指定方向的坚固面的谓词
 *
 * 扫描指定位置，判断某个方向上是否有实心方块的坚固面。
 * 使用 isSolidSide 判断方块的指定面是否坚固，检查方向取反
 * （即检查 pos 处方块在 direction 方向的面，需要查询该面相邻方块的反方向）。
 */
class HasSturdyFacePredicate : public BlockPredicate {
public:
    /**
     * @brief 构造谓词
     * @param direction 检查的方向（检查pos处方块在direction方向的面）
     * @param offset 相对偏移量（默认为零偏移）
     */
    explicit HasSturdyFacePredicate(Direction direction, BlockPos offset = BlockPos())
        : m_direction(direction)
        , m_offset(offset)
    {}

    [[nodiscard]] bool test(const IWorld& world, const BlockPos& pos) const override;
    [[nodiscard]] std::unique_ptr<BlockPredicate> clone() const override
    {
        return std::make_unique<HasSturdyFacePredicate>(m_direction, m_offset);
    }

    [[nodiscard]] Direction getDirection() const { return m_direction; }
    [[nodiscard]] const BlockPos& getOffset() const { return m_offset; }

private:
    Direction m_direction;
    BlockPos m_offset;
};

/**
 * @brief 检查方块是否匹配指定方块的谓词
 */
class MatchingBlockPredicate : public BlockPredicate {
public:
    explicit MatchingBlockPredicate(const Block* block)
        : m_block(block)
    {}

    [[nodiscard]] bool test(const IWorld& world, const BlockPos& pos) const override;
    [[nodiscard]] std::unique_ptr<BlockPredicate> clone() const override
    {
        return std::make_unique<MatchingBlockPredicate>(m_block);
    }

private:
    const Block* m_block;
};

/**
 * @brief 检查方块是否属于指定标签的谓词
 */
class TagMatchPredicate : public BlockPredicate {
public:
    explicit TagMatchPredicate(const std::string& tagName)
        : m_tagName(tagName)
    {}

    [[nodiscard]] bool test(const IWorld& world, const BlockPos& pos) const override;
    [[nodiscard]] std::unique_ptr<BlockPredicate> clone() const override
    {
        return std::make_unique<TagMatchPredicate>(m_tagName);
    }

    [[nodiscard]] const std::string& getTagName() const { return m_tagName; }

private:
    std::string m_tagName;
};

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

/**
 * @brief 检查方块是否可替换的谓词
 *
 * 当方块为空气或材质标记为可替换时返回true。
 */
class ReplaceablePredicate : public BlockPredicate {
public:
    [[nodiscard]] bool test(const IWorld& world, const BlockPos& pos) const override;
    [[nodiscard]] std::unique_ptr<BlockPredicate> clone() const override
    {
        return std::make_unique<ReplaceablePredicate>();
    }
};

/**
 * @brief 检查方块能否在指定位置存活的谓词
 *
 * 调用 Block::isValidPosition 判断方块是否可以放置在目标位置。
 */
class WouldSurvivePredicate : public BlockPredicate {
public:
    /**
     * @brief 构造谓词
     * @param state 要检查的方块状态
     * @param offset 相对偏移量（默认为零偏移）
     */
    WouldSurvivePredicate(const BlockState* state, BlockPos offset = BlockPos())
        : m_state(state)
        , m_offset(offset)
    {}

    [[nodiscard]] bool test(const IWorld& world, const BlockPos& pos) const override;
    [[nodiscard]] std::unique_ptr<BlockPredicate> clone() const override
    {
        return std::make_unique<WouldSurvivePredicate>(m_state, m_offset);
    }

private:
    const BlockState* m_state;
    BlockPos m_offset;
};

/**
 * @brief 检查位置是否在世界高度范围内的谓词
 *
 * 当 pos.y 在 [MIN_BUILD_HEIGHT, MAX_BUILD_HEIGHT) 范围内时返回true。
 */
class InsideWorldBoundsPredicate : public BlockPredicate {
public:
    [[nodiscard]] bool test(const IWorld& world, const BlockPos& pos) const override;
    [[nodiscard]] std::unique_ptr<BlockPredicate> clone() const override
    {
        return std::make_unique<InsideWorldBoundsPredicate>();
    }
};

/**
 * @brief 检查位置是否为空气或水的谓词
 *
 * 当方块状态为空、空气或液体时返回true。
 */
class OnlyInAirOrWaterPredicate : public BlockPredicate {
public:
    [[nodiscard]] bool test(const IWorld& world, const BlockPos& pos) const override;
    [[nodiscard]] std::unique_ptr<BlockPredicate> clone() const override
    {
        return std::make_unique<OnlyInAirOrWaterPredicate>();
    }
};

/**
 * @brief 环境扫描谓词
 *
 * 从起始位置沿指定方向扫描，寻找满足条件的方块面。
 * 用于洞穴植被放置中寻找天花板/地面。
 */
class EnvironmentScanPredicate {
public:
    /**
     * @brief 构造环境扫描谓词
     * @param direction 扫描方向
     * @param targetCondition 目标条件（找到此条件时停止）
     * @param abortCondition 终止条件（遇到此条件时停止，表示找不到）
     * @param maxSteps 最大扫描步数
     */
    EnvironmentScanPredicate(Direction direction,
        std::unique_ptr<BlockPredicate> targetCondition,
        std::unique_ptr<BlockPredicate> abortCondition,
        i32 maxSteps)
        : m_direction(direction)
        , m_targetCondition(std::move(targetCondition))
        , m_abortCondition(std::move(abortCondition))
        , m_maxSteps(maxSteps)
    {}

    /**
     * @brief 执行环境扫描
     * @param world 世界读取接口
     * @param startPos 起始位置（输出：扫描结果位置）
     * @return 是否找到满足条件的位置
     */
    [[nodiscard]] bool scan(const IWorld& world, BlockPos& startPos) const;

private:
    Direction m_direction;
    std::unique_ptr<BlockPredicate> m_targetCondition;
    std::unique_ptr<BlockPredicate> m_abortCondition;
    i32 m_maxSteps;
};

} // namespace mc::world::gen::feature::predicate
