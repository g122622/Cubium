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

#include "common/core/Types.hpp"

namespace mc::world::chunk {

class ChunkData;

/**
 * @brief 区块管理器接口
 *
 * `IWorld::chunkManager()` 把它暴露给 common 层的游戏逻辑（方块、实体、方块实体、
 * 寻路、进度条件等），使这些代码可以直接向区块子系统索取区块数据，而无需 `IWorld`
 * 为每种查询各转发一个方法。
 *
 * 服务端由 `ServerChunkManager` 实现。不持有区块存储的世界实现（客户端只读适配器、
 * 测试桩世界等）沿用 `IWorld::chunkManager()` 的默认返回值 `EmptyChunkManager`，
 * 其语义与"所有区块均未加载"完全一致。
 */
class IChunkManager {
public:
    virtual ~IChunkManager() = default;

    /**
     * @brief 获取当前已缓存的区块
     *
     * 只查询内存缓存，不触发存档解析或异步生成。
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return 内存中的区块指针；若不存在则返回 nullptr
     */
    [[nodiscard]] virtual ChunkData* tryToGetChunkInMem(ChunkCoord x, ChunkCoord z) = 0;

    /**
     * @brief 获取当前已缓存的区块（const 版本）
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return 内存中的区块指针；若不存在则返回 nullptr
     */
    [[nodiscard]] virtual const ChunkData* tryToGetChunkInMem(ChunkCoord x, ChunkCoord z) const = 0;

    /**
     * @brief 判断区块是否已存在于内存缓存中
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return 若该区块已在内存缓存中则返回 true
     */
    [[nodiscard]] virtual bool hasChunkInMem(ChunkCoord x, ChunkCoord z) const = 0;

    /**
     * @brief 以同步方式请求 FULL 区块
     *
     * 区块已在内存中则直接返回，否则同步触发加载/生成。
     * 与 `ServerChunkManager::requestFullChunkSync` 相同，仅在服务端主线程调用安全。
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return 成功时返回区块指针；失败时返回 nullptr
     */
    [[nodiscard]] virtual ChunkData* requestFullChunkSync(ChunkCoord x, ChunkCoord z) = 0;
};

/**
 * @brief 空区块管理器（null object）
 *
 * 作为 `IWorld::chunkManager()` 的默认返回值，供不持有区块存储的世界实现使用：
 * 所有查询都报告"未加载"，所有同步加载请求都失败。
 * 这正是区块访问方法并入 `IWorld` 之前，这些实现（返回 nullptr / false 的桩）
 * 所表达的语义。
 */
class EmptyChunkManager final : public IChunkManager {
public:
    [[nodiscard]] static EmptyChunkManager& instance() noexcept
    {
        static EmptyChunkManager s_instance;
        return s_instance;
    }

    [[nodiscard]] ChunkData* tryToGetChunkInMem(ChunkCoord /*x*/, ChunkCoord /*z*/) override { return nullptr; }

    [[nodiscard]] const ChunkData* tryToGetChunkInMem(ChunkCoord /*x*/, ChunkCoord /*z*/) const override
    {
        return nullptr;
    }

    [[nodiscard]] bool hasChunkInMem(ChunkCoord /*x*/, ChunkCoord /*z*/) const override { return false; }

    [[nodiscard]] ChunkData* requestFullChunkSync(ChunkCoord /*x*/, ChunkCoord /*z*/) override { return nullptr; }

private:
    EmptyChunkManager() = default;
};

} // namespace mc::world::chunk
