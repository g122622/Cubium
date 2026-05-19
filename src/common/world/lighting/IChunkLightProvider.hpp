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

#include "../../core/Types.hpp"
#include "../block/BlockPos.hpp"
#include "../chunk/ChunkPos.hpp"
#include "LightType.hpp"

namespace mc {

// 前向声明
class IChunk;
class IWorld;
class BlockState;

/**
 * @brief 区块光照提供者接口
 *
 * 为光照引擎提供区块访问和世界信息。
 * ServerWorld 和 ClientWorld 实现此接口。
 *
 * 参考: net.minecraft.world.chunk.IChunkLightProvider
 */
class StarLightLightingProvider {
public:
    virtual ~StarLightLightingProvider() = default;

    // ========================================================================
    // 区块访问
    // ========================================================================

    /**
     * @brief 获取用于光照计算的区块
     *
     * @param x 区块X坐标
     * @param z 区块Z坐标
     * @return 区块指针，如果未加载返回nullptr
     */
    [[nodiscard]] virtual IChunk* getChunkForLight(ChunkCoord x, ChunkCoord z) = 0;
    [[nodiscard]] virtual const IChunk* getChunkForLight(ChunkCoord x, ChunkCoord z) const = 0;

    /**
     * @brief 获取方块状态（用于光照计算）
     *
     * @param pos 方块位置
     * @return 方块状态指针，如果超出范围返回空气
     */
    [[nodiscard]] virtual const BlockState* getBlockStateForLight(const BlockPos& pos) const = 0;

    // ========================================================================
    // 世界信息
    // ========================================================================

    /**
     * @brief 获取世界接口
     */
    [[nodiscard]] virtual IWorld* getWorld() = 0;
    [[nodiscard]] virtual const IWorld* getWorld() const = 0;

    // ========================================================================
    // 光照通知
    // ========================================================================

    /**
     * @brief 标记光照已更改
     *
     * 当区块段的光照数据发生变化时调用。
     * 用于通知客户端更新光照。
     *
     * @param type 光照类型
     * @param pos 区块段位置
     */
    virtual void markLightChanged(LightType type, const SectionPos& pos) = 0;

    // ========================================================================
    // 维度信息
    // ========================================================================

    /**
     * @brief 是否有天空光照
     *
     * 主世界返回true，下界和末地返回false。
     */
    [[nodiscard]] virtual bool hasSkyLight() const = 0;

    /**
     * @brief 获取最小建筑高度
     */
    [[nodiscard]] virtual i32 getMinBuildHeight() const = 0;

    /**
     * @brief 获取最大建筑高度
     */
    [[nodiscard]] virtual i32 getMaxBuildHeight() const = 0;

    /**
     * @brief 获取区块段数量（高度）
     */
    [[nodiscard]] virtual i32 getSectionCount() const = 0;
};

} // namespace mc
