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
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/chunk/base/SectionPos.hpp"
#include "common/world/lighting/LightType.hpp"

namespace mc {
class WorldLightManager;
}

namespace mc::server {

// 前向声明
class ServerChunkManager;

namespace sync {

/**
 * @brief 光照同步管理器
 *
 * 负责将光照数据从 WorldLightManager 同步到 ChunkSection。
 * 网络发送由 MinecraftServer 通过 ServerWorld::setOnLightChanged 回调处理。
 */
class LightSyncManager {
public:
    /**
     * @brief 构造函数
     * @param lightManager 光照管理器引用
     * @param chunkManager 区块管理器引用
     */
    LightSyncManager(WorldLightManager& lightManager, ServerChunkManager& chunkManager) noexcept;

    /**
     * @brief 区块加载后初始化光照
     * @param x 区块X坐标
     * @param z 区块Z坐标
     */
    void initializeChunkLighting(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 方块变化时触发光照检查
     * @param x 方块X坐标
     * @param y 方块Y坐标
     * @param z 方块Z坐标
     * @param oldLightLevel 旧光照等级
     * @param newLightLevel 新光照等级
     */
    void onBlockStateChanged(i32 x, i32 y, i32 z, i32 oldLightLevel, i32 newLightLevel);

    /**
     * @brief 标记光照变化，同步数据到 ChunkSection
     * @param type 光照类型
     * @param pos 区块段位置
     */
    void markLightChanged(LightType type, const SectionPos& pos);

    /**
     * @brief 同步光照数据到 ChunkSection
     * @param type 光照类型
     * @param pos 区块段位置
     */
    void syncLightDataToChunk(LightType type, const SectionPos& pos);

private:
    WorldLightManager& m_lightManager;
    ServerChunkManager& m_chunkManager;
};

} // namespace sync
} // namespace mc::server