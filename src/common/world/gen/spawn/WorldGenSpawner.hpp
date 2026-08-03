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
#include "../../../util/math/Vector3.hpp"
#include "../../../util/math/random/IRandom.hpp"
#include "../../../world/spawn/EntitySpawnPlacementRegistry.hpp"
#include "../../biome/Biome.hpp"
#include "../../spawn/MobSpawnInfo.hpp"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {

// 前向声明
class WorldGenRegion;
class IChunkGenerator;
class Entity;
class MobEntity;

namespace entity {
class EntityType;
struct ILivingEntityData;
} // namespace entity

/**
 * @brief 生成的实体数据
 *
 * 用于在区块生成时记录应该生成的实体信息，
 * 后续由 ServerWorld 创建实际的实体对象。
 */
struct SpawnedEntityData {
    /// 实体类型ID（如 "minecraft:pig"）
    std::string entityTypeId;

    /// 生成位置 X
    f32 x = 0.0f;

    /// 生成位置 Y
    f32 y = 0.0f;

    /// 生成位置 Z
    f32 z = 0.0f;

    /// 生成原因（默认为区块生成）
    world::spawn::SpawnReason spawnReason = world::spawn::SpawnReason::ChunkGeneration;

    SpawnedEntityData() = default;

    SpawnedEntityData(std::string typeId,
        f32 px,
        f32 py,
        f32 pz,
        world::spawn::SpawnReason reason = world::spawn::SpawnReason::ChunkGeneration)
        : entityTypeId(std::move(typeId))
        , x(px)
        , y(py)
        , z(pz)
        , spawnReason(reason)
    {}
};

/**
 * @brief 区块生成时的生物放置器
 *
 * 在区块首次生成时放置被动动物（猪、牛、羊等）。
 *
 * 与 NaturalSpawner 的区别：
 * - WorldGenSpawner: 区块生成时放置动物（仅 Creature 分类）
 * - NaturalSpawner: 运行时自然生成（怪物、动物、环境生物等）
 *
 * 使用方式：
 * @code
 * WorldGenSpawner spawner;
 * std::vector<SpawnedEntityData> entities;
 * spawner.spawnInitialMobs(region, biome, chunkX, chunkZ, generator, random, entities);
 * // 之后由 ServerWorld 创建实体
 * @endcode
 */
class WorldGenSpawner {
public:
    WorldGenSpawner();
    ~WorldGenSpawner();

    /**
     * @brief 在区块生成时放置动物
     *
     * 只放置 Creature 分类（被动动物），不生成怪物。
     * 怪物通过 NaturalSpawner 在夜间/黑暗环境生成。
     *
     * @param region 世界生成区域
     * @param biome 区块中心的主要生物群系
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @param generator 区块生成器（用于获取高度）
     * @param random 随机数生成器
     * @param outEntities 输出：生成的实体数据列表
     * @return 实际生成的实体数量
     */
    i32 spawnInitialMobs(WorldGenRegion& region,
        const Biome& biome,
        i32 chunkX,
        i32 chunkZ,
        IChunkGenerator& generator,
        math::IRandom& random,
        std::vector<SpawnedEntityData>& outEntities);

    /**
     * @brief 设置是否启用生成
     */
    void setEnabled(bool enabled) { m_enabled = enabled; }

    /**
     * @brief 检查是否启用
     */
    [[nodiscard]] bool isEnabled() const { return m_enabled; }

private:
    bool m_enabled = true;

    /**
     * @brief 尝试在指定位置生成一组实体
     *
     * @param region 世界生成区域
     * @param entityType 实体类型
     * @param x X 坐标
     * @param y Y 坐标
     * @param z Z 坐标
     * @param count 生成数量
     * @param random 随机数生成器
     * @param outEntities 输出：生成的实体数据
     * @return 实际生成的数量
     */
    i32 _spawnGroup(WorldGenRegion& region,
        const entity::EntityType& entityType,
        f32 x,
        f32 y,
        f32 z,
        i32 count,
        math::IRandom& random,
        std::vector<SpawnedEntityData>& outEntities);

    /**
     * @brief 获取实体类型的生成高度
     *
     * @param region 世界生成区域
     * @param entityType 实体类型
     * @param x X 坐标
     * @param z Z 坐标
     * @return 生成高度，如果无法生成返回 -1
     */
    [[nodiscard]] i32 _getSpawnHeight(WorldGenRegion& region, const entity::EntityType& entityType, i32 x, i32 z) const;

    /**
     * @brief 检查位置是否可以生成实体
     *
     * @param region 世界生成区域
     * @param entityType 实体类型
     * @param x X 坐标
     * @param y Y 坐标
     * @param z Z 坐标
     * @return 是否可以生成
     */
    [[nodiscard]] bool _canSpawnAt(
        WorldGenRegion& region, const entity::EntityType& entityType, i32 x, i32 y, i32 z) const;
};

} // namespace mc
