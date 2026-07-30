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

#include <memory>

#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/network/ir/packets/play/ItemStackView.hpp"
#include <random>

namespace mc {

// Forward declarations
class Player;
class World;

/**
 * @brief 物品实体
 *
 * 掉落在世界中的物品实体。玩家靠近可以拾取。
 *
 * 特性：
 * - 10 tick 拾取延迟（刚丢出时不能被拾取）
 * - 5 分钟存活时间（超过后消失）
 * - 可以被其他物品实体合并
 * - 受重力和空气阻力影响
 * - 可以被玩家拾取
 */
class ItemEntity : public Entity {
public:
    // 物品本体（含 itemId/count/componentsPatch），经元数据 serializerId 7（ITEM_STACK）同步。
    // 对应 Java 1.21.11 ItemEntity.DATA_ITEM（EntityDataSerializers.ITEM_STACK）。
    static entity::DataParameter<network::ir::play::ItemStackView> DATA_ITEM_PARAM;

    /// 本类继承链标识（parent = Entity::classInfo()）。见 Entity::classInfo()。
    static const entity::EntityClassInfo& classInfo();

    // ========== 常量 ==========

    /// 默认拾取延迟（ticks）
    static constexpr i32 DEFAULT_PICKUP_DELAY = 10;

    /// 创造假物品拾取延迟（永不递减、不可拾取）
    static constexpr i32 FAKE_PICKUP_DELAY = 32767;

    /// 物品合并检测半径（AABB 水平扩展量）
    static constexpr f32 MERGE_RANGE = 0.5f;

    /// 默认存活时间（ticks）= 5分钟 = 6000 ticks
    static constexpr i32 DEFAULT_LIFETIME = 6000;

    /// 无限存活时间（用于创造模式等）
    static constexpr i32 INFINITE_LIFETIME = -1;

    /// 默认生命值（5 点，物品实体被伤害时消耗）
    static constexpr i32 DEFAULT_HEALTH = 5;

    /// 物品漂浮速度
    static constexpr f32 BUOYANCY = 0.1f;

    /// 水下下沉速度
    static constexpr f32 SINK_SPEED = 0.02f;

    /**
     * @brief 实体工厂方法
     *
     * 用于 EntityRegistry 注册
     * @param world 世界实例
     * @return 新创建的实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 构造函数 ==========

    /**
     * @brief 构造物品实体
     * @param id 实体ID
     * @param stack 物品堆
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     */
    ItemEntity(EntityInstanceId id, const ItemStack& stack, f32 x, f32 y, f32 z);

    /**
     * @brief 构造物品实体（带投掷速度）
     * @param id 实体ID
     * @param stack 物品堆
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param vx X方向速度
     * @param vy Y方向速度
     * @param vz Z方向速度
     */
    ItemEntity(EntityInstanceId id, const ItemStack& stack, f32 x, f32 y, f32 z, f32 vx, f32 vy, f32 vz);

    ~ItemEntity() override = default;

    // 禁止拷贝
    ItemEntity(const ItemEntity&) = delete;
    ItemEntity& operator=(const ItemEntity&) = delete;

    // 禁止移动（基类 Entity 不可移动）
    ItemEntity(ItemEntity&&) = delete;
    ItemEntity& operator=(ItemEntity&&) = delete;

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 0.25f; }
    [[nodiscard]] f32 height() const override { return 0.25f; }
    [[nodiscard]] f32 eyeHeight() const override { return 0.125f; }

    // 无战利品表，覆写基类方法返回空字符串
    [[nodiscard]] std::string getLootTableId() const override { return {}; }

    void tick() override;

    /**
     * @brief 处理物品实体受到伤害
     *
     * 物品实体有 5 点生命值，受到伤害时减少生命值。
     * 当生命值降至 0 或以下时，物品被销毁（调用 discard()）。
     * 防火物品（如下界合金物品、下界星）免疫火焰和岩浆伤害。
     * 当 mobGriefing 游戏规则关闭时，生物造成的伤害不会影响物品实体。
     */
    bool hurt(DamageSource& source, f32 amount) override;

    /**
     * @brief 检查物品实体是否免疫火焰
     *
     * 如果物品本身是防火的（如下界合金物品、下界星），则物品实体也免疫火焰。
     * 否则回退到实体类型的默认行为。
     */
    [[nodiscard]] bool isImmuneToFire() const override;

    /**
     * @brief 检查物品实体是否阻尼振动
     *
     * 当物品是羊毛物品时阻尼振动。
     */
    [[nodiscard]] bool dampensVibrations() const override;

    /**
     * @brief 判断是否应播放岩浆受伤音效
     *
     * 物品实体在岩浆中每 tick 都会受到伤害，如果每 tick 都播放音效会造成噪音。
     * 因此重写此方法，仅在物品被销毁时（生命值归零）或每 10 tick 播放一次音效。
     */
    [[nodiscard]] bool shouldPlayLavaHurtSound() const override
    {
        return m_health <= 0 || static_cast<i32>(ticksExisted()) % 10 == 0;
    }

    // ========== 物品相关 ==========

    /**
     * @brief 获取物品堆
     */
    [[nodiscard]] const ItemStack& getItemStack() const { return m_itemStack; }

    /**
     * @brief 设置物品堆
     */
    void setItemStack(const ItemStack& stack);

    /**
     * @brief 获取物品数量
     */
    [[nodiscard]] i32 getCount() const { return m_itemStack.getCount(); }

    /**
     * @brief 获取年龄（ticks）
     */
    [[nodiscard]] i32 getAge() const { return m_age; }

    /**
     * @brief 获取拾取延迟
     */
    [[nodiscard]] i32 getPickupDelay() const { return m_pickupDelay; }

    /**
     * @brief 设置拾取延迟
     * @param delay 延迟ticks
     */
    void setPickupDelay(i32 delay) { m_pickupDelay = delay; }

    /**
     * @brief 是否可以被拾取
     */
    [[nodiscard]] bool canBePickedUp() const { return m_pickupDelay <= 0 && !m_unpickable; }

    /**
     * @brief 设置不可拾取（创造模式丢弃的物品）
     */
    void setUnpickable() { m_unpickable = true; }

    /**
     * @brief 检查是否已过期（应该消失）
     */
    [[nodiscard]] bool isExpired() const { return m_lifetime != INFINITE_LIFETIME && m_age >= m_lifetime; }

    /**
     * @brief 设置存活时间
     * @param lifetime 存活时间（ticks），-1表示无限
     */
    void setLifetime(i32 lifetime) { m_lifetime = lifetime; }

    /**
     * @brief 设置所有者（防止立即拾取自己的物品）
     * @param ownerUuid 所有者UUID
     * @param throwerUuid 投掷者UUID（可选）
     */
    void setOwner(const std::string& ownerUuid, const std::string& throwerUuid = "");

    /**
     * @brief 获取所有者UUID
     */
    [[nodiscard]] const std::string& ownerUuid() const { return m_ownerUuid; }

    /**
     * @brief 获取投掷者UUID
     */
    [[nodiscard]] const std::string& throwerUuid() const { return m_throwerUuid; }

    /**
     * @brief 获取生命值
     */
    [[nodiscard]] i32 getHealth() const { return m_health; }

    /**
     * @brief 设置生命值
     * @param health 生命值
     */
    void setHealth(i32 health) { m_health = health; }

    // ========== 玩家拾取 ==========

    /**
     * @brief 玩家尝试拾取此物品
     * @param player 玩家
     * @return 是否成功拾取
     */
    bool onPlayerPickup(Player& player);

    // ========== 物品合并 ==========

    /**
     * @brief 尝试与另一个物品实体合并
     * @param other 另一个物品实体
     * @return 是否成功合并
     */
    bool tryMergeWith(ItemEntity& other);

    /**
     * @brief 检查是否可以与另一个物品实体合并
     * @param other 另一个物品实体
     */
    [[nodiscard]] bool canMergeWith(const ItemEntity& other) const;

    // ========== NBT 序列化 ==========

    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

private:
    /**
     * @brief 更新物理状态
     * @param world 世界（用于检测水和熔岩）
     */
    void _updatePhysics();

    /**
     * @brief 更新合并检测
     */
    void _updateMerge();

    /**
     * @brief 下沉速度（在水中）
     */
    void _applyWaterPhysics();

    /**
     * @brief 熔岩漂浮
     */
    void _applyLavaPhysics();

    /**
     * @brief 普通重力和阻力
     */
    void _applyNormalPhysics();

    ItemStack m_itemStack;
    i32 m_age = 0;                            // 存活时间（ticks）
    i32 m_lifetime = DEFAULT_LIFETIME;        // 最大存活时间
    i32 m_pickupDelay = DEFAULT_PICKUP_DELAY; // 拾取延迟
    i32 m_health = DEFAULT_HEALTH;            // 生命值
    bool m_unpickable = false;                // 是否不可拾取

    std::string m_ownerUuid;   // 所有者UUID（防止自己立即拾取）
    std::string m_throwerUuid; // 投掷者UUID
};

} // namespace mc
