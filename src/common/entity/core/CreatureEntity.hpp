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

#include "MobEntity.hpp"

namespace mc {

/**
 * @brief 生物实体基类
 *
 * 可移动的生物实体基类，提供寻路能力。
 * 大多数被动生物和怪物继承此类。
 */
class CreatureEntity : public MobEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    CreatureEntity(EntityInstanceId id);

    ~CreatureEntity() override = default;

    // 禁止拷贝
    CreatureEntity(const CreatureEntity&) = delete;
    CreatureEntity& operator=(const CreatureEntity&) = delete;

    // 禁止移动（基类 MobEntity 不可移动）
    CreatureEntity(CreatureEntity&&) = delete;
    CreatureEntity& operator=(CreatureEntity&&) = delete;

    // ========== 移动 ==========

    /**
     * @brief 尝试移动到目标位置
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param speed 移动速度倍率
     * @return 是否成功开始移动
     */
    bool tryMoveTo(f64 x, f64 y, f64 z, f64 speed);

    /**
     * @brief 获取移动速度倍率
     */
    [[nodiscard]] f64 moveSpeed() const { return m_moveSpeed; }

    /**
     * @brief 设置移动速度倍率
     */
    void setMoveSpeed(f64 speed) { m_moveSpeed = speed; }

    // ========== 寻路权重 ==========

    /**
     * @brief 获取路径权重（用于随机位置生成）
     *
     * 返回值决定实体偏好移动到该位置的程度：
     * - 正值越高：越偏好该位置
     * - 负值：避开该位置
     * - 0：中性
     *
     * @param x X 坐标
     * @param y Y 坐标
     * @param z Z 坐标
     * @return 路径权重值
     */
    [[nodiscard]] virtual f32 getPathWeight(f32 x, f32 y, f32 z) const;

    /**
     * @brief 获取路径权重（BlockPos 版本）
     *
     * @param pos 方块位置
     * @return 路径权重值
     */
    [[nodiscard]] f32 getPathWeight(const BlockPos& pos) const;

    /**
     * @brief 检查是否可以生成在该位置
     *
     * 基于 getPathWeight 的返回值判断：权重 >= 0 表示该位置适合生成。
     * 对应 MC PathfinderMob.checkSpawnRules。
     *
     * 已集成到以下生成路径：
     * - NaturalSpawner::_trySpawnAt：自然生成时在 finalizeSpawn 之前调用
     * - ServerWorld::spawnEntitiesFromChunkGeneration：区块生成时在 finalizeSpawn 之前调用
     *
     * 注意：WorldGenSpawner 在区块生成阶段只记录 SpawnedEntityData（无实体实例），
     * 因此实例级检查延迟到 ServerWorld 创建实体时执行。
     */
    [[nodiscard]] virtual bool canSpawnAt(f32 x, f32 y, f32 z) const;

protected:
    f64 m_moveSpeed = 1.0;
};

} // namespace mc
