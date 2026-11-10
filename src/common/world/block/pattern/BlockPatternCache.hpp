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
#include "common/world/block/base/types/BlockPos.hpp"

#include <unordered_map>

namespace mc {

class IWorld;

namespace blockpattern {

/**
 * @brief 一次模式匹配期间共享的方块引用缓存
 *
 * `find()` 在搜索开始时创建一个缓存，所有候选起点、所有 `(forwards, up)` 朝向组合
 * 共享它；`BlockPatternMatch` 持有同一个缓存，因此匹配成功后 `getBlock()` 读取的是
 * 匹配时刻的方块快照，而不是重新查询世界。
 *
 * 缓存按位置去重：同一个世界坐标在一次 `find()`/`matches()` 中最多查询一次世界。
 */
class BlockPatternCache {
public:
    /**
     * @brief 构造缓存
     *
     * @param world 世界引用
     * @param loadChunks 区块未加载时是否强制读取（false 时未加载区块视为不匹配）
     */
    BlockPatternCache(IWorld& world, bool loadChunks)
        : m_world(world)
        , m_loadChunks(loadChunks)
    {}

    /**
     * @brief 获取指定位置的方块引用
     *
     * 未命中时创建 `BlockInWorld` 并插入缓存；已命中时返回同一个对象。
     * 返回的引用在下一次插入（rehash）后仍然有效（unordered_map 节点稳定）。
     *
     * @param pos 世界坐标
     * @return 缓存中的方块引用
     */
    [[nodiscard]] const BlockInWorld& get(const BlockPos& pos);

private:
    IWorld& m_world;
    bool m_loadChunks;
    std::unordered_map<BlockPos, BlockInWorld> m_blocks;
};

} // namespace blockpattern
} // namespace mc
