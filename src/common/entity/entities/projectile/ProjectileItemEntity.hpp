#pragma once

#include "ThrowableEntity.hpp"
#include "../../../item/core/ItemStack.hpp"
#include <memory>

namespace mc {
namespace entity {

/**
 * @brief 投掷物品实体基类
 *
 * 用于雪球、鸡蛋、末影珍珠等可投掷物品。
 * 子类只需要实现 getDefaultItem() 和 onImpact()。
 *
 * 参考 MC 1.16.5 ProjectileItemEntity
 */
class ProjectileItemEntity : public ThrowableEntity {
public:
    ~ProjectileItemEntity() override = default;

    // ========== Entity 接口重写 ==========

    void tick() override;

    // ========== 投掷物品方法 ==========

    /**
     * @brief 获取默认物品
     * @return 物品指针
     */
    [[nodiscard]] virtual const Item* getDefaultItem() const = 0;

    /**
     * @brief 获取物品堆
     * @return 物品堆
     */
    [[nodiscard]] ItemStack getItemStack() const { return m_itemStack; }

    /**
     * @brief 设置物品堆
     */
    void setItemStack(const ItemStack& stack) { m_itemStack = stack; }

protected:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    ProjectileItemEntity(LegacyEntityType type, EntityId id);

    // 物品堆
    ItemStack m_itemStack;
};

/**
 * @brief 雪球实体
 *
 * 雪球对烈焰人造成3点伤害，对其他实体无伤害。
 *
 * 参考 MC 1.16.5 SnowballEntity
 */
class SnowballEntity : public ProjectileItemEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    SnowballEntity(LegacyEntityType type, EntityId id);

    [[nodiscard]] const Item* getDefaultItem() const override;

protected:
    void onEntityHit(const RayTraceResult& result) override;
    void onImpact(const RayTraceResult& result) override;
};

/**
 * @brief 鸡蛋实体
 *
 * 鸡蛋有概率孵化出小鸡。
 *
 * 参考 MC 1.16.5 EggEntity
 */
class EggEntity : public ProjectileItemEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    EggEntity(LegacyEntityType type, EntityId id);

    [[nodiscard]] const Item* getDefaultItem() const override;

protected:
    void onEntityHit(const RayTraceResult& result) override;
    void onImpact(const RayTraceResult& result) override;

private:
    /**
     * @brief 尝试孵化小鸡
     * @return 是否成功孵化
     */
    bool tryHatchChicken();
};

/**
 * @brief 末影珍珠实体
 *
 * 末影珍珠会将玩家传送至落点。
 *
 * 参考 MC 1.16.5 EnderPearlEntity
 */
class EnderPearlEntity : public ProjectileItemEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    EnderPearlEntity(LegacyEntityType type, EntityId id);

    [[nodiscard]] const Item* getDefaultItem() const override;

protected:
    void onEntityHit(const RayTraceResult& result) override;
    void onImpact(const RayTraceResult& result) override;
};

/**
 * @brief 药水实体
 *
 * 投掷型药水和滞留型药水。
 *
 * 参考 MC 1.16.5 PotionEntity
 */
class PotionEntity : public ProjectileItemEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    PotionEntity(LegacyEntityType type, EntityId id);

    [[nodiscard]] const Item* getDefaultItem() const override;

    /**
     * @brief 是否为滞留型药水
     */
    [[nodiscard]] bool isLingering() const { return m_lingering; }

    /**
     * @brief 设置是否为滞留型药水
     */
    void setLingering(bool lingering) { m_lingering = lingering; }

protected:
    void onImpact(const RayTraceResult& result) override;

private:
    bool m_lingering = false;
};

/**
 * @brief 经验瓶实体
 *
 * 投掷后破裂并释放经验球。
 *
 * 参考 MC 1.16.5 ExperienceBottleEntity
 */
class ExperienceBottleEntity : public ProjectileItemEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    ExperienceBottleEntity(LegacyEntityType type, EntityId id);

    [[nodiscard]] const Item* getDefaultItem() const override;

    /**
     * @brief 获取经验值
     */
    [[nodiscard]] i32 experience() const { return m_experience; }

    /**
     * @brief 设置经验值
     */
    void setExperience(i32 exp) { m_experience = exp; }

protected:
    void onImpact(const RayTraceResult& result) override;

private:
    i32 m_experience = 0;
};

} // namespace entity
} // namespace mc
