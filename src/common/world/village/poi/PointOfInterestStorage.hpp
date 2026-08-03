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

#include "../../../core/Types.hpp"
#include "../../../world/block/BlockPos.hpp"
#include "PointOfInterest.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/village/poi/PointOfInterestType.hpp"
#include <cstddef>
#include <functional>
#include <list>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mc {
namespace nbt {
namespace tags {
struct compound_tag;
}
} // namespace nbt

namespace world {
namespace village {
namespace poi {

/**
 * @brief POI搜索条件
 *
 * 用于过滤POI搜索结果
 */
struct POISearchCriteria {
    /// 是否要求未被占用
    bool requireUnoccupied = false;
    /// 排除的占用者ID（用于排除自己已占用的POI）
    std::optional<u64> excludeOwner;
    /// 最大搜索距离（0表示无限制）
    f32 maxDistance = 0.0f;
    /// 最小搜索距离
    f32 minDistance = 0.0f;
    /// 要求的POI类型（可选，为空则接受所有类型）
    std::optional<PointOfInterestType> type;
    /// 类型列表（如果提供，则匹配列表中的任意类型）
    std::vector<PointOfInterestType> types;
};

/**
 * @brief POI搜索结果
 */
struct POISearchResult {
    /// 找到的POI
    const PointOfInterest* poi = nullptr;
    /// 距离搜索中心的距离
    f32 distance = 0.0f;
    /// 是否有效
    [[nodiscard]] bool isValid() const { return poi != nullptr; }
};

/**
 * @brief POI存储类
 *
 * 管理世界中所有POI的注册、查询和占用。
 * 使用区块级别索引实现高效的空间查询。
 *
 * 线程安全：所有公共方法都是线程安全的。
 */
class PointOfInterestStorage {
public:
    /**
     * @brief 默认构造函数
     */
    PointOfInterestStorage() = default;

    // ========== POI注册 ==========

    /**
     * @brief 注册一个新的POI
     * @param pos 方块位置
     * @param type POI类型
     * @return 是否成功注册（如果位置已有POI则返回false）
     */
    bool registerPOI(BlockPos pos, PointOfInterestType type);

    /**
     * @brief 注销一个POI
     * @param pos 方块位置
     * @return 是否成功注销
     */
    bool unregisterPOI(BlockPos pos);

    /**
     * @brief 更新POI类型
     * @param pos 方块位置
     * @param newType 新类型
     * @return 是否成功更新
     */
    bool updatePOI(BlockPos pos, PointOfInterestType newType);

    /**
     * @brief 检查位置是否有POI
     * @param pos 方块位置
     * @return 是否存在POI
     */
    [[nodiscard]] bool hasPOI(BlockPos pos) const;

    /**
     * @brief 获取指定位置的POI
     * @param pos 方块位置
     * @return POI指针（如果不存在返回nullptr）
     */
    [[nodiscard]] const PointOfInterest* getPOI(BlockPos pos) const;

    // ========== 占用管理 ==========

    /**
     * @brief 占用POI
     * @param pos POI位置
     * @param ownerId 占用者ID
     * @param gameTime 当前游戏时间
     * @return 是否成功占用
     */
    bool acquirePOI(BlockPos pos, u64 ownerId, i64 gameTime);

    /**
     * @brief 释放POI占用
     * @param pos POI位置
     * @param ownerId 占用者ID
     * @return 是否成功释放
     */
    bool releasePOI(BlockPos pos, u64 ownerId);

    /**
     * @brief 释放指定所有者的所有POI
     * @param ownerId 占用者ID
     * @return 释放的POI数量
     */
    i32 releaseAllByOwner(u64 ownerId);

    // ========== 空间查询 ==========

    /**
     * @brief 查找最近的POI
     * @param center 搜索中心
     * @param criteria 搜索条件
     * @return 搜索结果
     */
    [[nodiscard]] POISearchResult findNearest(BlockPos center, const POISearchCriteria& criteria = {}) const;

    /**
     * @brief 查找最近的可占用POI
     * @param center 搜索中心
     * @param type POI类型
     * @param maxDistance 最大距离
     * @return POI位置（如果找到）
     */
    [[nodiscard]] std::optional<BlockPos> findNearestFree(
        BlockPos center, PointOfInterestType type, f32 maxDistance) const;

    /**
     * @brief 查找指定类型最近的POI（无论是否占用）
     * @param center 搜索中心
     * @param type POI类型
     * @param maxDistance 最大距离
     * @return POI位置（如果找到）
     */
    [[nodiscard]] std::optional<BlockPos> findNearest(BlockPos center, PointOfInterestType type, f32 maxDistance) const;

    /**
     * @brief 查找指定类型最近的未被指定实体占用的POI
     * @param center 搜索中心
     * @param type POI类型
     * @param maxDistance 最大距离
     * @param excludeOwnerId 排除的占用者ID
     * @return POI位置（如果找到）
     */
    [[nodiscard]] std::optional<BlockPos> findNearestUnacquired(
        BlockPos center, PointOfInterestType type, f32 maxDistance, u64 excludeOwnerId) const;

    /**
     * @brief 查找区块内的所有POI
     * @param chunkPos 区块坐标
     * @param typeFilter 类型过滤器（可选）
     * @return POI列表
     */
    [[nodiscard]] std::vector<const PointOfInterest*> findAllInChunk(
        ChunkCoord chunkX, ChunkCoord chunkZ, std::optional<PointOfInterestType> typeFilter = std::nullopt) const;

    /**
     * @brief 查找范围内的所有POI
     * @param center 中心位置
     * @param radius 半径
     * @param typeFilter 类型过滤器（可选）
     * @return POI列表
     */
    [[nodiscard]] std::vector<const PointOfInterest*> findAllInRange(
        BlockPos center, f32 radius, std::optional<PointOfInterestType> typeFilter = std::nullopt) const;

    /**
     * @brief 查找指定类型的所有POI
     * @param type POI类型
     * @return POI列表
     */
    [[nodiscard]] std::vector<const PointOfInterest*> findAllByType(PointOfInterestType type) const;

    // ========== 区块回调 ==========

    /**
     * @brief 当区块加载时调用
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     */
    void onChunkLoaded(ChunkCoord chunkX, ChunkCoord chunkZ);

    /**
     * @brief 当区块卸载时调用
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     */
    void onChunkUnloaded(ChunkCoord chunkX, ChunkCoord chunkZ);

    // ========== 统计 ==========

    /**
     * @brief 获取POI总数
     */
    [[nodiscard]] size_t getTotalCount() const;

    /**
     * @brief 获取指定类型的POI数量
     */
    [[nodiscard]] size_t getCountByType(PointOfInterestType type) const;

    /**
     * @brief 获取已加载的区块数
     */
    [[nodiscard]] size_t getLoadedChunkCount() const;

    // ========== 序列化 ==========

    /**
     * @brief 序列化到NBT
     * @param tag NBT标签
     */
    void serialize(nbt::tags::compound_tag& tag) const;

    /**
     * @brief 从NBT反序列化
     * @param tag NBT标签
     */
    void deserialize(const nbt::tags::compound_tag& tag);

private:
    /**
     * @brief 计算方块位置所属的区块键
     */
    [[nodiscard]] static u64 _getChunkKey(BlockPos pos);
    [[nodiscard]] static u64 _getChunkKey(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 计算两点之间的距离
     */
    [[nodiscard]] static f32 _distance(BlockPos a, BlockPos b);

    /**
     * @brief 检查POI是否匹配搜索条件
     */
    [[nodiscard]] bool _matchesCriteria(const PointOfInterest& poi, const POISearchCriteria& criteria) const;

    /**
     * @brief 获取或创建区块POI列表
     */
    std::list<PointOfInterest>& _getOrCreateChunkPOIs(u64 chunkKey);
    [[nodiscard]] const std::list<PointOfInterest>* _getChunkPOIs(u64 chunkKey) const;

private:
    /// 区块级别POI存储（使用list避免指针失效）
    std::unordered_map<u64, std::list<PointOfInterest>> m_chunkPOIs;

    /// 位置快速索引
    std::unordered_map<BlockPos, PointOfInterest*, std::hash<BlockPos>> m_byPosition;

    /// 类型索引
    std::unordered_map<PointOfInterestType, std::vector<PointOfInterest*>> m_byType;

    /// 已加载区块集合
    std::unordered_set<u64> m_loadedChunks;

    /// 线程安全锁
    mutable std::mutex m_mutex;
};

} // namespace poi
} // namespace village
} // namespace world
} // namespace mc
