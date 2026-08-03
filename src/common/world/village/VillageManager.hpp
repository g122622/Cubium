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

#include "Village.hpp"
#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "poi/PointOfInterestStorage.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace mc {
namespace nbt {
namespace tags {
struct compound_tag;
}
} // namespace nbt

// 前向声明
class IWorld;

namespace world {
namespace village {

/**
 * @brief 村庄管理器
 *
 * 管理世界中所有村庄的生命周期、查询和更新。
 * 负责：
 * - 检测新村庄的形成
 * - 维护村庄与区块的映射关系
 * - 处理村民加入/离开村庄
 * - 协调袭击事件
 */
class VillageManager {
public:
    /**
     * @brief 村庄ID类型
     */
    using VillageId = u64;

    /**
     * @brief 构造函数
     * @param world 世界接口引用
     */
    explicit VillageManager(IWorld& world);

    // ========== 村庄查询 ==========

    /**
     * @brief 获取位置所在的村庄
     * @param pos 方块位置
     * @return 村庄指针（如果不在任何村庄内返回nullptr）
     */
    [[nodiscard]] Village* getVillageAt(BlockPos pos);

    /**
     * @brief 获取位置所在的村庄（const版本）
     */
    [[nodiscard]] const Village* getVillageAt(BlockPos pos) const;

    /**
     * @brief 获取或创建村庄
     * @param pos 方块位置（如果附近有村庄则返回，否则创建新村庄）
     * @return 村庄指针
     */
    Village* getOrCreateVillage(BlockPos pos);

    /**
     * @brief 获取所有村庄
     */
    [[nodiscard]] const std::vector<std::unique_ptr<Village>>& getAllVillages() const { return m_villages; }

    /**
     * @brief 获取村庄数量
     */
    [[nodiscard]] size_t getVillageCount() const { return m_villages.size(); }

    /**
     * @brief 根据ID获取村庄
     */
    [[nodiscard]] Village* getVillageById(VillageId id);

    // ========== 村民管理 ==========

    /**
     * @brief 当村民加入村庄时调用
     * @param villagerId 村民实体ID
     * @param pos 村民位置
     */
    void onVillagerJoin(u64 villagerId, BlockPos pos);

    /**
     * @brief 当村民离开村庄时调用
     * @param villagerId 村民实体ID
     */
    void onVillagerLeave(u64 villagerId);

    /**
     * @brief 获取村民所属的村庄
     * @param villagerId 村民实体ID
     * @return 村庄指针（如果村民不属于任何村庄返回nullptr）
     */
    [[nodiscard]] Village* getVillageForVillager(u64 villagerId);

    /**
     * @brief 获取村庄内的所有村民
     * @param villageId 村庄ID
     */
    [[nodiscard]] std::vector<u64> getVillagersInVillage(VillageId villageId) const;

    // ========== POI管理 ==========

    /**
     * @brief 获取POI存储
     */
    [[nodiscard]] poi::PointOfInterestStorage& getPOIStorage() { return m_poiStorage; }
    [[nodiscard]] const poi::PointOfInterestStorage& getPOIStorage() const { return m_poiStorage; }

    /**
     * @brief 当方块放置时调用（可能创建新POI）
     * @param pos 方块位置
     * @param blockId 方块ID
     */
    void onBlockPlaced(BlockPos pos, u32 blockId);

    /**
     * @brief 当方块破坏时调用（可能移除POI）
     * @param pos 方块位置
     */
    void onBlockRemoved(BlockPos pos);

    // ========== 袭击管理 ==========

    /**
     * @brief 检查位置是否在袭击范围内
     */
    [[nodiscard]] bool isInRaidRange(BlockPos pos) const;

    /**
     * @brief 获取位置的活跃袭击（如果有）
     */
    [[nodiscard]] Village* getVillageUnderRaid(BlockPos pos);

    /**
     * @brief 检查玩家是否进入了村庄（用于袭击触发检测）
     * @param playerPos 玩家当前位置
     * @param prevPos 玩家之前位置（可选）
     * @return 如果玩家进入了一个村庄，返回该村庄指针；否则返回 nullptr
     */
    [[nodiscard]] Village* checkPlayerEnterVillage(BlockPos playerPos, BlockPos prevPos);

    // ========== Tick更新 ==========

    /**
     * @brief 每游戏tick更新
     * @param gameTime 当前游戏时间
     */
    void tick(i64 gameTime);

    // ========== 区块回调 ==========

    /**
     * @brief 当区块加载时调用
     */
    void onChunkLoaded(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 当区块卸载时调用
     */
    void onChunkUnloaded(ChunkCoord x, ChunkCoord z);

    // ========== 序列化 ==========

    /**
     * @brief 序列化到NBT
     */
    void serialize(nbt::tags::compound_tag& tag) const;

    /**
     * @brief 从NBT反序列化
     */
    void deserialize(const nbt::tags::compound_tag& tag);

    // ========== 事件回调 ==========

    /**
     * @brief 村庄创建回调类型
     */
    using VillageCreatedCallback = std::function<void(Village&)>;

    /**
     * @brief 设置村庄创建回调
     */
    void setVillageCreatedCallback(VillageCreatedCallback callback) { m_onVillageCreated = std::move(callback); }

private:
    /**
     * @brief 创建新村庄
     * @param center 村庄中心位置
     * @return 新村庄的指针
     */
    Village* _createVillage(BlockPos center);

    /**
     * @brief 删除空村庄
     */
    void _removeEmptyVillages();

    /**
     * @brief 更新村庄边界
     */
    void _updateVillageBounds();

    /**
     * @brief 计算区块键
     */
    [[nodiscard]] static u64 _getChunkKey(ChunkCoord x, ChunkCoord z);

private:
    /// 世界接口引用
    IWorld& m_world;

    /// 所有村庄
    std::vector<std::unique_ptr<Village>> m_villages;

    /// 村庄ID映射
    std::unordered_map<VillageId, Village*> m_villageById;

    /// 村民到村庄的映射
    std::unordered_map<u64, Village*> m_villagerToVillage;

    /// 区块到村庄的映射（用于快速查询）
    std::unordered_map<u64, std::unordered_set<VillageId>> m_chunkToVillages;

    /// POI存储
    poi::PointOfInterestStorage m_poiStorage;

    /// 下一个村庄ID
    VillageId m_nextVillageId = 1;

    /// 村庄创建回调
    VillageCreatedCallback m_onVillageCreated;
};

} // namespace village
} // namespace world
} // namespace mc
