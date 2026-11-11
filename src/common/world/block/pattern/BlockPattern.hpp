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

#include "BlockInWorld.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace mc::blockpattern {

// 前置声明：BlockPatternMatch::getBlock() 需要调用 BlockPattern::translateAndRotate，
// 且匹配结果持有匹配期间的共享缓存。
class BlockPattern;
class BlockPatternCache;

/**
 * @brief 3D 方块模式匹配结果
 *
 * 持有匹配起始位置（frontTopLeft）、前向（forwards）、上方向（up）以及匹配期间的
 * 共享缓存。通过 getBlock(width, height, depth) 获取模式内任意位置的 BlockInWorld；
 * 返回的是匹配时刻的缓存快照，匹配后世界变化不会影响已取得的 BlockInWorld。
 */
class BlockPatternMatch {
public:
    BlockPatternMatch(std::shared_ptr<BlockPatternCache> cache,
        BlockPos frontTopLeft,
        Direction forwards,
        Direction up,
        i32 width,
        i32 height,
        i32 depth)
        : m_cache(std::move(cache))
        , m_frontTopLeft(frontTopLeft)
        , m_forwards(forwards)
        , m_up(up)
        , m_width(width)
        , m_height(height)
        , m_depth(depth)
    {}

    [[nodiscard]] BlockPos frontTopLeft() const
    {
        return m_frontTopLeft;
    }

    [[nodiscard]] Direction forwards() const
    {
        return m_forwards;
    }

    [[nodiscard]] Direction up() const
    {
        return m_up;
    }

    [[nodiscard]] i32 width() const
    {
        return m_width;
    }

    [[nodiscard]] i32 height() const
    {
        return m_height;
    }

    [[nodiscard]] i32 depth() const
    {
        return m_depth;
    }

    /**
     * @brief 获取模式内指定位置的方块引用
     *
     * @param widthIdx 宽度方向索引 [0, width)
     * @param heightIdx 高度方向索引 [0, height)
     * @param depthIdx 深度方向索引 [0, depth)
     * @return 对应世界位置的 BlockInWorld（存活于匹配缓存中，匹配后世界变化不影响其状态）
     */
    [[nodiscard]] const BlockInWorld& getBlock(i32 widthIdx, i32 heightIdx, i32 depthIdx) const;

private:
    std::shared_ptr<BlockPatternCache> m_cache;
    BlockPos m_frontTopLeft;
    Direction m_forwards;
    Direction m_up;
    i32 m_width;
    i32 m_height;
    i32 m_depth;
};

/**
 * @brief 3D 方块模式
 *
 * pattern[depth][height][width] 三维谓词数组，find() 在世界中的指定位置附近
 * 搜索匹配的模式实例。模式匹配通过 6 个方向组合（forwards × up，正交对）实现旋转。
 *
 * 典型用法：
 * @code
 * auto pattern = BlockPatternBuilder::start()
 *     .aisle("       ", "       ", ...)
 *     .where('#', BlockInWorld::hasState([](const BlockState& s){ return s.is(VanillaBlocks::BEDROCK); }))
 *     .build();
 * auto match = pattern->find(world, BlockPos(0, 64, 0));
 * if (match.has_value()) {
 *     BlockPos center = match->getBlock(3, 3, 3).pos();
 * }
 * @endcode
 */
class BlockPattern {
public:
    /// 谓词类型：接受 BlockInWorld 引用，返回是否匹配
    using Predicate = std::function<bool(const BlockInWorld&)>;

    /**
     * @brief 构造方块模式
     *
     * @param pattern 三维谓词数组，索引顺序为 [depth][height][width]
     */
    explicit BlockPattern(std::vector<std::vector<std::vector<Predicate>>> pattern);

    [[nodiscard]] i32 depth() const
    {
        return m_depth;
    }

    [[nodiscard]] i32 height() const
    {
        return m_height;
    }

    [[nodiscard]] i32 width() const
    {
        return m_width;
    }

    /**
     * @brief 在指定位置附近搜索模式匹配
     *
     * 搜索范围为 [pos, pos + (max-1, max-1, max-1)]，其中 max = max(width, height, depth)。
     * 对每个候选起始位置，尝试所有正交的 (forwards, up) 方向组合（6×4=24 种，排除平行/相反），
     * 找到第一个完整匹配的组合后返回。
     *
     * @param world 世界引用
     * @param pos 搜索起始位置（通常为疑似模式位置）
     * @return 匹配结果（nullopt 表示未找到）
     */
    [[nodiscard]] std::optional<BlockPatternMatch> find(IWorld& world, const BlockPos& pos) const;

    /**
     * @brief 在指定位置和方向上尝试匹配
     *
     * @param world 世界引用
     * @param pos 起始位置
     * @param forwards 前向方向
     * @param up 上方向（必须与 forwards 正交）
     * @return 匹配结果（nullopt 表示不匹配）
     */
    [[nodiscard]] std::optional<BlockPatternMatch> matches(
        IWorld& world, const BlockPos& pos, Direction forwards, Direction up) const;

    /**
     * @brief 将模式坐标转换为世界坐标
     *
     * 算法：
     * - forwards 与 up 必须正交（非平行、非相反）
     * - vec3i = forwards 的单位向量
     * - vec3i1 = up 的单位向量
     * - vec3i2 = vec3i × vec3i1（右手叉积，得到侧向）
     * - 最终位置 = origin + vec3i1 * (-heightIdx) + vec3i2 * widthIdx + vec3i * depthIdx
     *
     * 叉积公式：
     *   cross(a, b) = (a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x)
     *
     * @param origin 模式起始位置（frontTopLeft）
     * @param forwards 前向方向
     * @param up 上方向
     * @param widthIdx 宽度索引
     * @param heightIdx 高度索引
     * @param depthIdx 深度索引
     * @return 世界坐标
     */
    [[nodiscard]] static BlockPos translateAndRotate(
        BlockPos origin, Direction forwards, Direction up, i32 widthIdx, i32 heightIdx, i32 depthIdx);

private:
    /**
     * @brief 在已创建的缓存上尝试匹配
     *
     * @param cache 匹配期间共享的方块缓存
     * @param pos 起始位置
     * @param forwards 前向方向
     * @param up 上方向（必须与 forwards 正交）
     * @return 匹配结果（nullopt 表示不匹配）
     */
    [[nodiscard]] std::optional<BlockPatternMatch> _matches(
        const std::shared_ptr<BlockPatternCache>& cache, const BlockPos& pos, Direction forwards, Direction up) const;

    std::vector<std::vector<std::vector<Predicate>>> m_pattern;
    i32 m_depth;
    i32 m_height;
    i32 m_width;
};

} // namespace mc::blockpattern
