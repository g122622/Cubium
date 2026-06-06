/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies of substantial portions of the Software.
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

#include "common/entity/core/Entity.hpp"
#include <memory>

namespace mc {
namespace entity {

/**
 * @brief 不祥物品生成器实体
 *
 * 不祥试炼刷怪笼激活时在玩家上方生成。
 * 延迟60-120 tick后在目标位置投掷物品/弹射物。
 * 投掷物从 trial_items_to_drop_when_ominous 战利品表选取。
 *
 * 行为：
 * 1. 从不祥试炼刷怪笼生成，出现在目标玩家上方
 * 2. 等待随机延迟（60-120 ticks）
 * 3. 从战利品表随机选取物品并投掷到目标位置
 * 4. 投掷完成后消失
 *
 * 命名空间ID: minecraft:ominous_item_spawner
 */
class OminousItemSpawnerEntity final : public Entity {
public:
    /// 最小投掷延迟（ticks）
    static constexpr i32 MIN_SPAWN_DELAY = 60;

    /// 最大投掷延迟（ticks）
    static constexpr i32 MAX_SPAWN_DELAY = 120;

    /**
     * @brief 构造不祥物品生成器
     * @param id 实体ID
     */
    explicit OminousItemSpawnerEntity(EntityId id);

    ~OminousItemSpawnerEntity() override = default;

    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 实体尺寸 ==========

    f32 width() const override { return 0.25f; }
    f32 height() const override { return 0.25f; }

    // ========== 生命周期 ==========

    void tick() override;

    /**
     * @brief 设置目标玩家UUID
     */
    void setTargetPlayer(const std::string& playerUuid);

    /**
     * @brief 设置生成延迟
     * @param delay 延迟ticks（60-120）
     */
    void setSpawnDelay(i32 delay);

private:
    /**
     * @brief 从战利品表选取物品并投掷
     */
    void spawnItem();

    /// 目标玩家UUID
    std::string m_targetPlayerUuid;

    /// 剩余延迟ticks
    i32 m_spawnDelay = 0;

    /// 是否已完成投掷
    bool m_hasSpawned = false;
};

} // namespace entity
} // namespace mc
