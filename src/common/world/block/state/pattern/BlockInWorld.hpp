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
#include <functional>
#include <optional>

namespace mc {

// 前向声明
class BlockState;
class IWorld;
class BlockEntity;

namespace blockpattern {

/**
 * @brief 世界中方块的轻量级缓存包装器
 *
 * 对应 MC Java: net.minecraft.world.level.block.state.pattern.BlockInWorld
 *
 * 延迟加载方块状态和方块实体，避免在模式匹配预扫描阶段大量查询世界。
 * 区块未加载时 getState() 返回 nullopt（loadChunks=false 路径）。
 *
 * 设计说明：
 * - MC Java 的 hasState(Predicate<BlockState>) 返回 Predicate<BlockInWorld>，
 *   Cubium 由于不存在 BlockPredicate 层级，改为 hasState(std::function<bool(const BlockState&)>)
 *   并接受 const BlockState&。调用方通过 lambda 传入具体判断逻辑（如 is(block)）。
 */
class BlockInWorld {
public:
    /**
     * @brief 构造方块引用
     *
     * @param world 世界引用
     * @param pos 方块位置（按值拷贝，作为不可变缓存）
     * @param loadChunks 区块未加载时是否强制加载（true）或返回 nullopt（false）
     */
    BlockInWorld(IWorld& world, BlockPos pos, bool loadChunks)
        : m_world(world)
        , m_pos(pos)
        , m_loadChunks(loadChunks)
    {}

    /**
     * @brief 获取方块状态（延迟加载）
     *
     * - loadChunks=true：始终查询世界，返回实际方块状态
     * - loadChunks=false：仅当区块已加载时返回方块状态，否则返回 nullopt
     *
     * @return 方块状态指针；nullptr 表示区块未加载或方块为空气
     */
    [[nodiscard]] const BlockState* getState() const;

    /**
     * @brief 获取方块实体（延迟加载）
     *
     * @return 方块实体指针（nullptr 表示无方块实体）
     */
    [[nodiscard]] BlockEntity* getEntity() const;

    /**
     * @brief 获取世界引用
     */
    [[nodiscard]] IWorld& world() const { return m_world; }

    /**
     * @brief 获取方块位置
     */
    [[nodiscard]] const BlockPos& pos() const { return m_pos; }

    /**
     * @brief 创建状态谓词
     *
     * 对应 MC Java: BlockInWorld.hasState(Predicate<BlockState>)
     *
     * 返回一个接受 const BlockInWorld* 的谓词，当 BlockInWorld 非空
     * 且其方块状态满足 predicate 时返回 true。
     *
     * @param predicate 状态判断函数（通常为 `[](const BlockState& s){ return s.is(block); }`）
     * @return BlockInWorld 谓词
     */
    [[nodiscard]] static std::function<bool(const BlockInWorld&)> hasState(
        std::function<bool(const BlockState&)> predicate);

private:
    IWorld& m_world;
    BlockPos m_pos;
    bool m_loadChunks;

    // 延迟加载缓存
    mutable bool m_stateCached = false;
    mutable const BlockState* m_state = nullptr;
    mutable bool m_entityCached = false;
    mutable BlockEntity* m_entity = nullptr;
};

} // namespace blockpattern
} // namespace mc
