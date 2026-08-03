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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <memory>

namespace mc {

namespace item {
class ProjectileItem;
}

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
    explicit OminousItemSpawnerEntity(EntityInstanceId id);

    ~OminousItemSpawnerEntity() override = default;

    /**
     * @brief 工厂方法（无物品）
     *
     * 创建不含物品的不祥物品生成器。
     * 注意：此方法不会设置物品和随机延迟，仅用于反序列化等场景。
     * 正常创建应使用 createWithItem()。
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 工厂方法（含物品）
     *
     * 创建不祥物品生成器并设置物品和随机延迟。
     *
     * @param world 世界引用，用于获取随机数生成器
     * @param stack 要投掷的物品堆
     * @return 创建的实体
     */
    static std::unique_ptr<Entity> createWithItem(IWorld& world, const ItemStack& stack);

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
     */
    [[nodiscard]] PushReaction getPushReaction() const override { return PushReaction::Ignore; }

    /**
     * @brief 检查实体是否不触发压力板/绊线
     * @return true，不祥物品生成器不触发压力板和绊线
     *
     * 对应 MC Java 的 OminousItemSpawner.isIgnoringBlockTriggers()
     */
    [[nodiscard]] bool doesEntityNotTriggerPressurePlate() const override { return true; }

    /**
     * @brief 检查此实体是否根本可以接受乘客
     * @return false，不祥物品生成器不允许任何实体骑乘
     *
     * 对应 MC Java 的 OminousItemSpawner.couldAcceptPassenger()
     */
    [[nodiscard]] bool couldAcceptPassenger() const override { return false; }

    /**
     * @brief 检查此实体是否可以添加指定乘客
     * @return false，不祥物品生成器不允许任何实体骑乘
     *
     * 对应 MC Java 的 OminousItemSpawner.canAddPassenger(Entity)
     */
    [[nodiscard]] bool canAddPassenger(const Entity& /*passenger*/) const override { return false; }

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
     * 如果物品是弹射物类型（如风弹、雪球等），向下发射弹射物；
     * 否则创建普通物品实体自然掉落。
     */
    void spawnItem();

    /**
     * @brief 生成弹射物实体
     *
     * 通过 ProjectileItem 接口创建弹射物，替代硬编码映射表。
     * 通过 ProjectileItem 接口创建弹射物，替代硬编码映射表。
     *
     * @param world 世界引用
     * @param projectileItem 弹射物物品接口
     * @param itemStack 物品堆（某些弹射物需要从中读取数据）
     * @return 生成的实体指针，失败返回 nullptr
     */
    Entity* spawnProjectile(IWorld& world, const item::ProjectileItem& projectileItem, const ItemStack& itemStack);

    /**
     * @brief 生成不祥粒子效果
     *
     * 在实体周围生成 1-3 个 OMINOUS_SPAWNING 粒子。
     */
    void addParticles();

    /// 存储的物品（待投掷）
    ItemStack m_item;

    /// 生成物品的绝对 tick 时间
    i64 m_spawnItemAfterTicks = 0;

    /// 是否已播放警告音效
    bool m_warnedSoundPlayed = false;
};

} // namespace entity
} // namespace mc
