#pragma once

#include "../../core/Entity.hpp"
#include "../../../item/ItemStack.hpp"
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
 *
 * 参考: net.minecraft.entity.item.ItemEntity
 */
class ItemEntity : public Entity {
public:
    // ========== 常量 ==========

    /// 默认拾取延迟（ticks）
    static constexpr i32 DEFAULT_PICKUP_DELAY = 10;

    /// 默认存活时间（ticks）= 5分钟 = 6000 ticks
    static constexpr i32 DEFAULT_LIFETIME = 6000;

    /// 无限存活时间（用于创造模式等）
    static constexpr i32 INFINITE_LIFETIME = -1;

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
    ItemEntity(EntityId id, const ItemStack& stack, f32 x, f32 y, f32 z);

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
    ItemEntity(EntityId id, const ItemStack& stack, f32 x, f32 y, f32 z,
                f32 vx, f32 vy, f32 vz);

    ~ItemEntity() override = default;

    // 禁止拷贝
    ItemEntity(const ItemEntity&) = delete;
    ItemEntity& operator=(const ItemEntity&) = delete;

    // 允许移动
    ItemEntity(ItemEntity&&) = default;
    ItemEntity& operator=(ItemEntity&&) = default;

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 0.25f; }
    [[nodiscard]] f32 height() const override { return 0.25f; }
    [[nodiscard]] f32 eyeHeight() const override { return 0.125f; }

    void tick() override;

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
    [[nodiscard]] bool isExpired() const {
        return m_lifetime != INFINITE_LIFETIME && m_age >= m_lifetime;
    }

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
    void setOwner(const String& ownerUuid, const String& throwerUuid = "");

    /**
     * @brief 获取所有者UUID
     */
    [[nodiscard]] const String& ownerUuid() const { return m_ownerUuid; }

    /**
     * @brief 获取投掷者UUID
     */
    [[nodiscard]] const String& throwerUuid() const { return m_throwerUuid; }

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

    // ========== 序列化 ==========

    void serialize(network::PacketSerializer& ser) const;
    [[nodiscard]] static Result<std::unique_ptr<ItemEntity>> deserialize(
        network::PacketDeserializer& deser, EntityId id);

private:
    /**
     * @brief 更新物理状态
     * @param world 世界（用于检测水和熔岩）
     */
    void updatePhysics();

    /**
     * @brief 更新合并检测
     */
    void updateMerge();

    /**
     * @brief 下沉速度（在水中）
     */
    void applyWaterPhysics();

    /**
     * @brief 熔岩漂浮
     */
    void applyLavaPhysics();

    /**
     * @brief 普通重力和阻力
     */
    void applyNormalPhysics();

    ItemStack m_itemStack;
    i32 m_age = 0;                  // 存活时间（ticks）
    i32 m_lifetime = DEFAULT_LIFETIME;  // 最大存活时间
    i32 m_pickupDelay = DEFAULT_PICKUP_DELAY;  // 拾取延迟
    bool m_unpickable = false;      // 是否不可拾取

    String m_ownerUuid;             // 所有者UUID（防止自己立即拾取）
    String m_throwerUuid;           // 投掷者UUID

    // 合并相关
    static constexpr f32 MERGE_RADIUS = 1.5f;  // 合并检测半径
};

} // namespace mc
