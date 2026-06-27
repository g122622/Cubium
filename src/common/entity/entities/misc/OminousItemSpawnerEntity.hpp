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

#include "common/entity/core/Entity.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <memory>

namespace mc {
namespace entity {

/**
 * @brief 不祥物品生成器实体
 *
 * 不祥试炼刷怪笼激活时在玩家上方生成。
 * 延迟60-120 tick后在目标位置投掷物品/弹射物。
 *
 * 对应 MC Java: net.minecraft.world.entity.OminousItemSpawner
 *
 * 行为：
 * 1. 从不祥试炼刷怪笼生成，出现在目标玩家上方
 * 2. 等待随机延迟（60-120 ticks）
 * 3. 在生成前36 ticks播放警告音效
 * 4. 投掷存储的物品（弹射物向下发射，普通物品自然掉落）
 * 5. 投掷完成后消失
 *
 * 命名空间ID: minecraft:ominous_item_spawner
 */
class OminousItemSpawnerEntity final : public Entity {
public:
    /// 最小投掷延迟（ticks）
    static constexpr i32 SPAWN_ITEM_DELAY_MIN = 60;

    /// 最大投掷延迟（ticks）
    static constexpr i32 SPAWN_ITEM_DELAY_MAX = 120;

    /// 生成前警告音效提前量（ticks）
    static constexpr i32 TICKS_BEFORE_ABOUT_TO_SPAWN_SOUND = 36;

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

    // ========== 物品存取 ==========

    /**
     * @brief 获取存储的物品
     * @return 物品堆的常引用，空物品表示无物品
     */
    [[nodiscard]] const ItemStack& getItem() const { return m_item; }

    /**
     * @brief 设置存储的物品
     * @param stack 要存储的物品堆
     */
    void setItem(const ItemStack& stack);

    // ========== 属性覆写 ==========

    /**
     * @brief 获取活塞推动反应
     * @return PushReaction::Ignore，实体不受活塞推动
     *
     * 对应 MC Java: OminousItemSpawner.getPistonPushReaction() = PushReaction.IGNORE
     */
    [[nodiscard]] PushReaction getPushReaction() const override { return PushReaction::Ignore; }

    // ========== 序列化 ==========

    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

private:
    /**
     * @brief 服务端 tick 逻辑
     */
    void tickServer();

    /**
     * @brief 客户端 tick 逻辑（粒子效果）
     */
    void tickClient();

    /**
     * @brief 从存储的物品生成实体（弹射物或物品实体）
     *
     * 对应 MC Java OminousItemSpawner.spawnItem()。
     * 如果物品是弹射物类型（如风弹、雪球等），向下发射弹射物；
     * 否则创建普通物品实体自然掉落。
     */
    void spawnItem();

    /**
     * @brief 生成弹射物实体
     * @param world 世界引用
     * @param item 物品（用于确定弹射物类型）
     * @return 生成的实体指针，失败返回 nullptr
     */
    Entity* spawnProjectile(IWorld& world, const Item& item);

    /**
     * @brief 生成不祥粒子效果
     *
     * 对应 MC Java OminousItemSpawner.addParticles()。
     * 在实体周围生成 1-3 个 OMINOUS_SPAWNING 粒子。
     */
    void addParticles();

    /// 存储的物品（待投掷）
    ItemStack m_item;

    /// 生成物品的绝对 tick 时间
    /// 对应 MC Java 的 spawnItemAfterTicks，基于 tickCount 判断
    i64 m_spawnItemAfterTicks = 0;

    /// 是否已播放警告音效
    bool m_warnedSoundPlayed = false;
};

} // namespace entity
} // namespace mc
